// system/debug_esp32.h - ESP-IDF Native Debug System for Wisp Engine
#pragma once

#include "esp32_common.h"

// Include board-specific debug pin definitions
#ifdef PLATFORM_S3
    #include "../../boards/esp32-s3_config.h"
#elif defined(PLATFORM_C6)
    #include "../../boards/esp32-c6_config.h"
#endif

enum WispLogLevel {
    LOG_ERROR = 0,
    LOG_WARNING = 1,
    LOG_INFO = 2,
    LOG_DEBUG = 3
};

// Debug system configuration
#ifndef WISP_DEBUG_ENABLED_DEFAULT
#define WISP_DEBUG_ENABLED_DEFAULT true
#endif

#ifndef WISP_SAFETY_DISABLED_DEFAULT  
#define WISP_SAFETY_DISABLED_DEFAULT false
#endif

#ifndef DEBUG_OUTPUT_PINS
#define DEBUG_OUTPUT_PINS true
#endif

#ifndef DEBUG_LOG_TO_SD
#define DEBUG_LOG_TO_SD true
#endif

#ifndef MAX_ERRORS_PER_SECOND
#define MAX_ERRORS_PER_SECOND 10
#endif

#ifndef DEBUG_SIGNAL_DURATION_MS
#define DEBUG_SIGNAL_DURATION_MS 100
#endif

#ifndef ERROR_LOG_MAX_SIZE
#define ERROR_LOG_MAX_SIZE (1024 * 1024) // 1MB
#endif

#ifndef ERROR_LOG_ROTATION_COUNT
#define ERROR_LOG_ROTATION_COUNT 5
#endif

// Default pin definitions if not in board config
#ifndef DEBUG_ERROR_PIN
#define DEBUG_ERROR_PIN 2
#endif

#ifndef DEBUG_WARNING_PIN
#define DEBUG_WARNING_PIN 3
#endif

#ifndef DEBUG_INFO_PIN
#define DEBUG_INFO_PIN 4
#endif

#ifndef DEBUG_HEARTBEAT_PIN
#define DEBUG_HEARTBEAT_PIN 5
#endif

#ifndef DEBUG_PIN_ACTIVE
#define DEBUG_PIN_ACTIVE 1
#endif

#ifndef DEBUG_PIN_INACTIVE
#define DEBUG_PIN_INACTIVE 0
#endif

class DebugSystem {
private:
    static bool debugMode;
    static bool safetyDisabled;
    static uint32_t errorCount;
    static uint32_t warningCount;
    static uint32_t lastHeartbeat;
    static bool pinsInitialized;
    static String currentAppName;
    
    // Error tracking
    static uint32_t errorsThisSecond;
    static uint32_t lastErrorSecond;
    
    // ESP-IDF logging tag
    static const char* TAG;
    
public:
    static void init(bool enableDebug = DEBUG_MODE_ENABLED, 
                    bool disableSafety = SAFETY_DISABLED) {
        debugMode = enableDebug;
        safetyDisabled = disableSafety;
        errorCount = 0;
        warningCount = 0;
        errorsThisSecond = 0;
        lastErrorSecond = 0;
        lastHeartbeat = millis();
        pinsInitialized = false;
        
        if (debugMode) {
            ESP_LOGI(TAG, "=== Wisp Debug System Initialized ===");
            ESP_LOGI(TAG, "Debug Mode: %s", debugMode ? "ENABLED" : "DISABLED");
            ESP_LOGI(TAG, "Safety: %s", safetyDisabled ? "DISABLED (DANGER MODE)" : "ENABLED");
            
            initDebugPins();
            
            if (safetyDisabled) {
                ESP_LOGW(TAG, "WARNING: SAFETY DISABLED - SYSTEM MAY CRASH!");
                ESP_LOGW(TAG, "This mode is for development stress testing only!");
                
                // Flash all debug pins as warning
                for (int i = 0; i < 10; i++) {
                    signalAllPins(true);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    signalAllPins(false);
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
            }
        }
    }
    
    static void setCurrentApp(const String& appName) {
        currentAppName = appName;
        if (debugMode) {
            ESP_LOGI(TAG, "Switched to app: %s", appName.c_str());
        }
    }
    
    static bool isDebugEnabled() { return debugMode; }
    static bool isSafetyDisabled() { return safetyDisabled; }
    
    // Safety check function - returns true if operation should proceed
    static bool checkQuotaLimit(const String& operation, bool withinLimit) {
        if (safetyDisabled) {
            // In unsafe mode, always allow operation but log it
            if (debugMode && !withinLimit) {
                ESP_LOGW(TAG, "Safety disabled - allowing quota violation: %s", operation.c_str());
                signalPin(DEBUG_WARNING_PIN);
            }
            return true; // Always proceed in unsafe mode
        }
        
        if (!withinLimit) {
            if (debugMode) {
                ESP_LOGE(TAG, "Quota limit exceeded: %s", operation.c_str());
                signalPin(DEBUG_ERROR_PIN);
            }
            return false; // Block operation when safety enabled
        }
        
        return true; // Within limits, proceed
    }
    
    // Error logging with different severity levels
    static void logMessage(WispLogLevel level, const String& category, const String& message) {
        if (!debugMode) return;
        
        const char* categoryStr = category.c_str();
        const char* messageStr = message.c_str();
        
        // Use ESP-IDF logging system
        switch (level) {
            case LOG_ERROR:
                ESP_LOGE(categoryStr, "%s", messageStr);
                break;
            case LOG_WARNING:
                ESP_LOGW(categoryStr, "%s", messageStr);
                break;
            case LOG_INFO:
                ESP_LOGI(categoryStr, "%s", messageStr);
                break;
            case LOG_DEBUG:
                ESP_LOGD(categoryStr, "%s", messageStr);
                break;
        }
        
        // Output to debug pins if enabled
        if (DEBUG_OUTPUT_PINS) {
            outputDebugSignal(level);
        }
        
        // Update counters
        updateErrorCounters(level);
        
        // Check for error storm (too many errors per second)
        if (!safetyDisabled && level == LOG_ERROR) {
            checkErrorStorm();
        }
    }
    
    // Convenience functions for different log levels
    static void logError(const String& category, const String& message) {
        logMessage(LOG_ERROR, category, message);
    }
    
    static void logWarning(const String& category, const String& message) {
        logMessage(LOG_WARNING, category, message);
    }
    
    static void logInfo(const String& category, const String& message) {
        logMessage(LOG_INFO, category, message);
    }
    
    static void logDebug(const String& category, const String& message) {
        logMessage(LOG_DEBUG, category, message);
    }
    
    // Resource quota violation logging
    static void logQuotaViolation(const String& resourceType, uint32_t current, uint32_t max) {
        std::string message = resourceType + " quota exceeded: " + std::to_string(current) + "/" + std::to_string(max);
        logError("QUOTA", message);
    }
    
    // Performance monitoring
    static void logPerformanceWarning(const String& operation, uint32_t timeUs, uint32_t limitUs) {
        std::string message = operation + " took " + std::to_string(timeUs) + "μs (limit: " + std::to_string(limitUs) + "μs)";
        logWarning("PERFORMANCE", message);
    }
    
    // System heartbeat for monitoring
    static void heartbeat() {
        uint32_t now = millis();
        if (now - lastHeartbeat > 1000) { // Every second
            lastHeartbeat = now;
            
            if (debugMode && DEBUG_OUTPUT_PINS) {
                // Pulse heartbeat pin
                signalPin(DEBUG_HEARTBEAT_PIN);
            }
            
            // Log system stats periodically
            if ((now / 1000) % 30 == 0) { // Every 30 seconds
                logSystemStats();
            }
        }
    }
    
    // Emergency mode activation
    static void activateEmergencyMode(const String& reason) {
        logError("EMERGENCY", "Emergency mode activated: " + reason);
        
        if (debugMode && DEBUG_OUTPUT_PINS) {
            // Flash all pins rapidly
            for (int i = 0; i < 20; i++) {
                signalAllPins(true);
                vTaskDelay(pdMS_TO_TICKS(50));
                signalAllPins(false);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }
    }
    
    // Get debug statistics
    static void getDebugStats(uint32_t& errors, uint32_t& warnings) {
        errors = errorCount;
        warnings = warningCount;
    }
    
    // Cleanup and shutdown
    static void shutdown() {
        if (debugMode) {
            ESP_LOGI(TAG, "Debug system shutting down");
            
            // Turn off all debug pins
            if (pinsInitialized) {
                signalAllPins(false);
            }
        }
    }
    
private:
    static void initDebugPins() {
        if (!DEBUG_OUTPUT_PINS) return;
        
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        
        // Configure debug pins
        io_conf.pin_bit_mask = ((1ULL << DEBUG_ERROR_PIN) | 
                               (1ULL << DEBUG_WARNING_PIN) | 
                               (1ULL << DEBUG_INFO_PIN) | 
                               (1ULL << DEBUG_HEARTBEAT_PIN));
        
        gpio_config(&io_conf);
        
        // Set all pins to inactive state
        gpio_set_level((gpio_num_t)DEBUG_ERROR_PIN, DEBUG_PIN_INACTIVE);
        gpio_set_level((gpio_num_t)DEBUG_WARNING_PIN, DEBUG_PIN_INACTIVE);
        gpio_set_level((gpio_num_t)DEBUG_INFO_PIN, DEBUG_PIN_INACTIVE);
        gpio_set_level((gpio_num_t)DEBUG_HEARTBEAT_PIN, DEBUG_PIN_INACTIVE);
        
        pinsInitialized = true;
        ESP_LOGI(TAG, "Debug pins initialized");
    }
    
    static void outputDebugSignal(WispLogLevel level) {
        if (!pinsInitialized) return;
        
        switch (level) {
            case LOG_ERROR:
                signalPin(DEBUG_ERROR_PIN);
                break;
            case LOG_WARNING:
                signalPin(DEBUG_WARNING_PIN);
                break;
            case LOG_INFO:
                signalPin(DEBUG_INFO_PIN);
                break;
            default:
                break;
        }
    }
    
    static void signalPin(int pin) {
        if (pinsInitialized) {
            gpio_set_level((gpio_num_t)pin, DEBUG_PIN_ACTIVE);
            
            // Create a simple timer to turn off the pin
            // In production, you'd use a proper timer
            static TaskHandle_t signalTask = nullptr;
            if (signalTask == nullptr) {
                xTaskCreate([](void* param) {
                    vTaskDelay(pdMS_TO_TICKS(DEBUG_SIGNAL_DURATION_MS));
                    gpio_set_level((gpio_num_t)(intptr_t)param, DEBUG_PIN_INACTIVE);
                    vTaskDelete(nullptr);
                }, "signal_pin", 1024, (void*)(intptr_t)pin, 1, &signalTask);
            }
        }
    }
    
    static void signalAllPins(bool active) {
        if (!pinsInitialized) return;
        
        int state = active ? DEBUG_PIN_ACTIVE : DEBUG_PIN_INACTIVE;
        
        gpio_set_level((gpio_num_t)DEBUG_ERROR_PIN, state);
        gpio_set_level((gpio_num_t)DEBUG_WARNING_PIN, state);
        gpio_set_level((gpio_num_t)DEBUG_INFO_PIN, state);
        gpio_set_level((gpio_num_t)DEBUG_HEARTBEAT_PIN, state);
    }
    
    static void updateErrorCounters(WispLogLevel level) {
        uint32_t currentSecond = millis() / 1000;
        
        if (currentSecond != lastErrorSecond) {
            errorsThisSecond = 0;
            lastErrorSecond = currentSecond;
        }
        
        switch (level) {
            case LOG_ERROR:
                errorCount++;
                errorsThisSecond++;
                break;
            case LOG_WARNING:
                warningCount++;
                break;
            default:
                break;
        }
    }
    
    static void checkErrorStorm() {
        if (errorsThisSecond >= MAX_ERRORS_PER_SECOND) {
            char errorMsg[128];
            snprintf(errorMsg, sizeof(errorMsg), "Too many errors per second: %lu", errorsThisSecond);
            activateEmergencyMode(errorMsg);
            
            // In safety mode, force emergency shutdown
            if (!safetyDisabled) {
                ESP_LOGE(TAG, "Error storm detected - forcing emergency mode");
                vTaskDelay(pdMS_TO_TICKS(1000)); // Give time for logs to flush
                // This would trigger the emergency menu in the main loop
            }
        }
    }
    
    static void logSystemStats() {
        uint32_t freeHeap = esp_get_free_heap_size();
        ESP_LOGI("STATS", "Heap: %lu bytes, Errors: %lu, Warnings: %lu%s%s", 
                freeHeap, errorCount, warningCount,
                currentAppName.empty() ? "" : ", App: ",
                currentAppName.empty() ? "" : currentAppName.c_str());
    }
};

// Static member definitions
bool DebugSystem::debugMode = false;
bool DebugSystem::safetyDisabled = false;
uint32_t DebugSystem::errorCount = 0;
uint32_t DebugSystem::warningCount = 0;
uint32_t DebugSystem::lastHeartbeat = 0;
bool DebugSystem::pinsInitialized = false;
String DebugSystem::currentAppName = "";
uint32_t DebugSystem::errorsThisSecond = 0;
uint32_t DebugSystem::lastErrorSecond = 0;
const char* DebugSystem::TAG = "DEBUG";

// Convenience macros for common debug operations
#define DEBUG_INIT(debug, safety) DebugSystem::init(debug, safety)
#define DEBUG_ERROR(category, message) DebugSystem::logError(category, message)
#define DEBUG_WARNING(category, message) DebugSystem::logWarning(category, message)
#define DEBUG_INFO(category, message) DebugSystem::logInfo(category, message)
#define DEBUG_CHECK_QUOTA(op, limit) DebugSystem::checkQuotaLimit(op, limit)
#define DEBUG_HEARTBEAT() DebugSystem::heartbeat()
#define DEBUG_SET_APP(name) DebugSystem::setCurrentApp(name)
