// app_loop_manager.h
// app_loop_manager.h - ESP32-C6/S3 App Loop Manager using ESP-IDF
// Application loop orchestration with memory and timing management for ESP32
#pragma once
#include "../system/esp32_common.h"  // Pure ESP-IDF native headers
#include "app_loop.h"
#include "frame_rate_manager.h"
#include "timekeeper.h"

// Forward declarations to avoid circular dependencies
class AppManager;

// App Loop Manager - Handles initialization and lifecycle of the app loop
class AppLoopManager {
private:
    AppLoop* appLoop;
    AppManager* appManager;  // Optional app manager integration
    FrameRateManager* frameRateManager; // Frame rate management
    bool initialized;
    bool running;
    
    // System references (injected, not owned)
    GraphicsEngine* graphics;
    PhysicsEngine* physics;
    AudioEngine* audio;
    InputController* input;
    
public:
    AppLoopManager() : appLoop(nullptr), appManager(nullptr), frameRateManager(nullptr), 
                       initialized(false), running(false), graphics(nullptr), physics(nullptr), 
                       audio(nullptr), input(nullptr) {}
    
    // Initialize the app loop with system references
    bool init(GraphicsEngine* gfx, PhysicsEngine* phys, AudioEngine* aud, InputController* inp) {
        if (initialized) {
            Serial.println("App Loop Manager already initialized");
            return false;
        }
        
        if (!gfx || !phys || !aud || !inp) {
            Serial.println("ERROR: Invalid system references for App Loop Manager");
            return false;
        }
        
        graphics = gfx;
        physics = phys;
        audio = aud;
        input = inp;
        
        // Create frame rate manager
        frameRateManager = new FrameRateManager();
        
        // Initialize timekeeper with frame rate manager
        Time::initWithFrameRateManager(frameRateManager);
        
        // Create app loop instance
        appLoop = new AppLoop();
        appLoop->init(graphics, physics, audio, input);
        
        initialized = true;
        Serial.println("App Loop Manager initialized with Frame Rate Manager");
        return true;
    }
    
    // Initialize with app requirements
    bool initWithApp(GraphicsEngine* gfx, PhysicsEngine* phys, AudioEngine* aud, 
                     InputController* inp, const AppHeader& appHeader) {
        if (!init(gfx, phys, aud, inp)) {
            return false;
        }
        
        // Configure frame rate based on app requirements
        frameRateManager->init(appHeader);
        
        Serial.println("App Loop Manager initialized with app-specific frame rate settings");
        return true;
    }
    
    // Start the app loop
    bool start() {
        if (!initialized || !appLoop) {
            Serial.println("ERROR: App Loop Manager not initialized");
            return false;
        }
        
        if (running) {
            Serial.println("App Loop already running");
            return true;
        }
        
        running = true;
        Serial.println("App Loop started");
        return true;
    }
    
    // Stop the app loop
    void stop() {
        if (!running) return;
        
        running = false;
        Serial.println("App Loop stopped");
    }
    
    // Update the app loop (call this every frame)
    void update() {
        if (!initialized || !running || !appLoop) {
            return;
        }
        
        // Check if frame is ready (handles timing and frame rate limiting)
        if (!Time::frameReady()) {
            return;
        }
        
        // Update app manager during logic stage
        if (appManager && appLoop->currentStage == STAGE_LOGIC_UPDATE) {
            // Call app manager update - this will handle C++ application callbacks
            updateAppManager();
        }
        
        appLoop->update();
        
        // End frame processing (performance tracking)
        Time::frameEnd();
    }
    
    // Set app manager for C++ application integration
    void setAppManager(AppManager* manager) {
        appManager = manager;
        if (gameLoop) {
            gameLoop->appManager = manager;
        }
    }
    
    // Check if app loop is running
    bool isRunning() const {
        return initialized && running;
    }
    
    // Get access to the app loop for advanced operations
    AppLoop* getAppLoop() {
        return appLoop;
    }
    
    // Performance and debugging
    void printPerformanceStats() {
        if (!appLoop) return;
        
        Serial.println("=== App Loop Performance ===");
        Serial.print("Frame: ");
        Serial.println(appLoop->frameCount);
    // Performance and frame rate management
    uint32_t getCurrentFPS() const {
        return frameRateManager ? frameRateManager->getCurrentFPS() : 0;
    }
    
    uint32_t getTargetFPS() const {
        return frameRateManager ? frameRateManager->getTargetFPS() : 0;
    }
    
    float getFrameDropPercentage() const {
        return frameRateManager ? frameRateManager->getFrameDropPercentage() : 0.0f;
    }
    
    void setTargetFrameRate(AppFrameRate frameRate) {
        if (frameRateManager) {
            frameRateManager->setTargetFrameRate(frameRate);
        }
    }
    
    void setAdaptiveFrameRateScaling(bool enabled) {
        if (frameRateManager) {
            frameRateManager->setAdaptiveScaling(enabled);
        }
    }
    
    void printPerformanceReport() {
        if (frameRateManager) {
            frameRateManager->printPerformanceReport();
        }
        
        if (!appLoop) return;
        
        Serial.println("=== App Loop Performance ===");
        Serial.print("Current Stage: ");
        Serial.println(appLoop->currentStage);
        Serial.print("Delta Time: ");
        Serial.print(appLoop->deltaTime);
        Serial.println(" μs");
        
        Serial.println("Stage Timings (μs):");
        for (int i = 0; i < STAGE_COUNT; i++) {
            Serial.print("  Stage ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(appLoop->stageTimings[i]);
        }
        
        Serial.print("Entities: ");
        Serial.println(appLoop->entities.size());
        Serial.print("Regions: ");
        Serial.println(appLoop->regions.size());
        Serial.print("Frame Events: ");
        Serial.println(appLoop->frameEvents.size());
    }
    
    // Entity management convenience methods
    uint16_t createEntity(int16_t x, int16_t y, uint16_t w, uint16_t h, 
                         uint8_t collisionMask = MASK_ALL, uint8_t triggerMask = MASK_ALL) {
        if (!appLoop) return 0xFFFF;
        return appLoop->createEntity(x, y, w, h, collisionMask, triggerMask);
    }
    
    uint16_t createRegion(int16_t x, int16_t y, uint16_t w, uint16_t h, 
                         RegionType type, uint8_t mask = MASK_ALL,
                         PhysicsRegion::TriggerLogic logic = PhysicsRegion::TRIGGER_ON_ENTER) {
        if (!appLoop) return 0xFFFF;
        return appLoop->createRegion(x, y, w, h, type, mask, logic);
    }
    
    AppEntity* getEntity(uint16_t entityId) {
        if (!appLoop) return nullptr;
        return appLoop->getEntity(entityId);
    }
    
    PhysicsRegion* getRegion(uint16_t regionId) {
        if (!appLoop) return nullptr;
        return appLoop->getRegion(regionId);
    }
    
    // Cleanup
    ~AppLoopManager() {
        if (appLoop) {
            delete appLoop;
            appLoop = nullptr;
        }
        
        if (frameRateManager) {
            delete frameRateManager;
            frameRateManager = nullptr;
        }
        
        initialized = false;
        running = false;
    }
    
private:
    void updateAppManager() {
        // This will be called during STAGE_LOGIC_UPDATE
        // Forward declaration implementation handled elsewhere to avoid circular deps
    }
};
