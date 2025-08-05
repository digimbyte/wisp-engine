// debug.cpp - ESP32-C6/S3 Debug System Implementation (ESP-IDF Native)
// Complete implementation of WispDebugSystem class
#include "debug.h"
#include "../../system/esp32_common.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <stdio.h>
#include <string.h>

using namespace WispEngine::Core;

// Static member definitions
bool WispDebugSystem::debugMode = false;
bool WispDebugSystem::safetyDisabled = false;
uint32_t WispDebugSystem::errorCount = 0;
uint32_t WispDebugSystem::warningCount = 0;
uint32_t WispDebugSystem::lastHeartbeat = 0;
bool WispDebugSystem::pinsInitialized = false;
char WispDebugSystem::currentAppName[32] = "";
uint32_t WispDebugSystem::errorsThisSecond = 0;
uint32_t WispDebugSystem::lastErrorSecond = 0;

// Main initialization method
void WispDebugSystem::init(bool enableDebug, bool disableSafety) {
    debugMode = enableDebug;
    safetyDisabled = disableSafety;
    errorCount = 0;
    warningCount = 0;
    errorsThisSecond = 0;
    lastErrorSecond = 0;
    lastHeartbeat = get_millis();
    pinsInitialized = false;
    
    if (debugMode) {
        ESP_LOGI("DEBUG", "=== Wisp Debug System Initialized ===");
        ESP_LOGI("DEBUG", "Debug Mode: %s", debugMode ? "ENABLED" : "DISABLED");
        ESP_LOGI("DEBUG", "Safety: %s", safetyDisabled ? "DISABLED (DANGER MODE)" : "ENABLED");
        
        initDebugPins();
        
        if (safetyDisabled) {
            ESP_LOGW("DEBUG", "WARNING: SAFETY DISABLED - SYSTEM MAY CRASH!");
            ESP_LOGW("DEBUG", "This mode is for development stress testing only!");
            
            // Flash all debug pins as warning
            for (int i = 0; i < 10; i++) {
                signalAllPins(true);
                delay_ms(100);
                signalAllPins(false);
                delay_ms(100);
            }
        }
    }
}

void WispDebugSystem::setCurrentApp(const char* appName) {
    strncpy(currentAppName, appName, sizeof(currentAppName) - 1);
    currentAppName[sizeof(currentAppName) - 1] = '\0';  // Ensure null termination
    if (debugMode) {
        char message[64];
        snprintf(message, sizeof(message), "Switched to app: %s", appName);
        logMessage(WISP_LOG_INFO, "App", message);
    }
}

bool WispDebugSystem::isDebugEnabled() {
    return debugMode;
}

bool WispDebugSystem::isSafetyDisabled() {
    return safetyDisabled;
}

bool WispDebugSystem::checkQuotaLimit(const char* operation, bool withinLimit) {
    if (safetyDisabled) {
        // In unsafe mode, always allow operation but log it
        if (debugMode && !withinLimit) {
            char message[128];
            snprintf(message, sizeof(message), "Safety disabled - allowing quota violation: %s", operation);
            logMessage(WISP_LOG_WARNING, "QUOTA", message);
            // TODO: Implement signalPin for debug pins
        }
        return true; // Always proceed in unsafe mode
    }
    
    if (!withinLimit) {
        if (debugMode) {
            char message[128];
            snprintf(message, sizeof(message), "Quota limit exceeded: %s", operation);
            logMessage(WISP_LOG_ERROR, "QUOTA", message);
            // TODO: Implement signalPin for debug pins
        }
        return false; // Block operation when safety enabled
    }
    
    return true; // Within limits, proceed
}

void WispDebugSystem::logMessage(WispLogLevel level, const char* category, const char* message) {
    if (!debugMode) return;
    
    const char* timestamp = getTimestamp();
    const char* levelStr = getLevelString(level);
    (void)timestamp; // Unused for now
    (void)levelStr;  // Unused for now
    
    // Output using ESP-IDF logging system
    switch (level) {
        case WISP_LOG_ERROR:
            ESP_LOGE(category, "%s", message);
            break;
        case WISP_LOG_WARNING:
            ESP_LOGW(category, "%s", message);
            break;
        case WISP_LOG_INFO:
            ESP_LOGI(category, "%s", message);
            break;
        case WISP_LOG_DEBUG:
            ESP_LOGD(category, "%s", message);
            break;
    }
    
    // Update counters
    updateErrorCounters(level);
    
    // Check for error storm (too many errors per second)
    if (!safetyDisabled && level == WISP_LOG_ERROR) {
        checkErrorStorm();
    }
}

void WispDebugSystem::logError(const char* category, const char* message) {
    logMessage(WISP_LOG_ERROR, category, message);
}

void WispDebugSystem::logWarning(const char* category, const char* message) {
    logMessage(WISP_LOG_WARNING, category, message);
}

void WispDebugSystem::logInfo(const char* category, const char* message) {
    logMessage(WISP_LOG_INFO, category, message);
}

void WispDebugSystem::logDebug(const char* category, const char* message) {
    logMessage(WISP_LOG_DEBUG, category, message);
}

void WispDebugSystem::logQuotaViolation(const char* resourceType, uint32_t current, uint32_t max) {
    char message[128];
    snprintf(message, sizeof(message), "%s quota exceeded: %lu/%lu", resourceType, (unsigned long)current, (unsigned long)max);
    logError("QUOTA", message);
}

void WispDebugSystem::logPerformanceWarning(const char* operation, uint32_t timeUs, uint32_t limitUs) {
    char message[128];
    snprintf(message, sizeof(message), "%s took %luμs (limit: %luμs)", operation, (unsigned long)timeUs, (unsigned long)limitUs);
    logWarning("PERFORMANCE", message);
}

void WispDebugSystem::heartbeat() {
    uint32_t now = get_millis();
    if (now - lastHeartbeat > 1000) { // Every second
        lastHeartbeat = now;
        
        // Log system stats periodically
        if ((now / 1000) % 30 == 0) { // Every 30 seconds
            logSystemStats();
        }
    }
}

void WispDebugSystem::activateEmergencyMode(const char* reason) {
    char emergencyMsg[256];
    snprintf(emergencyMsg, sizeof(emergencyMsg), "Emergency mode activated: %s", reason);
    logError("EMERGENCY", emergencyMsg);
    
    if (debugMode) {
        // Flash all pins rapidly (TODO: implement when debug pins are ready)
        for (int i = 0; i < 20; i++) {
            signalAllPins(true);
            delay_ms(50);
            signalAllPins(false);
            delay_ms(50);
        }
    }
}

void WispDebugSystem::getDebugStats(uint32_t& errors, uint32_t& warnings) {
    errors = errorCount;
    warnings = warningCount;
}

void WispDebugSystem::shutdown() {
    if (debugMode) {
        logInfo("SYSTEM", "Debug system shutting down");
        
        // Turn off all debug pins
        if (pinsInitialized) {
            signalAllPins(false);
        }
    }
}

// Private implementation methods
void WispDebugSystem::initDebugPins() {
    // TODO: Implement debug pin initialization when board configs are ready
    pinsInitialized = true;
    ESP_LOGI("DEBUG", "Debug pins initialized (placeholder)");
}

void WispDebugSystem::initErrorLog() {
    // TODO: Implement SD card error logging when filesystem is ready
    ESP_LOGI("DEBUG", "Error log initialized (placeholder)");
}

void WispDebugSystem::rotateLogFiles() {
    // TODO: Implement log file rotation
    ESP_LOGI("DEBUG", "Log files rotated (placeholder)");
}

void WispDebugSystem::writeToErrorLog(const char* message) {
    // TODO: Implement SD card writing
    // For now, just use ESP-IDF logging
}

void WispDebugSystem::outputDebugSignal(WispLogLevel level) {
    // TODO: Implement debug pin signaling
}

void WispDebugSystem::signalPin(int pin) {
    // TODO: Implement individual pin signaling
}

void WispDebugSystem::signalAllPins(bool active) {
    // TODO: Implement all pins signaling
}

void WispDebugSystem::updateErrorCounters(WispLogLevel level) {
    uint32_t currentSecond = get_millis() / 1000;
    
    if (currentSecond != lastErrorSecond) {
        errorsThisSecond = 0;
        lastErrorSecond = currentSecond;
    }
    
    switch (level) {
        case WISP_LOG_ERROR:
            errorCount++;
            errorsThisSecond++;
            break;
        case WISP_LOG_WARNING:
            warningCount++;
            break;
        default:
            break;
    }
}

void WispDebugSystem::checkErrorStorm() {
    const uint32_t MAX_ERRORS_PER_SECOND = 10; // TODO: Move to config
    
    if (errorsThisSecond >= MAX_ERRORS_PER_SECOND) {
        char errorMsg[128];
        snprintf(errorMsg, sizeof(errorMsg), "Too many errors per second: %lu", (unsigned long)errorsThisSecond);
        activateEmergencyMode(errorMsg);
        
        // In safety mode, force emergency shutdown
        if (!safetyDisabled) {
            logError("EMERGENCY", "Error storm detected - forcing emergency mode");
            delay_ms(1000); // Give time for logs to flush
            // This would trigger the emergency menu in the main loop
        }
    }
}

void WispDebugSystem::logSystemStats() {
    char stats[256];
    int len = snprintf(stats, sizeof(stats), "Heap: %lu bytes, Errors: %lu, Warnings: %lu", 
                      (unsigned long)get_free_heap(), (unsigned long)errorCount, (unsigned long)warningCount);
    
    if (strlen(currentAppName) > 0) {
        snprintf(stats + len, sizeof(stats) - len, ", App: %s", currentAppName);
    }
    logInfo("STATS", stats);
}

const char* WispDebugSystem::getTimestamp() {
    static char timestamp[16];  // Static buffer for timestamp string
    uint32_t ms = get_millis();
    uint32_t seconds = ms / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    
    ms %= 1000;
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    snprintf(timestamp, sizeof(timestamp), "%02lu:%02lu:%02lu.%03lu", 
             (unsigned long)hours, (unsigned long)minutes, (unsigned long)seconds, (unsigned long)ms);
    return timestamp;
}

const char* WispDebugSystem::getLevelString(WispLogLevel level) {
    switch (level) {
        case WISP_LOG_ERROR: return "ERROR";
        case WISP_LOG_WARNING: return "WARN ";
        case WISP_LOG_INFO: return "INFO ";
        case WISP_LOG_DEBUG: return "DEBUG";
        default: return "UNKNOWN";
    }
}
