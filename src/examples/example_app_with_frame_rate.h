// example_app_with_frame_rate.h
#pragma once
#include "../system/app_header.h"
#include "../core/game_loop_manager.h"

// Example app demonstrating configurable frame rates
class ExampleFrameRateApp {
private:
    GameLoopManager* gameLoopManager;
    uint16_t playerEntity;
    uint32_t lastPerformanceReport;
    
public:
    // App Header - Configure frame rate and performance requirements
    static const AppHeader getAppHeader() {
        AppHeader header;
        
        // Basic app info
        header.magic = APP_HEADER_MAGIC;
        header.version = 1;
        strncpy(header.name, "FrameRateDemo", sizeof(header.name) - 1);
        strncpy(header.author, "WispEngine", sizeof(header.author) - 1);
        
        // Frame rate configuration
        header.targetFrameRate = FRAMERATE_30FPS;      // Prefer 30 FPS
        header.minimumFrameRate = FRAMERATE_15FPS;     // Can drop to 15 FPS
        header.allowFrameRateScaling = true;           // Enable adaptive scaling
        
        // Performance profile
        header.performanceProfile = PERFORMANCE_BALANCED;
        
        // Resource requirements
        header.resourceRequirements.minRamKB = 8;      // Need at least 8KB RAM
        header.resourceRequirements.minStorageKB = 2;  // Need 2KB storage
        header.resourceRequirements.requiresWiFi = false;
        header.resourceRequirements.requiresBluetooth = false;
        header.resourceRequirements.requiresAccelerometer = false;
        
        // Calculate CRC
        header.crc = AppHeaderUtils::calculateCRC(&header);
        
        return header;
    }
    
    void init(GameLoopManager* manager) {
        gameLoopManager = manager;
        lastPerformanceReport = 0;
        
        // Create a simple player entity
        playerEntity = gameLoopManager->createEntity(50, 50, 10, 10);
        
        Serial.println("=== Frame Rate Demo App Started ===");
        Serial.println("This app demonstrates:");
        Serial.println("- Target 30 FPS with minimum 15 FPS");
        Serial.println("- Adaptive frame rate scaling enabled");
        Serial.println("- Performance monitoring every 5 seconds");
        Serial.println("- Simple moving entity for load testing");
    }
    
    void update() {
        if (!gameLoopManager) return;
        
        // Simple entity movement to create some load
        GameEntity* player = gameLoopManager->getEntity(playerEntity);
        if (player) {
            // Move in a circle
            static float angle = 0.0f;
            angle += 0.1f;
            if (angle > 6.28f) angle = 0.0f;
            
            player->x = 100 + (int16_t)(50 * cos(angle));
            player->y = 100 + (int16_t)(50 * sin(angle));
        }
        
        // Print performance report every 5 seconds
        uint32_t now = millis();
        if (now - lastPerformanceReport > 5000) {
            printAppPerformanceReport();
            lastPerformanceReport = now;
        }
    }
    
    void printAppPerformanceReport() {
        Serial.println("\n=== App Performance Report ===");
        
        // Frame rate information
        Serial.print("Current FPS: ");
        Serial.println(gameLoopManager->getCurrentFPS());
        
        Serial.print("Target FPS: ");
        Serial.println(gameLoopManager->getTargetFPS());
        
        Serial.print("Frame Drop %: ");
        Serial.println(gameLoopManager->getFrameDropPercentage(), 2);
        
        // Memory usage (if available)
        Serial.print("Free Heap: ");
        Serial.print(ESP.getFreeHeap());
        Serial.println(" bytes");
        
        // Entity status
        GameEntity* player = gameLoopManager->getEntity(playerEntity);
        if (player) {
            Serial.print("Player Position: (");
            Serial.print(player->x);
            Serial.print(", ");
            Serial.print(player->y);
            Serial.println(")");
        }
        
        Serial.println("============================\n");
    }
    
    // Test different frame rate configurations
    void testFrameRateScaling() {
        Serial.println("Testing frame rate scaling...");
        
        // Test high performance mode
        Serial.println("Setting 60 FPS...");
        gameLoopManager->setTargetFrameRate(FRAMERATE_60FPS);
        delay(3000);
        
        // Test balanced mode
        Serial.println("Setting 30 FPS...");
        gameLoopManager->setTargetFrameRate(FRAMERATE_30FPS);
        delay(3000);
        
        // Test power saving mode
        Serial.println("Setting 15 FPS...");
        gameLoopManager->setTargetFrameRate(FRAMERATE_15FPS);
        delay(3000);
        
        // Test ultra power saving
        Serial.println("Setting 8 FPS...");
        gameLoopManager->setTargetFrameRate(FRAMERATE_8FPS);
        delay(3000);
        
        // Back to default
        Serial.println("Back to 30 FPS...");
        gameLoopManager->setTargetFrameRate(FRAMERATE_30FPS);
    }
    
    // Enable/disable adaptive scaling
    void toggleAdaptiveScaling() {
        static bool adaptiveEnabled = true;
        adaptiveEnabled = !adaptiveEnabled;
        
        gameLoopManager->setAdaptiveFrameRateScaling(adaptiveEnabled);
        
        Serial.print("Adaptive frame rate scaling: ");
        Serial.println(adaptiveEnabled ? "ENABLED" : "DISABLED");
    }
};

// Global instance for easy access
ExampleFrameRateApp frameRateApp;
