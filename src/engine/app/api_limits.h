// engine/wisp_api_limits.h
// api_limits.h - ESP32-C6/S3 API Limits using ESP-IDF
// Resource limits and quotas for safe ESP32 application execution
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers

// Wisp Engine API Limits - Enforced restrictions to prevent system crashes
// These limits ensure apps can't overwhelm the ESP32's resources

// Core System Limits
#define WISP_MAX_ENTITIES           64      // Maximum entities per app
#define WISP_MAX_SPRITES            32      // Maximum sprites loaded simultaneously  
#define WISP_MAX_AUDIO_CHANNELS     4       // Maximum concurrent audio streams
#define WISP_MAX_PARTICLES          128     // Maximum particles in system
#define WISP_MAX_LEVEL_CHUNKS       8       // Maximum chunks loaded at once
#define WISP_MAX_INPUT_EVENTS       16      // Maximum input events per frame
#define WISP_MAX_TIMERS             16      // Maximum active timers
#define WISP_MAX_ANIMATIONS         32      // Maximum concurrent animations
#define WISP_MAX_APPS               32      // Maximum apps in system

// Memory Limits (in bytes)
#define WISP_MAX_APP_MEMORY         (64 * 1024)    // 64KB max for app data
#define WISP_MAX_SPRITE_SIZE        (8 * 1024)     // 8KB max per sprite
#define WISP_MAX_AUDIO_SIZE         (16 * 1024)    // 16KB max per audio file
#define WISP_MAX_LEVEL_DATA_SIZE    (32 * 1024)    // 32KB max level data
#define WISP_MAX_STRING_LENGTH      256             // Maximum string length

// Performance Limits
#define WISP_MAX_FRAME_TIME_US      16667   // Must complete within 16.67ms (60fps)
#define WISP_MAX_UPDATE_TIME_US     8000    // Max 8ms for app update()
#define WISP_MAX_RENDER_TIME_US     8000    // Max 8ms for app render()
#define WISP_MAX_DRAW_CALLS         256     // Maximum draw calls per frame
#define WISP_MAX_COLLISION_CHECKS   512     // Maximum collision checks per frame

// Resource Access Limits
#define WISP_MAX_FILE_OPERATIONS    4       // Max file ops per frame
#define WISP_MAX_NETWORK_REQUESTS   2       // Max network requests per frame
#define WISP_MAX_MALLOC_CALLS       8       // Max memory allocations per frame
#define WISP_MAX_RECURSION_DEPTH    16      // Max function call depth

// Safety Timeouts (in milliseconds)
#define WISP_WATCHDOG_TIMEOUT       5000    // 5 seconds before force reset
#define WISP_INIT_TIMEOUT           10000   // 10 seconds max for app init
#define WISP_LOAD_TIMEOUT           3000    // 3 seconds max for resource loading

// Error Recovery
#define WISP_MAX_ERRORS_PER_SECOND  10      // Max errors before emergency mode
#define WISP_MAX_CONSECUTIVE_FRAME_DROPS 30 // Max dropped frames before restart

// Feature Restrictions
#define WISP_ALLOW_DYNAMIC_ALLOCATION   true    // Allow malloc/free
#define WISP_ALLOW_FILE_WRITE          false   // Prevent file system corruption
#define WISP_ALLOW_NETWORK_ACCESS      true    // Allow WiFi/Bluetooth
#define WISP_ALLOW_HARDWARE_ACCESS     false   // Prevent direct GPIO access
#define WISP_ALLOW_SYSTEM_CALLS        false   // Prevent system() calls

// Debug and Development Modes
#define WISP_DEBUG_MODE_ENABLED         false   // Enable debug logging and error tracking
#define WISP_SAFETY_DISABLED           false   // DANGER: Disable all safety limits for stress testing
#define WISP_DEBUG_LOG_TO_SD           true    // Log errors to SD card when debug enabled
#define WISP_DEBUG_OUTPUT_PINS         true    // Output debug signals to pins when debug enabled

// Debug Configuration
#define WISP_ERROR_LOG_MAX_SIZE        (1024 * 1024)  // 1MB max error log size
#define WISP_ERROR_LOG_ROTATION_COUNT  5              // Keep 5 rotated log files
#define WISP_DEBUG_SIGNAL_DURATION_MS  100            // Debug pin signal duration

// Resource Quotas
struct WispResourceQuota {
    uint16_t maxEntities;
    uint16_t maxSprites;
    uint16_t maxAudioChannels;
    uint16_t maxParticles;
    uint32_t maxMemoryUsage;
    uint16_t maxDrawCalls;
    uint16_t maxCollisionChecks;
    
    // Current usage tracking
    uint16_t currentEntities;
    uint16_t currentSprites;
    uint16_t currentAudioChannels;
    uint16_t currentParticles;
    uint32_t currentMemoryUsage;
    uint16_t currentDrawCalls;
    uint16_t currentCollisionChecks;
    
    WispResourceQuota() {
        // Set defaults from limits
        maxEntities = WISP_MAX_ENTITIES;
        maxSprites = WISP_MAX_SPRITES;
        maxAudioChannels = WISP_MAX_AUDIO_CHANNELS;
        maxParticles = WISP_MAX_PARTICLES;
        maxMemoryUsage = WISP_MAX_APP_MEMORY;
        maxDrawCalls = WISP_MAX_DRAW_CALLS;
        maxCollisionChecks = WISP_MAX_COLLISION_CHECKS;
        
        // Initialize current usage
        currentEntities = 0;
        currentSprites = 0;
        currentAudioChannels = 0;
        currentParticles = 0;
        currentMemoryUsage = 0;
        currentDrawCalls = 0;
        currentCollisionChecks = 0;
    }
    
    // Check if operation would exceed quota
    bool canAllocateEntity() const { return currentEntities < maxEntities; }
    bool canLoadSprite() const { return currentSprites < maxSprites; }
    bool canPlayAudio() const { return currentAudioChannels < maxAudioChannels; }
    bool canCreateParticle() const { return currentParticles < maxParticles; }
    bool canAllocateMemory(uint32_t bytes) const { 
        return (currentMemoryUsage + bytes) <= maxMemoryUsage; 
    }
    bool canDraw() const { return currentDrawCalls < maxDrawCalls; }
    bool canCheckCollision() const { return currentCollisionChecks < maxCollisionChecks; }
    
    // Safe operation methods that check debug/safety settings
    bool safeAllocateEntity() { 
        bool withinLimit = canAllocateEntity();
        if (checkSafetyLimit("Entity allocation", withinLimit)) {
            if (withinLimit) currentEntities++;
            return true;
        }
        return false;
    }
    
    bool safeLoadSprite() { 
        bool withinLimit = canLoadSprite();
        if (checkSafetyLimit("Sprite loading", withinLimit)) {
            if (withinLimit) currentSprites++;
            return true;
        }
        return false;
    }
    
    bool safePlayAudio() { 
        bool withinLimit = canPlayAudio();
        if (checkSafetyLimit("Audio channel allocation", withinLimit)) {
            if (withinLimit) currentAudioChannels++;
            return true;
        }
        return false;
    }
    
    bool safeCreateParticle() { 
        bool withinLimit = canCreateParticle();
        if (checkSafetyLimit("Particle creation", withinLimit)) {
            if (withinLimit) currentParticles++;
            return true;
        }
        return false;
    }
    
    bool safeAllocateMemory(uint32_t bytes) { 
        bool withinLimit = canAllocateMemory(bytes);
        String operation = "Memory allocation (" + String(bytes) + " bytes)";
        if (checkSafetyLimit(operation, withinLimit)) {
            if (withinLimit) currentMemoryUsage += bytes;
            return true;
        }
        return false;
    }
    
    bool safeDraw() { 
        bool withinLimit = canDraw();
        if (checkSafetyLimit("Draw call", withinLimit)) {
            if (withinLimit) currentDrawCalls++;
            return true;
        }
        return false;
    }
    
    bool safeCheckCollision() { 
        bool withinLimit = canCheckCollision();
        if (checkSafetyLimit("Collision check", withinLimit)) {
            if (withinLimit) currentCollisionChecks++;
            return true;
        }
        return false;
    }
    
    // Update usage counters
    void allocateEntity() { if (canAllocateEntity()) currentEntities++; }
    void freeEntity() { if (currentEntities > 0) currentEntities--; }
    void loadSprite() { if (canLoadSprite()) currentSprites++; }
    void unloadSprite() { if (currentSprites > 0) currentSprites--; }
    void startAudio() { if (canPlayAudio()) currentAudioChannels++; }
    void stopAudio() { if (currentAudioChannels > 0) currentAudioChannels--; }
    void createParticle() { if (canCreateParticle()) currentParticles++; }
    void destroyParticle() { if (currentParticles > 0) currentParticles--; }
    void allocateMemory(uint32_t bytes) { 
        if (canAllocateMemory(bytes)) currentMemoryUsage += bytes; 
    }
    void freeMemory(uint32_t bytes) { 
        if (currentMemoryUsage >= bytes) currentMemoryUsage -= bytes; 
        else currentMemoryUsage = 0;
    }
    void draw() { if (canDraw()) currentDrawCalls++; }
    void checkCollision() { if (canCheckCollision()) currentCollisionChecks++; }
    
    // Reset per-frame counters
    void resetFrameCounters() {
        currentDrawCalls = 0;
        currentCollisionChecks = 0;
    }
    
    // Get usage percentages
    float getEntityUsage() const { return (float)currentEntities / maxEntities; }
    float getSpriteUsage() const { return (float)currentSprites / maxSprites; }
    float getMemoryUsage() const { return (float)currentMemoryUsage / maxMemoryUsage; }
    float getDrawCallUsage() const { return (float)currentDrawCalls / maxDrawCalls; }
    
    // Check if approaching limits (80% threshold)
    bool isEntityUsageHigh() const { return getEntityUsage() > 0.8f; }
    bool isSpriteUsageHigh() const { return getSpriteUsage() > 0.8f; }
    bool isMemoryUsageHigh() const { return getMemoryUsage() > 0.8f; }
    bool isDrawCallUsageHigh() const { return getDrawCallUsage() > 0.8f; }
    
    void printUsageStats() const {
        Serial.println("=== Resource Quota Usage ===");
        Serial.print("Entities: ");
        Serial.print(currentEntities);
        Serial.print("/");
        Serial.print(maxEntities);
        Serial.print(" (");
        Serial.print((int)(getEntityUsage() * 100));
        Serial.println("%)");
        
        Serial.print("Sprites: ");
        Serial.print(currentSprites);
        Serial.print("/");
        Serial.print(maxSprites);
        Serial.print(" (");
        Serial.print((int)(getSpriteUsage() * 100));
        Serial.println("%)");
        
        Serial.print("Memory: ");
        Serial.print(currentMemoryUsage);
        Serial.print("/");
        Serial.print(maxMemoryUsage);
        Serial.print(" bytes (");
        Serial.print((int)(getMemoryUsage() * 100));
        Serial.println("%)");
        
        Serial.print("Draw Calls: ");
        Serial.print(currentDrawCalls);
        Serial.print("/");
        Serial.print(maxDrawCalls);
        Serial.print(" (");
        Serial.print((int)(getDrawCallUsage() * 100));
        Serial.println("%)");
        
        Serial.println("============================");
    }
    
private:
    // Safety check method that integrates with debug system
    bool checkSafetyLimit(const String& operation, bool withinLimit) const {
        // This will be implemented by including the debug system
        // For now, we'll use a simplified version
        #if WISP_SAFETY_DISABLED
            // In unsafe mode, log violation but allow operation
            if (!withinLimit) {
                Serial.print("SAFETY DISABLED - Quota violation: ");
                Serial.println(operation);
            }
            return true;
        #else
            // In safe mode, block operation if over limit
            if (!withinLimit) {
                Serial.print("QUOTA LIMIT EXCEEDED: ");
                Serial.println(operation);
            }
            return withinLimit;
        #endif
    }
};
