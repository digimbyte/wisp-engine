// src/apps/test_cartridge_app.h
#pragma once

#include "../system/wisp_app_interface.h"
#include "../engine/wisp_cartridge_system.h"
#include "../engine/core/debug.h"  // Use WISP debug system instead

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
        WISP_DEBUG_INFO("CARTRIDGE", "Initializing...");
        
        // Get cartridge system
        if (!g_CartridgeSystem) {
            WISP_DEBUG_ERROR("CARTRIDGE", "No cartridge system available");
            return false;
        }
        
        // Load required assets
        if (!g_CartridgeSystem->loadAsset("palette.wlut")) {
            WISP_DEBUG_WARNING("CARTRIDGE", "Could not load palette asset");
        } else {
            paletteData = g_CartridgeSystem->getAssetData("palette.wlut");
        }
        
        if (!g_CartridgeSystem->loadAsset("sprite.art")) {
            WISP_DEBUG_WARNING("CARTRIDGE", "Could not load sprite asset");
        } else {
            spriteData = g_CartridgeSystem->getAssetData("sprite.art");
        }
        
        WISP_DEBUG_INFO("CARTRIDGE", "Initialized successfully");
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
            
            WISP_DEBUG_INFO("CARTRIDGE", "FPS updated");
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
            WISP_DEBUG_INFO("CARTRIDGE", "Rendering sprite position");
            
            // Print asset status
            WISP_DEBUG_INFO("CARTRIDGE", "Assets status updated");
            
            lastDebugTime = currentTime;
        }
    }
    
    void internalCleanup() override {
        WISP_DEBUG_INFO("CARTRIDGE", "Cleaning up...");
        
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
        WISP_DEBUG_INFO("CARTRIDGE", "=== TestCartridgeApp Stats ===");
        WISP_DEBUG_INFO("CARTRIDGE", "Current FPS and Performance");
        WISP_DEBUG_INFO("CARTRIDGE", "Sprite Position and Velocity");
        WISP_DEBUG_INFO("CARTRIDGE", "Assets Loaded Status");
        WISP_DEBUG_INFO("CARTRIDGE", "==============================");
    }
};
