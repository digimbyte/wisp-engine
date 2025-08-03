// src/apps/test_cartridge_app.h
#pragma once

#include "../system/wisp_app_interface.h"
#include "../engine/wisp_cartridge_system.h"
#include "esp_log.h"

static const char* TAG = "TEST_CARTRIDGE_APP";

/**
 * Test Application for Cartridge System
 * Demonstrates loading assets, rendering sprites, and system integration
 */
class TestCartridgeApp : public WispAppBase {
private:
    uint32_t frameCount;
    uint32_t lastFPSTime;
    uint16_t currentFPS;
    
    // Test sprites
    const uint8_t* spriteData;
    const uint8_t* paletteData;
    
    // Animation state
    float spriteX, spriteY;
    float velocityX, velocityY;
    
public:
    TestCartridgeApp() : 
        frameCount(0), 
        lastFPSTime(0), 
        currentFPS(0),
        spriteData(nullptr),
        paletteData(nullptr),
        spriteX(32.0f), 
        spriteY(32.0f),
        velocityX(1.0f), 
        velocityY(0.5f) {
    }
    
    // WispAppBase interface implementation
    bool internalInit() override {
        ESP_LOGI(TAG, "Initializing...");
        
        // Get cartridge system
        if (!g_CartridgeSystem) {
            ESP_LOGE(TAG, "No cartridge system available");
            return false;
        }
        
        // Load required assets
        if (!g_CartridgeSystem->loadAsset("palette.wlut")) {
            ESP_LOGW(TAG, "Could not load palette asset");
        } else {
            paletteData = g_CartridgeSystem->getAssetData("palette.wlut");
        }
        
        if (!g_CartridgeSystem->loadAsset("sprite.art")) {
            ESP_LOGW(TAG, "Could not load sprite asset");
        } else {
            spriteData = g_CartridgeSystem->getAssetData("sprite.art");
        }
        
        ESP_LOGI(TAG, "Initialized successfully");
        return true;
    }
    
    void internalUpdate(uint32_t deltaTime) override {
        frameCount++;
        
        // Update FPS counter
        uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (currentTime - lastFPSTime >= 1000) {
            currentFPS = frameCount;
            frameCount = 0;
            lastFPSTime = currentTime;
            
            ESP_LOGI(TAG, "FPS: %d", currentFPS);
        }
        
        // Update sprite position (bouncing animation)
        spriteX += velocityX;
        spriteY += velocityY;
        
        // Bounce off screen edges (assuming 128x128 screen)
        if (spriteX <= 0 || spriteX >= 128-16) {
            velocityX = -velocityX;
        }
        if (spriteY <= 0 || spriteY >= 128-16) {
            velocityY = -velocityY;
        }
        
        // Clamp to screen bounds
        spriteX = constrain(spriteX, 0, 128-16);
        spriteY = constrain(spriteY, 0, 128-16);
    }
    
    void internalRender() override {
        // Clear screen (fill with color 0)
        // TODO: Implement actual rendering when graphics system is available
        
        // For now, just output debug info occasionally
        static uint32_t lastDebugTime = 0;
        uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        if (currentTime - lastDebugTime >= 5000) {  // Every 5 seconds
            ESP_LOGI(TAG, "Rendering sprite at (%d, %d)", (int)spriteX, (int)spriteY);
            
            // Print asset status
            String assets = "Assets loaded: ";
            if (spriteData) assets += "sprite.art ";
            if (paletteData) assets += "palette.wlut ";
            ESP_LOGI(TAG, "%s", assets.c_str());
            
            lastDebugTime = currentTime;
        }
    }
    
    void internalCleanup() override {
        ESP_LOGI(TAG, "Cleaning up...");
        
        // Unload assets
        if (g_CartridgeSystem) {
            g_CartridgeSystem->unloadAsset("sprite.art");
            g_CartridgeSystem->unloadAsset("palette.wlut");
        }
        
        spriteData = nullptr;
        paletteData = nullptr;
    }
    
    // Input handling
    void handleInput(uint8_t inputMask) override {
        // Example: Change velocity on button press
        if (inputMask & 0x01) {  // Button A
            velocityX *= 1.1f;
            velocityY *= 1.1f;
        }
        if (inputMask & 0x02) {  // Button B
            velocityX *= 0.9f;
            velocityY *= 0.9f;
        }
    }
    
    // App metadata
    const char* getAppName() const override {
        return "Test Cartridge App";
    }
    
    const char* getAppVersion() const override {
        return "1.0.0";
    }
    
    uint32_t getRequiredMemory() const override {
        return 32768;  // 32KB
    }
    
    uint16_t getTargetFPS() const override {
        return 16;
    }
    
    // Test-specific methods
    void printStats() const {
        ESP_LOGI(TAG, "=== TestCartridgeApp Stats ===");
        ESP_LOGI(TAG, "Current FPS: %d", currentFPS);
        ESP_LOGI(TAG, "Sprite Position: (%d, %d)", (int)spriteX, (int)spriteY);
        ESP_LOGI(TAG, "Velocity: (%.2f, %.2f)", velocityX, velocityY);
        
        String assets = "Assets Loaded: ";
        assets += spriteData ? "sprite " : "";
        assets += paletteData ? "palette " : "";
        ESP_LOGI(TAG, "%s", assets.c_str());
        ESP_LOGI(TAG, "==============================");
    }
};
