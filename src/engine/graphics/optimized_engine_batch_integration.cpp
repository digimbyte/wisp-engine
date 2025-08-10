// Optimized Graphics Engine - Batch Processing Integration Implementation
// Integrates 16x16 sprite batch system with tile-based rendering engine

#include "optimized_engine.h"
#include "sprite_batch_system.h"
#include "../core/debug.h"

// Batch processing integration methods implementation

uint8_t OptimizedGraphicsEngine::loadBatchedSprite(const uint8_t* rawSpriteData, uint32_t dataSize, SpriteArtType artType) {
    if (!rawSpriteData || dataSize == 0) {
        Serial.println("ERROR: Invalid sprite data for batching");
        return 0xFF;
    }
    
    // Generate unique sprite ID for batching system
    static uint16_t nextBatchedId = 1;
    uint16_t batchedSpriteId = nextBatchedId++;
    
    // Process sprite into 16x16 chunks using the batch processor
    bool success = spriteBatchProcessor.processSpriteToChunks(rawSpriteData, dataSize, artType, batchedSpriteId);
    
    if (success) {
        Serial.print("Batched sprite loaded: ID=");
        Serial.print(batchedSpriteId);
        Serial.print(", Art type=");
        Serial.print(artType);
        Serial.print(", Data size=");
        Serial.print(dataSize);
        Serial.println(" bytes");
        
        return batchedSpriteId;
    } else {
        Serial.println("ERROR: Failed to process sprite into batches");
        return 0xFF;
    }
}

bool OptimizedGraphicsEngine::addBatchedSprite(uint16_t batchedSpriteId, OptimizedLayer layer, 
                                             int16_t x, int16_t y, SpriteFlipMode flipMode, uint8_t priority) {
    if (activeSpriteCount >= MAX_SPRITES_ACTIVE) {
        Serial.println("ERROR: Too many active sprites");
        return false;
    }
    
    // Get batched sprite info
    const SpriteBatchProcessor::ProcessedSprite* batchedSprite = 
        spriteBatchProcessor.getProcessedSprite(batchedSpriteId);
    
    if (!batchedSprite) {
        Serial.print("ERROR: Batched sprite not found: ID=");
        Serial.println(batchedSpriteId);
        return false;
    }
    
    // Create sprite instance with batch information
    uint8_t instanceId = activeSpriteCount++;
    SpriteInstance& instance = activeSprites[instanceId];
    
    instance.spriteId = batchedSpriteId;  // Store batched sprite ID
    instance.layer = layer;
    instance.frame = 0;
    instance.priority = priority;
    instance.x = x;
    instance.y = y;
    instance.flags = 0x01 | (flipMode << 1);  // Visible + flip mode in upper bits
    
    // Add to layer
    if (layerSpriteCount[layer] < MAX_SPRITES_ACTIVE) {
        layerSprites[layer][layerSpriteCount[layer]++] = instanceId;
    }
    
    // Mark affected tiles as dirty based on batched sprite dimensions
    uint16_t spriteWidth = batchedSprite->header.chunksWidth * SPRITE_BATCH_CHUNK_SIZE;
    uint16_t spriteHeight = batchedSprite->header.chunksHeight * SPRITE_BATCH_CHUNK_SIZE;
    markTilesDirty(x, y, spriteWidth, spriteHeight);
    
    return true;
}

void OptimizedGraphicsEngine::renderBatchedSpriteToTile(uint16_t batchedSpriteId, int16_t tileStartX, int16_t tileStartY,
                                                       int16_t spriteX, int16_t spriteY, SpriteFlipMode flipMode) {
    const SpriteBatchProcessor::ProcessedSprite* batchedSprite = 
        spriteBatchProcessor.getProcessedSprite(batchedSpriteId);
    
    if (!batchedSprite) {
        return;  // Sprite not found
    }
    
    // Calculate which 16x16 chunks intersect with this tile
    int16_t tileEndX = tileStartX + TILE_SIZE;
    int16_t tileEndY = tileStartY + TILE_SIZE;
    
    // Skip if sprite doesn't intersect tile at all
    int16_t spriteEndX = spriteX + (batchedSprite->header.chunksWidth * SPRITE_BATCH_CHUNK_SIZE);
    int16_t spriteEndY = spriteY + (batchedSprite->header.chunksHeight * SPRITE_BATCH_CHUNK_SIZE);
    
    if (spriteEndX <= tileStartX || spriteX >= tileEndX || 
        spriteEndY <= tileStartY || spriteY >= tileEndY) {
        return;  // No intersection
    }
    
    // Render intersecting chunks
    for (uint8_t i = 0; i < batchedSprite->chunkCount; i++) {
        const SpriteChunk* chunk = batchedSprite->chunks[i];
        if (!chunk) continue;
        
        // Calculate chunk position in world coordinates
        int16_t chunkWorldX = spriteX + (chunk->x * SPRITE_BATCH_CHUNK_SIZE);
        int16_t chunkWorldY = spriteY + (chunk->y * SPRITE_BATCH_CHUNK_SIZE);
        
        // Skip chunks that don't intersect this tile
        if (chunkWorldX + SPRITE_BATCH_CHUNK_SIZE <= tileStartX || chunkWorldX >= tileEndX ||
            chunkWorldY + SPRITE_BATCH_CHUNK_SIZE <= tileStartY || chunkWorldY >= tileEndY) {
            continue;
        }
        
        // Render this chunk to the tile buffer
        renderChunkToTileBuffer(chunk, tileStartX, tileStartY, chunkWorldX, chunkWorldY, flipMode);
    }
}

void OptimizedGraphicsEngine::renderChunkToTileBuffer(const SpriteChunk* chunk, 
                                                     int16_t tileStartX, int16_t tileStartY,
                                                     int16_t chunkWorldX, int16_t chunkWorldY, 
                                                     SpriteFlipMode flipMode) {
    // Calculate chunk position relative to tile
    int16_t chunkTileX = chunkWorldX - tileStartX;
    int16_t chunkTileY = chunkWorldY - tileStartY;
    
    // Calculate clipping bounds
    int16_t startX = std::max(0, -chunkTileX);
    int16_t startY = std::max(0, -chunkTileY);
    int16_t endX = std::min(SPRITE_BATCH_CHUNK_SIZE, TILE_SIZE - chunkTileX);
    int16_t endY = std::min(SPRITE_BATCH_CHUNK_SIZE, TILE_SIZE - chunkTileY);
    
    // Render pixels with flipping support
    for (int16_t y = startY; y < endY; y++) {
        for (int16_t x = startX; x < endX; x++) {
            uint8_t sourceX = x;
            uint8_t sourceY = y;
            
            // Apply flip transformations
            if (flipMode & FLIP_HORIZONTAL) {
                sourceX = SPRITE_BATCH_CHUNK_SIZE - 1 - x;
            }
            if (flipMode & FLIP_VERTICAL) {
                sourceY = SPRITE_BATCH_CHUNK_SIZE - 1 - y;
            }
            
            uint8_t colorIndex = chunk->data[sourceY * SPRITE_BATCH_CHUNK_SIZE + sourceX];
            
            // Skip transparent pixels
            if (colorIndex == 0) continue;
            
            // Get color from LUT
            uint8_t lutX = colorIndex % SMALL_LUT_SIZE;
            uint8_t lutY = colorIndex / SMALL_LUT_SIZE;
            uint16_t color = colorLUT[lutY * SMALL_LUT_SIZE + lutX];
            
            // Write to tile buffer
            int16_t bufferX = chunkTileX + x;
            int16_t bufferY = chunkTileY + y;
            
            if (bufferX >= 0 && bufferX < TILE_SIZE && bufferY >= 0 && bufferY < TILE_SIZE) {
                tileCtx.tileBuffer[bufferY * TILE_SIZE + bufferX] = color;
            }
        }
    }
}

uint32_t OptimizedGraphicsEngine::getBatchedMemoryUsage() const {
    return spriteBatchProcessor.getMemoryUsage();
}

// Enhanced rendering method that supports both traditional and batched sprites
void OptimizedGraphicsEngine::renderLayerToTile(uint8_t layer, uint8_t tileX, uint8_t tileY) {
    int16_t tileStartX = tileX * TILE_SIZE;
    int16_t tileStartY = tileY * TILE_SIZE;
    int16_t tileEndX = tileStartX + TILE_SIZE;
    int16_t tileEndY = tileStartY + TILE_SIZE;
    
    // Render sprites in this layer
    for (int i = 0; i < layerSpriteCount[layer]; i++) {
        uint8_t instanceId = layerSprites[layer][i];
        if (instanceId >= activeSpriteCount) continue;
        
        SpriteInstance& instance = activeSprites[instanceId];
        if (!(instance.flags & 0x01)) continue; // Not visible
        
        // Extract flip mode from flags
        SpriteFlipMode flipMode = (SpriteFlipMode)((instance.flags >> 1) & 0x03);
        
        // Check if this is a batched sprite (ID > traditional sprite count)
        if (instance.spriteId >= loadedSpriteCount) {
            // This is a batched sprite
            uint16_t batchedSpriteId = instance.spriteId;
            
            const SpriteBatchProcessor::ProcessedSprite* batchedSprite = 
                spriteBatchProcessor.getProcessedSprite(batchedSpriteId);
            
            if (!batchedSprite) continue;
            
            // Quick bounds check for batched sprite
            uint16_t spriteWidth = batchedSprite->header.chunksWidth * SPRITE_BATCH_CHUNK_SIZE;
            uint16_t spriteHeight = batchedSprite->header.chunksHeight * SPRITE_BATCH_CHUNK_SIZE;
            
            if (instance.x >= tileEndX || instance.x + spriteWidth <= tileStartX ||
                instance.y >= tileEndY || instance.y + spriteHeight <= tileStartY) {
                continue;  // No intersection
            }
            
            // Render batched sprite
            renderBatchedSpriteToTile(batchedSpriteId, tileStartX, tileStartY, 
                                    instance.x, instance.y, flipMode);
        } else {
            // Traditional sprite - use existing rendering path
            OptimizedSprite& sprite = sprites[instance.spriteId];
            if (!sprite.loaded) continue;
            
            // Quick bounds check - skip if sprite doesn't intersect tile
            if (instance.x >= tileEndX || instance.x + sprite.header.width <= tileStartX ||
                instance.y >= tileEndY || instance.y + sprite.header.height <= tileStartY) {
                continue;
            }
            
            // Render traditional sprite to tile buffer
            renderSpriteToTile(instance, sprite, tileStartX, tileStartY);
            sprites[instance.spriteId].lastUsed = get_millis();
        }
    }
}
