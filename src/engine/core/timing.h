// engine/timing.h - ESP32-C6/S3 Timing System using ESP-IDF native timers
// High-precision timing for deterministic frame rates using esp_timer
#pragma once
#include "engine_common.h"  // Pure ESP-IDF native headers

// Clean timing system - no more global state mess
class TimingSystem {
private:
    uint32_t lastTick;
    uint32_t currentTime;
    float deltaTime;
    uint32_t frameTime;
    uint32_t targetFrameTime;
    
    // Frame rate control
    bool frameRateControlEnabled;
    uint32_t nextFrameDeadline;
    
    // Statistics
    uint32_t totalFrames;
    uint32_t averageFrameTime;
    uint32_t frameTimeHistory[16];
    uint8_t historyIndex;
    
public:
    TimingSystem() :
        lastTick(0),
        currentTime(0),
        deltaTime(0.0f),
        frameTime(0),
        targetFrameTime(16666), // 60 FPS
        frameRateControlEnabled(true),
        nextFrameDeadline(0),
        totalFrames(0),
        averageFrameTime(0),
        historyIndex(0) {
        
        // Initialize history
        for (int i = 0; i < 16; i++) {
            frameTimeHistory[i] = targetFrameTime;
        }
    }
    
    // Core timing functions
    void initialize();
    void update();
    
    // Frame control
    bool isFrameReady();
    void beginFrame();
    void endFrame();
    
    // Configuration
    void setTargetFrameRate(uint8_t fps);
    void setFrameRateControl(bool enabled);
    
    // Accessors
    float getDeltaTime() const { return deltaTime; }
    uint32_t getFrameTime() const { return frameTime; }
    uint32_t getCurrentTime() const { return currentTime; }
    uint32_t getTargetFrameTime() const { return targetFrameTime; }
    uint32_t getTotalFrames() const { return totalFrames; }
    uint32_t getAverageFrameTime() const { return averageFrameTime; }
    
    // Performance analysis
    float getCurrentFPS() const;
    float getAverageFPS() const;
    float getFrameTimeVariance() const;
    bool isPerformanceStable() const;
    
    // Debug
    void printTimingStats() const;
    void resetStats();
    
private:
    void updateFrameTimeHistory();
    void calculateAverageFrameTime();
};
