// engine/optimized_graphics_engine.h - ESP32-C6/S3 Optimized Graphics using ESP-IDF
// High-performance graphics engine optimized for ESP32 with LovyanGFX
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include <LovyanGFX.hpp>

// ESP32-optimized graphics engine
// Designed for 320x172 display with minimal memory footprint

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 172
#define SCREEN_BUFFER_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT)

// Optimized constants for ESP32
#define MAX_SPRITES_ACTIVE 32        // Reduced from 256
#define MAX_SPRITE_SIZE 64           // 64x64 max sprite size
#define TILE_SIZE 32                 // 32x32 tiles for backgrounds
#define SMALL_LUT_SIZE 64            // 64x64 LUT instead of 128x128

// Memory-efficient sprite header
struct OptimizedSpriteHeader {
    uint8_t width, height;           // Max 255x255 (sufficient)
    uint8_t frameCount;              // Number of animation frames
    uint8_t paletteId;               // Which palette to use (0-3)
    uint16_t dataSize;               // Size of compressed data
    uint8_t flags;                   // Compression, transparency flags
    uint8_t reserved;                // Padding for alignment
};

// Simplified sprite in memory  
struct OptimizedSprite {
    OptimizedSpriteHeader header;
    uint8_t* pixelData;              // Palette indices or compressed data
    bool loaded;
    uint32_t lastUsed;               // For LRU eviction
};

// Tile-based rendering context
struct TileRenderContext {
    uint16_t* tileBuffer;            // 32x32 tile buffer (2KB)
    uint8_t tileX, tileY;            // Current tile being rendered
    bool tileDirty[10][6];           // 320/32 x 172/32 dirty flags
    uint32_t frameCount;             // For dirty tracking
};

// Optimized 4-layer system
enum OptimizedLayer {
    LAYER_BACKGROUND = 0,            // Tiled backgrounds
    LAYER_GAME = 1,                  // Game sprites
    LAYER_UI = 2,                    // UI elements  
    LAYER_TEXT = 3,                  // Text overlays
    LAYER_COUNT = 4
};

// Simple sprite instance
struct SpriteInstance {
    uint8_t spriteId;                // Index into sprite array
    uint8_t layer;                   // Which layer (0-3)
    uint8_t frame;                   // Current animation frame
    uint8_t priority;                // Render priority within layer
    int16_t x, y;                    // Screen position
    uint8_t flags;                   // Flip, visible, etc.
    uint8_t reserved;                // Padding
};

class OptimizedGraphicsEngine {
private:
    // Display
    LGFX* display;
    
    // Tile-based rendering
    TileRenderContext tileCtx;
    
    // Sprite management (static allocation)
    OptimizedSprite sprites[MAX_SPRITES_ACTIVE];
    SpriteInstance activeSprites[MAX_SPRITES_ACTIVE];
    uint8_t loadedSpriteCount;
    uint8_t activeSpriteCount;
    
    // Small color LUT (8KB instead of 32KB)
    uint16_t colorLUT[SMALL_LUT_SIZE * SMALL_LUT_SIZE];
    
    // Layer management
    uint8_t layerSpriteCount[LAYER_COUNT];
    uint8_t layerSprites[LAYER_COUNT][MAX_SPRITES_ACTIVE];
    
    // Performance tracking
    uint32_t frameStartTime;
    uint32_t renderTime;
    uint16_t spritesRendered;
    
public:
    OptimizedGraphicsEngine() : 
        display(nullptr), loadedSpriteCount(0), activeSpriteCount(0),
        frameStartTime(0), renderTime(0), spritesRendered(0) {
        
        // Clear arrays
        memset(sprites, 0, sizeof(sprites));
        memset(activeSprites, 0, sizeof(activeSprites));
        memset(layerSpriteCount, 0, sizeof(layerSpriteCount));
        memset(layerSprites, 0xFF, sizeof(layerSprites));
        
        // Initialize tile context
        tileCtx.tileBuffer = nullptr;
        tileCtx.tileX = 0;
        tileCtx.tileY = 0;
        tileCtx.frameCount = 0;
        memset(tileCtx.tileDirty, true, sizeof(tileCtx.tileDirty)); // Start with all dirty
    }
    
    // Initialize with minimal memory allocation
    bool init(LGFX* displayPtr) {
        display = displayPtr;
        
        // Allocate only the tile buffer (2KB)
        tileCtx.tileBuffer = (uint16_t*)malloc(TILE_SIZE * TILE_SIZE * 2);
        if (!tileCtx.tileBuffer) {
            Serial.println("ERROR: Failed to allocate tile buffer");
            return false;
        }
        
        // Generate small test LUT
        generateSmallLUT();
        
        Serial.print("Optimized Graphics Engine initialized - Memory usage: ");
        Serial.print(getMemoryUsage());
        Serial.println(" bytes");
        
        return true;
    }
    
    // Get total memory usage
    size_t getMemoryUsage() const {
        size_t total = sizeof(OptimizedGraphicsEngine);
        total += TILE_SIZE * TILE_SIZE * 2; // Tile buffer
        
        // Count loaded sprite data
        for (int i = 0; i < loadedSpriteCount; i++) {
            if (sprites[i].loaded && sprites[i].pixelData) {
                total += sprites[i].header.dataSize;
            }
        }
        
        return total;
    }
    
    // Generate small 64x64 LUT (8KB vs 32KB)
    void generateSmallLUT() {
        for (int y = 0; y < SMALL_LUT_SIZE; y++) {
            for (int x = 0; x < SMALL_LUT_SIZE; x++) {
                // Simple gradient for testing
                uint8_t r = (x * 255) / SMALL_LUT_SIZE;
                uint8_t g = (y * 255) / SMALL_LUT_SIZE;
                uint8_t b = ((x + y) * 255) / (SMALL_LUT_SIZE * 2);
                
                // Convert to RGB565
                uint16_t color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                colorLUT[y * SMALL_LUT_SIZE + x] = color;
            }
        }
        Serial.println("Small color LUT generated (64x64)");
    }
    
    // Load sprite with memory efficiency
    uint8_t loadSprite(const uint8_t* spriteData) {
        if (loadedSpriteCount >= MAX_SPRITES_ACTIVE) {
            // Try to evict least recently used sprite
            uint8_t lruIndex = findLRUSprite();
            if (lruIndex != 0xFF) {
                unloadSprite(lruIndex);
            } else {
                Serial.println("ERROR: No sprite slots available");
                return 0xFF;
            }
        }
        
        uint8_t spriteId = loadedSpriteCount++;
        OptimizedSprite& sprite = sprites[spriteId];
        
        // Copy header
        memcpy(&sprite.header, spriteData, sizeof(OptimizedSpriteHeader));
        
        // Allocate and copy pixel data
        sprite.pixelData = (uint8_t*)malloc(sprite.header.dataSize);
        if (!sprite.pixelData) {
            Serial.println("ERROR: Failed to allocate sprite data");
            loadedSpriteCount--;
            return 0xFF;
        }
        
        memcpy(sprite.pixelData, spriteData + sizeof(OptimizedSpriteHeader), sprite.header.dataSize);
        sprite.loaded = true;
        sprite.lastUsed = millis();
        
        Serial.print("Sprite loaded: ");
        Serial.print(spriteId);
        Serial.print(" (");
        Serial.print(sprite.header.width);
        Serial.print("x");
        Serial.print(sprite.header.height);
        Serial.print(", ");
        Serial.print(sprite.header.dataSize);
        Serial.println(" bytes)");
        
        return spriteId;
    }
    
    // Add sprite to rendering
    bool addSprite(uint8_t spriteId, OptimizedLayer layer, int16_t x, int16_t y, uint8_t priority = 128) {
        if (activeSpriteCount >= MAX_SPRITES_ACTIVE || spriteId >= loadedSpriteCount) {
            return false;
        }
        
        if (!sprites[spriteId].loaded) {
            return false;
        }
        
        uint8_t instanceId = activeSpriteCount++;
        SpriteInstance& instance = activeSprites[instanceId];
        
        instance.spriteId = spriteId;
        instance.layer = layer;
        instance.frame = 0;
        instance.priority = priority;
        instance.x = x;
        instance.y = y;
        instance.flags = 0x01; // Visible
        
        // Add to layer
        if (layerSpriteCount[layer] < MAX_SPRITES_ACTIVE) {
            layerSprites[layer][layerSpriteCount[layer]++] = instanceId;
        }
        
        // Mark affected tiles as dirty
        markTilesDirty(x, y, sprites[spriteId].header.width, sprites[spriteId].header.height);
        
        return true;
    }
    
    // Tile-based rendering
    void renderFrame() {
        frameStartTime = micros();
        spritesRendered = 0;
        
        display->startWrite();
        
        // Render only dirty tiles
        for (int ty = 0; ty < 6; ty++) {  // 172/32 = 5.4, round up to 6
            for (int tx = 0; tx < 10; tx++) { // 320/32 = 10
                if (tileCtx.tileDirty[tx][ty]) {
                    renderTile(tx, ty);
                    tileCtx.tileDirty[tx][ty] = false;
                }
            }
        }
        
        display->endWrite();
        
        renderTime = micros() - frameStartTime;
        tileCtx.frameCount++;
    }
    
    // Render single 32x32 tile
    void renderTile(uint8_t tileX, uint8_t tileY) {
        int16_t screenX = tileX * TILE_SIZE;
        int16_t screenY = tileY * TILE_SIZE;
        
        // Clear tile buffer
        memset(tileCtx.tileBuffer, 0, TILE_SIZE * TILE_SIZE * 2);
        
        // Render layers in order
        for (int layer = 0; layer < LAYER_COUNT; layer++) {
            renderLayerToTile(layer, tileX, tileY);
        }
        
        // Output tile to display
        display->setAddrWindow(screenX, screenY, TILE_SIZE, TILE_SIZE);
        display->writePixels(tileCtx.tileBuffer, TILE_SIZE * TILE_SIZE);
        
        spritesRendered += layerSpriteCount[layer];
    }
    
    // Render layer sprites to tile buffer
    void renderLayerToTile(uint8_t layer, uint8_t tileX, uint8_t tileY) {
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
            
            OptimizedSprite& sprite = sprites[instance.spriteId];
            if (!sprite.loaded) continue;
            
            // Quick bounds check - skip if sprite doesn't intersect tile
            if (instance.x >= tileEndX || instance.x + sprite.header.width <= tileStartX ||
                instance.y >= tileEndY || instance.y + sprite.header.height <= tileStartY) {
                continue;
            }
            
            // Render sprite to tile buffer
            renderSpriteToTile(instance, sprite, tileStartX, tileStartY);
            sprites[instance.spriteId].lastUsed = millis();
        }
    }
    
    // Render sprite to tile buffer (clipped)
    void renderSpriteToTile(const SpriteInstance& instance, const OptimizedSprite& sprite, 
                           int16_t tileStartX, int16_t tileStartY) {
        
        int16_t spriteX = instance.x - tileStartX;
        int16_t spriteY = instance.y - tileStartY;
        
        // Calculate clipping
        int16_t startX = max(0, -spriteX);
        int16_t startY = max(0, -spriteY);
        int16_t endX = min((int16_t)sprite.header.width, TILE_SIZE - spriteX);
        int16_t endY = min((int16_t)sprite.header.height, TILE_SIZE - spriteY);
        
        // Render pixels
        for (int16_t y = startY; y < endY; y++) {
            for (int16_t x = startX; x < endX; x++) {
                uint8_t pixelIndex = y * sprite.header.width + x;
                uint8_t colorIndex = sprite.pixelData[pixelIndex];
                
                // Skip transparent pixels
                if (colorIndex == 0) continue;
                
                // Get color from small LUT
                uint8_t lutX = colorIndex % SMALL_LUT_SIZE;
                uint8_t lutY = colorIndex / SMALL_LUT_SIZE;
                uint16_t color = colorLUT[lutY * SMALL_LUT_SIZE + lutX];
                
                // Write to tile buffer
                int16_t bufferX = spriteX + x;
                int16_t bufferY = spriteY + y;
                if (bufferX >= 0 && bufferX < TILE_SIZE && bufferY >= 0 && bufferY < TILE_SIZE) {
                    tileCtx.tileBuffer[bufferY * TILE_SIZE + bufferX] = color;
                }
            }
        }
    }
    
    // Mark tiles as dirty for redrawing
    void markTilesDirty(int16_t x, int16_t y, uint8_t width, uint8_t height) {
        int16_t startTileX = x / TILE_SIZE;
        int16_t startTileY = y / TILE_SIZE;
        int16_t endTileX = (x + width + TILE_SIZE - 1) / TILE_SIZE;
        int16_t endTileY = (y + height + TILE_SIZE - 1) / TILE_SIZE;
        
        for (int16_t ty = startTileY; ty <= endTileY && ty < 6; ty++) {
            for (int16_t tx = startTileX; tx <= endTileX && tx < 10; tx++) {
                if (tx >= 0 && ty >= 0) {
                    tileCtx.tileDirty[tx][ty] = true;
                }
            }
        }
    }
    
    // Clear all sprites
    void clearSprites() {
        activeSpriteCount = 0;
        memset(layerSpriteCount, 0, sizeof(layerSpriteCount));
        
        // Mark all tiles dirty
        memset(tileCtx.tileDirty, true, sizeof(tileCtx.tileDirty));
    }
    
    // Find least recently used sprite for eviction
    uint8_t findLRUSprite() {
        uint32_t oldestTime = UINT32_MAX;
        uint8_t lruIndex = 0xFF;
        
        for (uint8_t i = 0; i < loadedSpriteCount; i++) {
            if (sprites[i].loaded && sprites[i].lastUsed < oldestTime) {
                oldestTime = sprites[i].lastUsed;
                lruIndex = i;
            }
        }
        
        return lruIndex;
    }
    
    // Unload sprite to free memory
    void unloadSprite(uint8_t spriteId) {
        if (spriteId >= loadedSpriteCount || !sprites[spriteId].loaded) return;
        
        OptimizedSprite& sprite = sprites[spriteId];
        if (sprite.pixelData) {
            free(sprite.pixelData);
            sprite.pixelData = nullptr;
        }
        sprite.loaded = false;
        
        Serial.print("Sprite unloaded: ");
        Serial.println(spriteId);
    }
    
    // Performance monitoring
    uint32_t getRenderTimeUs() const { return renderTime; }
    uint16_t getSpritesRendered() const { return spritesRendered; }
    uint8_t getLoadedSpriteCount() const { return loadedSpriteCount; }
    uint8_t getActiveSpriteCount() const { return activeSpriteCount; }
    
    // Debug methods
    void printStats() {
        Serial.println("=== Optimized Graphics Engine Stats ===");
        Serial.print("Memory usage: ");
        Serial.print(getMemoryUsage());
        Serial.println(" bytes");
        Serial.print("Loaded sprites: ");
        Serial.print(loadedSpriteCount);
        Serial.print("/");
        Serial.println(MAX_SPRITES_ACTIVE);
        Serial.print("Active sprites: ");
        Serial.println(activeSpriteCount);
        Serial.print("Last render time: ");
        Serial.print(renderTime);
        Serial.println(" us");
        Serial.print("Sprites rendered: ");
        Serial.println(spritesRendered);
        Serial.println("=======================================");
    }
    
    ~OptimizedGraphicsEngine() {
        // Clean up
        if (tileCtx.tileBuffer) {
            free(tileCtx.tileBuffer);
        }
        
        for (uint8_t i = 0; i < loadedSpriteCount; i++) {
            if (sprites[i].loaded && sprites[i].pixelData) {
                free(sprites[i].pixelData);
            }
        }
    }
};

// Memory usage comparison:
// Original: ~250KB+ (frame buffer + depth buffer + LUT + vectors)
// Optimized: ~15KB base + sprite data (typically <32KB total)
// Savings: ~200KB+ available for actual game content!
