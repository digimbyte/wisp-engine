// engine/wisp_sprite_layers.h - ESP32-C6/S3 Sprite Layer System using ESP-IDF
// Depth-sorted sprite rendering with memory optimization for ESP32
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include "../../utils/math/math.h"
#include <string>

// Forward declarations
namespace WispEngine {
    namespace Graphics {
        class GraphicsEngine;
    }
}
using GraphicsEngine = WispEngine::Graphics::GraphicsEngine;

// Sprite layer definitions (8 total layers)
enum WispSpriteLayer {
    LAYER_0_GRADIENTS = 0,      // Bottom layer - gradients, simple fills
    LAYER_1_BACKGROUNDS = 1,    // Background sprites only (static, tiling)
    LAYER_2_GAME_BACK = 2,      // Game sprites - back layer
    LAYER_3_GAME_MID = 3,       // Game sprites - middle layer  
    LAYER_4_GAME_FRONT = 4,     // Game sprites - front layer
    LAYER_5_GAME_TOP = 5,       // Game sprites - top layer
    LAYER_6_EFFECTS = 6,        // Effects, particles, animations
    LAYER_7_UI = 7,             // UI elements, HUD
    LAYER_8_TEXT = 8,           // Text and overlays (top layer)
    
    WISP_LAYER_COUNT = 9
};

// Sprite types with specific behaviors per layer
enum WispSpriteType {
    SPRITE_TYPE_GRADIENT,       // Layer 0 only - simple gradients
    SPRITE_TYPE_BACKGROUND,     // Layer 1 only - static backgrounds with tiling
    SPRITE_TYPE_STANDARD,       // Layers 2-6 - regular sprites with animations
    SPRITE_TYPE_UI,             // Layer 7 - UI elements
    SPRITE_TYPE_TEXT            // Layer 8 - text rendering
};

// Tiling modes for background sprites
enum WispTilingMode {
    TILE_NONE,                  // No tiling (stretch or clip)
    TILE_REPEAT,                // Repeat infinitely
    TILE_REPEAT_X,              // Repeat horizontally only
    TILE_REPEAT_Y,              // Repeat vertically only
    TILE_MIRROR,                // Mirror at edges
    TILE_MIRROR_X,              // Mirror horizontally
    TILE_MIRROR_Y               // Mirror vertically
};

// Sprite slicing for 9-patch UI elements
struct WispSpriteSlice {
    uint16_t left, right;       // Left and right slice borders
    uint16_t top, bottom;       // Top and bottom slice borders
    bool enabled;               // Is slicing enabled
    
    WispSpriteSlice() : left(0), right(0), top(0), bottom(0), enabled(false) {}
    WispSpriteSlice(uint16_t l, uint16_t r, uint16_t t, uint16_t b) 
        : left(l), right(r), top(t), bottom(b), enabled(true) {}
};

// Animation frame data
struct WispAnimationFrame {
    uint16_t frameIndex;        // Frame index in sprite sheet
    uint16_t duration;          // Duration in milliseconds
    int16_t offsetX, offsetY;   // Frame-specific offset
    uint8_t alpha;              // Frame-specific alpha
    
    WispAnimationFrame() : frameIndex(0), duration(100), offsetX(0), offsetY(0), alpha(255) {}
    WispAnimationFrame(uint16_t frame, uint16_t dur) 
        : frameIndex(frame), duration(dur), offsetX(0), offsetY(0), alpha(255) {}
};

// Animation sequence
struct WispAnimation {
    static const int MAX_ANIMATION_FRAMES = 32;
    WispAnimationFrame frames[MAX_ANIMATION_FRAMES];
    int frameCount;
    bool loop;                  // Should animation loop
    bool pingpong;              // Reverse at end
    bool paused;                // Is animation paused
    uint16_t currentFrame;      // Current frame index
    uint32_t frameStartTime;    // When current frame started
    bool reverse;               // Currently playing in reverse (for pingpong)
    
    WispAnimation() : frameCount(0), loop(true), pingpong(false), paused(false), 
                     currentFrame(0), frameStartTime(0), reverse(false) {}
};

// Depth mask for multi-layer sprites
struct WispDepthMask {
    uint8_t layerMask;          // Bitmask of layers (bit 0 = layer 0, etc.)
    uint8_t depthValues[WISP_LAYER_COUNT]; // Depth value per layer
    bool enabled;
    
    WispDepthMask() : layerMask(0), enabled(false) {
        memset(depthValues, 5, sizeof(depthValues)); // Default depth 5
    }
    
    // Set which layers this sprite appears on
    void setLayers(const WispSpriteLayer* layers, int count) {
        layerMask = 0;
        for (int i = 0; i < count; i++) {
            layerMask |= (1 << layers[i]);
        }
        enabled = true;
    }
    
    // Set depth for a specific layer
    void setDepth(WispSpriteLayer layer, uint8_t depth) {
        if (layer < WISP_LAYER_COUNT) {
            depthValues[layer] = depth;
        }
    }
    
    // Check if sprite appears on layer
    bool isOnLayer(WispSpriteLayer layer) const {
        return enabled && (layerMask & (1 << layer));
    }
};

// Main sprite instance with layer system support
struct WispLayeredSprite {
    uint16_t spriteId;          // Reference to loaded sprite data
    WispSpriteType type;        // Type determines behavior
    WispSpriteLayer primaryLayer; // Primary layer (for type validation)
    
    // Position and transform
    float x, y;                 // World position
    float scaleX, scaleY;       // Scale factors
    float rotation;             // Rotation in degrees
    uint8_t alpha;              // Global alpha
    bool visible;               // Is sprite visible
    
    // Layer and depth
    WispDepthMask depthMask;    // Multi-layer depth information
    uint8_t renderPriority;     // Priority within layer (0=back, 255=front)
    
    // Background-specific (Layer 1)
    WispTilingMode tilingMode;  // How to tile the background
    float scrollX, scrollY;     // Background scroll offset
    float parallaxX, parallaxY; // Parallax multipliers (1.0 = normal)
    
    // Animation
    WispAnimation animation;    // Animation data
    bool hasAnimation;          // Does this sprite animate
    
    // UI-specific (Layer 7)
    WispSpriteSlice slice;      // 9-patch slicing
    float targetWidth, targetHeight; // Target size for sliced sprites
    
    // Runtime state
    bool isDirty;               // Needs re-rendering
    uint32_t lastUpdateTime;    // Last time sprite was updated
    
    WispLayeredSprite() : 
        spriteId(0xFFFF), type(SPRITE_TYPE_STANDARD), primaryLayer(LAYER_3_GAME_MID),
        x(0), y(0), scaleX(1.0f), scaleY(1.0f), rotation(0), alpha(255), visible(true),
        renderPriority(128), tilingMode(TILE_NONE), scrollX(0), scrollY(0), 
        parallaxX(1.0f), parallaxY(1.0f), hasAnimation(false), 
        targetWidth(0), targetHeight(0), isDirty(true), lastUpdateTime(0) {}
};

// Layer rendering system
class WispSpriteLayerSystem {
private:
    GraphicsEngine* graphics;
    
    // Sprites organized by layer for efficient rendering
    static const int MAX_SPRITES_PER_LAYER = 50;
    WispLayeredSprite* layers[WISP_LAYER_COUNT][MAX_SPRITES_PER_LAYER];
    int layerCounts[WISP_LAYER_COUNT];
    
    // Camera/viewport for parallax and scrolling
    float cameraX, cameraY;
    float viewportWidth, viewportHeight;
    
    // Performance tracking
    uint32_t spritesRendered;
    uint32_t layersRendered;
    
    // Layer-specific settings
    bool layerEnabled[WISP_LAYER_COUNT];
    uint8_t layerAlpha[WISP_LAYER_COUNT];
    
public:
    WispSpriteLayerSystem(GraphicsEngine* gfx) : 
        graphics(gfx), cameraX(0), cameraY(0), viewportWidth(320), viewportHeight(240),
        spritesRendered(0), layersRendered(0) {
        
        // Initialize layer counts and settings
        for (int i = 0; i < WISP_LAYER_COUNT; i++) {
            layerCounts[i] = 0;    // Initialize all layer counts to 0
            layerEnabled[i] = true;
            layerAlpha[i] = 255;
        }
    }
    
    // Sprite management
    bool addSprite(WispLayeredSprite* sprite);
    bool removeSprite(WispLayeredSprite* sprite);
    void clearLayer(WispSpriteLayer layer);
    void clearAllSprites();
    
    // Rendering
    void renderAllLayers();
    void renderLayer(WispSpriteLayer layer);
    void renderSprite(WispLayeredSprite* sprite, WispSpriteLayer layer);
    
    // Background rendering (Layer 1)
    void renderBackgroundSprite(WispLayeredSprite* sprite);
    void renderTiledBackground(WispLayeredSprite* sprite);
    
    // UI rendering (Layers 7-8)
    void renderUISprite(WispLayeredSprite* sprite);
    void renderSlicedSprite(WispLayeredSprite* sprite);
    void renderTextSprite(WispLayeredSprite* sprite);
    
    // Animation system
    void updateAnimations(uint32_t deltaTime);
    void updateSpriteAnimation(WispLayeredSprite* sprite, uint32_t deltaTime);
    bool setAnimation(WispLayeredSprite* sprite, const WispAnimationFrame* frames, int frameCount);
    void playAnimation(WispLayeredSprite* sprite, bool loop = true);
    void pauseAnimation(WispLayeredSprite* sprite);
    void stopAnimation(WispLayeredSprite* sprite);
    
    // Camera and viewport
    void setCamera(float x, float y);
    void setCameraSmooth(float x, float y, float smoothing = 0.1f);
    void setViewport(float width, float height);
    Vec2 getCamera() const { return Vec2(cameraX, cameraY); }
    
    // Layer control
    void setLayerEnabled(WispSpriteLayer layer, bool enabled);
    void setLayerAlpha(WispSpriteLayer layer, uint8_t alpha);
    bool isLayerEnabled(WispSpriteLayer layer) const;
    
    // Sprite creation helpers
    WispLayeredSprite* createGradientSprite(float x, float y, float width, float height, 
                                           uint32_t colorTop, uint32_t colorBottom);
    WispLayeredSprite* createBackgroundSprite(uint16_t spriteId, WispTilingMode tiling = TILE_REPEAT);
    WispLayeredSprite* createGameSprite(uint16_t spriteId, WispSpriteLayer layer = LAYER_3_GAME_MID);
    WispLayeredSprite* createUISprite(uint16_t spriteId, float x, float y);
    WispLayeredSprite* createTextSprite(const std::string& text, float x, float y);
    
    // Multi-layer sprites (appear on multiple layers with depth masking)
    void setMultiLayer(WispLayeredSprite* sprite, const WispSpriteLayer* layers, int count);
    void setLayerDepth(WispLayeredSprite* sprite, WispSpriteLayer layer, uint8_t depth);
    
    // Utility
    void sortLayer(WispSpriteLayer layer); // Sort by render priority
    void validateSprite(WispLayeredSprite* sprite);
    uint32_t getSpritesRendered() const { return spritesRendered; }
    uint32_t getLayersRendered() const { return layersRendered; }
    
    // Debug
    void printLayerStats();
    void printSpriteInfo(WispLayeredSprite* sprite);
    
private:
    // Internal rendering helpers
    void renderGradient(WispLayeredSprite* sprite);
    void renderStandardSprite(WispLayeredSprite* sprite, WispSpriteLayer layer);
    Vec2 applyParallax(WispLayeredSprite* sprite, float worldX, float worldY);
    Vec2 worldToScreen(float worldX, float worldY);
    bool isInViewport(float x, float y, float width, float height);
    
    // Animation helpers
    WispAnimationFrame* getCurrentFrame(WispLayeredSprite* sprite);
    void advanceAnimation(WispLayeredSprite* sprite);
    
    // Tiling calculations
    void calculateTileRegion(WispLayeredSprite* sprite, int& startTileX, int& startTileY, 
                            int& endTileX, int& endTileY, int& tileWidth, int& tileHeight);
};

// Global layer system instance
extern WispSpriteLayerSystem* g_LayerSystem;

// Helper macros for layer validation
#define VALIDATE_LAYER(layer) ((layer) >= 0 && (layer) < WISP_LAYER_COUNT)
#define VALIDATE_SPRITE_TYPE(sprite, layer) \
    (((sprite)->type == SPRITE_TYPE_GRADIENT && (layer) == LAYER_0_GRADIENTS) || \
     ((sprite)->type == SPRITE_TYPE_BACKGROUND && (layer) == LAYER_1_BACKGROUNDS) || \
     ((sprite)->type == SPRITE_TYPE_STANDARD && (layer) >= LAYER_2_GAME_BACK && (layer) <= LAYER_6_EFFECTS) || \
     ((sprite)->type == SPRITE_TYPE_UI && (layer) == LAYER_7_UI) || \
     ((sprite)->type == SPRITE_TYPE_TEXT && (layer) == LAYER_8_TEXT))

// Layer naming for debugging
const char* getLayerName(WispSpriteLayer layer);
const char* getSpriteTypeName(WispSpriteType type);
const char* getTilingModeName(WispTilingMode mode);
