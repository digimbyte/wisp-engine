// Sprite Batch System Implementation - ESP32 16x16 Chunk Processing
// Memory-optimized sprite batching with auto-padding and flip operations

#include "sprite_batch_system.h"
#include "../core/debug.h"
#include <cstring>
#include <algorithm>

namespace WispEngine {
namespace Graphics {

// Global instance
SpriteBatchProcessor spriteBatchProcessor;

// === SPRITE CHUNK POOL IMPLEMENTATION ===

SpriteChunkPool::SpriteChunkPool() : allocatedCount(0) {
    memset(memoryPool, 0, sizeof(memoryPool));
    memset(chunkAllocated, false, sizeof(chunkAllocated));
    memset(allocationTime, 0, sizeof(allocationTime));
}

uint8_t* SpriteChunkPool::allocateChunk(uint16_t chunkId) {
    // Find free slot
    for (uint8_t i = 0; i < SPRITE_BATCH_POOL_SIZE; i++) {
        if (!chunkAllocated[i]) {
            chunkAllocated[i] = true;
            allocatedCount++;
            allocationTime[i] = esp_timer_get_time() / 1000; // ESP-IDF time in ms
            
            uint8_t* chunkData = &memoryPool[i * SPRITE_BATCH_CHUNK_BYTES];
            memset(chunkData, 0, SPRITE_BATCH_CHUNK_BYTES); // Clear chunk
            
            return chunkData;
        }
    }
    
    // No free slots - try LRU eviction
    freeOldestChunk();
    
    // Try again after eviction
    for (uint8_t i = 0; i < SPRITE_BATCH_POOL_SIZE; i++) {
        if (!chunkAllocated[i]) {
            chunkAllocated[i] = true;
            allocatedCount++;
            allocationTime[i] = esp_timer_get_time() / 1000;
            
            uint8_t* chunkData = &memoryPool[i * SPRITE_BATCH_CHUNK_BYTES];
            memset(chunkData, 0, SPRITE_BATCH_CHUNK_BYTES);
            
            return chunkData;
        }
    }
    
    return nullptr; // Pool completely full
}

void SpriteChunkPool::freeChunk(uint16_t chunkId) {
    // Find chunk in pool by comparing data addresses
    for (uint8_t i = 0; i < SPRITE_BATCH_POOL_SIZE; i++) {
        if (chunkAllocated[i]) {
            uint8_t* chunkData = &memoryPool[i * SPRITE_BATCH_CHUNK_BYTES];
            // Simple approach: free by index since we don't store IDs directly
            chunkAllocated[i] = false;
            allocatedCount--;
            allocationTime[i] = 0;
            break;
        }
    }
}

void SpriteChunkPool::freeOldestChunk() {
    uint32_t oldestTime = UINT32_MAX;
    uint8_t oldestIndex = 0;
    
    for (uint8_t i = 0; i < SPRITE_BATCH_POOL_SIZE; i++) {
        if (chunkAllocated[i] && allocationTime[i] < oldestTime) {
            oldestTime = allocationTime[i];
            oldestIndex = i;
        }
    }
    
    if (oldestTime != UINT32_MAX) {
        chunkAllocated[oldestIndex] = false;
        allocatedCount--;
        allocationTime[oldestIndex] = 0;
    }
}

uint8_t* SpriteChunkPool::getChunk(uint16_t chunkId) {
    // For now, simple lookup by allocated status
    for (uint8_t i = 0; i < SPRITE_BATCH_POOL_SIZE; i++) {
        if (chunkAllocated[i]) {
            return &memoryPool[i * SPRITE_BATCH_CHUNK_BYTES];
        }
    }
    return nullptr;
}

void SpriteChunkPool::printPoolStats() const {
    WISP_DEBUG_INFO("BATCH", "=== Sprite Chunk Pool Stats ===");
    WISP_DEBUG_INFO("BATCH", "Pool size: %d chunks (%d bytes)", 
                    SPRITE_BATCH_POOL_SIZE, SPRITE_BATCH_POOL_BYTES);
    WISP_DEBUG_INFO("BATCH", "Allocated: %d/%d chunks", allocatedCount, SPRITE_BATCH_POOL_SIZE);
    WISP_DEBUG_INFO("BATCH", "Free: %d chunks", getFreeChunks());
    WISP_DEBUG_INFO("BATCH", "Memory usage: %d/%d bytes", 
                    allocatedCount * SPRITE_BATCH_CHUNK_BYTES, SPRITE_BATCH_POOL_BYTES);
}

// === SPRITE BATCH PROCESSOR IMPLEMENTATION ===

SpriteBatchProcessor::SpriteBatchProcessor() : cachedCount(0) {
    memset(cachedSprites, 0, sizeof(cachedSprites));
}

SpriteBatchProcessor::~SpriteBatchProcessor() {
    // Free all cached sprites
    for (uint8_t i = 0; i < cachedCount; i++) {
        for (uint8_t j = 0; j < cachedSprites[i].chunkCount; j++) {
            if (cachedSprites[i].chunks[j]) {
                // Free chunk memory (simplified - in real implementation would track properly)
                chunkPool.freeChunk(j);
            }
        }
    }
}

bool SpriteBatchProcessor::processSpriteToChunks(const uint8_t* rawSpriteData, uint32_t dataSize,
                                               SpriteArtType artType, uint16_t spriteId) {
    if (!rawSpriteData || dataSize == 0) {
        WISP_DEBUG_ERROR("BATCH", "Invalid sprite data");
        return false;
    }
    
    // For this example, assume rawSpriteData starts with width, height, then pixel data
    // In a real implementation, this would parse the actual sprite format
    uint16_t width = ((uint16_t*)rawSpriteData)[0];
    uint16_t height = ((uint16_t*)rawSpriteData)[1];
    const uint8_t* pixelData = rawSpriteData + 4; // Skip width/height
    
    if (width == 0 || height == 0 || width > 1024 || height > 1024) {
        WISP_DEBUG_ERROR("BATCH", "Invalid sprite dimensions: %dx%d", width, height);
        return false;
    }
    
    // Check if we have room in cache
    if (cachedCount >= MAX_CACHED_SPRITES) {
        evictOldestCached();
    }
    
    ProcessedSprite* sprite = &cachedSprites[cachedCount];
    
    // Initialize sprite header
    sprite->header.magic = MAGIC_BATCHED_SPRITE;
    sprite->header.artType = artType;
    sprite->header.originalWidth = width;
    sprite->header.originalHeight = height;
    sprite->header.chunksWidth = calculateChunksWidth(width);
    sprite->header.chunksHeight = calculateChunksHeight(height);
    sprite->header.totalChunks = calculateTotalChunks(width, height);
    sprite->header.paddingColor = detectBestPaddingColor(pixelData, width, height);
    sprite->header.flags = 0;
    sprite->header.chunkDataOffset = sizeof(BatchedSpriteHeader);
    sprite->header.totalDataSize = sprite->header.totalChunks * sizeof(SpriteChunk);
    
    // Clear animation info for non-entity sprites
    memset(&sprite->header.animation, 0, sizeof(sprite->header.animation));
    
    if (artType == ART_ENTITY) {
        // Auto-detect animation frames (simplified logic)
        sprite->header.animation.frameCount = 1; // Default to static
        sprite->header.animation.framesPerRow = 1;
        sprite->header.animation.defaultFPS = 12;
        sprite->header.animation.loopMode = 0; // Loop
    }
    
    // Convert sprite to chunks
    convertToChunks(pixelData, width, height, sprite, sprite->header.paddingColor);
    
    sprite->cached = true;
    sprite->lastAccessed = esp_timer_get_time() / 1000;
    
    cachedCount++;
    
    WISP_DEBUG_INFO("BATCH", "Processed sprite %d: %dx%d -> %dx%d chunks", 
                   spriteId, width, height, sprite->header.chunksWidth, sprite->header.chunksHeight);
    
    return true;
}

void SpriteBatchProcessor::convertToChunks(const uint8_t* pixelData, uint16_t width, uint16_t height,
                                         ProcessedSprite* sprite, uint8_t transparentColor) {
    sprite->chunkCount = 0;
    
    for (uint16_t chunkY = 0; chunkY < sprite->header.chunksHeight; chunkY++) {
        for (uint16_t chunkX = 0; chunkX < sprite->header.chunksWidth; chunkX++) {
            if (sprite->chunkCount >= MAX_SPRITE_BATCH_CHUNKS) {
                WISP_DEBUG_WARNING("BATCH", "Too many chunks for sprite, truncating");
                break;
            }
            
            // Allocate chunk from pool
            uint8_t* chunkData = chunkPool.allocateChunk(sprite->chunkCount);
            if (!chunkData) {
                WISP_DEBUG_ERROR("BATCH", "Failed to allocate chunk from pool");
                break;
            }
            
            // Create chunk structure
            SpriteChunk* chunk = (SpriteChunk*)chunkData;
            chunk->chunkId = sprite->chunkCount;
            chunk->x = chunkX;
            chunk->y = chunkY;
            chunk->transparentPixels = 0;
            chunk->flags = 0;
            
            // Process chunk region
            processChunkRegion(pixelData, width, height, chunkX, chunkY, chunk, transparentColor);
            
            sprite->chunks[sprite->chunkCount] = chunk;
            sprite->chunkCount++;
        }
    }
}

void SpriteBatchProcessor::processChunkRegion(const uint8_t* sourceData, uint16_t sourceWidth, uint16_t sourceHeight,
                                            uint16_t chunkX, uint16_t chunkY, SpriteChunk* targetChunk,
                                            uint8_t transparentColor) {
    uint16_t startPixelX = chunkX * SPRITE_BATCH_CHUNK_SIZE;
    uint16_t startPixelY = chunkY * SPRITE_BATCH_CHUNK_SIZE;
    
    targetChunk->transparentPixels = 0;
    
    for (uint8_t y = 0; y < SPRITE_BATCH_CHUNK_SIZE; y++) {
        for (uint8_t x = 0; x < SPRITE_BATCH_CHUNK_SIZE; x++) {
            uint16_t sourceX = startPixelX + x;
            uint16_t sourceY = startPixelY + y;
            
            uint8_t pixel;
            
            if (sourceX < sourceWidth && sourceY < sourceHeight) {
                // Copy pixel from source
                pixel = sourceData[sourceY * sourceWidth + sourceX];
            } else {
                // Pad with transparent color
                pixel = transparentColor;
                targetChunk->transparentPixels++;
            }
            
            targetChunk->data[y * SPRITE_BATCH_CHUNK_SIZE + x] = pixel;
            
            if (pixel == transparentColor) {
                targetChunk->transparentPixels++;
            }
        }
    }
    
    // Set chunk flags based on content
    if (targetChunk->transparentPixels == SPRITE_BATCH_CHUNK_PIXELS) {
        targetChunk->flags |= 0x01; // Completely transparent
    } else if (targetChunk->transparentPixels == 0) {
        targetChunk->flags |= 0x02; // Completely opaque
    }
}

uint8_t SpriteBatchProcessor::detectBestPaddingColor(const uint8_t* spriteData, uint16_t width, uint16_t height) {
    // Simple heuristic: check corners and edges for most common transparent value
    uint8_t cornerPixels[4] = {
        spriteData[0],                                    // Top-left
        spriteData[width - 1],                           // Top-right
        spriteData[(height - 1) * width],                // Bottom-left
        spriteData[(height - 1) * width + width - 1]    // Bottom-right
    };
    
    // Check if any corner pixel repeats (likely transparent)
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (cornerPixels[i] == cornerPixels[j]) {
                return cornerPixels[i];
            }
        }
    }
    
    // Default to index 0 (usually transparent in palette-based sprites)
    return 0;
}

void SpriteBatchProcessor::flipSpriteChunks(uint16_t spriteId, SpriteFlipMode flipMode) {
    ProcessedSprite* sprite = findCachedSprite(spriteId);
    if (!sprite) {
        WISP_DEBUG_ERROR("BATCH", "Sprite %d not found for flipping", spriteId);
        return;
    }
    
    for (uint8_t i = 0; i < sprite->chunkCount; i++) {
        SpriteChunk* chunk = sprite->chunks[i];
        if (chunk) {
            switch (flipMode) {
                case FLIP_HORIZONTAL:
                    flipChunkHorizontal(chunk);
                    break;
                case FLIP_VERTICAL:
                    flipChunkVertical(chunk);
                    break;
                case FLIP_BOTH:
                    flipChunkBoth(chunk);
                    break;
                default:
                    break;
            }
        }
    }
    
    sprite->lastAccessed = esp_timer_get_time() / 1000;
}

void SpriteBatchProcessor::flipChunkHorizontal(SpriteChunk* chunk) {
    uint8_t temp[SPRITE_BATCH_CHUNK_BYTES];
    memcpy(temp, chunk->data, SPRITE_BATCH_CHUNK_BYTES);
    
    for (uint8_t y = 0; y < SPRITE_BATCH_CHUNK_SIZE; y++) {
        for (uint8_t x = 0; x < SPRITE_BATCH_CHUNK_SIZE; x++) {
            uint8_t sourceX = SPRITE_BATCH_CHUNK_SIZE - 1 - x;
            chunk->data[y * SPRITE_BATCH_CHUNK_SIZE + x] = 
                temp[y * SPRITE_BATCH_CHUNK_SIZE + sourceX];
        }
    }
}

void SpriteBatchProcessor::flipChunkVertical(SpriteChunk* chunk) {
    uint8_t temp[SPRITE_BATCH_CHUNK_BYTES];
    memcpy(temp, chunk->data, SPRITE_BATCH_CHUNK_BYTES);
    
    for (uint8_t y = 0; y < SPRITE_BATCH_CHUNK_SIZE; y++) {
        for (uint8_t x = 0; x < SPRITE_BATCH_CHUNK_SIZE; x++) {
            uint8_t sourceY = SPRITE_BATCH_CHUNK_SIZE - 1 - y;
            chunk->data[y * SPRITE_BATCH_CHUNK_SIZE + x] = 
                temp[sourceY * SPRITE_BATCH_CHUNK_SIZE + x];
        }
    }
}

void SpriteBatchProcessor::flipChunkBoth(SpriteChunk* chunk) {
    flipChunkHorizontal(chunk);
    flipChunkVertical(chunk);
}

void SpriteBatchProcessor::renderChunk(const SpriteChunk* chunk, uint8_t* targetBuffer, uint16_t targetWidth,
                                     uint16_t targetX, uint16_t targetY, SpriteFlipMode flipMode) {
    if (!chunk || !targetBuffer) return;
    
    for (uint8_t y = 0; y < SPRITE_BATCH_CHUNK_SIZE; y++) {
        for (uint8_t x = 0; x < SPRITE_BATCH_CHUNK_SIZE; x++) {
            uint16_t renderX = targetX + x;
            uint16_t renderY = targetY + y;
            
            uint8_t sourceX = x;
            uint8_t sourceY = y;
            
            // Apply flip mode during rendering
            if (flipMode & FLIP_HORIZONTAL) {
                sourceX = SPRITE_BATCH_CHUNK_SIZE - 1 - x;
            }
            if (flipMode & FLIP_VERTICAL) {
                sourceY = SPRITE_BATCH_CHUNK_SIZE - 1 - y;
            }
            
            uint8_t pixel = chunk->data[sourceY * SPRITE_BATCH_CHUNK_SIZE + sourceX];
            
            // Skip transparent pixels (assuming 0 is transparent)
            if (pixel != 0) {
                uint32_t targetIndex = renderY * targetWidth + renderX;
                targetBuffer[targetIndex] = pixel;
            }
        }
    }
}

void SpriteBatchProcessor::renderBatchedSprite(uint16_t spriteId, uint8_t* targetBuffer, uint16_t targetWidth,
                                             uint16_t targetX, uint16_t targetY, SpriteFlipMode flipMode) {
    const ProcessedSprite* sprite = getProcessedSprite(spriteId);
    if (!sprite) {
        WISP_DEBUG_ERROR("BATCH", "Cannot render sprite %d - not found", spriteId);
        return;
    }
    
    for (uint8_t i = 0; i < sprite->chunkCount; i++) {
        const SpriteChunk* chunk = sprite->chunks[i];
        if (chunk) {
            uint16_t chunkPixelX = targetX + chunkToPixelX(chunk->x);
            uint16_t chunkPixelY = targetY + chunkToPixelY(chunk->y);
            
            renderChunk(chunk, targetBuffer, targetWidth, chunkPixelX, chunkPixelY, flipMode);
        }
    }
}

const SpriteBatchProcessor::ProcessedSprite* SpriteBatchProcessor::getProcessedSprite(uint16_t spriteId) {
    return findCachedSprite(spriteId);
}

SpriteBatchProcessor::ProcessedSprite* SpriteBatchProcessor::findCachedSprite(uint16_t spriteId) {
    // Simple linear search (in real implementation might use hash table)
    for (uint8_t i = 0; i < cachedCount; i++) {
        if (cachedSprites[i].cached) {
            // For now, match by position in cache (would need proper ID tracking)
            return &cachedSprites[i];
        }
    }
    return nullptr;
}

void SpriteBatchProcessor::evictOldestCached() {
    if (cachedCount == 0) return;
    
    uint32_t oldestTime = UINT32_MAX;
    uint8_t oldestIndex = 0;
    
    for (uint8_t i = 0; i < cachedCount; i++) {
        if (cachedSprites[i].cached && cachedSprites[i].lastAccessed < oldestTime) {
            oldestTime = cachedSprites[i].lastAccessed;
            oldestIndex = i;
        }
    }
    
    // Free chunks for oldest sprite
    ProcessedSprite* sprite = &cachedSprites[oldestIndex];
    for (uint8_t j = 0; j < sprite->chunkCount; j++) {
        if (sprite->chunks[j]) {
            chunkPool.freeChunk(j);
            sprite->chunks[j] = nullptr;
        }
    }
    
    sprite->cached = false;
    sprite->chunkCount = 0;
    
    // Shift remaining sprites down
    for (uint8_t i = oldestIndex; i < cachedCount - 1; i++) {
        cachedSprites[i] = cachedSprites[i + 1];
    }
    cachedCount--;
}

uint32_t SpriteBatchProcessor::getMemoryUsage() const {
    uint32_t total = sizeof(SpriteBatchProcessor);
    total += chunkPool.getAllocatedCount() * SPRITE_BATCH_CHUNK_BYTES;
    return total;
}

void SpriteBatchProcessor::printBatchStats() const {
    WISP_DEBUG_INFO("BATCH", "=== Sprite Batch Processor Stats ===");
    WISP_DEBUG_INFO("BATCH", "Cached sprites: %d/%d", cachedCount, MAX_CACHED_SPRITES);
    WISP_DEBUG_INFO("BATCH", "Memory usage: %d bytes", getMemoryUsage());
    WISP_DEBUG_INFO("BATCH", "Chunk pool usage: %d bytes", getChunkPoolUsage());
    
    chunkPool.printPoolStats();
}

} // namespace Graphics
} // namespace WispEngine
