// Fallback Asset System
// Provides default assets from compiled ROM when game assets are missing
// ESP32 embedded system - assets compiled into binary with magic numbers

#pragma once
#include <stdint.h>
#include "../app/wisp_runtime_loader.h"
#include "../../system/asset_types.h"

using namespace WispAssets;

namespace WispEngine {
namespace Graphics {

// Expanded Sprite Art Definitions
enum SpriteArtType {
    ART_SPLASH,     // Backgrounds that stretch over canvas/panel, can render front/behind, depth mechanic
    ART_ENTITY,     // Animated items (torch, NPC character with multiple animations)
    ART_TILE,       // World tiles, static
    ART_UI          // Center repeats seamlessly, supports 9-segmentation via 'width'
};

// Sprite header structure (matches .art format)
struct FallbackSpriteHeader {
    uint32_t magic;              // 'WART' format magic
    uint32_t format;             // Sprite format type
    uint16_t width, height;      // Individual sprite/frame dimensions
    uint16_t frameCount;         // Number of frames (1 for static)
    
    // Animation layout (for animated sprites)
    uint8_t cols, rows;          // Frame grid layout (cols x rows = frameCount)
    uint8_t animType;            // Animation type/behavior
    uint8_t defaultFPS;          // Suggested frames per second
    
    // Tiling info (for tiled sprites)
    uint8_t tileFlags;           // 9-segment, repeat modes
    uint8_t segmentData[9];      // 9-segment tile regions (if applicable)
    
    // Depth info (for depth-enabled sprites)
    uint16_t depthOffset;        // Offset to depth map data
    uint16_t depthSize;          // Size of depth map
    
    uint8_t reserved[8];         // Future expansion
} __attribute__((packed));

// Animation behavior types
enum AnimationBehavior {
    ANIM_LOOP,                   // Loop continuously
    ANIM_PING_PONG,             // Forward then reverse
    ANIM_ONCE,                  // Play once and stop
    ANIM_TRIGGERED,             // Wait for trigger to advance
    ANIM_RANDOM,                // Random frame selection
    ANIM_SEQUENCE_BASED         // Custom sequence pattern
};

// Built-in fallback assets (compiled into ROM)
struct FallbackAssetEntry {
    SpriteArtType type;
    const char* name;
    const uint8_t* data;    // Points to ROM data
    uint32_t dataSize;
    const char* description;
};

// Simple fallback system for ESP32
class FallbackAssetSystem {
private:
    static const uint8_t MAX_FALLBACKS = 12;
    const FallbackAssetEntry* fallbacks[MAX_FALLBACKS];
    uint8_t fallbackCount;
    
public:
    FallbackAssetSystem();
    
    // Initialize built-in fallback assets (ROM references)
    void initialize();
    
    // Find fallback asset by name or type
    const uint8_t* getFallbackAsset(const String& assetName, uint32_t& dataSize);
    const uint8_t* getFallbackByType(SpriteArtType type, uint32_t& dataSize);
    
    // Check if fallback exists
    bool hasFallback(const String& assetName);
    
private:
    // Simple name matching for ESP32
    bool matches(const String& assetName, const char* fallbackName);
    bool containsKeyword(const String& assetName, const char* keyword);
};

// Global instance
extern FallbackAssetSystem fallbackAssets;

// Built-in fallback asset data (in ROM)
extern const uint8_t FALLBACK_SPLASH_DEFAULT[];
extern const uint8_t FALLBACK_ENTITY_PLACEHOLDER[];
extern const uint8_t FALLBACK_TILE_BASIC[];
extern const uint8_t FALLBACK_UI_BUTTON[];
extern const uint8_t FALLBACK_UI_PANEL[];

// Magic numbers for sprite art types
#define MAGIC_SPLASH 0x4C505357  // 'WSPL'
#define MAGIC_ENTITY 0x544E4557  // 'WENT' 
#define MAGIC_TILE   0x4C495457  // 'WTIL'
#define MAGIC_UI     0x5F495557  // 'WUI_'

} // namespace Graphics
} // namespace WispEngine
