// minimal_engine.cpp - Implementation of ultra-lean engine for ESP32-C6
#include "minimal_engine.h"
#include "../engine_common.h"
#include "../../system/display_driver.h"
#include "../../core/timekeeper.h"  // For get_millis()
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <cmath>

#ifndef PI
#define PI 3.14159265358979323846
#endif

namespace WispEngine {
namespace Minimal {

// === STATIC MEMBER DEFINITIONS ===

// Graphics system static members (GBA-style enhanced)
SpriteSlot SimpleGraphics::spriteSlots[MAX_SPRITE_SLOTS];
SpriteInstance SimpleGraphics::activeSprites[MAX_ACTIVE_SPRITES]; 
uint8_t SimpleGraphics::activeSpriteCount = 0;
uint8_t SimpleGraphics::nextSlot = 0;

// Background tile system
Tile SimpleGraphics::tileSet[MAX_TILES];
BackgroundLayer SimpleGraphics::backgrounds[MAX_BACKGROUND_LAYERS];
uint8_t SimpleGraphics::loadedTileCount = 0;

// Palette system  
uint16_t SimpleGraphics::colorPalette[COLOR_PALETTE_SIZE];
uint16_t SimpleGraphics::colorLUT[COLOR_PALETTE_SIZE];  // Alias for compatibility

// V-blank callback
void (*SimpleGraphics::vblankCallback)() = nullptr;

// Define the global display instance
LGFX display;

// Audio system static members (GBA-style enhanced)
AudioChannel SimpleAudio::channels[MAX_AUDIO_CHANNELS];
int16_t SimpleAudio::mixBuffer[AUDIO_BUFFER_SIZE];
uint8_t SimpleAudio::dacBuffer[AUDIO_BUFFER_SIZE];  
uint8_t SimpleAudio::masterVolume = 15;
bool SimpleAudio::initialized = false;// Engine static members
SimpleGraphics Engine::gfx;
SimpleStorage Engine::store;
SimpleAudio Engine::sound;
SimpleInput Engine::controls;
bool Engine::initialized = false;

// External display instance (from display_driver.h)
extern LGFX display;

// === GRAPHICS IMPLEMENTATION ===

void SimpleGraphics::drawPixel(int x, int y, uint16_t color) {
    extern LGFX display;  // Access the global display instance
    display.drawPixel(x, y, color);
}

void SimpleGraphics::drawLine(int x0, int y0, int x1, int y1, uint16_t color) {
    extern LGFX display;
    display.drawLine(x0, y0, x1, y1, color);
}

void SimpleGraphics::drawRect(int x, int y, int w, int h, uint16_t color) {
    extern LGFX display;
    display.drawRect(x, y, w, h, color);
}

void SimpleGraphics::fillRect(int x, int y, int w, int h, uint16_t color) {
    extern LGFX display;
    display.fillRect(x, y, w, h, color);
}

void SimpleGraphics::drawText(int x, int y, const char* text, uint16_t color) {
    extern LGFX display;
    display.setTextColor(color);
    display.setCursor(x, y);
    display.print(text);
}

uint8_t SimpleGraphics::loadSprite(uint16_t spriteId) {
    // Check if sprite already loaded
    uint8_t existingSlot = findSpriteSlot(spriteId);
    if (existingSlot != 255) {
        spriteSlots[existingSlot].lastAccess = get_millis();
        return existingSlot;
    }
    
    // Get available slot (LRU eviction if needed)
    uint8_t slotId = getAvailableSlot();
    
    // Load sprite data from flash
    loadSpriteFromFlash(spriteId, &spriteSlots[slotId]);
    
    return slotId;
}

void SimpleGraphics::drawSprite(uint16_t spriteId, int x, int y, uint8_t scale, 
                               bool flipX, bool flipY, uint8_t priority) {
    if (activeSpriteCount >= MAX_ACTIVE_SPRITES) return;
    
    // Load sprite to slot if needed
    uint8_t slotId = loadSprite(spriteId);
    
    // Add to active sprites for rendering
    SpriteInstance& sprite = activeSprites[activeSpriteCount++];
    sprite.slotId = slotId;
    sprite.x = x;
    sprite.y = y;
    sprite.scale = scale;
    sprite.flipX = flipX;
    sprite.flipY = flipY;
    sprite.priority = priority;
    
    // Immediate render (could be batched for optimization)
    renderSpriteFromSlot(slotId, x, y, scale, flipX, flipY, priority);
}

void SimpleGraphics::clearSprites() {
    activeSpriteCount = 0;
}

void SimpleGraphics::clear(uint16_t color) {
    extern LGFX display;
    display.fillScreen(color);
    clearSprites();
}

void SimpleGraphics::display() {
    // LovyanGFX handles the actual display update
    // Sprites are rendered immediately in drawSprite()
}

uint8_t SimpleGraphics::findSpriteSlot(uint16_t spriteId) {
    for (uint8_t i = 0; i < MAX_SPRITE_SLOTS; i++) {
        if (spriteSlots[i].inUse && spriteSlots[i].spriteId == spriteId) {
            return i;
        }
    }
    return 255; // Not found
}

uint8_t SimpleGraphics::getAvailableSlot() {
    // First, try to find an empty slot
    for (uint8_t i = 0; i < MAX_SPRITE_SLOTS; i++) {
        if (!spriteSlots[i].inUse) {
            return i;
        }
    }
    
    // If no empty slots, use LRU eviction
    uint32_t oldestTime = UINT32_MAX;
    uint8_t oldestSlot = 0;
    
    for (uint8_t i = 0; i < MAX_SPRITE_SLOTS; i++) {
        if (spriteSlots[i].lastAccess < oldestTime) {
            oldestTime = spriteSlots[i].lastAccess;
            oldestSlot = i;
        }
    }
    
    return oldestSlot;
}

void SimpleGraphics::loadSpriteFromFlash(uint16_t spriteId, SpriteSlot* slot) {
    // For ESP32-C6, implement basic sprite loading from embedded data
    slot->spriteId = spriteId;
    slot->inUse = true;
    slot->lastAccess = get_millis();
    
    // Create basic sprites based on ID (for testing/demo)
    switch (spriteId) {
        case 1: // Player sprite
            for (int y = 0; y < 16; y++) {
                for (int x = 0; x < 16; x++) {
                    if ((x >= 6 && x <= 9) && (y >= 6 && y <= 9)) {
                        slot->spriteData[y * 16 + x] = 0x1F; // Blue center
                    } else if ((x >= 4 && x <= 11) && (y >= 4 && y <= 11)) {
                        slot->spriteData[y * 16 + x] = 0x03; // Dark border
                    } else {
                        slot->spriteData[y * 16 + x] = 0; // Transparent
                    }
                }
            }
            break;
        case 2: // Enemy sprite
            for (int y = 0; y < 16; y++) {
                for (int x = 0; x < 16; x++) {
                    if ((x >= 6 && x <= 9) && (y >= 6 && y <= 9)) {
                        slot->spriteData[y * 16 + x] = 0xE0; // Red center
                    } else if ((x >= 4 && x <= 11) && (y >= 4 && y <= 11)) {
                        slot->spriteData[y * 16 + x] = 0x08; // Dark border
                    } else {
                        slot->spriteData[y * 16 + x] = 0; // Transparent
                    }
                }
            }
            break;
        default:
            // Fill with pattern based on ID
            for (int i = 0; i < 256; i++) {
                slot->spriteData[i] = (spriteId + i) & 0xFF;
            }
            break;
    }
}

void SimpleGraphics::renderSpriteFromSlot(uint8_t slotId, int x, int y, uint8_t scale, 
                                        bool flipX, bool flipY, uint8_t priority) {
    extern LGFX display;
    if (slotId >= MAX_SPRITE_SLOTS || !spriteSlots[slotId].inUse) return;
    
    SpriteSlot& slot = spriteSlots[slotId];
    
    // Simple 16x16 sprite rendering with LUT color mapping and flipping
    for (int py = 0; py < 16; py++) {
        for (int px = 0; px < 16; px++) {
            // Apply flipping
            int srcX = flipX ? (15 - px) : px;
            int srcY = flipY ? (15 - py) : py;
            
            uint8_t colorIndex = slot.spriteData[srcY * 16 + srcX];
            if (colorIndex > 0) { // 0 = transparent
                uint16_t color = colorLUT[colorIndex];
                
                // Apply scale
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        display.drawPixel(x + px * scale + sx, y + py * scale + sy, color);
                    }
                }
            }
        }
    }
}

// === GBA-STYLE BACKGROUND TILE FUNCTIONS ===

bool SimpleGraphics::loadTile(uint8_t tileId, const uint8_t* tileData, uint8_t paletteOffset) {
    if (tileId >= MAX_TILES || loadedTileCount >= MAX_TILES) {
        return false;
    }
    
    Tile* tile = &tileSet[tileId];
    if (tileData) {
        // Copy provided tile data
        memcpy(tile->tileData, tileData, 64);
    } else {
        // Generate procedural tile pattern for demo
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                int idx = y * 8 + x;
                // Create checkerboard pattern with palette colors
                if ((x + y) % 2 == 0) {
                    tile->tileData[idx] = paletteOffset + 1;
                } else {
                    tile->tileData[idx] = paletteOffset + 3;
                }
            }
        }
    }
    
    tile->paletteOffset = paletteOffset;
    loadedTileCount++;
    return true;
}

void SimpleGraphics::setTile(uint8_t layer, uint8_t x, uint8_t y, uint8_t tileId, 
                           bool flipX, bool flipY, uint8_t priority) {
    if (layer >= MAX_BACKGROUND_LAYERS || x >= TILEMAP_WIDTH || y >= TILEMAP_HEIGHT) {
        return;
    }
    
    TilemapEntry* entry = &backgrounds[layer].tilemap[y * TILEMAP_WIDTH + x];
    entry->tileId = tileId;
    entry->flipX = flipX;
    entry->flipY = flipY;
    entry->priority = priority;
}

void SimpleGraphics::scrollBackground(uint8_t layer, int16_t x, int16_t y) {
    if (layer >= MAX_BACKGROUND_LAYERS) return;
    
    backgrounds[layer].scrollX = x;
    backgrounds[layer].scrollY = y;
}

void SimpleGraphics::setBackgroundEnabled(uint8_t layer, bool enabled) {
    if (layer >= MAX_BACKGROUND_LAYERS) return;
    
    backgrounds[layer].enabled = enabled;
}

// === PALETTE MANAGEMENT ===

void SimpleGraphics::setPaletteColor(uint8_t index, uint16_t color) {
    if (index < COLOR_PALETTE_SIZE) {
        colorPalette[index] = color;
    }
}

uint16_t SimpleGraphics::getPaletteColor(uint8_t index) {
    if (index < COLOR_PALETTE_SIZE) {
        return colorPalette[index];
    }
    return 0x0000; // Black default
}

void SimpleGraphics::loadPalette(const uint16_t* palette, uint8_t startIndex, uint8_t count) {
    if (startIndex + count <= COLOR_PALETTE_SIZE) {
        memcpy(&colorPalette[startIndex], palette, count * sizeof(uint16_t));
    }
}

// === GBA-STYLE AUDIO IMPLEMENTATION ===

bool SimpleAudio::init() {
    if (initialized) return true;
    
    // Initialize audio channels
    for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
        channels[i].type = (AudioChannelType)i;
        channels[i].enabled = false;
        channels[i].frequency = 0;
        channels[i].volume = 0;
        channels[i].duty = 2;  // 50% duty cycle
        channels[i].duration = 0;
        channels[i].envelope = 0;
        channels[i].sweep = 0;
    }
    
    // Initialize audio buffers
    memset(mixBuffer, 0, sizeof(mixBuffer));
    memset(dacBuffer, 0, sizeof(dacBuffer));
    
    masterVolume = 15;  // Max volume
    
    // TODO: Initialize ESP32 DAC or I2S for audio output
    
    initialized = true;
    return true;
}

void SimpleAudio::playNote(AudioChannelType channel, uint16_t frequency, uint8_t volume, 
                         uint16_t duration, uint8_t duty) {
    if (!initialized || channel >= MAX_AUDIO_CHANNELS) return;
    
    AudioChannel* ch = &channels[channel];
    ch->enabled = true;
    ch->frequency = frequency;
    ch->volume = volume & 0x0F;  // 4-bit volume
    ch->duty = duty & 0x03;      // 2-bit duty cycle
    ch->duration = duration;
}

void SimpleAudio::stopChannel(AudioChannelType channel) {
    if (channel >= MAX_AUDIO_CHANNELS) return;
    
    channels[channel].enabled = false;
    channels[channel].duration = 0;
}

void SimpleAudio::setChannelVolume(AudioChannelType channel, uint8_t volume) {
    if (channel >= MAX_AUDIO_CHANNELS) return;
    
    channels[channel].volume = volume & 0x0F;
}

void SimpleAudio::setMasterVolume(uint8_t volume) {
    masterVolume = volume & 0x0F;
}

// Legacy compatibility functions
void SimpleAudio::playTone(uint16_t freq, uint16_t duration) {
    playNote(CHANNEL_SQUARE1, freq, 8, duration / 16);  // Convert ms to frames
}

void SimpleAudio::playBeep() {
    playNote(CHANNEL_SQUARE1, 800, 10, 15);  // 800Hz beep for 15 frames
}

void SimpleAudio::silence() {
    for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
        stopChannel((AudioChannelType)i);
    }
}

void SimpleAudio::update() {
    if (!initialized) return;
    
    // Simple audio update - more complex mixing would go here
    // TODO: Implement full GBA-style audio mixing and DAC output
}

// === STORAGE IMPLEMENTATION ===

bool SimpleStorage::saveValue(const char* key, const void* data, size_t size) {
    // TODO: Use ESP32 NVS for simple key-value storage
    return false;
}

bool SimpleStorage::loadValue(const char* key, void* data, size_t maxSize) {
    // TODO: Use ESP32 NVS for simple key-value storage
    return false;
}

bool SimpleStorage::hasKey(const char* key) {
    // TODO: Use ESP32 NVS for simple key-value storage
    return false;
}

void SimpleStorage::clearAll() {
    // TODO: Clear NVS partition
}

// === INPUT IMPLEMENTATION ===

bool SimpleInput::isButtonPressed(uint8_t button) {
    // TODO: Read GPIO state for buttons
    return false;
}

bool SimpleInput::wasButtonJustPressed(uint8_t button) {
    // TODO: Implement edge detection for button presses
    return false;
}

void SimpleInput::update() {
    // TODO: Update button states
}

// === ENGINE IMPLEMENTATION ===

bool Engine::init() {
    extern LGFX display;
    if (initialized) return true;
    
    // Initialize display
    display.init();
    display.setBrightness(128);
    
    // Initialize color LUT with realistic game palette
    // Create a 256-color palette suitable for retro games
    for (int i = 0; i < 256; i++) {
        uint8_t r, g, b;
        if (i == 0) {
            // Index 0 = transparent/black
            r = g = b = 0;
        } else if (i < 16) {
            // Grayscale ramp (1-15)
            uint8_t gray = (i * 255) / 15;
            r = g = b = gray;
        } else if (i < 32) {
            // Red palette (16-31) 
            r = ((i - 16) * 255) / 15;
            g = b = 0;
        } else if (i < 48) {
            // Green palette (32-47)
            g = ((i - 32) * 255) / 15;
            r = b = 0;
        } else if (i < 64) {
            // Blue palette (48-63)
            b = ((i - 48) * 255) / 15;
            r = g = 0;
        } else {
            // Extended colors for sprites (64-255)
            r = (i * 3) & 0xFF;
            g = (i * 5) & 0xFF;  
            b = (i * 7) & 0xFF;
        }
        SimpleGraphics::colorLUT[i] = display.color565(r, g, b);
    }
    
    // Initialize subsystems
    if (!sound.init()) {
        return false;
    }
    
    // Clear sprite slots
    memset(SimpleGraphics::spriteSlots, 0, sizeof(SimpleGraphics::spriteSlots));
    SimpleGraphics::activeSpriteCount = 0;
    SimpleGraphics::nextSlot = 0;
    
    initialized = true;
    
    // Print memory usage for verification
    printMemoryStats();
    
    return true;
}

void Engine::update() {
    if (!initialized) return;
    
    controls.update();
    sound.update();
}

void Engine::shutdown() {
    initialized = false;
}

size_t Engine::getUsedMemory() {
    return heap_caps_get_total_size(MALLOC_CAP_DEFAULT) - heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
}

size_t Engine::getFreeMemory() {
    return heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
}

void Engine::printMemoryStats() {
    size_t spriteMemory = sizeof(SimpleGraphics::spriteSlots) + sizeof(SimpleGraphics::activeSprites) + sizeof(SimpleGraphics::colorLUT);
    size_t audioMemory = sizeof(SimpleAudio::mixBuffer) + sizeof(SimpleAudio::dacBuffer);
    size_t tileMemory = sizeof(SimpleGraphics::tileSet) + sizeof(SimpleGraphics::backgrounds);
    size_t paletteMemory = sizeof(SimpleGraphics::colorPalette) + sizeof(SimpleGraphics::colorLUT);
    size_t totalEngineMemory = spriteMemory + audioMemory + tileMemory + paletteMemory;
    
    ESP_LOGI("MinimalEngine", "=== GBA-STYLE ENGINE MEMORY USAGE ===");
    ESP_LOGI("MinimalEngine", "Sprite System: %d bytes (slots: %d, instances: %d)", 
             spriteMemory, sizeof(SimpleGraphics::spriteSlots), sizeof(SimpleGraphics::activeSprites));
    ESP_LOGI("MinimalEngine", "Audio System: %d bytes (mix: %d, dac: %d)", 
             audioMemory, sizeof(SimpleAudio::mixBuffer), sizeof(SimpleAudio::dacBuffer));
    ESP_LOGI("MinimalEngine", "Tile System: %d bytes", tileMemory);
    ESP_LOGI("MinimalEngine", "Palette System: %d bytes", paletteMemory);
    ESP_LOGI("MinimalEngine", "Total Engine: %d bytes (%.1f KB)", totalEngineMemory, totalEngineMemory / 1024.0f);
    ESP_LOGI("MinimalEngine", "ESP32-C6 RAM Usage: %.1f%% of 320KB", (totalEngineMemory * 100.0f) / (320 * 1024));
    ESP_LOGI("MinimalEngine", "Free Heap: %d bytes (%.1f KB)", getFreeMemory(), getFreeMemory() / 1024.0f);
}

} // namespace Minimal
} // namespace WispEngine

// For ESP32-C6, use the minimal engine by default
#if defined(PLATFORM_C6)
namespace WispEngine {
    using MinimalEngine = Minimal::Engine;
}
#endif
