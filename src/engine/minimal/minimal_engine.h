// minimal_engine.h - GBA-inspired minimal engine for ESP32-C6
// Enhanced from ultra-lean to "minified GBA" feature set
#pragma once
#include "../engine_common.h"

namespace WispEngine {
namespace Minimal {

// === GBA-INSPIRED CONFIGURATION ===
// Graphics limits (inspired by GBA capabilities)
#define MAX_SPRITE_SLOTS 64         // GBA had 128 OAM entries, we use 64
#define MAX_ACTIVE_SPRITES 32       // Increased for more complex games
#define SPRITE_SIZE 16              // Fixed 16x16 sprites (GBA had multiple sizes)
#define SPRITE_SLOT_SIZE 260        // Includes metadata + 16x16 sprite data

// Background system (GBA-style)
#define MAX_BACKGROUND_LAYERS 2     // Simplified from GBA's 4 layers
#define TILE_SIZE 8                 // 8x8 pixel tiles like GBA
#define TILEMAP_WIDTH 32            // 32x32 tile tilemap (256 pixels)
#define TILEMAP_HEIGHT 24           // 24 tiles high (192 pixels)
#define MAX_TILES 128               // Reduced tile count for memory efficiency

// Audio system (GBA-style channels)
#define MAX_AUDIO_CHANNELS 4        // GBA had 4 audio channels
#define AUDIO_BUFFER_SIZE 1024      // Increased for better quality
#define AUDIO_SAMPLE_RATE 22050     // Still reduced to save memory

// Memory pools
#define COLOR_PALETTE_SIZE 256      // 256-color palette like GBA

struct SpriteSlot {
    uint16_t spriteId;          // Which sprite is loaded (0 = empty)
    uint8_t spriteData[256];    // Cached sprite pixels (16x16 max)
    bool inUse;                 // Slot occupied
    uint32_t lastAccess;        // For LRU eviction
}; // Total: 260 bytes per slot

struct SpriteInstance {
    uint8_t slotId;             // Which sprite slot to use
    int16_t x, y;               // Position
    uint8_t scale;              // Size modifier (0-255)
    bool flipX, flipY;          // GBA-style sprite flipping
    uint8_t priority;           // Rendering priority (0-3)
}; // Enhanced for GBA features

// === GBA-STYLE BACKGROUND SYSTEM ===
struct Tile {
    uint8_t tileData[64];       // 8x8 pixels, 1 byte per pixel
    uint8_t paletteOffset;      // Which 16-color sub-palette to use
}; // Total: 65 bytes per tile

struct TilemapEntry {
    uint8_t tileId;             // Which tile to display
    bool flipX, flipY;          // Tile flipping
    uint8_t priority;           // Background priority
}; // Total: 4 bytes per tilemap entry

struct BackgroundLayer {
    TilemapEntry tilemap[TILEMAP_WIDTH * TILEMAP_HEIGHT]; // 32x24 = 768 entries
    int16_t scrollX, scrollY;   // GBA-style scrolling
    bool enabled;               // Layer on/off
    uint8_t priority;           // Layer priority
}; // Total: ~3KB per background layer

// === GBA-STYLE GRAPHICS API ===
class SimpleGraphics {
public:
    // Basic drawing functions
    static void drawPixel(int x, int y, uint16_t color);
    static void drawLine(int x0, int y0, int x1, int y1, uint16_t color);
    static void drawRect(int x, int y, int w, int h, uint16_t color);
    static void fillRect(int x, int y, int w, int h, uint16_t color);
    static void drawText(int x, int y, const char* text, uint16_t color);
    
    // Enhanced sprite system (GBA-style)
    static uint8_t loadSprite(uint16_t spriteId);  // Returns slot ID
    static void drawSprite(uint16_t spriteId, int x, int y, uint8_t scale = 1, 
                          bool flipX = false, bool flipY = false, uint8_t priority = 0);
    static void clearSprites();
    static void setSpriteVisible(uint16_t spriteId, bool visible);
    
    // Background tile system (GBA-inspired)
    static bool loadTile(uint8_t tileId, const uint8_t* tileData, uint8_t paletteOffset = 0);
    static void setTile(uint8_t layer, uint8_t x, uint8_t y, uint8_t tileId, 
                       bool flipX = false, bool flipY = false, uint8_t priority = 0);
    static void scrollBackground(uint8_t layer, int16_t x, int16_t y);
    static void setBackgroundEnabled(uint8_t layer, bool enabled);
    
    // Palette management (GBA-style)
    static void setPaletteColor(uint8_t index, uint16_t color);
    static uint16_t getPaletteColor(uint8_t index);
    static void loadPalette(const uint16_t* palette, uint8_t startIndex = 0, uint8_t count = 255);
    
    // Display management
    static void clear(uint16_t color = 0x0000);
    static void display();
    static void setVBlankCallback(void (*callback)()); // GBA-style V-blank interrupt simulation
    
    // Public stats for debugging
    static uint8_t getTileCount() { return loadedTileCount; }
    
    // Allow Engine to access internals for initialization and memory reporting
    friend class Engine;
    
private:
    // Sprite system
    static SpriteSlot spriteSlots[MAX_SPRITE_SLOTS];        // 64 * 260 = 16.6KB
    static SpriteInstance activeSprites[MAX_ACTIVE_SPRITES]; // 32 * 8 = 256 bytes
    static uint8_t activeSpriteCount;
    static uint8_t nextSlot;
    
    // Background tile system
    static Tile tileSet[MAX_TILES];                         // 128 * 65 = 8.3KB
    static BackgroundLayer backgrounds[MAX_BACKGROUND_LAYERS]; // 2 * 3KB = 6KB
    static uint8_t loadedTileCount;
    
    // Palette system  
    static uint16_t colorPalette[COLOR_PALETTE_SIZE];       // 256 * 2 = 512 bytes
    static uint16_t colorLUT[COLOR_PALETTE_SIZE];           // Alias for compatibility
    
    // V-blank simulation
    static void (*vblankCallback)();
    
    // Internal functions
    static uint8_t findSpriteSlot(uint16_t spriteId);
    static uint8_t getAvailableSlot();
    static void loadSpriteFromFlash(uint16_t spriteId, SpriteSlot* slot);
    static void renderSpriteFromSlot(uint8_t slotId, int x, int y, uint8_t scale, 
                                   bool flipX, bool flipY, uint8_t priority);
    static void renderBackgroundLayer(uint8_t layer);
    static void renderTile(uint8_t tileId, int x, int y, bool flipX, bool flipY);
};

// === MINIMAL STORAGE API ===
class SimpleStorage {
public:
    // Key-value storage only - no complex database
    static bool saveValue(const char* key, const void* data, size_t size);
    static bool loadValue(const char* key, void* data, size_t maxSize);
    static bool hasKey(const char* key);
    static void clearAll();
};

// === GBA-STYLE AUDIO API ===
// Audio configuration - enhanced for GBA-style sound
#ifndef AUDIO_SAMPLE_RATE
#define AUDIO_SAMPLE_RATE 22050     // Still reduced to save memory
#endif

enum AudioChannelType {
    CHANNEL_SQUARE1 = 0,    // Square wave with sweep
    CHANNEL_SQUARE2 = 1,    // Square wave 
    CHANNEL_TRIANGLE = 2,   // Triangle wave
    CHANNEL_NOISE = 3       // Noise channel
};

struct AudioChannel {
    AudioChannelType type;
    bool enabled;
    uint16_t frequency;     // Note frequency
    uint8_t volume;         // 0-15 volume
    uint8_t duty;           // Duty cycle for square waves (0-3)
    uint16_t duration;      // Note duration in frames
    uint16_t envelope;      // Volume envelope settings
    uint16_t sweep;         // Frequency sweep (channel 1 only)
};

class SimpleAudio {
public:
    // GBA-style multi-channel audio
    static bool init();
    static void update();
    
    // Channel control (GBA-style)
    static void playNote(AudioChannelType channel, uint16_t frequency, uint8_t volume, 
                        uint16_t duration, uint8_t duty = 2);
    static void stopChannel(AudioChannelType channel);
    static void setChannelVolume(AudioChannelType channel, uint8_t volume);
    static void setMasterVolume(uint8_t volume);
    
    // Simple sound effects
    static void playTone(uint16_t freq, uint16_t duration); // Legacy compatibility
    static void playBeep();
    static void silence();
    
    // Allow Engine to access internals for memory reporting
    friend class Engine;
    
private:
    // Multi-channel audio system
    static AudioChannel channels[MAX_AUDIO_CHANNELS];      // 4 channels
    static int16_t mixBuffer[AUDIO_BUFFER_SIZE];           // 1024 * 2 = 2KB
    static uint8_t dacBuffer[AUDIO_BUFFER_SIZE];           // 1024 bytes  
    static uint8_t masterVolume;
    static bool initialized;
    
    // Audio generation functions
    static int16_t generateSquareWave(uint16_t freq, uint8_t duty, uint32_t phase);
    static int16_t generateTriangleWave(uint16_t freq, uint32_t phase);
    static int16_t generateNoise(uint32_t phase);
    static void mixChannels();
    // Total audio memory: ~3KB vs original 1.5KB
};

// === MINIMAL INPUT API ===
class SimpleInput {
public:
    static bool isButtonPressed(uint8_t button);
    static bool wasButtonJustPressed(uint8_t button);
    static void update();
};

// === UNIFIED MINIMAL ENGINE ===
class Engine {
public:
    static bool init();
    static void update();
    static void shutdown();
    
    // Memory usage reporting
    static size_t getUsedMemory();
    static size_t getFreeMemory();
    static void printMemoryStats();
    
    static SimpleGraphics& graphics() { return gfx; }
    static SimpleStorage& storage() { return store; }
    static SimpleAudio& audio() { return sound; }
    static SimpleInput& input() { return controls; }
    
    // Friend access for Engine to access private members
    friend class SimpleGraphics;
    friend class SimpleAudio;
    
private:
    static SimpleGraphics gfx;
    static SimpleStorage store;
    static SimpleAudio sound;
    static SimpleInput controls;
    static bool initialized;
};

// Memory usage constants for enhanced GBA-style engine
#define SPRITE_SYSTEM_MEMORY (MAX_SPRITE_SLOTS * SPRITE_SLOT_SIZE + MAX_ACTIVE_SPRITES * 8 + COLOR_PALETTE_SIZE * 2)  // ~17.4KB
#define BACKGROUND_SYSTEM_MEMORY (MAX_TILES * 65 + MAX_BACKGROUND_LAYERS * 3072)  // ~14.3KB  
#define AUDIO_SYSTEM_MEMORY (AUDIO_BUFFER_SIZE * 3 + MAX_AUDIO_CHANNELS * 16)  // ~3.1KB
#define TOTAL_ENGINE_MEMORY (SPRITE_SYSTEM_MEMORY + BACKGROUND_SYSTEM_MEMORY + AUDIO_SYSTEM_MEMORY + 1024)  // ~35.8KB

// Still excellent for ESP32-C6! ~11% of 320KB RAM for full GBA-style features

} // namespace Minimal
} // namespace WispEngine

// For ESP32-C6, use the minimal engine by default
#if defined(PLATFORM_C6)
namespace WispEngine {
    using MinimalEngine = Minimal::Engine;
}
#endif
