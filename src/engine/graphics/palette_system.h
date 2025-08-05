// optimized_palette_system.h - ESP32-C6/S3 Palette System using ESP-IDF
// Memory-optimized palette system for ESP32 with LUT acceleration
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include "sprite_system_config.h"

// Optimized palette system for ESP32-C6
// Massive memory savings vs 128x128 LUT (32KB -> 128 bytes!)

// Special color index for transparency
#define TRANSPARENT_COLOR_INDEX 0

// Optimized palette sizes based on memory profile
#if WISP_MEMORY_PROFILE == PROFILE_MINIMAL
    #define COLORS_PER_PALETTE 16      // 4-bit color depth (Game Boy style)
    #define MAX_ACTIVE_PALETTES 4      // 4 palettes total
    #define PALETTE_BITS 4
#elif WISP_MEMORY_PROFILE == PROFILE_BALANCED
    #define COLORS_PER_PALETTE 64      // 6-bit color depth (good balance)
    #define MAX_ACTIVE_PALETTES 4      // 4 palettes total
    #define PALETTE_BITS 6
#else // PROFILE_FULL
    #define COLORS_PER_PALETTE 256     // 8-bit color depth (full range)
    #define MAX_ACTIVE_PALETTES 4      // 4 palettes total
    #define PALETTE_BITS 8
#endif

// Memory usage calculation
#define PALETTE_MEMORY_BYTES (MAX_ACTIVE_PALETTES * COLORS_PER_PALETTE * 2)  // RGB565 = 2 bytes

// Compact palette entry (just RGB565 color)
struct CompactPaletteEntry {
    uint16_t rgb565;        // RGB565 color value
};

// Animated palette color (optional, for special effects)
struct AnimatedPaletteEntry {
    uint16_t frames[4];     // Max 4 animation frames to save memory
    uint8_t frameCount;     // 1-4 frames
    uint8_t frameDuration;  // Duration in 60fps ticks
    uint8_t currentFrame;   // Current frame index
    uint8_t frameTimer;     // Timer countdown
};

class OptimizedPaletteSystem {
private:
    // Compact palette storage (much smaller than 128x128 LUT!)
    CompactPaletteEntry palettes[MAX_ACTIVE_PALETTES][COLORS_PER_PALETTE];
    
    // Animated entries (only allocate as needed)
    AnimatedPaletteEntry* animatedEntries[MAX_ACTIVE_PALETTES];
    uint8_t animatedCounts[MAX_ACTIVE_PALETTES];
    bool animatedAllocated[MAX_ACTIVE_PALETTES];
    
    // Active palette for rendering
    uint8_t activePalette;
    
    // Performance tracking
    uint32_t updateTime;
    uint8_t animationsUpdated;
    
public:
    OptimizedPaletteSystem() : activePalette(0), updateTime(0), animationsUpdated(0) {
        // Initialize palettes to black/transparent
        memset(palettes, 0, sizeof(palettes));
        memset(animatedEntries, 0, sizeof(animatedEntries));
        memset(animatedCounts, 0, sizeof(animatedCounts));
        memset(animatedAllocated, 0, sizeof(animatedAllocated));
        
        // Set transparent color (index 0) to black in all palettes
        for (uint8_t p = 0; p < MAX_ACTIVE_PALETTES; p++) {
            palettes[p][0].rgb565 = 0x0000; // Black = transparent
        }
        
        Serial.print("Palette system initialized: ");
        Serial.print(MAX_ACTIVE_PALETTES);
        Serial.print(" palettes × ");
        Serial.print(COLORS_PER_PALETTE);
        Serial.print(" colors = ");
        Serial.print(getMemoryUsage());
        Serial.println(" bytes");
    }
    
    ~OptimizedPaletteSystem() {
        // Free any allocated animated entries
        for (uint8_t p = 0; p < MAX_ACTIVE_PALETTES; p++) {
            if (animatedAllocated[p] && animatedEntries[p]) {
                free(animatedEntries[p]);
            }
        }
    }
    
    // Get memory usage
    size_t getMemoryUsage() const {
        size_t total = sizeof(palettes);
        
        // Add animated entries
        for (uint8_t p = 0; p < MAX_ACTIVE_PALETTES; p++) {
            if (animatedAllocated[p]) {
                total += animatedCounts[p] * sizeof(AnimatedPaletteEntry);
            }
        }
        
        return total;
    }
    
    // Load a palette from RGB565 array
    bool loadPalette(uint8_t paletteId, const uint16_t* colors, uint8_t colorCount) {
        if (paletteId >= MAX_ACTIVE_PALETTES) {
            Serial.println("ERROR: Invalid palette ID");
            return false;
        }
        
        if (colorCount > COLORS_PER_PALETTE) {
            Serial.print("WARNING: Color count truncated to ");
            Serial.println(COLORS_PER_PALETTE);
            colorCount = COLORS_PER_PALETTE;
        }
        
        // Load colors starting from index 1 (0 = transparent)
        for (uint8_t i = 1; i < colorCount + 1 && i < COLORS_PER_PALETTE; i++) {
            palettes[paletteId][i].rgb565 = colors[i - 1];
        }
        
        Serial.print("Palette ");
        Serial.print(paletteId);
        Serial.print(" loaded with ");
        Serial.print(colorCount);
        Serial.println(" colors");
        
        return true;
    }
    
    // Load Game Boy style 4-color palette
    bool loadGameBoyPalette(uint8_t paletteId, uint16_t color1, uint16_t color2, uint16_t color3, uint16_t color4) {
        if (paletteId >= MAX_ACTIVE_PALETTES) return false;
        
        palettes[paletteId][0].rgb565 = 0x0000;  // Transparent
        palettes[paletteId][1].rgb565 = color1;  // Lightest
        palettes[paletteId][2].rgb565 = color2;  // Light
        palettes[paletteId][3].rgb565 = color3;  // Dark
        
        #if COLORS_PER_PALETTE > 4
        palettes[paletteId][4].rgb565 = color4;  // Darkest
        #endif
        
        Serial.print("Game Boy palette ");
        Serial.print(paletteId);
        Serial.println(" loaded");
        
        return true;
    }
    
    // Create palette from HSV range (automatic color generation)
    bool generatePalette(uint8_t paletteId, uint16_t baseHue, uint8_t steps) {
        if (paletteId >= MAX_ACTIVE_PALETTES || steps > COLORS_PER_PALETTE - 1) {
            return false;
        }
        
        palettes[paletteId][0].rgb565 = 0x0000; // Transparent
        
        for (uint8_t i = 1; i <= steps; i++) {
            // Generate colors with varying saturation/value
            uint8_t sat = 255 - (i * 32);  // Decrease saturation
            uint8_t val = 128 + (i * 16);  // Increase brightness
            
            palettes[paletteId][i].rgb565 = hsvToRgb565(baseHue, sat, val);
        }
        
        Serial.print("Generated palette ");
        Serial.print(paletteId);
        Serial.print(" with ");
        Serial.print(steps);
        Serial.println(" colors");
        
        return true;
    }
    
    // Set individual color
    bool setColor(uint8_t paletteId, uint8_t colorIndex, uint16_t rgb565) {
        if (paletteId >= MAX_ACTIVE_PALETTES || colorIndex >= COLORS_PER_PALETTE) {
            return false;
        }
        
        palettes[paletteId][colorIndex].rgb565 = rgb565;
        return true;
    }
    
    // Get color from palette
    uint16_t getColor(uint8_t paletteId, uint8_t colorIndex) const {
        if (paletteId >= MAX_ACTIVE_PALETTES || colorIndex >= COLORS_PER_PALETTE) {
            return 0x0000; // Return transparent/black for invalid indices
        }
        
        return palettes[paletteId][colorIndex].rgb565;
    }
    
    // Set active palette for rendering
    void setActivePalette(uint8_t paletteId) {
        if (paletteId < MAX_ACTIVE_PALETTES) {
            activePalette = paletteId;
        }
    }
    
    uint8_t getActivePalette() const {
        return activePalette;
    }
    
    // Add animated color to palette
    bool addAnimatedColor(uint8_t paletteId, uint8_t colorIndex, 
                         const uint16_t* frames, uint8_t frameCount, uint8_t frameDuration) {
        
        if (paletteId >= MAX_ACTIVE_PALETTES || colorIndex >= COLORS_PER_PALETTE || 
            frameCount > 4 || frameCount == 0) {
            return false;
        }
        
        // Allocate animated entries array if needed
        if (!animatedAllocated[paletteId]) {
            animatedEntries[paletteId] = (AnimatedPaletteEntry*)malloc(
                COLORS_PER_PALETTE * sizeof(AnimatedPaletteEntry));
            if (!animatedEntries[paletteId]) {
                Serial.println("ERROR: Failed to allocate animated palette memory");
                return false;
            }
            animatedAllocated[paletteId] = true;
            animatedCounts[paletteId] = 0;
        }
        
        // Find or add animated entry
        AnimatedPaletteEntry* entry = &animatedEntries[paletteId][colorIndex];
        
        // Copy frame data
        for (uint8_t i = 0; i < frameCount; i++) {
            entry->frames[i] = frames[i];
        }
        
        entry->frameCount = frameCount;
        entry->frameDuration = frameDuration;
        entry->currentFrame = 0;
        entry->frameTimer = frameDuration;
        
        // Update the base palette with first frame
        palettes[paletteId][colorIndex].rgb565 = frames[0];
        
        // Update count if this is a new animated color
        if (colorIndex >= animatedCounts[paletteId]) {
            animatedCounts[paletteId] = colorIndex + 1;
        }
        
        Serial.print("Animated color added: palette=");
        Serial.print(paletteId);
        Serial.print(" index=");
        Serial.print(colorIndex);
        Serial.print(" frames=");
        Serial.println(frameCount);
        
        return true;
    }
    
    // Update animated colors
    void updateAnimations() {
        uint32_t startTime = get_micros();
        animationsUpdated = 0;
        
        for (uint8_t p = 0; p < MAX_ACTIVE_PALETTES; p++) {
            if (!animatedAllocated[p] || animatedCounts[p] == 0) continue;
            
            for (uint8_t i = 0; i < animatedCounts[p]; i++) {
                AnimatedPaletteEntry& entry = animatedEntries[p][i];
                
                if (entry.frameCount <= 1) continue;
                
                if (entry.frameTimer > 0) {
                    entry.frameTimer--;
                } else {
                    // Advance frame
                    entry.currentFrame = (entry.currentFrame + 1) % entry.frameCount;
                    entry.frameTimer = entry.frameDuration;
                    
                    // Update the palette with new frame
                    palettes[p][i].rgb565 = entry.frames[entry.currentFrame];
                    animationsUpdated++;
                }
            }
        }
        
        updateTime = get_micros() - startTime;
    }
    
    // Utility: Convert HSV to RGB565
    uint16_t hsvToRgb565(uint16_t h, uint8_t s, uint8_t v) const {
        // Simplified HSV to RGB conversion
        uint8_t r, g, b;
        
        uint8_t region = h / 43;
        uint8_t remainder = (h - (region * 43)) * 6;
        
        uint8_t p = (v * (255 - s)) >> 8;
        uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
        uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
        
        switch (region) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            default: r = v; g = p; b = q; break;
        }
        
        // Convert to RGB565
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
    
    // Create common retro palettes
    void loadDefaultPalettes() {
        // Palette 0: Original Game Boy (green tints)
        loadGameBoyPalette(0, 
            0xE7FF,  // Light green
            0xA534,  // Medium green  
            0x5269,  // Dark green
            0x1084   // Very dark green
        );
        
        // Palette 1: NES-style palette
        uint16_t nesColors[15] = {
            0xFFFF, 0xF800, 0x07E0, 0x001F,  // White, Red, Green, Blue
            0xFFE0, 0xF81F, 0x07FF, 0x7BEF,  // Yellow, Magenta, Cyan, Light Gray
            0x39C7, 0x2104, 0x4208, 0x8410,  // Medium Gray, Dark Red, Dark Green, Dark Blue
            0x8C51, 0x6B4D, 0x4A69           // Browns and darker tones
        };
        loadPalette(1, nesColors, 15);
        
        // Palette 2: Monochrome (black to white)
        uint16_t monoColors[15] = {
            0x0000, 0x1082, 0x2104, 0x3186,  // Black to dark gray
            0x4208, 0x528A, 0x630C, 0x738E,  // Medium grays
            0x8410, 0x9492, 0xA514, 0xB596,  // Light grays
            0xC618, 0xD69A, 0xE71C           // Light gray to white
        };
        loadPalette(2, monoColors, 15);
        
        // Palette 3: Vibrant colors
        uint16_t vibrantColors[15] = {
            0xF800, 0xFD20, 0xFFE0, 0x87E0,  // Red, Orange, Yellow, Lime
            0x07E0, 0x07F3, 0x07FF, 0x005F,  // Green, Teal, Cyan, Dark Blue
            0x001F, 0x8010, 0xF81F, 0xFC10,  // Blue, Purple, Magenta, Pink
            0xFDA0, 0xFEB0, 0xFFC0           // Light variations
        };
        loadPalette(3, vibrantColors, 15);
        
        Serial.println("Default retro palettes loaded");
    }
    
    // Debug and stats
    void printStats() {
        Serial.println("=== Optimized Palette System Stats ===");
        Serial.print("Palettes: ");
        Serial.print(MAX_ACTIVE_PALETTES);
        Serial.print(" × ");
        Serial.print(COLORS_PER_PALETTE);
        Serial.println(" colors");
        Serial.print("Memory usage: ");
        Serial.print(getMemoryUsage());
        Serial.println(" bytes");
        Serial.print("vs 128×128 LUT: ");
        Serial.print(32768); // 128*128*2
        Serial.print(" bytes → ");
        Serial.print(100 - (getMemoryUsage() * 100 / 32768));
        Serial.println("% savings!");
        Serial.print("Active palette: ");
        Serial.println(activePalette);
        
        for (uint8_t p = 0; p < MAX_ACTIVE_PALETTES; p++) {
            if (animatedCounts[p] > 0) {
                Serial.print("Palette ");
                Serial.print(p);
                Serial.print(": ");
                Serial.print(animatedCounts[p]);
                Serial.println(" animated colors");
            }
        }
        
        Serial.print("Last update time: ");
        Serial.print(updateTime);
        Serial.println(" us");
        Serial.print("Animations updated: ");
        Serial.println(animationsUpdated);
        Serial.println("======================================");
    }
    
    // Copy palette to external buffer (for display drivers)
    void copyPalette(uint8_t paletteId, uint16_t* buffer, uint8_t maxColors) const {
        if (paletteId >= MAX_ACTIVE_PALETTES) return;
        
        uint8_t copyCount = min(maxColors, COLORS_PER_PALETTE);
        for (uint8_t i = 0; i < copyCount; i++) {
            buffer[i] = palettes[paletteId][i].rgb565;
        }
    }
};

// Memory usage comparison:
/*
Old system (128×128 LUT):
- Color LUT: 128 × 128 × 2 bytes = 32,768 bytes (32KB!)

New optimized system:
- MINIMAL (16 colors × 4 palettes): 16 × 4 × 2 = 128 bytes
- BALANCED (64 colors × 4 palettes): 64 × 4 × 2 = 512 bytes  
- FULL (256 colors × 4 palettes): 256 × 4 × 2 = 2,048 bytes (2KB)

Memory savings:
- MINIMAL: 32KB → 128 bytes = 99.6% savings!
- BALANCED: 32KB → 512 bytes = 98.4% savings!
- FULL: 32KB → 2KB = 93.8% savings!
*/
