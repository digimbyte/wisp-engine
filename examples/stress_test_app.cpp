// example_stress_test_app.cpp - Example app showing debug and safety features
/*
This example demonstrates:
1. How to configure debug/safety modes
2. How to intentionally stress test the quota system
3. How errors are logged and handled
4. How safety limits protect the system

To compile with different modes:
- Production: No special defines (uses defaults)
- Development: #define WISP_DEV_MODE
- Stress Testing: #define WISP_STRESS_TEST_MODE
*/

// =============================================================================
// CONFIGURATION SECTION
// =============================================================================

// Uncomment one of these to test different modes:

// #define WISP_DEV_MODE                        // Safe development with debugging
#define WISP_STRESS_TEST_MODE                   // Unsafe stress testing mode
// #define WISP_PRODUCTION_MODE                 // Production mode (no debug)

// Custom quotas for this stress test app
#define WISP_APP_MAX_ENTITIES 32                // Lower than default for testing
#define WISP_APP_MAX_SPRITES 16                 // Lower than default for testing

// Include configuration and engine
#include "src/engine/wisp_app_config.h"
#include "src/engine/wisp_app_interface.h"
#include "src/engine/wisp_debug_system.h"

// =============================================================================
// STRESS TEST APP IMPLEMENTATION
// =============================================================================

class StressTestApp : public WispAppBase {
private:
    struct TestEntity {
        float x, y;
        float vx, vy;
        uint16_t color;
        bool active;
    };
    
    static const int MAX_TEST_ENTITIES = 128; // Intentionally higher than quota
    TestEntity entities[MAX_TEST_ENTITIES];
    int entityCount = 0;
    
    int stressTestPhase = 0;
    uint32_t phaseStartTime = 0;
    uint32_t frameCount = 0;
    
    // Test counters
    int successfulAllocations = 0;
    int failedAllocations = 0;
    int drawCallCount = 0;
    
public:
    bool init() override {
        WISP_CONFIG_SUMMARY(); // Print configuration summary
        
        WISP_DEBUG_INFO("STRESS_APP", "Stress test app initializing");
        
        // Initialize all entities as inactive
        for (int i = 0; i < MAX_TEST_ENTITIES; i++) {
            entities[i].active = false;
        }
        
        stressTestPhase = 0;
        phaseStartTime = millis();
        frameCount = 0;
        
        WISP_DEBUG_INFO("STRESS_APP", "Initialization complete - starting stress tests");
        return true;
    }
    
    void update() override {
        frameCount++;
        uint32_t currentTime = millis();
        
        // Run different stress test phases
        switch (stressTestPhase) {
            case 0:
                stressTestEntityAllocation();
                if (currentTime - phaseStartTime > 5000) { // 5 seconds
                    nextPhase("Entity Allocation Test");
                }
                break;
                
            case 1:
                stressTestMemoryAllocation();
                if (currentTime - phaseStartTime > 5000) {
                    nextPhase("Memory Allocation Test");
                }
                break;
                
            case 2:
                stressTestDrawCalls();
                if (currentTime - phaseStartTime > 5000) {
                    nextPhase("Draw Call Test");
                }
                break;
                
            case 3:
                stressTestPerformance();
                if (currentTime - phaseStartTime > 10000) { // Longer test
                    nextPhase("Performance Test");
                }
                break;
                
            case 4:
                stressTestErrorGeneration();
                if (currentTime - phaseStartTime > 3000) {
                    nextPhase("Error Generation Test");
                }
                break;
                
            default:
                // Test complete, cycle back
                stressTestPhase = 0;
                phaseStartTime = currentTime;
                WISP_DEBUG_INFO("STRESS_APP", "All stress tests complete - cycling");
                break;
        }
        
        // Update active entities
        updateEntities();
        
        // Log stats periodically
        if (frameCount % 300 == 0) { // Every 5 seconds at 60fps
            logTestStats();
        }
    }
    
    void render() override {
        auto* gfx = api->graphics();
        
        // Clear background
        gfx->fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BLACK);
        
        // Draw title
        gfx->setTextColor(COLOR_WHITE);
        gfx->setTextSize(2);
        gfx->drawText("STRESS TEST", SCREEN_WIDTH / 2, 10, true);
        
        // Draw current phase
        gfx->setTextSize(1);
        String phaseText = "Phase " + String(stressTestPhase + 1) + ": " + getPhaseDescription();
        gfx->drawText(phaseText.c_str(), SCREEN_WIDTH / 2, 35, true);
        
        // Draw test statistics
        renderTestStats();
        
        // Draw entities (with quota-safe draw calls)
        renderEntities();
        
        // Draw configuration info
        renderConfigInfo();
    }
    
    void cleanup() override {
        WISP_DEBUG_INFO("STRESS_APP", "Stress test app cleaning up");
        
        // Log final statistics
        logFinalStats();
        
        // Clean up entities
        entityCount = 0;
        for (int i = 0; i < MAX_TEST_ENTITIES; i++) {
            entities[i].active = false;
        }
    }
    
private:
    void stressTestEntityAllocation() {
        // Try to allocate entities beyond the quota
        if (api->quota()->safeAllocateEntity()) {
            if (entityCount < MAX_TEST_ENTITIES) {
                entities[entityCount].x = random(0, SCREEN_WIDTH);
                entities[entityCount].y = random(50, SCREEN_HEIGHT - 50);
                entities[entityCount].vx = random(-2, 3);
                entities[entityCount].vy = random(-2, 3);
                entities[entityCount].color = random(0x1000, 0xFFFF);
                entities[entityCount].active = true;
                entityCount++;
                successfulAllocations++;
            }
        } else {
            failedAllocations++;
            WISP_DEBUG_WARNING("STRESS_APP", "Entity allocation blocked by quota");
        }
    }
    
    void stressTestMemoryAllocation() {
        // Try to allocate large chunks of memory
        uint32_t allocSize = random(1024, 8192); // 1-8KB chunks
        
        if (api->quota()->safeAllocateMemory(allocSize)) {
            // In a real app, you'd actually allocate memory here
            // For this test, we just track the quota
            successfulAllocations++;
            WISP_DEBUG_INFO("STRESS_APP", "Memory allocation allowed: " + String(allocSize) + " bytes");
            
            // Simulate freeing it immediately
            api->quota()->freeMemory(allocSize);
        } else {
            failedAllocations++;
            WISP_DEBUG_WARNING("STRESS_APP", "Memory allocation blocked: " + String(allocSize) + " bytes");
        }
    }
    
    void stressTestDrawCalls() {
        // Try to make excessive draw calls
        auto* gfx = api->graphics();
        
        for (int i = 0; i < 500; i++) { // Intentionally excessive
            if (api->quota()->safeDraw()) {
                // This would normally result in an actual draw call
                drawCallCount++;
            } else {
                WISP_DEBUG_WARNING("STRESS_APP", "Draw call blocked by quota");
                break;
            }
        }
    }
    
    void stressTestPerformance() {
        // Intentionally slow operations to test frame time limits
        uint32_t startTime = micros();
        
        // Simulate complex calculations
        volatile float result = 0;
        for (int i = 0; i < 10000; i++) {
            result += sin(i * 0.01f) * cos(i * 0.01f);
        }
        
        uint32_t elapsed = micros() - startTime;
        if (elapsed > WISP_MAX_UPDATE_TIME_US) {
            WISP_DEBUG_WARNING("STRESS_APP", 
                "Performance test exceeded time limit: " + 
                String(elapsed) + "μs > " + String(WISP_MAX_UPDATE_TIME_US) + "μs");
        }
    }
    
    void stressTestErrorGeneration() {
        // Intentionally generate various types of errors
        static int errorType = 0;
        
        switch (errorType % 4) {
            case 0:
                WISP_DEBUG_ERROR("STRESS_APP", "Simulated critical error");
                break;
            case 1:
                WISP_DEBUG_WARNING("STRESS_APP", "Simulated warning condition");
                break;
            case 2:
                WISP_DEBUG_INFO("STRESS_APP", "Simulated info message");
                break;
            case 3:
                // Test quota violation logging
                for (int i = 0; i < 5; i++) {
                    if (!api->quota()->canAllocateEntity()) {
                        WISP_DEBUG_ERROR("STRESS_APP", "Entity quota exhausted");
                        break;
                    }
                }
                break;
        }
        
        errorType++;
    }
    
    void updateEntities() {
        for (int i = 0; i < entityCount; i++) {
            if (entities[i].active) {
                entities[i].x += entities[i].vx;
                entities[i].y += entities[i].vy;
                
                // Bounce off edges
                if (entities[i].x <= 0 || entities[i].x >= SCREEN_WIDTH) {
                    entities[i].vx = -entities[i].vx;
                }
                if (entities[i].y <= 50 || entities[i].y >= SCREEN_HEIGHT - 10) {
                    entities[i].vy = -entities[i].vy;
                }
            }
        }
    }
    
    void renderEntities() {
        auto* gfx = api->graphics();
        
        for (int i = 0; i < entityCount; i++) {
            if (entities[i].active && api->quota()->safeDraw()) {
                gfx->fillCircle((int)entities[i].x, (int)entities[i].y, 3, entities[i].color);
            }
        }
    }
    
    void renderTestStats() {
        auto* gfx = api->graphics();
        
        gfx->setTextColor(COLOR_GREEN);
        gfx->setTextSize(1);
        
        int y = 60;
        gfx->drawText(("Entities: " + String(entityCount) + "/" + String(WISP_MAX_ENTITIES)).c_str(), 
                     10, y, false);
        y += 15;
        
        gfx->drawText(("Successful: " + String(successfulAllocations)).c_str(), 10, y, false);
        y += 15;
        
        gfx->drawText(("Failed: " + String(failedAllocations)).c_str(), 10, y, false);
        y += 15;
        
        gfx->drawText(("Frame: " + String(frameCount)).c_str(), 10, y, false);
        
        // Memory usage
        float memUsage = api->quota()->getMemoryUsage() * 100;
        gfx->drawText(("Memory: " + String((int)memUsage) + "%").c_str(), 
                     SCREEN_WIDTH - 10, 60, false, true);
    }
    
    void renderConfigInfo() {
        auto* gfx = api->graphics();
        
        // Show current configuration at bottom
        gfx->setTextColor(COLOR_YELLOW);
        gfx->setTextSize(1);
        
        String configText = "Debug: ";
        configText += WISP_APP_DEBUG_MODE ? "ON" : "OFF";
        configText += " | Safety: ";
        configText += WISP_APP_SAFETY_DISABLED ? "OFF" : "ON";
        
        gfx->drawText(configText.c_str(), SCREEN_WIDTH / 2, SCREEN_HEIGHT - 15, true);
    }
    
    void nextPhase(const String& completedPhase) {
        WISP_DEBUG_INFO("STRESS_APP", completedPhase + " completed");
        stressTestPhase++;
        phaseStartTime = millis();
        
        // Reset some counters for next phase
        if (stressTestPhase % 2 == 0) {
            successfulAllocations = 0;
            failedAllocations = 0;
        }
    }
    
    String getPhaseDescription() {
        switch (stressTestPhase) {
            case 0: return "Entity Allocation";
            case 1: return "Memory Allocation";
            case 2: return "Draw Call Stress";
            case 3: return "Performance Test";
            case 4: return "Error Generation";
            default: return "Complete";
        }
    }
    
    void logTestStats() {
        String stats = "Frame " + String(frameCount) + 
                      " | Entities: " + String(entityCount) + 
                      " | Success: " + String(successfulAllocations) + 
                      " | Failed: " + String(failedAllocations);
        WISP_DEBUG_INFO("STATS", stats);
        
        // Log quota usage
        api->quota()->printUsageStats();
    }
    
    void logFinalStats() {
        WISP_DEBUG_INFO("STRESS_APP", "=== FINAL STRESS TEST RESULTS ===");
        WISP_DEBUG_INFO("STRESS_APP", "Total frames: " + String(frameCount));
        WISP_DEBUG_INFO("STRESS_APP", "Successful operations: " + String(successfulAllocations));
        WISP_DEBUG_INFO("STRESS_APP", "Failed operations: " + String(failedAllocations));
        WISP_DEBUG_INFO("STRESS_APP", "Max entities reached: " + String(entityCount));
        
        uint32_t errors, warnings;
        WispDebugSystem::getDebugStats(errors, warnings);
        WISP_DEBUG_INFO("STRESS_APP", "Debug errors: " + String(errors));
        WISP_DEBUG_INFO("STRESS_APP", "Debug warnings: " + String(warnings));
        
        WISP_DEBUG_INFO("STRESS_APP", "================================");
    }
};

// =============================================================================
// APP FACTORY FUNCTION
// =============================================================================

// This would be called by the app loader
WispAppBase* createStressTestApp() {
    return new StressTestApp();
}

/*
USAGE INSTRUCTIONS:
===================

1. DEVELOPMENT MODE:
   - Uncomment #define WISP_DEV_MODE
   - Safe testing with full debug logging
   - Quota limits enforced, violations logged

2. STRESS TEST MODE:
   - Uncomment #define WISP_STRESS_TEST_MODE  
   - DANGEROUS: Safety limits disabled
   - System may crash, but all violations logged
   - Use only for testing app robustness

3. PRODUCTION MODE:
   - Uncomment #define WISP_PRODUCTION_MODE
   - No debug logging, all safety enforced
   - Maximum performance

EXPECTED BEHAVIOR:
==================

Debug Mode:
- Entity allocation stops at quota limit
- Memory allocation stops at quota limit
- Draw calls stop at quota limit
- All violations logged to SD card and debug pins
- System remains stable

Stress Test Mode:
- All operations allowed regardless of quotas
- Violations logged but not blocked
- System may crash when resources exhausted
- Provides feedback on app behavior under stress

The debug pins will flash:
- Pin 12: Error conditions
- Pin 13: Warning conditions  
- Pin 14: Info messages
- Pin 15: System heartbeat (every second)

Check the /error.log file on SD card for detailed logs.
*/
