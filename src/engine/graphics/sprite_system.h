// engine/optimized_sprite_system.h - ESP32-C6/S3 Sprite System using ESP-IDF
// Native ESP32 implementation with LUT palette optimization
#pragma once
#include "../engine_common.h"
#include "sprite_system_config.h"

// Configurable sprite system for ESP32-C6
// Adapts to memory profile: Game Boy style (minimal) to modern indie game (full features)
// Memory allocation automatically calculated based on profile selection

// Simplified sprite types
enum SpriteType {
    SPRITE_STATIC,       // Non-animated sprite
    SPRITE_ANIMATED,     // Simple frame-based animation
    SPRITE_TILED,        // Background tile (layer 0 only)
    SPRITE_UI            // UI element (layer 2-3 only)
};

// Simple animation (max 8 frames to save memory)
struct SimpleAnimation {
    uint8_t frameCount;      // Number of frames (1-8)
    uint8_t frameDuration;   // Duration per frame in 60fps ticks
    uint8_t currentFrame;    // Current frame index
    uint8_t frameTimer;      // Countdown timer
    bool loop;               // Should animation loop
    bool playing;            // Is animation playing
    uint8_t reserved[2];     // Padding for alignment
};

// Lightweight sprite instance
struct SimpleSpriteInstance {
    uint8_t spriteId;        // Which loaded sprite
    uint8_t layer;           // Which layer (0-3) 
    int16_t x, y;            // Position
    uint8_t priority;        // Render order within layer
    uint8_t flags;           // Visible, flipped, etc.
    SimpleAnimation anim;    // Animation state
    uint8_t reserved[2];     // Padding
};

// Background tile for layer 0
struct BackgroundTile {
    uint8_t spriteId;        // Tile sprite
    int16_t scrollX, scrollY; // Scroll offset
    uint8_t repeatX, repeatY; // Repeat pattern
    uint8_t flags;           // Mirror, wrap modes
    uint8_t reserved[3];     // Padding
};

// Simple sprite management system
class OptimizedSpriteSystem {
private:
    OptimizedGraphicsEngine* graphics;
    
    // Fixed-size arrays for optimal performance
    SimpleSpriteInstance sprites[MAX_SPRITES_ACTIVE];
    uint8_t spriteCount;
    
    // Layer organization
    uint8_t layerCounts[LAYER_COUNT];
    uint8_t layerSprites[LAYER_COUNT][MAX_SPRITES_ACTIVE];
    
    // Background tiling (layer 0)
    BackgroundTile background;
    bool backgroundActive;
    
    // Camera for scrolling
    int16_t cameraX, cameraY;
    
    // Performance tracking
    uint32_t updateTime;
    uint8_t animationsUpdated;
    
public:
    OptimizedSpriteSystem(OptimizedGraphicsEngine* gfx) : 
        graphics(gfx), spriteCount(0), backgroundActive(false),
        cameraX(0), cameraY(0), updateTime(0), animationsUpdated(0) {
        
        memset(sprites, 0, sizeof(sprites));
        memset(layerCounts, 0, sizeof(layerCounts));
        memset(layerSprites, 0xFF, sizeof(layerSprites));
        memset(&background, 0, sizeof(background));
    }
    
    // Get memory usage
    size_t getMemoryUsage() const {
        return sizeof(OptimizedSpriteSystem);
    }
    
    // Add sprite to system
    uint8_t addSprite(uint8_t spriteId, OptimizedLayer layer, int16_t x, int16_t y, 
                     SpriteType type = SPRITE_STATIC, uint8_t priority = 128) {
        
        if (spriteCount >= MAX_SPRITES_ACTIVE) {
            // Use WISP debug system instead of Serial
            return 0xFF;
        }
        
        if (layer >= LAYER_COUNT) {
            // Use WISP debug system instead of Serial
            return 0xFF;
        }
        
        uint8_t instanceId = spriteCount++;
        SimpleSpriteInstance& sprite = sprites[instanceId];
        
        sprite.spriteId = spriteId;
        sprite.layer = layer;
        sprite.x = x;
        sprite.y = y;
        sprite.priority = priority;
        sprite.flags = 0x01; // Visible
        
        // Initialize animation
        sprite.anim.frameCount = 1;
        sprite.anim.frameDuration = 60; // 1 second at 60fps
        sprite.anim.currentFrame = 0;
        sprite.anim.frameTimer = 0;
        sprite.anim.loop = true;
        sprite.anim.playing = false;
        
        // Add to layer
        if (layerCounts[layer] < MAX_SPRITES_ACTIVE) {
            layerSprites[layer][layerCounts[layer]++] = instanceId;
        }
        
        // Debug logging removed for ESP-IDF compatibility
        // Use WISP debug system for logging when needed
        
        return instanceId;
    }
    
    // Remove sprite
    bool removeSprite(uint8_t instanceId) {
        if (instanceId >= spriteCount) return false;
        
        SimpleSpriteInstance& sprite = sprites[instanceId];
        uint8_t layer = sprite.layer;
        
        // Remove from layer
        for (uint8_t i = 0; i < layerCounts[layer]; i++) {
            if (layerSprites[layer][i] == instanceId) {
                // Shift remaining sprites down
                for (uint8_t j = i; j < layerCounts[layer] - 1; j++) {
                    layerSprites[layer][j] = layerSprites[layer][j + 1];
                }
                layerCounts[layer]--;
                break;
            }
        }
        
        // Mark as removed (don't compact array to avoid ID changes)
        sprite.flags = 0x00; // Not visible
        
        return true;
    }
    
    // Set background tile
    void setBackground(uint8_t spriteId, bool repeatX = true, bool repeatY = true) {
        background.spriteId = spriteId;
        background.scrollX = 0;
        background.scrollY = 0;
        background.repeatX = repeatX ? 1 : 0;
        background.repeatY = repeatY ? 1 : 0;
        background.flags = 0;
        backgroundActive = true;
        
        Serial.print("Background set: sprite=");
        Serial.println(spriteId);
    }
    
    // Set sprite animation
    bool setAnimation(uint8_t instanceId, uint8_t frameCount, uint8_t frameDuration, bool loop = true) {
        if (instanceId >= spriteCount || frameCount > 8 || frameCount == 0) {
            return false;
        }
        
        SimpleSpriteInstance& sprite = sprites[instanceId];
        sprite.anim.frameCount = frameCount;
        sprite.anim.frameDuration = frameDuration;
        sprite.anim.currentFrame = 0;
        sprite.anim.frameTimer = frameDuration;
        sprite.anim.loop = loop;
        sprite.anim.playing = false;
        
        return true;
    }
    
    // Control animation playback
    void playAnimation(uint8_t instanceId) {
        if (instanceId < spriteCount) {
            sprites[instanceId].anim.playing = true;
            sprites[instanceId].anim.frameTimer = sprites[instanceId].anim.frameDuration;
        }
    }
    
    void pauseAnimation(uint8_t instanceId) {
        if (instanceId < spriteCount) {
            sprites[instanceId].anim.playing = false;
        }
    }
    
    void stopAnimation(uint8_t instanceId) {
        if (instanceId < spriteCount) {
            sprites[instanceId].anim.playing = false;
            sprites[instanceId].anim.currentFrame = 0;
            sprites[instanceId].anim.frameTimer = sprites[instanceId].anim.frameDuration;
        }
    }
    
    // Move sprite
    void moveSprite(uint8_t instanceId, int16_t x, int16_t y) {
        if (instanceId < spriteCount) {
            sprites[instanceId].x = x;
            sprites[instanceId].y = y;
        }
    }
    
    // Set sprite visibility
    void setVisible(uint8_t instanceId, bool visible) {
        if (instanceId < spriteCount) {
            if (visible) {
                sprites[instanceId].flags |= 0x01;
            } else {
                sprites[instanceId].flags &= ~0x01;
            }
        }
    }
    
    // Set camera position
    void setCamera(int16_t x, int16_t y) {
        cameraX = x;
        cameraY = y;
        
        // Update background scroll
        if (backgroundActive) {
            background.scrollX = -cameraX / 2; // Parallax effect
            background.scrollY = -cameraY / 2;
        }
    }
    
    // Update animations and system
    void update() {
        uint32_t startTime = get_micros();
        animationsUpdated = 0;
        
        // Update all animations
        for (uint8_t i = 0; i < spriteCount; i++) {
            SimpleSpriteInstance& sprite = sprites[i];
            
            if (!(sprite.flags & 0x01)) continue; // Not visible
            if (!sprite.anim.playing) continue;   // Not playing
            if (sprite.anim.frameCount <= 1) continue; // No animation
            
            // Update frame timer
            if (sprite.anim.frameTimer > 0) {
                sprite.anim.frameTimer--;
            } else {
                // Advance frame
                sprite.anim.currentFrame++;
                if (sprite.anim.currentFrame >= sprite.anim.frameCount) {
                    if (sprite.anim.loop) {
                        sprite.anim.currentFrame = 0;
                    } else {
                        sprite.anim.currentFrame = sprite.anim.frameCount - 1;
                        sprite.anim.playing = false;
                    }
                }
                sprite.anim.frameTimer = sprite.anim.frameDuration;
                animationsUpdated++;
            }
        }
        
        updateTime = get_micros() - startTime;
    }
    
    // Render all sprites
    void render() {
        // Clear previous frame
        graphics->clearSprites();
        
        // Render background first (layer 0)
        if (backgroundActive) {
            renderBackground();
        }
        
        // Render layers 1-3 (game, UI, text)
        for (uint8_t layer = 1; layer < LAYER_COUNT; layer++) {
            renderLayer(layer);
        }
        
        // Trigger graphics rendering
        graphics->renderFrame();
    }
    
    // Render background layer
    void renderBackground() {
        if (!backgroundActive) return;
        
        // Simple tiled background rendering
        // For now, just place one tile - full tiling would be more complex
        int16_t bgX = background.scrollX;
        int16_t bgY = background.scrollY;
        
        graphics->addSprite(background.spriteId, LAYER_BACKGROUND, bgX, bgY, 0);
    }
    
    // Render specific layer
    void renderLayer(uint8_t layer) {
        if (layer >= LAYER_COUNT) return;
        
        // Sort sprites by priority (simple bubble sort - fine for small counts)
        sortLayerByPriority(layer);
        
        // Add sprites to graphics engine
        for (uint8_t i = 0; i < layerCounts[layer]; i++) {
            uint8_t instanceId = layerSprites[layer][i];
            if (instanceId >= spriteCount) continue;
            
            SimpleSpriteInstance& sprite = sprites[instanceId];
            if (!(sprite.flags & 0x01)) continue; // Not visible
            
            // Apply camera transform for game layer
            int16_t renderX = sprite.x;
            int16_t renderY = sprite.y;
            if (layer == LAYER_GAME) {
                renderX -= cameraX;
                renderY -= cameraY;
            }
            
            // Use animation frame
            uint8_t spriteId = sprite.spriteId + sprite.anim.currentFrame;
            
            graphics->addSprite(spriteId, (OptimizedLayer)layer, renderX, renderY, sprite.priority);
        }
    }
    
    // Simple priority sorting
    void sortLayerByPriority(uint8_t layer) {
        if (layerCounts[layer] <= 1) return;
        
        // Bubble sort (sufficient for small arrays)
        for (uint8_t i = 0; i < layerCounts[layer] - 1; i++) {
            for (uint8_t j = 0; j < layerCounts[layer] - 1 - i; j++) {
                uint8_t id1 = layerSprites[layer][j];
                uint8_t id2 = layerSprites[layer][j + 1];
                
                if (id1 < spriteCount && id2 < spriteCount) {
                    if (sprites[id1].priority > sprites[id2].priority) {
                        // Swap
                        layerSprites[layer][j] = id2;
                        layerSprites[layer][j + 1] = id1;
                    }
                }
            }
        }
    }
    
    // Clear all sprites
    void clearAllSprites() {
        spriteCount = 0;
        memset(layerCounts, 0, sizeof(layerCounts));
        backgroundActive = false;
    }
    
    // Get sprite info
    SimpleSpriteInstance* getSprite(uint8_t instanceId) {
        if (instanceId < spriteCount) {
            return &sprites[instanceId];
        }
        return nullptr;
    }
    
    // Debug and stats
    void printStats() {
        Serial.println("=== Optimized Sprite System Stats ===");
        Serial.print("Memory usage: ");
        Serial.print(getMemoryUsage());
        Serial.println(" bytes");
        Serial.print("Active sprites: ");
        Serial.print(spriteCount);
        Serial.print("/");
        Serial.println(MAX_SPRITES_ACTIVE);
        
        for (uint8_t layer = 0; layer < LAYER_COUNT; layer++) {
            Serial.print("Layer ");
            Serial.print(layer);
            Serial.print(": ");
            Serial.print(layerCounts[layer]);
            Serial.println(" sprites");
        }
        
        Serial.print("Background active: ");
        Serial.println(backgroundActive ? "YES" : "NO");
        Serial.print("Camera: (");
        Serial.print(cameraX);
        Serial.print(", ");
        Serial.print(cameraY);
        Serial.println(")");
        Serial.print("Last update time: ");
        Serial.print(updateTime);
        Serial.println(" us");
        Serial.print("Animations updated: ");
        Serial.println(animationsUpdated);
        Serial.println("====================================");
    }
};

// Usage example:
/*
// Note: Don't create global instances - use pointers/heap allocation instead
// OptimizedGraphicsEngine graphics;  // COMMENTED OUT - causes massive memory usage!
OptimizedSpriteSystem sprites(&graphics);

// Initialize
graphics.init(display);

// Load sprites
uint8_t playerSprite = graphics.loadSprite(playerData);
uint8_t bgTile = graphics.loadSprite(tileData);

// Setup scene
sprites.setBackground(bgTile);
uint8_t player = sprites.addSprite(playerSprite, LAYER_GAME, 100, 100);
sprites.setAnimation(player, 4, 15, true); // 4 frames, 15 ticks each
sprites.playAnimation(player);

// Game loop
while (true) {
    sprites.update();
    sprites.render();
    delay_ms(16); // ~60fps
}
*/
