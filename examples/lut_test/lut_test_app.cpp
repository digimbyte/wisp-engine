// examples/lut_test_app.cpp - Look-Up Table (LUT) System Test
// Tests palette LUT system, color mapping, and dynamic LUT updates

#include "../src/engine/app/interface.h"

class LUTTestApp : public WispAppBase {
private:
    // LUT test modes
    enum LUTMode {
        MODE_BASIC_PALETTE,      // Basic 16-color palette
        MODE_GRADIENT,           // Smooth gradients
        MODE_ANIMATED_PALETTE,   // Animated color cycling
        MODE_LOOKUP_TABLE,       // Direct LUT manipulation
        MODE_COUNT
    };
    
    LUTMode currentMode = MODE_BASIC_PALETTE;
    
    // Animation state
    float colorCycleTime = 0.0f;
    uint8_t paletteOffset = 0;
    
    // Test patterns
    uint8_t testPattern[64*48]; // 64x48 palette index pattern
    
    // LUT data (simplified - would interface with graphics engine)
    struct LUTEntry {
        uint8_t r, g, b;
    };
    LUTEntry currentLUT[256];
    
    // Interactive controls
    uint8_t selectedPaletteIndex = 0;
    bool editMode = false;

public:
    bool init() override {
        setAppInfo("LUT Test", "1.0.0", "Wisp Engine Team");
        
        // Generate test pattern
        generateTestPattern();
        
        // Initialize with basic palette
        setBasicPalette();
        
        api->print("LUT Test App initialized");
        api->print("Controls: Up/Down - Mode, A - Edit LUT, B - Reset, Left/Right - Select Color");
        return true;
    }
    
    void generateTestPattern() {
        // Create a test pattern that uses palette indices
        for (int y = 0; y < 48; y++) {
            for (int x = 0; x < 64; x++) {
                int index = y * 64 + x;
                
                // Create various patterns
                if (y < 12) {
                    // Color bars
                    testPattern[index] = (x / 8) % 16;
                } else if (y < 24) {
                    // Gradient
                    testPattern[index] = (x * 16) / 64;
                } else if (y < 36) {
                    // Checkerboard
                    testPattern[index] = ((x / 4) + (y / 4)) % 2 ? 15 : 0;
                } else {
                    // Noise pattern
                    testPattern[index] = (x + y * 7 + x * y / 3) % 16;
                }
            }
        }
    }
    
    void setBasicPalette() {
        // Basic 16-color palette
        WispColor basicColors[16] = {
            WispColor(0, 0, 0),       // Black
            WispColor(128, 0, 0),     // Dark Red
            WispColor(0, 128, 0),     // Dark Green  
            WispColor(128, 128, 0),   // Dark Yellow
            WispColor(0, 0, 128),     // Dark Blue
            WispColor(128, 0, 128),   // Dark Magenta
            WispColor(0, 128, 128),   // Dark Cyan
            WispColor(192, 192, 192), // Light Gray
            WispColor(128, 128, 128), // Dark Gray
            WispColor(255, 0, 0),     // Red
            WispColor(0, 255, 0),     // Green
            WispColor(255, 255, 0),   // Yellow
            WispColor(0, 0, 255),     // Blue
            WispColor(255, 0, 255),   // Magenta
            WispColor(0, 255, 255),   // Cyan
            WispColor(255, 255, 255)  // White
        };
        
        for (int i = 0; i < 16; i++) {
            currentLUT[i] = {basicColors[i].r, basicColors[i].g, basicColors[i].b};
        }
        
        // Fill remaining entries with black
        for (int i = 16; i < 256; i++) {
            currentLUT[i] = {0, 0, 0};
        }
    }
    
    void setGradientPalette() {
        // Smooth RGB gradient
        for (int i = 0; i < 256; i++) {
            float t = i / 255.0f;
            
            if (t < 0.33f) {
                // Red to Green
                float localT = t / 0.33f;
                currentLUT[i] = {
                    (uint8_t)(255 * (1.0f - localT)),
                    (uint8_t)(255 * localT),
                    0
                };
            } else if (t < 0.66f) {
                // Green to Blue
                float localT = (t - 0.33f) / 0.33f;
                currentLUT[i] = {
                    0,
                    (uint8_t)(255 * (1.0f - localT)),
                    (uint8_t)(255 * localT)
                };
            } else {
                // Blue to Red
                float localT = (t - 0.66f) / 0.34f;
                currentLUT[i] = {
                    (uint8_t)(255 * localT),
                    0,
                    (uint8_t)(255 * (1.0f - localT))
                };
            }
        }
    }
    
    void updateAnimatedPalette(float deltaTime) {
        colorCycleTime += deltaTime;
        
        // Cycle through HSV color space
        for (int i = 0; i < 16; i++) {
            float hue = (colorCycleTime * 60.0f + i * 22.5f);
            while (hue >= 360.0f) hue -= 360.0f;
            
            WispColor color = hsvToRgb(hue, 1.0f, 1.0f);
            currentLUT[i] = {color.r, color.g, color.b};
        }
    }
    
    WispColor hsvToRgb(float h, float s, float v) {
        float c = v * s;
        float x = c * (1.0f - abs(fmod(h / 60.0f, 2.0f) - 1.0f));
        float m = v - c;
        
        float r, g, b;
        if (h < 60) { r = c; g = x; b = 0; }
        else if (h < 120) { r = x; g = c; b = 0; }
        else if (h < 180) { r = 0; g = c; b = x; }
        else if (h < 240) { r = 0; g = x; b = c; }
        else if (h < 300) { r = x; g = 0; b = c; }
        else { r = c; g = 0; b = x; }
        
        return WispColor((r + m) * 255, (g + m) * 255, (b + m) * 255);
    }
    
    std::string getModeName(LUTMode mode) {
        const char* names[] = {"Basic Palette", "Gradient", "Animated Palette", "Direct LUT"};
        return names[mode];
    }
    
    void update() override {
        float deltaTime = api->getDeltaTime() / 1000.0f;
        
        // Handle input
        const WispInputState& input = api->getInput();
        static WispInputState lastInput;
        
        // Mode selection
        if (input.up && !lastInput.up) {
            currentMode = (LUTMode)((currentMode + 1) % MODE_COUNT);
            api->print("LUT Mode: " + getModeName(currentMode));
            
            // Apply mode-specific setup
            switch (currentMode) {
                case MODE_BASIC_PALETTE: setBasicPalette(); break;
                case MODE_GRADIENT: setGradientPalette(); break;
                case MODE_ANIMATED_PALETTE: setBasicPalette(); break;
                case MODE_LOOKUP_TABLE: /* Keep current */ break;
            }
        }
        
        if (input.down && !lastInput.down) {
            currentMode = (LUTMode)((currentMode - 1 + MODE_COUNT) % MODE_COUNT);
            api->print("LUT Mode: " + getModeName(currentMode));
        }
        
        // Palette index selection
        if (input.left && !lastInput.left) {
            selectedPaletteIndex = (selectedPaletteIndex - 1) % 16;
            api->print("Selected palette index: " + std::to_string(selectedPaletteIndex));
        }
        if (input.right && !lastInput.right) {
            selectedPaletteIndex = (selectedPaletteIndex + 1) % 16;
            api->print("Selected palette index: " + std::to_string(selectedPaletteIndex));
        }
        
        // Edit mode toggle
        if (input.buttonA && !lastInput.buttonA) {
            editMode = !editMode;
            api->print("Edit mode: " + std::string(editMode ? "ON" : "OFF"));
        }
        
        // Reset LUT
        if (input.buttonB && !lastInput.buttonB) {
            setBasicPalette();
            colorCycleTime = 0.0f;
            api->print("LUT reset to basic palette");
        }
        
        lastInput = input;
        
        // Update mode-specific animations
        if (currentMode == MODE_ANIMATED_PALETTE) {
            updateAnimatedPalette(deltaTime);
        }
        
        // Manual color editing
        if (editMode && currentMode == MODE_LOOKUP_TABLE) {
            // Simple color cycling for selected index
            uint8_t colorValue = (uint8_t)(128 + 127 * sin(api->getTime() / 200.0f));
            currentLUT[selectedPaletteIndex] = {colorValue, colorValue / 2, 255 - colorValue};
        }
    }
    
    void render() override {
        // Clear with dark background
        api->drawRect(0, 0, 320, 240, WispColor(20, 20, 30), 0);
        
        // Draw title
        api->drawText("LUT TEST", 160, 10, WispColor(255, 255, 255), 10);
        
        // Draw current mode
        api->drawText(getModeName(currentMode), 160, 25, WispColor(200, 200, 255), 9);
        
        // Draw test pattern using LUT (simulated)
        int patternStartX = 50;
        int patternStartY = 50;
        int pixelSize = 3;
        
        for (int y = 0; y < 48; y++) {
            for (int x = 0; x < 64; x++) {
                uint8_t paletteIndex = testPattern[y * 64 + x];
                LUTEntry& color = currentLUT[paletteIndex];
                
                WispColor pixelColor(color.r, color.g, color.b);
                api->drawRect(
                    patternStartX + x * pixelSize,
                    patternStartY + y * pixelSize,
                    pixelSize - 1, pixelSize - 1,
                    pixelColor, 5
                );
            }
        }
        
        // Draw palette preview
        int paletteX = 50;
        int paletteY = 200;
        for (int i = 0; i < 16; i++) {
            LUTEntry& color = currentLUT[i];
            WispColor paletteColor(color.r, color.g, color.b);
            
            // Highlight selected color
            if (i == selectedPaletteIndex) {
                api->drawRect(paletteX + i * 12 - 1, paletteY - 1, 12, 12, WispColor(255, 255, 255), 7);
            }
            
            api->drawRect(paletteX + i * 12, paletteY, 10, 10, paletteColor, 6);
            
            // Draw index number
            api->drawText(std::to_string(i), paletteX + i * 12 + 1, paletteY + 12, WispColor(255, 255, 255), 8);
        }
        
        // Draw controls
        api->drawText("Up/Down: Mode  Left/Right: Select Color", 10, 165, WispColor(180, 180, 180), 8);
        api->drawText("A: Edit Mode  B: Reset", 10, 175, WispColor(180, 180, 180), 8);
        
        // Draw selected color info
        if (selectedPaletteIndex < 16) {
            LUTEntry& selected = currentLUT[selectedPaletteIndex];
            std::string colorInfo = "Index " + std::to_string(selectedPaletteIndex) + 
                                   ": RGB(" + std::to_string(selected.r) + "," + 
                                   std::to_string(selected.g) + "," + std::to_string(selected.b) + ")";
            
            WispColor textColor = editMode ? WispColor(255, 255, 0) : WispColor(200, 200, 200);
            api->drawText(colorInfo, 250, 200, textColor, 8);
        }
        
        if (editMode) {
            api->drawText("EDIT MODE", 250, 210, WispColor(255, 255, 0), 8);
        }
    }
    
    void cleanup() override {
        api->print("LUT Test App cleaned up");
    }
};

// Export function for the engine
extern "C" WispAppBase* createLUTTestApp() {
    return new LUTTestApp();
}

extern "C" void destroyLUTTestApp(WispAppBase* app) {
    delete app;
}
