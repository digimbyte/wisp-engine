// palette_framebuffer_system.h - ESP32-C6/S3 Palette Framebuffer using ESP-IDF
// Memory-optimized palette-based framebuffer for massive memory savings on ESP32
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include "sprite_system_config.h"

// Palette-based framebuffer system for massive memory savings
// Instead of storing RGB565 (16-bit) per pixel, store palette index (8-bit or 4-bit)
// Reduces framebuffer memory by 50-75%!

// Color depth options
enum ColorDepth {
    DEPTH_8BIT = 8,    // 256 colors per palette (1 byte per pixel)
    DEPTH_4BIT = 4,    // 16 colors per palette (0.5 bytes per pixel)
    DEPTH_2BIT = 2     // 4 colors per palette (0.25 bytes per pixel - Game Boy style!)
};

// Choose color depth based on memory profile
#if WISP_MEMORY_PROFILE == PROFILE_MINIMAL
    #define PALETTE_COLOR_DEPTH DEPTH_4BIT    // 16 colors - classic Game Boy style
    #define PALETTES_COUNT 4                  // 4 palettes = 64 total colors
#elif WISP_MEMORY_PROFILE == PROFILE_BALANCED
    #define PALETTE_COLOR_DEPTH DEPTH_8BIT    // 256 colors - full palette
    #define PALETTES_COUNT 2                  // 2 palettes = 512 total colors
#else // PROFILE_FULL
    #define PALETTE_COLOR_DEPTH DEPTH_8BIT    // 256 colors - multiple palettes
    #define PALETTES_COUNT 4                  // 4 palettes = 1024 total colors
#endif

// Memory calculations
#define PIXELS_TOTAL (DISPLAY_WIDTH_PX * DISPLAY_HEIGHT_PX)  // 172 * 320 = 55,040

#if PALETTE_COLOR_DEPTH == DEPTH_8BIT
    #define FRAMEBUFFER_SIZE_BYTES PIXELS_TOTAL                    // 55KB (vs 110KB RGB565)
    #define PIXELS_PER_BYTE 1
    typedef uint8_t PixelIndex;
#elif PALETTE_COLOR_DEPTH == DEPTH_4BIT  
    #define FRAMEBUFFER_SIZE_BYTES (PIXELS_TOTAL / 2)             // 27.5KB (vs 110KB RGB565)
    #define PIXELS_PER_BYTE 2
    typedef uint8_t PixelIndex;  // Still use uint8_t, but pack 2 pixels per byte
#elif PALETTE_COLOR_DEPTH == DEPTH_2BIT
    #define FRAMEBUFFER_SIZE_BYTES (PIXELS_TOTAL / 4)             // 13.75KB (vs 110KB RGB565)
    #define PIXELS_PER_BYTE 4
    typedef uint8_t PixelIndex;  // Pack 4 pixels per byte
#endif

// Palette entry (RGB565 output color)
struct PaletteColor {
    uint16_t rgb565;        // Final RGB565 color
    uint8_t flags;          // Transparent, animated, etc.
    uint8_t reserved;       // Padding
};

// Animated palette color
struct AnimatedPaletteColor {
    uint16_t frames[8];     // Up to 8 animation frames
    uint8_t frameCount;     // Number of frames
    uint8_t frameDuration;  // Duration per frame in 60fps ticks
    uint8_t currentFrame;   // Current frame
    uint8_t frameTimer;     // Timer countdown
};

class PaletteFramebuffer {
private:
    // Indexed framebuffer (much smaller than RGB565!)
    uint8_t framebuffer[FRAMEBUFFER_SIZE_BYTES];
    
    // Color palettes
    PaletteColor palettes[PALETTES_COUNT][1 << PALETTE_COLOR_DEPTH];
    
    // Animated colors (optional)
    AnimatedPaletteColor* animatedColors[PALETTES_COUNT];
    uint8_t animatedCounts[PALETTES_COUNT];
    
    // Current active palette
    uint8_t activePalette;
    
    // Double buffering support
    #if WISP_MEMORY_PROFILE == PROFILE_FULL
    uint8_t backbuffer[FRAMEBUFFER_SIZE_BYTES];
    bool doubleBuffering;
    #endif
    
public:
    PaletteFramebuffer() : activePalette(0) {
        memset(framebuffer, 0, sizeof(framebuffer));
        memset(palettes, 0, sizeof(palettes));
        memset(animatedColors, 0, sizeof(animatedColors));
        memset(animatedCounts, 0, sizeof(animatedCounts));
        
        #if WISP_MEMORY_PROFILE == PROFILE_FULL
        memset(backbuffer, 0, sizeof(backbuffer));
        doubleBuffering = true;
        #endif
    }
    
    // Get memory usage
    size_t getMemoryUsage() const {
        size_t total = sizeof(framebuffer) + sizeof(palettes);
        #if WISP_MEMORY_PROFILE == PROFILE_FULL
        if (doubleBuffering) total += sizeof(backbuffer);
        #endif
        return total;
    }
    
    // Load a palette
    void loadPalette(uint8_t paletteId, const uint16_t* colors, uint8_t colorCount) {
        if (paletteId >= PALETTES_COUNT) return;
        
        uint8_t maxColors = 1 << PALETTE_COLOR_DEPTH;
        if (colorCount > maxColors) colorCount = maxColors;
        
        for (uint8_t i = 0; i < colorCount; i++) {
            palettes[paletteId][i].rgb565 = colors[i];
            palettes[paletteId][i].flags = 0;
        }
        
        Serial.print("Palette ");
        Serial.print(paletteId);
        Serial.print(" loaded with ");
        Serial.print(colorCount);
        Serial.println(" colors");
    }
    
    // Set active palette
    void setActivePalette(uint8_t paletteId) {
        if (paletteId < PALETTES_COUNT) {
            activePalette = paletteId;
        }
    }
    
    // Set pixel using palette index
    void setPixel(int16_t x, int16_t y, PixelIndex colorIndex) {
        if (x < 0 || x >= DISPLAY_WIDTH_PX || y < 0 || y >= DISPLAY_HEIGHT_PX) return;
        
        uint32_t pixelPos = y * DISPLAY_WIDTH_PX + x;
        
        #if WISP_MEMORY_PROFILE == PROFILE_FULL
        uint8_t* buffer = doubleBuffering ? backbuffer : framebuffer;
        #else
        uint8_t* buffer = framebuffer;
        #endif
        
        #if PALETTE_COLOR_DEPTH == DEPTH_8BIT
            buffer[pixelPos] = colorIndex;
            
        #elif PALETTE_COLOR_DEPTH == DEPTH_4BIT
            uint32_t bytePos = pixelPos / 2;
            uint8_t bitShift = (pixelPos & 1) * 4;
            buffer[bytePos] = (buffer[bytePos] & ~(0xF << bitShift)) | ((colorIndex & 0xF) << bitShift);
            
        #elif PALETTE_COLOR_DEPTH == DEPTH_2BIT
            uint32_t bytePos = pixelPos / 4;
            uint8_t bitShift = (pixelPos & 3) * 2;
            buffer[bytePos] = (buffer[bytePos] & ~(0x3 << bitShift)) | ((colorIndex & 0x3) << bitShift);
        #endif
    }
    
    // Get pixel palette index
    PixelIndex getPixel(int16_t x, int16_t y) const {
        if (x < 0 || x >= DISPLAY_WIDTH_PX || y < 0 || y >= DISPLAY_HEIGHT_PX) return 0;
        
        uint32_t pixelPos = y * DISPLAY_WIDTH_PX + x;
        
        #if PALETTE_COLOR_DEPTH == DEPTH_8BIT
            return framebuffer[pixelPos];
            
        #elif PALETTE_COLOR_DEPTH == DEPTH_4BIT
            uint32_t bytePos = pixelPos / 2;
            uint8_t bitShift = (pixelPos & 1) * 4;
            return (framebuffer[bytePos] >> bitShift) & 0xF;
            
        #elif PALETTE_COLOR_DEPTH == DEPTH_2BIT
            uint32_t bytePos = pixelPos / 4;
            uint8_t bitShift = (pixelPos & 3) * 2;
            return (framebuffer[bytePos] >> bitShift) & 0x3;
        #endif
    }
    
    // Clear framebuffer
    void clear(PixelIndex colorIndex = 0) {
        #if WISP_MEMORY_PROFILE == PROFILE_FULL
        uint8_t* buffer = doubleBuffering ? backbuffer : framebuffer;
        #else
        uint8_t* buffer = framebuffer;
        #endif
        
        #if PALETTE_COLOR_DEPTH == DEPTH_8BIT
            memset(buffer, colorIndex, FRAMEBUFFER_SIZE_BYTES);
        #else
            // For packed pixels, create fill pattern
            uint8_t fillByte = 0;
            for (int i = 0; i < PIXELS_PER_BYTE; i++) {
                fillByte |= (colorIndex << (i * PALETTE_COLOR_DEPTH));
            }
            memset(buffer, fillByte, FRAMEBUFFER_SIZE_BYTES);
        #endif
    }
    
    // Draw sprite at position using palette indices
    void drawSprite(int16_t x, int16_t y, const uint8_t* spriteData, 
                   uint8_t width, uint8_t height, uint8_t paletteOffset = 0) {
        
        for (uint8_t sy = 0; sy < height; sy++) {
            for (uint8_t sx = 0; sx < width; sx++) {
                uint8_t colorIndex = spriteData[sy * width + sx];
                if (colorIndex != 0) {  // 0 = transparent
                    setPixel(x + sx, y + sy, colorIndex + paletteOffset);
                }
            }
        }
    }
    
    // Fill rectangle
    void fillRect(int16_t x, int16_t y, uint8_t width, uint8_t height, PixelIndex colorIndex) {
        for (uint8_t ry = 0; ry < height; ry++) {
            for (uint8_t rx = 0; rx < width; rx++) {
                setPixel(x + rx, y + ry, colorIndex);
            }
        }
    }
    
    // Convert palette framebuffer to RGB565 for display
    void renderToDisplay(uint16_t* displayBuffer) {
        const uint8_t* buffer = framebuffer;
        
        for (uint32_t pixelPos = 0; pixelPos < PIXELS_TOTAL; pixelPos++) {
            PixelIndex colorIndex;
            
            #if PALETTE_COLOR_DEPTH == DEPTH_8BIT
                colorIndex = buffer[pixelPos];
                
            #elif PALETTE_COLOR_DEPTH == DEPTH_4BIT
                uint32_t bytePos = pixelPos / 2;
                uint8_t bitShift = (pixelPos & 1) * 4;
                colorIndex = (buffer[bytePos] >> bitShift) & 0xF;
                
            #elif PALETTE_COLOR_DEPTH == DEPTH_2BIT
                uint32_t bytePos = pixelPos / 4;
                uint8_t bitShift = (pixelPos & 3) * 2;
                colorIndex = (buffer[bytePos] >> bitShift) & 0x3;
            #endif
            
            // Look up RGB565 color in palette
            displayBuffer[pixelPos] = palettes[activePalette][colorIndex].rgb565;
        }
    }
    
    // Swap buffers (if double buffering enabled)
    void swapBuffers() {
        #if WISP_MEMORY_PROFILE == PROFILE_FULL
        if (doubleBuffering) {
            memcpy(framebuffer, backbuffer, FRAMEBUFFER_SIZE_BYTES);
        }
        #endif
    }
    
    // Update animated palette colors
    void updateAnimations() {
        for (uint8_t p = 0; p < PALETTES_COUNT; p++) {
            for (uint8_t i = 0; i < animatedCounts[p]; i++) {
                AnimatedPaletteColor& anim = animatedColors[p][i];
                
                if (anim.frameTimer > 0) {
                    anim.frameTimer--;
                } else {
                    anim.currentFrame = (anim.currentFrame + 1) % anim.frameCount;
                    anim.frameTimer = anim.frameDuration;
                    
                    // Update palette entry with new frame
                    palettes[p][i].rgb565 = anim.frames[anim.currentFrame];
                }
            }
        }
    }
    
    // Debug and stats
    void printStats() {
        Serial.println("=== Palette Framebuffer Stats ===");
        Serial.print("Color depth: ");
        Serial.print(PALETTE_COLOR_DEPTH);
        Serial.println(" bits");
        Serial.print("Palettes: ");
        Serial.println(PALETTES_COUNT);
        Serial.print("Colors per palette: ");
        Serial.println(1 << PALETTE_COLOR_DEPTH);
        Serial.print("Framebuffer size: ");
        Serial.print(FRAMEBUFFER_SIZE_BYTES);
        Serial.println(" bytes");
        
        #if PALETTE_COLOR_DEPTH == DEPTH_8BIT
        Serial.println("Memory savings: 50% vs RGB565");
        #elif PALETTE_COLOR_DEPTH == DEPTH_4BIT
        Serial.println("Memory savings: 75% vs RGB565");
        #elif PALETTE_COLOR_DEPTH == DEPTH_2BIT
        Serial.println("Memory savings: 87.5% vs RGB565");
        #endif
        
        Serial.print("Total memory usage: ");
        Serial.print(getMemoryUsage());
        Serial.println(" bytes");
        Serial.print("Active palette: ");
        Serial.println(activePalette);
        Serial.println("================================");
    }
};

// Memory savings comparison
/*
Original RGB565 framebuffer: 172 × 320 × 2 = 110,080 bytes (~110KB)

Palette-based alternatives:
- 8-bit indexed: 172 × 320 × 1 = 55,040 bytes (~55KB)   - 50% savings
- 4-bit indexed: 172 × 320 ÷ 2 = 27,520 bytes (~27KB)   - 75% savings  
- 2-bit indexed: 172 × 320 ÷ 4 = 13,760 bytes (~14KB)   - 87.5% savings

Plus palette storage:
- 8-bit: 256 colors × 2 bytes = 512 bytes per palette
- 4-bit: 16 colors × 2 bytes = 32 bytes per palette
- 2-bit: 4 colors × 2 bytes = 8 bytes per palette

Total savings with 4 palettes:
- 8-bit: 55KB + 2KB = 57KB (48% savings vs 110KB)
- 4-bit: 27KB + 128 bytes = ~27KB (75% savings vs 110KB)
- 2-bit: 14KB + 32 bytes = ~14KB (87% savings vs 110KB)
*/
