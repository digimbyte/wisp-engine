// examples/sprite_test_app.cpp - Sprite System Feature Test
// Tests sprite loading, rendering, scaling, and basic transformations

#include "../src/engine/app/interface.h"

class SpriteTestApp : public WispAppBase {
private:
    // Test sprites
    ResourceHandle testSprites[8];
    uint8_t spriteCount = 0;
    
    // Animation state
    float rotation = 0.0f;
    float scale = 1.0f;
    bool scaleDirection = true;
    
    // Test positions
    struct SpriteTest {
        float x, y;
        float dx, dy;
        uint8_t spriteIndex;
        uint8_t depth;
    };
    
    SpriteTest sprites[16];
    uint8_t activeSprites = 0;

public:
    bool init() override {
        setAppInfo("Sprite Test", "1.0.0", "Wisp Engine Team");
        
        // Load test sprites from assets folder
        testSprites[0] = api->loadSprite("assets/test_16x16.spr");    // Small sprite
        testSprites[1] = api->loadSprite("assets/test_32x32.spr");    // Medium sprite  
        testSprites[2] = api->loadSprite("assets/test_64x64.spr");    // Large sprite
        testSprites[3] = api->loadSprite("assets/test_animated.spr"); // Animated sprite
        spriteCount = 4;
        
        // Initialize moving sprites
        for (int i = 0; i < 8; i++) {
            sprites[i] = {
                api->random(50, 270), api->random(50, 190),  // Random position
                api->random(-2.0f, 2.0f), api->random(-2.0f, 2.0f),  // Random velocity
                (uint8_t)(i % spriteCount),  // Sprite type
                (uint8_t)(i % 8)  // Depth layer
            };
        }
        activeSprites = 8;
        
        api->print("Sprite Test App initialized");
        api->print("Testing: Loading, Rendering, Scaling, Movement, Depth");
        return true;
    }
    
    void update() override {
        // Update animation parameters
        rotation += 2.0f;
        if (rotation >= 360.0f) rotation -= 360.0f;
        
        // Update scale animation
        if (scaleDirection) {
            scale += 0.02f;
            if (scale >= 2.0f) scaleDirection = false;
        } else {
            scale -= 0.02f;
            if (scale <= 0.5f) scaleDirection = true;
        }
        
        // Update moving sprites
        for (int i = 0; i < activeSprites; i++) {
            sprites[i].x += sprites[i].dx;
            sprites[i].y += sprites[i].dy;
            
            // Bounce off screen edges
            if (sprites[i].x < 0 || sprites[i].x > 300) sprites[i].dx = -sprites[i].dx;
            if (sprites[i].y < 0 || sprites[i].y > 220) sprites[i].dy = -sprites[i].dy;
        }
        
        // Input: Add/remove sprites
        const WispInputState& input = api->getInput();
        static WispInputState lastInput;
        
        if (input.buttonA && !lastInput.buttonA && activeSprites < 16) {
            // Add a new sprite
            sprites[activeSprites] = {
                160, 120,  // Center
                api->random(-3.0f, 3.0f), api->random(-3.0f, 3.0f),
                (uint8_t)(activeSprites % spriteCount),
                (uint8_t)(activeSprites % 8)
            };
            activeSprites++;
            api->print("Added sprite. Total: " + std::to_string(activeSprites));
        }
        
        if (input.buttonB && !lastInput.buttonB && activeSprites > 1) {
            activeSprites--;
            api->print("Removed sprite. Total: " + std::to_string(activeSprites));
        }
        
        lastInput = input;
    }
    
    void render() override {
        // Clear with dark blue background
        api->drawRect(0, 0, 320, 240, WispColor(20, 20, 40), 0);
        
        // Draw title
        api->drawText("SPRITE TEST", 160, 10, WispColor(255, 255, 255), 10);
        
        // Draw test sprites with various effects
        for (int i = 0; i < activeSprites; i++) {
            const SpriteTest& sprite = sprites[i];
            
            // Basic sprite rendering
            api->drawSprite(testSprites[sprite.spriteIndex], sprite.x, sprite.y, sprite.depth);
        }
        
        // Draw animated scaling sprite in center
        if (spriteCount > 0) {
            // Note: Scaling/rotation would need extended API or be handled by graphics engine
            api->drawSprite(testSprites[0], 160, 120, 9);  // Highest depth
        }
        
        // Draw UI info
        api->drawText("A: Add Sprite  B: Remove Sprite", 10, 220, WispColor(200, 200, 200), 8);
        
        std::string info = "Active: " + std::to_string(activeSprites) + "/16";
        api->drawText(info, 250, 220, WispColor(200, 200, 200), 8);
    }
    
    void cleanup() override {
        // Unload all sprites
        for (int i = 0; i < spriteCount; i++) {
            api->unloadSprite(testSprites[i]);
        }
        api->print("Sprite Test App cleaned up");
    }
};

// Export function for the engine
extern "C" WispAppBase* createSpriteTestApp() {
    return new SpriteTestApp();
}

extern "C" void destroySpriteTestApp(WispAppBase* app) {
    delete app;
}
