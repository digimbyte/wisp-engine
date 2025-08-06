// examples/depth_test_app.cpp - Depth Buffer and Z-Ordering Test
// Tests sprite depth layering, depth buffer functionality, and layer sorting

#include "../src/engine/app/interface.h"

class DepthTestApp : public WispAppBase {
private:
    // Depth layer visualization
    struct DepthLayer {
        float x, y;
        uint8_t depth;
        WispColor color;
        bool moving;
        float dx, dy;
    };
    
    DepthLayer layers[10];
    uint8_t layerCount = 10;
    
    // Interactive depth control
    uint8_t selectedLayer = 0;
    bool showDepthNumbers = true;

public:
    bool init() override {
        setAppInfo("Depth Test", "1.0.0", "Wisp Engine Team");
        
        // Initialize depth layers with different colors and depths
        WispColor depthColors[10] = {
            WispColor(255, 0, 0),    // Red - depth 0 (back)
            WispColor(255, 128, 0),  // Orange - depth 1
            WispColor(255, 255, 0),  // Yellow - depth 2
            WispColor(128, 255, 0),  // Lime - depth 3
            WispColor(0, 255, 0),    // Green - depth 4
            WispColor(0, 255, 128),  // Cyan - depth 5
            WispColor(0, 128, 255),  // Light Blue - depth 6
            WispColor(0, 0, 255),    // Blue - depth 7
            WispColor(128, 0, 255),  // Purple - depth 8
            WispColor(255, 0, 255)   // Magenta - depth 9 (front)
        };
        
        for (int i = 0; i < 10; i++) {
            layers[i] = {
                160 + (i - 5) * 8.0f,  // Slightly offset positions
                120 + (i - 5) * 6.0f,
                (uint8_t)i,  // Depth corresponds to index
                depthColors[i],
                (i % 3 == 0),  // Every 3rd layer moves
                api->random(-1.0f, 1.0f),  // Random movement
                api->random(-1.0f, 1.0f)
            };
        }
        
        api->print("Depth Test App initialized");
        api->print("Controls: Up/Down - Select Layer, A - Toggle Movement, B - Toggle Numbers");
        return true;
    }
    
    void update() override {
        // Handle input
        const WispInputState& input = api->getInput();
        static WispInputState lastInput;
        
        // Layer selection
        if (input.up && !lastInput.up) {
            selectedLayer = (selectedLayer + 1) % layerCount;
            api->print("Selected layer: " + std::to_string(selectedLayer));
        }
        if (input.down && !lastInput.down) {
            selectedLayer = (selectedLayer - 1 + layerCount) % layerCount;
            api->print("Selected layer: " + std::to_string(selectedLayer));
        }
        
        // Toggle movement for selected layer
        if (input.buttonA && !lastInput.buttonA) {
            layers[selectedLayer].moving = !layers[selectedLayer].moving;
            api->print("Layer " + std::to_string(selectedLayer) + " movement: " + 
                      (layers[selectedLayer].moving ? "ON" : "OFF"));
        }
        
        // Toggle depth number display
        if (input.buttonB && !lastInput.buttonB) {
            showDepthNumbers = !showDepthNumbers;
            api->print("Depth numbers: " + std::string(showDepthNumbers ? "ON" : "OFF"));
        }
        
        // Swap depths with left/right
        if (input.left && !lastInput.left && selectedLayer > 0) {
            // Swap with layer below
            std::swap(layers[selectedLayer].depth, layers[selectedLayer - 1].depth);
            api->print("Swapped depths: " + std::to_string(selectedLayer) + " <-> " + std::to_string(selectedLayer - 1));
        }
        if (input.right && !lastInput.right && selectedLayer < layerCount - 1) {
            // Swap with layer above  
            std::swap(layers[selectedLayer].depth, layers[selectedLayer + 1].depth);
            api->print("Swapped depths: " + std::to_string(selectedLayer) + " <-> " + std::to_string(selectedLayer + 1));
        }
        
        lastInput = input;
        
        // Update moving layers
        for (int i = 0; i < layerCount; i++) {
            if (layers[i].moving) {
                layers[i].x += layers[i].dx;
                layers[i].y += layers[i].dy;
                
                // Bounce off screen edges
                if (layers[i].x < 20 || layers[i].x > 300) layers[i].dx = -layers[i].dx;
                if (layers[i].y < 40 || layers[i].y > 200) layers[i].dy = -layers[i].dy;
            }
        }
    }
    
    void render() override {
        // Clear with dark background
        api->drawRect(0, 0, 320, 240, WispColor(10, 10, 20), 0);
        
        // Draw title
        api->drawText("DEPTH TEST", 160, 10, WispColor(255, 255, 255), 10);
        
        // Draw depth layers (each as colored rectangles)
        for (int i = 0; i < layerCount; i++) {
            const DepthLayer& layer = layers[i];
            
            // Highlight selected layer
            if (i == selectedLayer) {
                // Draw highlight border
                api->drawRect(layer.x - 22, layer.y - 22, 44, 44, WispColor(255, 255, 255), layer.depth);
            }
            
            // Draw main layer rectangle
            api->drawRect(layer.x - 20, layer.y - 20, 40, 40, layer.color, layer.depth);
            
            // Draw depth number if enabled
            if (showDepthNumbers) {
                std::string depthStr = std::to_string(layer.depth);
                WispColor textColor = (layer.color.r + layer.color.g + layer.color.b > 384) ? 
                    WispColor(0, 0, 0) : WispColor(255, 255, 255);
                api->drawText(depthStr, layer.x - 4, layer.y - 4, textColor, layer.depth + 1);
            }
        }
        
        // Draw UI instructions
        api->drawText("Up/Down: Select Layer", 10, 210, WispColor(200, 200, 200), 10);
        api->drawText("Left/Right: Swap Depths", 10, 225, WispColor(200, 200, 200), 10);
        api->drawText("A: Toggle Movement  B: Toggle Numbers", 180, 210, WispColor(200, 200, 200), 10);
        
        // Draw selected layer info
        std::string info = "Selected: Layer " + std::to_string(selectedLayer) + 
                          " (Depth " + std::to_string(layers[selectedLayer].depth) + ")";
        api->drawText(info, 160, 30, WispColor(255, 255, 0), 10);
    }
    
    void cleanup() override {
        api->print("Depth Test App cleaned up");
    }
};

// Export function for the engine
extern "C" WispAppBase* createDepthTestApp() {
    return new DepthTestApp();
}

extern "C" void destroyDepthTestApp(WispAppBase* app) {
    delete app;
}
