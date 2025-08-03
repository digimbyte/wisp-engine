// examples/sprite_layer_demo.cpp
#include "../src/engine/wisp_sprite_layers.h"
#include "../src/engine/graphics_engine.h"

// Example demonstrating all 8 sprite layers
class SpriteLayerDemo {
private:
    WispSpriteLayerSystem* layerSystem;
    GraphicsEngine* graphics;
    
    // Demo sprites for each layer
    WispLayeredSprite* gradientBg;      // Layer 0
    WispLayeredSprite* tiledBg;         // Layer 1
    WispLayeredSprite* backDecor;       // Layer 2
    WispLayeredSprite* player;          // Layer 3
    WispLayeredSprite* enemy;           // Layer 4
    WispLayeredSprite* projectile;      // Layer 5
    WispLayeredSprite* explosion;       // Layer 6
    WispLayeredSprite* healthBar;       // Layer 7
    WispLayeredSprite* scoreText;       // Layer 8
    
    // Multi-layer sprite demonstration
    WispLayeredSprite* depthTree;       // Appears on layers 2, 3, 4 with depth masking
    
    float cameraX, cameraY;
    float playerX, playerY;
    uint32_t lastUpdateTime;
    
public:
    SpriteLayerDemo(WispSpriteLayerSystem* layers, GraphicsEngine* gfx) :
        layerSystem(layers), graphics(gfx), 
        cameraX(0), cameraY(0), playerX(160), playerY(120), lastUpdateTime(0) {
        
        // Initialize global layer system
        g_LayerSystem = layerSystem;
        
        createDemoSprites();
        setupAnimations();
        setupDepthMasking();
    }
    
    void createDemoSprites() {
        Serial.println("Creating demo sprites for all layers...");
        
        // Layer 0: Gradient background
        gradientBg = layerSystem->createGradientSprite(0, 0, 320, 240, 0x001F, 0x7C00); // Blue to red gradient
        Serial.println("Created gradient background");
        
        // Layer 1: Tiled background (assuming sprite ID 1 is a tile texture)
        tiledBg = layerSystem->createBackgroundSprite(1, TILE_REPEAT);
        tiledBg->parallaxX = 0.5f; // Slower parallax for depth
        tiledBg->parallaxY = 0.5f;
        Serial.println("Created tiled background");
        
        // Layer 2: Back decoration (trees, rocks, etc.)
        backDecor = layerSystem->createGameSprite(2, LAYER_2_GAME_BACK);
        backDecor->x = 100;
        backDecor->y = 150;
        backDecor->renderPriority = 50; // Mid-priority in back layer
        Serial.println("Created back decoration");
        
        // Layer 3: Player sprite
        player = layerSystem->createGameSprite(3, LAYER_3_GAME_MID);
        player->x = playerX;
        player->y = playerY;
        player->renderPriority = 128; // Normal priority
        Serial.println("Created player sprite");
        
        // Layer 4: Enemy sprite
        enemy = layerSystem->createGameSprite(4, LAYER_4_GAME_FRONT);
        enemy->x = 250;
        enemy->y = 100;
        enemy->renderPriority = 100;
        Serial.println("Created enemy sprite");
        
        // Layer 5: Projectile
        projectile = layerSystem->createGameSprite(5, LAYER_5_GAME_TOP);
        projectile->x = 200;
        projectile->y = 120;
        projectile->renderPriority = 200; // High priority in top layer
        Serial.println("Created projectile sprite");
        
        // Layer 6: Explosion effect
        explosion = layerSystem->createGameSprite(6, LAYER_6_EFFECTS);
        explosion->x = 180;
        explosion->y = 90;
        explosion->renderPriority = 150;
        explosion->alpha = 200; // Semi-transparent effect
        Serial.println("Created explosion effect");
        
        // Layer 7: UI Health bar
        healthBar = layerSystem->createUISprite(7, 10, 10);
        healthBar->slice = WispSpriteSlice(4, 60, 4, 12); // 9-patch slicing
        healthBar->targetWidth = 100;
        healthBar->targetHeight = 16;
        Serial.println("Created UI health bar");
        
        // Layer 8: Score text
        scoreText = layerSystem->createTextSprite("SCORE: 12345", 10, 200);
        Serial.println("Created score text");
    }
    
    void setupAnimations() {
        Serial.println("Setting up animations...");
        
        // Player walking animation (4 frames, 150ms each)
        WispAnimationFrame walkFrames[4] = {
            WispAnimationFrame(0, 150), // Frame 0 for 150ms
            WispAnimationFrame(1, 150), // Frame 1 for 150ms
            WispAnimationFrame(2, 150), // Frame 2 for 150ms
            WispAnimationFrame(3, 150)  // Frame 3 for 150ms
        };
        
        if (layerSystem->setAnimation(player, walkFrames, 4)) {
            layerSystem->playAnimation(player, true); // Loop animation
            Serial.println("Player walk animation set");
        }
        
        // Explosion animation (8 frames, 100ms each, no loop)
        WispAnimationFrame explosionFrames[8];
        for (int i = 0; i < 8; i++) {
            explosionFrames[i] = WispAnimationFrame(i, 100);
        }
        
        if (layerSystem->setAnimation(explosion, explosionFrames, 8)) {
            explosion->animation.loop = false; // Play once
            layerSystem->playAnimation(explosion, false);
            Serial.println("Explosion animation set");
        }
        
        // Enemy floating animation with offset (pingpong movement)
        WispAnimationFrame floatFrames[2] = {
            WispAnimationFrame(0, 500, 0, -2), // Frame 0, offset up 2 pixels
            WispAnimationFrame(0, 500, 0, 2)   // Frame 0, offset down 2 pixels
        };
        
        if (layerSystem->setAnimation(enemy, floatFrames, 2)) {
            enemy->animation.pingpong = true;
            layerSystem->playAnimation(enemy, true);
            Serial.println("Enemy float animation set");
        }
    }
    
    void setupDepthMasking() {
        Serial.println("Setting up depth masking demo...");
        
        // Create a tree sprite that appears on multiple layers with different depths
        depthTree = layerSystem->createGameSprite(8, LAYER_3_GAME_MID);
        depthTree->x = 200;
        depthTree->y = 80;
        
        // Set up multi-layer appearance
        layerSystem->setMultiLayer(depthTree, {LAYER_2_GAME_BACK, LAYER_3_GAME_MID, LAYER_4_GAME_FRONT});
        
        // Set different depths for each layer
        layerSystem->setLayerDepth(depthTree, LAYER_2_GAME_BACK, 8);   // Faded on back layer
        layerSystem->setLayerDepth(depthTree, LAYER_3_GAME_MID, 10);   // Full opacity on mid layer
        layerSystem->setLayerDepth(depthTree, LAYER_4_GAME_FRONT, 6);  // More faded on front layer
        
        Serial.println("Multi-layer tree sprite configured");
    }
    
    void update() {
        uint32_t currentTime = millis();
        uint32_t deltaTime = currentTime - lastUpdateTime;
        lastUpdateTime = currentTime;
        
        // Update animations
        layerSystem->updateAnimations(deltaTime);
        
        // Move player in a circle for demo
        float time = currentTime * 0.001f; // Convert to seconds
        playerX = 160 + cos(time) * 50;
        playerY = 120 + sin(time) * 30;
        
        player->x = playerX;
        player->y = playerY;
        
        // Move projectile
        projectile->x += 2.0f;
        if (projectile->x > 320) {
            projectile->x = -32;
        }
        
        // Scroll background
        tiledBg->scrollX += 0.5f;
        tiledBg->scrollY += 0.2f;
        
        // Follow player with camera (smooth)
        layerSystem->setCameraSmooth(playerX - 160, playerY - 120, 0.05f);
        
        // Demonstrate layer control
        static uint32_t layerToggleTime = 0;
        if (currentTime - layerToggleTime > 3000) { // Every 3 seconds
            layerToggleTime = currentTime;
            
            // Toggle layer visibility for demo
            static bool effectsVisible = true;
            effectsVisible = !effectsVisible;
            layerSystem->setLayerEnabled(LAYER_6_EFFECTS, effectsVisible);
            
            Serial.print("Effects layer: ");
            Serial.println(effectsVisible ? "VISIBLE" : "HIDDEN");
        }
        
        // Pulse layer alpha for demo
        uint8_t alpha = (uint8_t)(128 + 127 * sin(time * 2.0f));
        layerSystem->setLayerAlpha(LAYER_6_EFFECTS, alpha);
    }
    
    void render() {
        // Clear graphics buffers
        graphics->clearBuffers(0x0000);
        
        // Render all layers in order
        layerSystem->renderAllLayers();
        
        // Present to screen
        graphics->present();
    }
    
    void handleInput(bool left, bool right, bool up, bool down, bool buttonA, bool buttonB) {
        // Manual player control (overrides circular movement temporarily)
        static uint32_t lastInputTime = 0;
        uint32_t currentTime = millis();
        
        if (left || right || up || down) {
            lastInputTime = currentTime;
            
            if (left) playerX -= 2.0f;
            if (right) playerX += 2.0f;
            if (up) playerY -= 2.0f;
            if (down) playerY += 2.0f;
            
            // Constrain to screen
            playerX = constrain(playerX, 0, 320 - 32);
            playerY = constrain(playerY, 0, 240 - 32);
            
            player->x = playerX;
            player->y = playerY;
        }
        
        // Button A: Create explosion at player position
        if (buttonA) {
            explosion->x = playerX;
            explosion->y = playerY;
            explosion->animation.currentFrame = 0;
            explosion->animation.paused = false;
            layerSystem->playAnimation(explosion, false);
            Serial.println("Explosion triggered!");
        }
        
        // Button B: Toggle UI layer
        if (buttonB) {
            static bool uiVisible = true;
            uiVisible = !uiVisible;
            layerSystem->setLayerEnabled(LAYER_7_UI, uiVisible);
            layerSystem->setLayerEnabled(LAYER_8_TEXT, uiVisible);
            Serial.print("UI layers: ");
            Serial.println(uiVisible ? "VISIBLE" : "HIDDEN");
        }
    }
    
    void printLayerStats() {
        Serial.println("\n=== SPRITE LAYER STATISTICS ===");
        Serial.print("Total sprites rendered: ");
        Serial.println(layerSystem->getSpritesRendered());
        Serial.print("Active layers rendered: ");
        Serial.println(layerSystem->getLayersRendered());
        
        WispVec2 camera = layerSystem->getCamera();
        Serial.print("Camera position: (");
        Serial.print(camera.x);
        Serial.print(", ");
        Serial.print(camera.y);
        Serial.println(")");
        
        Serial.print("Player position: (");
        Serial.print(playerX);
        Serial.print(", ");
        Serial.print(playerY);
        Serial.println(")");
        
        layerSystem->printLayerStats();
        Serial.println("================================\n");
    }
    
    // Demonstrate layer masking for specific effects
    void demonstrateLayerMasking() {
        Serial.println("Demonstrating layer masking effects...");
        
        // Create a fog effect that appears on multiple layers
        WispLayeredSprite* fog = layerSystem->createGameSprite(9, LAYER_4_GAME_FRONT);
        fog->x = 0;
        fog->y = 0;
        fog->alpha = 100; // Semi-transparent
        
        // Apply to background and mid layers with different intensities
        layerSystem->setMultiLayer(fog, {LAYER_1_BACKGROUNDS, LAYER_2_GAME_BACK, LAYER_3_GAME_MID});
        layerSystem->setLayerDepth(fog, LAYER_1_BACKGROUNDS, 3);  // Very faded
        layerSystem->setLayerDepth(fog, LAYER_2_GAME_BACK, 5);    // Medium fade
        layerSystem->setLayerDepth(fog, LAYER_3_GAME_MID, 7);     // Less fade
        
        Serial.println("Fog effect created with depth masking");
    }
    
    // Show tiling modes
    void demonstrateTiling() {
        Serial.println("Demonstrating different tiling modes...");
        
        // Switch tiling modes every few seconds
        static WispTilingMode modes[] = {TILE_REPEAT, TILE_REPEAT_X, TILE_REPEAT_Y, TILE_MIRROR, TILE_NONE};
        static int currentMode = 0;
        static uint32_t lastModeChange = 0;
        
        if (millis() - lastModeChange > 4000) { // Change every 4 seconds
            lastModeChange = millis();
            currentMode = (currentMode + 1) % 5;
            
            tiledBg->tilingMode = modes[currentMode];
            
            Serial.print("Tiling mode changed to: ");
            Serial.println(getTilingModeName(modes[currentMode]));
        }
    }
};

// Example usage in main application
void setupSpriteLayerDemo() {
    // This would be called from your main application setup
    Serial.println("Setting up Sprite Layer Demo");
    
    // Assumes graphics engine and layer system are already initialized
    // SpriteLayerDemo* demo = new SpriteLayerDemo(g_LayerSystem, g_GraphicsEngine);
    
    // In your main loop:
    // demo->update();
    // demo->render();
    
    // For debugging:
    // demo->printLayerStats();
}
