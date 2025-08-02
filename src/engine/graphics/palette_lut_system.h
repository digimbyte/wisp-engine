// hybrid_palette_lut_system.h - ESP32-C6/S3 Hybrid Palette LUT using ESP-IDF
// Advanced color mixing with 64x64 LUT for complex effects on ESP32
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include "sprite_system_config.h"

// Hybrid palette system with optional 64x64 LUT for complex color mixing
// Provides both compact palettes AND advanced color blending capabilities

// LUT configuration based on memory profile
#if WISP_MEMORY_PROFILE == PROFILE_MINIMAL
    #define USE_COLOR_LUT false
    #define LUT_SIZE 0
#elif WISP_MEMORY_PROFILE == PROFILE_BALANCED
    #define USE_COLOR_LUT true
    #define LUT_SIZE 64              // 64x64 LUT = 8KB (reasonable)
#else // PROFILE_FULL
    #define USE_COLOR_LUT true  
    #define LUT_SIZE 64              // 64x64 LUT = 8KB (still reasonable)
#endif

// Compact LUT index packing for 64x64
#if USE_COLOR_LUT && LUT_SIZE == 64
    #define LUT_INDEX_BITS 6         // 6 bits per dimension (0-63)
    #define LUT_TOTAL_BITS 12        // 6+6 = 12 bits total
    #define LUT_MAX_INDEX 63         // 0-63 range
    typedef uint16_t PackedLutIndex;  // 12 bits used, 4 bits free for flags
#endif

// Memory calculations
#if USE_COLOR_LUT
    #define LUT_MEMORY_BYTES (LUT_SIZE * LUT_SIZE * 2)  // RGB565 = 2 bytes per entry
#else
    #define LUT_MEMORY_BYTES 0
#endif

// Enhanced palette system with optional LUT support
class HybridPaletteSystem {
private:
    // Basic palette system (always present)
    uint16_t palettes[MAX_ACTIVE_PALETTES][COLORS_PER_PALETTE];
    uint8_t activePalette;
    
    #if USE_COLOR_LUT
    // Optional 64x64 color LUT for advanced effects
    uint16_t colorLUT[LUT_SIZE * LUT_SIZE];
    bool lutLoaded;
    #endif
    
    // Performance tracking
    uint32_t updateTime;
    
public:
    HybridPaletteSystem() : activePalette(0), updateTime(0) {
        // Initialize palettes
        memset(palettes, 0, sizeof(palettes));
        
        // Set transparent color (index 0) in all palettes
        for (uint8_t p = 0; p < MAX_ACTIVE_PALETTES; p++) {
            palettes[p][0] = 0x0000; // Black = transparent
        }
        
        #if USE_COLOR_LUT
        memset(colorLUT, 0, sizeof(colorLUT));
        lutLoaded = false;
        Serial.print("Hybrid palette system with ");
        Serial.print(LUT_SIZE);
        Serial.print("x");
        Serial.print(LUT_SIZE);
        Serial.print(" LUT (");
        Serial.print(LUT_MEMORY_BYTES);
        Serial.println(" bytes)");
        #else
        Serial.println("Pure palette system (no LUT)");
        #endif
    }
    
    // Get total memory usage
    size_t getMemoryUsage() const {
        return sizeof(palettes) + LUT_MEMORY_BYTES;
    }
    
    // Load basic palette (same as before)
    bool loadPalette(uint8_t paletteId, const uint16_t* colors, uint8_t colorCount) {
        if (paletteId >= MAX_ACTIVE_PALETTES) return false;
        
        if (colorCount > COLORS_PER_PALETTE) {
            colorCount = COLORS_PER_PALETTE;
        }
        
        // Load colors starting from index 1 (0 = transparent)
        for (uint8_t i = 1; i < colorCount + 1 && i < COLORS_PER_PALETTE; i++) {
            palettes[paletteId][i] = colors[i - 1];
        }
        
        return true;
    }
    
    #if USE_COLOR_LUT
    // Load 64x64 color LUT for advanced mixing
    bool loadColorLUT(const uint16_t* lutData) {
        if (!lutData) return false;
        
        memcpy(colorLUT, lutData, LUT_MEMORY_BYTES);
        lutLoaded = true;
        
        Serial.print("64x64 Color LUT loaded (");
        Serial.print(LUT_MEMORY_BYTES);
        Serial.println(" bytes)");
        
        return true;
    }
    
    // Generate procedural LUT (for testing/demos)
    void generateTestLUT() {
        for (uint8_t y = 0; y < LUT_SIZE; y++) {
            for (uint8_t x = 0; x < LUT_SIZE; x++) {
                // Create gradient: X controls hue, Y controls brightness
                uint16_t hue = (x * 360) / LUT_SIZE;
                uint8_t brightness = (y * 255) / LUT_SIZE;
                
                colorLUT[y * LUT_SIZE + x] = hsvToRgb565(hue, 255, brightness);
            }
        }
        lutLoaded = true;
        Serial.println("Test LUT generated");
    }
    
    // Pack X,Y coordinates into compact 12-bit index
    PackedLutIndex packLutCoords(uint8_t x, uint8_t y) const {
        if (x > LUT_MAX_INDEX) x = LUT_MAX_INDEX;
        if (y > LUT_MAX_INDEX) y = LUT_MAX_INDEX;
        return (PackedLutIndex)((y << LUT_INDEX_BITS) | x);
    }
    
    // Unpack 12-bit index into X,Y coordinates  
    void unpackLutCoords(PackedLutIndex packed, uint8_t& x, uint8_t& y) const {
        x = packed & ((1 << LUT_INDEX_BITS) - 1);           // Lower 6 bits
        y = (packed >> LUT_INDEX_BITS) & ((1 << LUT_INDEX_BITS) - 1); // Upper 6 bits
    }
    
    // Get color from LUT using packed coordinates
    uint16_t getLutColor(PackedLutIndex packedCoords) const {
        if (!lutLoaded) return 0x0000;
        
        uint8_t x, y;
        unpackLutCoords(packedCoords, x, y);
        
        return colorLUT[y * LUT_SIZE + x];
    }
    
    // Get color from LUT using separate X,Y
    uint16_t getLutColor(uint8_t x, uint8_t y) const {
        if (!lutLoaded || x >= LUT_SIZE || y >= LUT_SIZE) return 0x0000;
        
        return colorLUT[y * LUT_SIZE + x];
    }
    
    // Advanced: Blend palette color with LUT color
    uint16_t getBlendedColor(uint8_t paletteId, uint8_t colorIndex, uint8_t lutX, uint8_t lutY) const {
        // Get base color from palette
        uint16_t baseColor = getColor(paletteId, colorIndex);
        if (baseColor == 0x0000) return 0x0000; // Transparent
        
        // Get blend color from LUT
        uint16_t blendColor = getLutColor(lutX, lutY);
        
        // Simple additive blend (can be made more sophisticated)
        uint16_t r1 = (baseColor >> 11) & 0x1F;
        uint16_t g1 = (baseColor >> 5) & 0x3F;  
        uint16_t b1 = baseColor & 0x1F;
        
        uint16_t r2 = (blendColor >> 11) & 0x1F;
        uint16_t g2 = (blendColor >> 5) & 0x3F;
        uint16_t b2 = blendColor & 0x1F;
        
        uint16_t r = min(31, r1 + (r2 >> 1));
        uint16_t g = min(63, g1 + (g2 >> 1)); 
        uint16_t b = min(31, b1 + (b2 >> 1));
        
        return (r << 11) | (g << 5) | b;
    }
    #endif // USE_COLOR_LUT
    
    // Get color from palette (standard method)
    uint16_t getColor(uint8_t paletteId, uint8_t colorIndex) const {
        if (paletteId >= MAX_ACTIVE_PALETTES || colorIndex >= COLORS_PER_PALETTE) {
            return 0x0000;
        }
        
        return palettes[paletteId][colorIndex];
    }
    
    // Set palette color
    bool setColor(uint8_t paletteId, uint8_t colorIndex, uint16_t rgb565) {
        if (paletteId >= MAX_ACTIVE_PALETTES || colorIndex >= COLORS_PER_PALETTE) {
            return false;
        }
        
        palettes[paletteId][colorIndex] = rgb565;
        return true;
    }
    
    // Active palette management
    void setActivePalette(uint8_t paletteId) {
        if (paletteId < MAX_ACTIVE_PALETTES) {
            activePalette = paletteId;
        }
    }
    
    uint8_t getActivePalette() const {
        return activePalette;
    }
    
    // Utility: HSV to RGB565 conversion
    uint16_t hsvToRgb565(uint16_t h, uint8_t s, uint8_t v) const {
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
        
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
    
    // Debug and stats
    void printStats() {
        Serial.println("=== Hybrid Palette System Stats ===");
        Serial.print("Palettes: ");
        Serial.print(MAX_ACTIVE_PALETTES);
        Serial.print(" × ");
        Serial.print(COLORS_PER_PALETTE);
        Serial.println(" colors");
        
        #if USE_COLOR_LUT
        Serial.print("LUT: ");
        Serial.print(LUT_SIZE);
        Serial.print("×");
        Serial.print(LUT_SIZE);
        Serial.print(" (");
        Serial.print(LUT_MEMORY_BYTES);
        Serial.print(" bytes) - ");
        Serial.println(lutLoaded ? "LOADED" : "EMPTY");
        #else
        Serial.println("LUT: DISABLED (pure palette mode)");
        #endif
        
        Serial.print("Total memory: ");
        Serial.print(getMemoryUsage());
        Serial.println(" bytes");
        Serial.print("Active palette: ");
        Serial.println(activePalette);
        Serial.println("===================================");
    }
};

// Compact sprite data structure for 64x64 LUT
#if USE_COLOR_LUT
struct CompactSpriteData {
    uint8_t paletteIndex;           // Base palette color index
    PackedLutIndex lutCoords : 12;  // Packed LUT coordinates (6+6 bits)
    uint8_t flags : 4;              // Sprite flags (visible, flipped, etc.)
    
    // Helper methods
    void setLutCoords(uint8_t x, uint8_t y) {
        if (x > LUT_MAX_INDEX) x = LUT_MAX_INDEX;
        if (y > LUT_MAX_INDEX) y = LUT_MAX_INDEX;
        lutCoords = (y << LUT_INDEX_BITS) | x;
    }
    
    void getLutCoords(uint8_t& x, uint8_t& y) const {
        x = lutCoords & ((1 << LUT_INDEX_BITS) - 1);
        y = (lutCoords >> LUT_INDEX_BITS) & ((1 << LUT_INDEX_BITS) - 1);
    }
};
#endif

// Memory usage comparison:
/*
128×128 LUT: 32,768 bytes
64×64 LUT:    8,192 bytes (75% savings)
Pure palette: 128-2048 bytes (96-99% savings)

Profile recommendations:
- MINIMAL: Pure palette (128 bytes) - maximum game memory
- BALANCED: 64×64 LUT (8KB) - good for color effects and mixing  
- FULL: 64×64 LUT (8KB) - advanced graphics with reasonable memory
*/
