// frame_rate_manager.h
// frame_rate_manager.h - ESP32-C6/S3 Frame Rate Manager using ESP-IDF
// Deterministic frame rate control optimized for ESP32 real-time constraints
#pragma once
#include "../system/esp32_common.h"  // Pure ESP-IDF native headers
#include "app_header.h"

// Frame rate manager - handles dynamic frame rate adjustment
class FrameRateManager {
private:
    AppFrameRate currentTargetFPS;
    AppFrameRate minimumAllowedFPS;
    uint32_t frameTimeUs;          // Target frame time in microseconds
    uint32_t lastFrameTime;        // When last frame started
    uint32_t frameStartTime;       // Current frame start time
    
    // Performance monitoring
    uint32_t actualFrameTimes[16]; // Rolling average of actual frame times
    uint8_t frameTimeIndex;
    uint32_t averageFrameTime;
    
    // Adaptive scaling
    bool enableAdaptiveScaling;
    uint8_t missedFrameCount;
    uint8_t scalingCooldown;
    
    // Statistics
    uint32_t totalFrames;
    uint32_t droppedFrames;
    uint32_t averageFPS;
    
public:
    FrameRateManager() : 
        currentTargetFPS(FRAMERATE_24FPS),
        minimumAllowedFPS(FRAMERATE_8FPS),
        frameTimeUs(41666), // 24 FPS default
        lastFrameTime(0),
        frameStartTime(0),
        frameTimeIndex(0),
        averageFrameTime(0),
        enableAdaptiveScaling(true),
        missedFrameCount(0),
        scalingCooldown(0),
        totalFrames(0),
        droppedFrames(0),
        averageFPS(0) {
        
        // Initialize frame time array
        for (int i = 0; i < 16; i++) {
            actualFrameTimes[i] = frameTimeUs;
        }
    }
    
    // Initialize with app requirements
    void init(const AppHeader& appHeader) {
        currentTargetFPS = appHeader.targetFrameRate;
        minimumAllowedFPS = appHeader.minimumFrameRate;
        enableAdaptiveScaling = appHeader.allowFrameRateScaling;
        
        setTargetFrameRate(currentTargetFPS);
        
        Serial.print("Frame Rate Manager: Target ");
        Serial.print(static_cast<uint8_t>(currentTargetFPS));
        Serial.print(" FPS, Min ");
        Serial.print(static_cast<uint8_t>(minimumAllowedFPS));
        Serial.print(" FPS, Adaptive: ");
        Serial.println(enableAdaptiveScaling ? "On" : "Off");
    }
    
    // Check if it's time for the next frame
    bool isFrameReady() {
        uint32_t currentTime = micros();
        
        if (lastFrameTime == 0 || (currentTime - lastFrameTime) >= frameTimeUs) {
            frameStartTime = currentTime;
            return true;
        }
        
        return false;
    }
    
    // Call at the start of each frame
    void frameStart() {
        frameStartTime = micros();
        totalFrames++;
    }
    
    // Call at the end of each frame
    void frameEnd() {
        uint32_t currentTime = micros();
        uint32_t actualFrameTime = currentTime - frameStartTime;
        
        // Update rolling average
        actualFrameTimes[frameTimeIndex] = actualFrameTime;
        frameTimeIndex = (frameTimeIndex + 1) % 16;
        
        // Calculate average frame time
        uint32_t sum = 0;
        for (int i = 0; i < 16; i++) {
            sum += actualFrameTimes[i];
        }
        averageFrameTime = sum / 16;
        
        // Check if we're missing our target
        if (actualFrameTime > frameTimeUs * 1.2f) { // 20% tolerance
            missedFrameCount++;
            droppedFrames++;
        } else {
            missedFrameCount = 0;
        }
        
        // Adaptive scaling logic
        if (enableAdaptiveScaling && scalingCooldown == 0) {
            if (missedFrameCount >= 5) {
                // Scale down frame rate
                scaleDownFrameRate();
                missedFrameCount = 0;
                scalingCooldown = 60; // Wait 60 frames before next adjustment
            } else if (missedFrameCount == 0 && totalFrames % 120 == 0) {
                // Every 120 frames, try to scale up if performance is good
                tryScaleUpFrameRate();
            }
        }
        
        if (scalingCooldown > 0) {
            scalingCooldown--;
        }
        
        // Update FPS calculation
        if (totalFrames % 60 == 0) {
            averageFPS = 1000000 / averageFrameTime;
        }
        
        lastFrameTime = frameStartTime;
    }
    
    // Manually set target frame rate
    void setTargetFrameRate(AppFrameRate frameRate) {
        currentTargetFPS = frameRate;
        frameTimeUs = AppHeaderUtils::getFrameTimeUs(frameRate);
        
        Serial.print("Frame rate set to ");
        Serial.print(static_cast<uint8_t>(frameRate));
        Serial.print(" FPS (");
        Serial.print(frameTimeUs);
        Serial.println(" μs per frame)");
    }
    
    // Get current performance metrics
    uint32_t getCurrentFPS() const {
        return averageFPS;
    }
    
    uint32_t getTargetFPS() const {
        return static_cast<uint32_t>(currentTargetFPS);
    }
    
    uint32_t getAverageFrameTime() const {
        return averageFrameTime;
    }
    
    uint32_t getTargetFrameTime() const {
        return frameTimeUs;
    }
    
    float getFrameTimeVariance() const {
        // Calculate variance in frame times
        uint32_t sum = 0;
        for (int i = 0; i < 16; i++) {
            int32_t diff = actualFrameTimes[i] - averageFrameTime;
            sum += diff * diff;
        }
        return sqrt(sum / 16.0f);
    }
    
    uint32_t getDroppedFrameCount() const {
        return droppedFrames;
    }
    
    float getFrameDropPercentage() const {
        if (totalFrames == 0) return 0.0f;
        return (droppedFrames * 100.0f) / totalFrames;
    }
    
    // Performance analysis
    bool isPerformanceGood() const {
        return (missedFrameCount < 3) && (averageFrameTime < frameTimeUs * 1.1f);
    }
    
    bool isPerformancePoor() const {
        return (missedFrameCount >= 5) || (averageFrameTime > frameTimeUs * 1.5f);
    }
    
    // Debug output
    void printPerformanceReport() {
        Serial.println("=== Frame Rate Performance ===");
        Serial.print("Target FPS: "); Serial.println(static_cast<uint8_t>(currentTargetFPS));
        Serial.print("Current FPS: "); Serial.println(averageFPS);
        Serial.print("Target Frame Time: "); Serial.print(frameTimeUs); Serial.println(" μs");
        Serial.print("Average Frame Time: "); Serial.print(averageFrameTime); Serial.println(" μs");
        Serial.print("Frame Time Variance: "); Serial.println(getFrameTimeVariance());
        Serial.print("Total Frames: "); Serial.println(totalFrames);
        Serial.print("Dropped Frames: "); Serial.print(droppedFrames);
        Serial.print(" ("); Serial.print(getFrameDropPercentage(), 1); Serial.println("%)");
        Serial.print("Performance: ");
        if (isPerformanceGood()) {
            Serial.println("Good");
        } else if (isPerformancePoor()) {
            Serial.println("Poor");
        } else {
            Serial.println("Moderate");
        }
    }
    
    // Enable/disable adaptive scaling
    void setAdaptiveScaling(bool enabled) {
        enableAdaptiveScaling = enabled;
        Serial.print("Adaptive frame rate scaling: ");
        Serial.println(enabled ? "Enabled" : "Disabled");
    }
    
    // Reset statistics
    void resetStats() {
        totalFrames = 0;
        droppedFrames = 0;
        missedFrameCount = 0;
        frameTimeIndex = 0;
        
        for (int i = 0; i < 16; i++) {
            actualFrameTimes[i] = frameTimeUs;
        }
        
        Serial.println("Frame rate statistics reset");
    }
    
private:
    void scaleDownFrameRate() {
        AppFrameRate newRate = currentTargetFPS;
        
        switch (currentTargetFPS) {
            case FRAMERATE_60FPS: newRate = FRAMERATE_30FPS; break;
            case FRAMERATE_30FPS: newRate = FRAMERATE_24FPS; break;
            case FRAMERATE_24FPS: newRate = FRAMERATE_20FPS; break;
            case FRAMERATE_20FPS: newRate = FRAMERATE_15FPS; break;
            case FRAMERATE_15FPS: newRate = FRAMERATE_12FPS; break;
            case FRAMERATE_12FPS: newRate = FRAMERATE_10FPS; break;
            case FRAMERATE_10FPS: newRate = FRAMERATE_8FPS; break;
            default: return; // Can't scale down further
        }
        
        // Don't go below minimum allowed FPS
        if (static_cast<uint8_t>(newRate) < static_cast<uint8_t>(minimumAllowedFPS)) {
            return;
        }
        
        setTargetFrameRate(newRate);
        Serial.print("Performance: Scaled down to ");
        Serial.print(static_cast<uint8_t>(newRate));
        Serial.println(" FPS");
    }
    
    void tryScaleUpFrameRate() {
        AppFrameRate newRate = currentTargetFPS;
        
        switch (currentTargetFPS) {
            case FRAMERATE_8FPS: newRate = FRAMERATE_10FPS; break;
            case FRAMERATE_10FPS: newRate = FRAMERATE_12FPS; break;
            case FRAMERATE_12FPS: newRate = FRAMERATE_15FPS; break;
            case FRAMERATE_15FPS: newRate = FRAMERATE_20FPS; break;
            case FRAMERATE_20FPS: newRate = FRAMERATE_24FPS; break;
            case FRAMERATE_24FPS: newRate = FRAMERATE_30FPS; break;
            case FRAMERATE_30FPS: newRate = FRAMERATE_60FPS; break;
            default: return; // Already at maximum
        }
        
        setTargetFrameRate(newRate);
        Serial.print("Performance: Scaled up to ");
        Serial.print(static_cast<uint8_t>(newRate));
        Serial.println(" FPS");
    }
};
