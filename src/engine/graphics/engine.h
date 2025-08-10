// graphics_engine.h - ESP32-C6/S3 Graphics Engine using ESP-IDF with LovyanGFX
// Native ESP32 implementation with Pure ESP32 native implementation
#pragma once

// Use centralized engine header for namespace organization
#include "../wisp_engine.h"
#include "engine_common.h"  // Includes display_driver.h

#include <LovyanGFX.hpp>
#include <string>
#include "renderer.h"
#include "lut_system.h"
#include "magic_channel_system.h"
#include "../../../exports/lut_palette_data.h"

// Implement Graphics namespace
namespace WispEngine::Graphics {

static const char* TAG = "GraphicsEngine";

constexpr size_t SPRITE_LUT_SIZE = 64;
constexpr size_t MAX_SPRITES = 256;
// MAX_DEPTH_LAYERS defined in definitions.h (13)

// Map platform-defined display constants to graphics engine constants
constexpr uint16_t SCREEN_WIDTH = DISPLAY_WIDTH;
constexpr uint16_t SCREEN_HEIGHT = DISPLAY_HEIGHT;
constexpr size_t SCREEN_BUFFER_SIZE = (SCREEN_WIDTH * SCREEN_HEIGHT);

// Sprite data format - compact representation
struct SpriteHeader {
    uint16_t width;            // Full sprite sheet width in pixels
    uint16_t height;           // Full sprite sheet height in pixels
    uint16_t colorDataSize;    // Size of color index array
    uint16_t depthDataSize;    // Size of depth run-length data
    uint8_t paletteId;         // Which palette slot to use (0-3)
    uint8_t flags;             // Sprite flags (reserved for future use)
    uint8_t frameRows;         // Number of frame rows in sprite sheet
    uint8_t frameCols;         // Number of frame columns in sprite sheet
    uint16_t frameWidth;       // Width of individual frame
    uint16_t frameHeight;      // Height of individual frame
};

// Depth run-length encoding entry
struct DepthRun {
    uint8_t depth;      // Depth value (0-12)
    uint16_t distance;  // How many pixels this depth applies to
};

// Loaded sprite in memory
struct Sprite {
    SpriteHeader header;
    uint8_t* colorData;        // Color indices (width * height)
    DepthRun* depthRuns;       // Run-length encoded depth data
    uint16_t depthRunCount;
    bool loaded;
};

// Render context for depth sorting and effects
struct RenderContext {
    uint8_t* depthBuffer;      // Per-pixel depth values
    uint16_t* frameBuffer;     // RGB565 frame buffer
    uint8_t currentPalette;    // Active palette for rendering
    uint8_t currentAlpha;      // Global alpha for layer rendering
    uint8_t spriteAlpha;       // Individual sprite alpha
    bool depthTestEnabled;
};

class GraphicsEngine {
public:
    // Core systems
    LGFX* display;
    ColorRenderer* palette;
    RenderContext renderCtx;
    
    // Enhanced LUT system instance
    EnhancedLUTSystem enhancedLUT;
    
    // Magic channel animation system
    MagicChannelSystem* magicChannels;
    
    // Sprite management
    Sprite sprites[MAX_SPRITES];
    uint16_t loadedSpriteCount;
    
    // Color LUT (64x64 lookup table)
    uint16_t colorLUT[SPRITE_LUT_SIZE * SPRITE_LUT_SIZE];
    bool lutLoaded;
    bool useEnhancedLUT;  // Whether to use enhanced LUT system with dynamic slots

    void init(LGFX* displayPtr, ColorRenderer* palettePtr) {
        display = displayPtr;
        palette = palettePtr;
        loadedSpriteCount = 0;
        lutLoaded = false;
        useEnhancedLUT = true;  // Default to enhanced LUT system
        magicChannels = nullptr;  // Will be set by app loader
        
        // Initialize render context
        renderCtx.depthBuffer = (uint8_t*)malloc(SCREEN_BUFFER_SIZE);
        renderCtx.frameBuffer = (uint16_t*)malloc(SCREEN_BUFFER_SIZE * 2);
        renderCtx.currentPalette = 0;
        renderCtx.currentAlpha = 255;
        renderCtx.spriteAlpha = 255;
        renderCtx.depthTestEnabled = true;
        
        // Clear buffers
        memset(renderCtx.depthBuffer, 0, SCREEN_BUFFER_SIZE);
        memset(renderCtx.frameBuffer, 0, SCREEN_BUFFER_SIZE * 2);
        
        // Initialize sprite array
        memset(sprites, 0, sizeof(sprites));
        
        ESP_LOGI(TAG, "Graphics Engine initialized with Enhanced LUT support");
    }
    
    // Load the 64x64 color LUT from file or memory (legacy compatibility)
    bool loadColorLUT(const uint16_t* lutData) {
        if (!lutData) return false;
        
        memcpy(colorLUT, lutData, SPRITE_LUT_SIZE * SPRITE_LUT_SIZE * 2);
        lutLoaded = true;
        
        ESP_LOGI(TAG, "Color LUT loaded (64x64)");
        return true;
    }
    
    // Load Enhanced LUT (64x64 with dynamic slots)
    bool loadEnhancedLUT(const uint16_t* lutData) {
        if (!lutData) return false;
        
        bool success = enhancedLUT.loadBaseLUT(lutData);
        if (success) {
            useEnhancedLUT = true;
            ESP_LOGI(TAG, "Enhanced LUT loaded (64x64 with dynamic slots)");
        }
        return success;
    }
    
    // Update enhanced LUT slots for current app frame
    void updateLUTForFrame(uint32_t currentFrameTick) {
        if (useEnhancedLUT) {
            enhancedLUT.updateSlotsForFrame(currentFrameTick);
        }
        
        // Update magic channels
        if (magicChannels && magicChannels->isEnabled()) {
            magicChannels->updateChannelsForFrame(currentFrameTick);
        }
    }
    
    // Configure dynamic slot animation
    bool configureLUTSlot(uint8_t slotIndex, const uint16_t* colorSequence, uint8_t sequenceLength) {
        if (!useEnhancedLUT) return false;
        return enhancedLUT.setSlotSequence(slotIndex, colorSequence, sequenceLength);
    }
    
    // Disable LUT slot (make transparent)
    void disableLUTSlot(uint8_t slotIndex) {
        if (useEnhancedLUT) {
            enhancedLUT.disableSlot(slotIndex);
        }
    }
    
    // Load the 64x64 color LUT from SD card image
    bool loadColorLUTFromFile(const std::string& filePath) {
        // TODO: Load from lut3d_flat.png and convert to RGB565 array
        // For now, generate a test LUT
        generateTestLUT();
        return true;
    }
    
    // Generate a test LUT for development
    void generateTestLUT() {
        for (int y = 0; y < SPRITE_LUT_SIZE; y++) {
            for (int x = 0; x < SPRITE_LUT_SIZE; x++) {
                // Create a gradient pattern for testing
                uint8_t r = (x * 255) / SPRITE_LUT_SIZE;
                uint8_t g = (y * 255) / SPRITE_LUT_SIZE;
                uint8_t b = ((x + y) * 255) / (SPRITE_LUT_SIZE * 2);
                
                // Convert RGB888 to RGB565
                uint16_t color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                colorLUT[y * SPRITE_LUT_SIZE + x] = color;
            }
        }
        lutLoaded = true;
        ESP_LOGI(TAG, "Test Color LUT generated");
    }
    
    // Load sprite from memory data
    uint16_t loadSprite(const uint8_t* spriteData) {
        if (loadedSpriteCount >= MAX_SPRITES) {
            ESP_LOGE(TAG, "ERROR: Maximum sprites loaded");
            return 0xFFFF;
        }
        
        uint16_t spriteId = loadedSpriteCount++;
        Sprite& sprite = sprites[spriteId];
        
        // Parse header
        memcpy(&sprite.header, spriteData, sizeof(SpriteHeader));
        const uint8_t* dataPtr = spriteData + sizeof(SpriteHeader);
        
        // Allocate and copy color data
        sprite.colorData = (uint8_t*)malloc(sprite.header.colorDataSize);
        memcpy(sprite.colorData, dataPtr, sprite.header.colorDataSize);
        dataPtr += sprite.header.colorDataSize;
        
        // Parse depth runs
        sprite.depthRunCount = sprite.header.depthDataSize / sizeof(DepthRun);
        sprite.depthRuns = (DepthRun*)malloc(sprite.header.depthDataSize);
        memcpy(sprite.depthRuns, dataPtr, sprite.header.depthDataSize);
        
        sprite.loaded = true;
        
        ESP_LOGI(TAG, "Sprite loaded: %d (%dx%d)", spriteId, sprite.header.width, sprite.header.height);
        
        return spriteId;
    }
    
    // Clear frame and depth buffers
    void clearBuffers(uint16_t clearColor = 0x0000) {
        // Clear depth buffer to maximum depth
        memset(renderCtx.depthBuffer, 255, SCREEN_BUFFER_SIZE);
        
        // Clear frame buffer to specified color
        for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
            renderCtx.frameBuffer[i] = clearColor;
        }
    }
    
    // Render sprite to frame buffer with depth testing
    void drawSprite(uint16_t spriteId, int16_t x, int16_t y, uint8_t paletteOverride = 0xFF) {
        drawSpriteFrame(spriteId, x, y, 0, 0, paletteOverride); // Default to frame 0,0
    }
    
    // Render specific frame from sprite sheet
    void drawSpriteFrame(uint16_t spriteId, int16_t x, int16_t y, uint8_t frameRow, uint8_t frameCol, uint8_t paletteOverride = 0xFF) {
        if (spriteId >= loadedSpriteCount || !sprites[spriteId].loaded) {
            return;
        }
        
        const Sprite& sprite = sprites[spriteId];
        uint8_t activePalette = (paletteOverride != 0xFF) ? paletteOverride : sprite.header.paletteId;
        
        // Validate frame coordinates
        if (frameRow >= sprite.header.frameRows || frameCol >= sprite.header.frameCols) {
            ESP_LOGE(TAG, "ERROR: Invalid frame coordinates for sprite");
            return;
        }
        
        // Calculate frame dimensions
        uint16_t frameWidth = sprite.header.frameWidth;
        uint16_t frameHeight = sprite.header.frameHeight;
        
        // Calculate starting position in sprite data for this frame
        uint16_t frameStartX = frameCol * frameWidth;
        uint16_t frameStartY = frameRow * frameHeight;
        
        // Build depth map from run-length encoding
        uint8_t* spriteDepthMap = expandDepthRuns(sprite.depthRuns, sprite.depthRunCount, 
                                                  sprite.header.width * sprite.header.height);
        
        // Render frame pixel by pixel with depth testing
        for (uint16_t py = 0; py < frameHeight; py++) {
            for (uint16_t px = 0; px < frameWidth; px++) {
                int16_t screenX = x + px;
                int16_t screenY = y + py;
                
                // Bounds check
                if (screenX < 0 || screenX >= SCREEN_WIDTH || 
                    screenY < 0 || screenY >= SCREEN_HEIGHT) {
                    continue;
                }
                
                // Calculate source pixel position in full sprite
                uint16_t srcX = frameStartX + px;
                uint16_t srcY = frameStartY + py;
                uint16_t pixelIndex = srcY * sprite.header.width + srcX;
                
                // Bounds check for source pixel
                if (srcX >= sprite.header.width || srcY >= sprite.header.height) {
                    continue;
                }
                
                uint8_t colorIndex = sprite.colorData[pixelIndex];
                uint8_t pixelDepth = spriteDepthMap[pixelIndex];
                
                // Skip invisible pixels (color index 0 or transparent in LUT)
                if (colorIndex == 0) {
                    continue;
                }
                
                uint16_t bufferIndex = screenY * SCREEN_WIDTH + screenX;
                
                // Depth test
                if (renderCtx.depthTestEnabled && pixelDepth >= renderCtx.depthBuffer[bufferIndex]) {
                    continue;
                }
                
                uint16_t finalColor;
                
                if (useEnhancedLUT) {
                    // Use Enhanced LUT system (64x64 with dynamic slots)
                    uint8_t lutX = colorIndex % ENHANCED_LUT_WIDTH;
                    uint8_t lutY = colorIndex / ENHANCED_LUT_WIDTH;
                    
                    // Check for transparency in enhanced LUT
                    if (enhancedLUT.isTransparent(lutX, lutY)) {
                        continue;  // Pixel is completely culled (100% transparent)
                    }
                    
                    finalColor = enhancedLUT.lookupColor(lutX, lutY);
                } else {
                    // Use legacy LUT system (64x64)
                    uint8_t lutX = colorIndex % SPRITE_LUT_SIZE;
                    uint8_t lutY = colorIndex / SPRITE_LUT_SIZE;
                    uint16_t baseColor = colorLUT[lutY * SPRITE_LUT_SIZE + lutX];
                    finalColor = baseColor;
                }
                
                // Apply magic channel resolution if available
                if (magicChannels && magicChannels->isEnabled()) {
                    finalColor = magicChannels->resolveMagicColor(finalColor);
                }
                
                // Apply palette color modification if available
                uint16_t paletteColor = palette->resolveColor(activePalette, colorIndex);
                if (paletteColor != 0) {
                    finalColor = paletteColor; // Palette override
                }
                
                // Write to buffers
                renderCtx.frameBuffer[bufferIndex] = finalColor;
                renderCtx.depthBuffer[bufferIndex] = pixelDepth;
            }
        }
        
        free(spriteDepthMap);
    }
    
    // Expand run-length encoded depth data to full depth map
    uint8_t* expandDepthRuns(const DepthRun* runs, uint16_t runCount, uint16_t totalPixels) {
        uint8_t* depthMap = (uint8_t*)malloc(totalPixels);
        uint16_t pixelIndex = 0;
        
        for (uint16_t i = 0; i < runCount && pixelIndex < totalPixels; i++) {
            const DepthRun& run = runs[i];
            
            for (uint16_t j = 0; j < run.distance && pixelIndex < totalPixels; j++) {
                depthMap[pixelIndex++] = run.depth;
            }
        }
        
        // Fill remaining pixels with max depth if runs don't cover everything
        while (pixelIndex < totalPixels) {
            depthMap[pixelIndex++] = 12; // Max depth
        }
        
        return depthMap;
    }
    
    // Present frame buffer to display
    void present() {
        // Copy frame buffer to display
        display->startWrite();
        display->setAddrWindow(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        display->writePixels(renderCtx.frameBuffer, SCREEN_WIDTH * SCREEN_HEIGHT);
        display->endWrite();
    }
    
    // Draw basic shapes (bypassing sprite system)
    void drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color, uint8_t depth = 0) {
        for (uint16_t py = 0; py < h; py++) {
            for (uint16_t px = 0; px < w; px++) {
                int16_t screenX = x + px;
                int16_t screenY = y + py;
                
                if (screenX >= 0 && screenX < SCREEN_WIDTH && 
                    screenY >= 0 && screenY < SCREEN_HEIGHT) {
                    
                    uint16_t bufferIndex = screenY * SCREEN_WIDTH + screenX;
                    
                    // Depth test
                    if (!renderCtx.depthTestEnabled || depth < renderCtx.depthBuffer[bufferIndex]) {
                        renderCtx.frameBuffer[bufferIndex] = color;
                        renderCtx.depthBuffer[bufferIndex] = depth;
                    }
                }
            }
        }
    }
    
    void drawPixel(int16_t x, int16_t y, uint16_t color, uint8_t depth = 0) {
        if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
            uint16_t bufferIndex = y * SCREEN_WIDTH + x;
            
            // Depth test
            if (!renderCtx.depthTestEnabled || depth < renderCtx.depthBuffer[bufferIndex]) {
                renderCtx.frameBuffer[bufferIndex] = color;
                renderCtx.depthBuffer[bufferIndex] = depth;
            }
        }
    }
    
    // Utility functions
    void setPalette(uint8_t paletteId) {
        renderCtx.currentPalette = paletteId;
    }
    
    void setDepthTest(bool enabled) {
        renderCtx.depthTestEnabled = enabled;
    }
    
    // Enhanced LUT system controls
    void setUseEnhancedLUT(bool enabled) {
        useEnhancedLUT = enabled;
        ESP_LOGI(TAG, "Enhanced LUT system: %s", enabled ? "Enabled" : "Disabled");
    }
    
    bool isUsingEnhancedLUT() const {
        return useEnhancedLUT;
    }
    
    // Setup common LUT slot effects
    void setupLUTPulseEffect(uint8_t slotIndex, uint16_t baseColor, uint8_t steps = 8) {
        if (useEnhancedLUT) {
            enhancedLUT.setupPulseEffect(slotIndex, baseColor, steps);
        }
    }
    
    void setupLUTColorCycle(uint8_t slotIndex, const uint16_t* colors, uint8_t colorCount) {
        if (useEnhancedLUT) {
            enhancedLUT.setupColorCycle(slotIndex, colors, colorCount);
        }
    }
    
    void setupLUTFlashEffect(uint8_t slotIndex, uint16_t color1, uint16_t color2, uint8_t flashRate = 4) {
        if (useEnhancedLUT) {
            enhancedLUT.setupFlashEffect(slotIndex, color1, color2, flashRate);
        }
    }
    
    // Debug LUT slots
    void debugPrintLUTSlots() const {
        if (useEnhancedLUT) {
            enhancedLUT.debugPrintSlots();
        } else {
            ESP_LOGI(TAG, "Using legacy LUT system (64x64, no dynamic slots)");
        }
    }
    
    // Get total number of frames in sprite
    uint16_t getSpriteFrameCount(uint16_t spriteId) {
        if (spriteId >= loadedSpriteCount || !sprites[spriteId].loaded) {
            return 0;
        }
        const Sprite& sprite = sprites[spriteId];
        return sprite.header.frameRows * sprite.header.frameCols;
    }
    
    // Get frame dimensions
    void getSpriteFrameSize(uint16_t spriteId, uint16_t* width, uint16_t* height) {
        if (spriteId >= loadedSpriteCount || !sprites[spriteId].loaded) {
            *width = 0;
            *height = 0;
            return;
        }
        const Sprite& sprite = sprites[spriteId];
        *width = sprite.header.frameWidth;
        *height = sprite.header.frameHeight;
    }
    
    // Get sprite sheet dimensions  
    void getSpriteSheetLayout(uint16_t spriteId, uint8_t* rows, uint8_t* cols) {
        if (spriteId >= loadedSpriteCount || !sprites[spriteId].loaded) {
            *rows = 0;
            *cols = 0;
            return;
        }
        const Sprite& sprite = sprites[spriteId];
        *rows = sprite.header.frameRows;
        *cols = sprite.header.frameCols;
    }
    
    // Layer system support - additional rendering methods
    void setGlobalAlpha(uint8_t alpha) {
        renderCtx.currentAlpha = alpha;
    }
    
    void setAlpha(uint8_t alpha) {
        renderCtx.spriteAlpha = alpha;
    }
    
    // Enhanced sprite drawing with transform support
    void drawSprite(uint16_t spriteId, float x, float y, float scaleX = 1.0f, float scaleY = 1.0f, float rotation = 0.0f) {
        drawSpriteTransformed(spriteId, x, y, 0, 0, scaleX, scaleY, rotation);
    }
    
    void drawSpriteTransformed(uint16_t spriteId, float x, float y, uint8_t frameRow, uint8_t frameCol, 
                              float scaleX, float scaleY, float rotation) {
        if (spriteId >= loadedSpriteCount || !sprites[spriteId].loaded) {
            return;
        }
        
        // For now, ignore transforms and use basic rendering
        // TODO: Implement actual transform matrix rendering
        drawSpriteFrame(spriteId, (int16_t)x, (int16_t)y, frameRow, frameCol);
    }
    
    // Region-based sprite drawing for 9-patch UI
    void drawSpriteRegion(uint16_t spriteId, float destX, float destY, 
                         uint16_t srcX, uint16_t srcY, uint16_t srcW, uint16_t srcH,
                         float destW = 0, float destH = 0) {
        if (spriteId >= loadedSpriteCount || !sprites[spriteId].loaded) {
            return;
        }
        
        const Sprite& sprite = sprites[spriteId];
        
        // Use source dimensions if dest not specified
        if (destW == 0) destW = srcW;
        if (destH == 0) destH = srcH;
        
        // Simple region copy (no scaling for now)
        // TODO: Implement proper region scaling
        for (uint16_t py = 0; py < srcH && py < destH; py++) {
            for (uint16_t px = 0; px < srcW && px < destW; px++) {
                int16_t screenX = (int16_t)(destX + px);
                int16_t screenY = (int16_t)(destY + py);
                
                // Bounds check
                if (screenX < 0 || screenX >= SCREEN_WIDTH || 
                    screenY < 0 || screenY >= SCREEN_HEIGHT) {
                    continue;
                }
                
                // Calculate source pixel position
                uint16_t pixelIndex = (srcY + py) * sprite.header.width + (srcX + px);
                
                if ((srcX + px) >= sprite.header.width || (srcY + py) >= sprite.header.height) {
                    continue;
                }
                
                uint8_t colorIndex = sprite.colorData[pixelIndex];
                
                // Skip transparent pixels
                if (colorIndex == 0) {
                    continue;
                }
                
                // Get color from LUT
                uint8_t lutX = colorIndex % SPRITE_LUT_SIZE;
                uint8_t lutY = colorIndex / SPRITE_LUT_SIZE;
                uint16_t color = colorLUT[lutY * SPRITE_LUT_SIZE + lutX];
                
                // Apply alpha blending if needed
                if (renderCtx.spriteAlpha < 255) {
                    color = blendAlpha(color, renderCtx.frameBuffer[screenY * SCREEN_WIDTH + screenX], renderCtx.spriteAlpha);
                }
                
                renderCtx.frameBuffer[screenY * SCREEN_WIDTH + screenX] = color;
            }
        }
    }
    
    // Gradient rendering for layer 0
    void drawGradient(float x, float y, float width, float height, uint16_t colorTop, uint16_t colorBottom) {
        for (int py = 0; py < (int)height; py++) {
            for (int px = 0; px < (int)width; px++) {
                int16_t screenX = (int16_t)(x + px);
                int16_t screenY = (int16_t)(y + py);
                
                if (screenX >= 0 && screenX < SCREEN_WIDTH && 
                    screenY >= 0 && screenY < SCREEN_HEIGHT) {
                    
                    // Calculate gradient interpolation
                    float t = (float)py / height;
                    uint16_t color = lerpColor(colorTop, colorBottom, t);
                    
                    uint16_t bufferIndex = screenY * SCREEN_WIDTH + screenX;
                    renderCtx.frameBuffer[bufferIndex] = color;
                }
            }
        }
    }
    
    // Alpha blending utility
    uint16_t blendAlpha(uint16_t src, uint16_t dst, uint8_t alpha) {
        if (alpha == 255) return src;
        if (alpha == 0) return dst;
        
        // Extract RGB components
        uint8_t srcR = (src >> 11) & 0x1F;
        uint8_t srcG = (src >> 5) & 0x3F;
        uint8_t srcB = src & 0x1F;
        
        uint8_t dstR = (dst >> 11) & 0x1F;
        uint8_t dstG = (dst >> 5) & 0x3F;
        uint8_t dstB = dst & 0x1F;
        
        // Blend
        uint8_t blendR = (srcR * alpha + dstR * (255 - alpha)) / 255;
        uint8_t blendG = (srcG * alpha + dstG * (255 - alpha)) / 255;
        uint8_t blendB = (srcB * alpha + dstB * (255 - alpha)) / 255;
        
        return (blendR << 11) | (blendG << 5) | blendB;
    }
    
    // Color interpolation
    uint16_t lerpColor(uint16_t color1, uint16_t color2, float t) {
        uint8_t r1 = (color1 >> 11) & 0x1F;
        uint8_t g1 = (color1 >> 5) & 0x3F;
        uint8_t b1 = color1 & 0x1F;
        
        uint8_t r2 = (color2 >> 11) & 0x1F;
        uint8_t g2 = (color2 >> 5) & 0x3F;
        uint8_t b2 = color2 & 0x1F;
        
        uint8_t r = (uint8_t)(r1 + (r2 - r1) * t);
        uint8_t g = (uint8_t)(g1 + (g2 - g1) * t);
        uint8_t b = (uint8_t)(b1 + (b2 - b1) * t);
        
        return (r << 11) | (g << 5) | b;
    }
    
    // Debug functions
    void debugPrintSpriteInfo(uint16_t spriteId) {
        if (spriteId >= loadedSpriteCount) return;
        
        const Sprite& sprite = sprites[spriteId];
        ESP_LOGI(TAG, "Sprite %d: %dx%d, Frames: %dx%d (%dx%d each), Palette: %d, Depth runs: %d", 
                 spriteId, sprite.header.width, sprite.header.height,
                 sprite.header.frameRows, sprite.header.frameCols,
                 sprite.header.frameWidth, sprite.header.frameHeight,
                 sprite.header.paletteId, sprite.depthRunCount);
    }
    
    void debugDrawDepthBuffer() {
        // Visualize depth buffer as grayscale
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            for (int x = 0; x < SCREEN_WIDTH; x++) {
                uint8_t depth = renderCtx.depthBuffer[y * SCREEN_WIDTH + x];
                uint8_t gray = (depth * 255) / 12;  // Scale 0-12 to 0-255
                uint16_t color = ((gray & 0xF8) << 8) | ((gray & 0xFC) << 3) | (gray >> 3);
                renderCtx.frameBuffer[y * SCREEN_WIDTH + x] = color;
            }
        }
    }
    
    ~GraphicsEngine() {
        // Clean up allocated memory
        if (renderCtx.depthBuffer) free(renderCtx.depthBuffer);
        if (renderCtx.frameBuffer) free(renderCtx.frameBuffer);
        
        for (uint16_t i = 0; i < loadedSpriteCount; i++) {
            if (sprites[i].loaded) {
                if (sprites[i].colorData) free(sprites[i].colorData);
                if (sprites[i].depthRuns) free(sprites[i].depthRuns);
            }
        }
    }
};

} // namespace WispEngine::Graphics
