// debug.cpp - ESP32-C6/S3 Debug System Implementation (ESP-IDF Native)
// Complete implementation of WispDebugSystem class
#include "debug.h"
#include "../../system/esp32_common.h"
#include <iostream>
#include <iomanip>
#include <sstream>

// Static member definitions
bool WispDebugSystem::debugMode = false;
bool WispDebugSystem::safetyDisabled = false;
uint32_t WispDebugSystem::errorCount = 0;
uint32_t WispDebugSystem::warningCount = 0;
uint32_t WispDebugSystem::lastHeartbeat = 0;
bool WispDebugSystem::pinsInitialized = false;
std::string WispDebugSystem::currentAppName = "";
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
    lastHeartbeat = millis();
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

void WispDebugSystem::setCurrentApp(const std::string& appName) {
    currentAppName = appName;
    if (debugMode) {
        logMessage(WISP_LOG_INFO, "App", "Switched to app: " + appName);
    }
}

bool WispDebugSystem::isDebugEnabled() {
    return debugMode;
}

bool WispDebugSystem::isSafetyDisabled() {
    return safetyDisabled;
}

bool WispDebugSystem::checkQuotaLimit(const std::string& operation, bool withinLimit) {
    if (safetyDisabled) {
        // In unsafe mode, always allow operation but log it
        if (debugMode && !withinLimit) {
            logMessage(WISP_LOG_WARNING, "QUOTA", 
                      "Safety disabled - allowing quota violation: " + operation);
            // TODO: Implement signalPin for debug pins
        }
        return true; // Always proceed in unsafe mode
    }
    
    if (!withinLimit) {
        if (debugMode) {
            logMessage(WISP_LOG_ERROR, "QUOTA", 
                      "Quota limit exceeded: " + operation);
            // TODO: Implement signalPin for debug pins
        }
        return false; // Block operation when safety enabled
    }
    
    return true; // Within limits, proceed
}

void WispDebugSystem::logMessage(WispLogLevel level, const std::string& category, const std::string& message) {
    if (!debugMode) return;
    
    std::string timestamp = getTimestamp();
    std::string levelStr = getLevelString(level);
    std::string fullMessage = "[" + timestamp + "] [" + levelStr + "] [" + category + "] " + message;
    
    // Output using ESP-IDF logging system
    switch (level) {
        case WISP_LOG_ERROR:
            ESP_LOGE(category.c_str(), "%s", message.c_str());
            break;
        case WISP_LOG_WARNING:
            ESP_LOGW(category.c_str(), "%s", message.c_str());
            break;
        case WISP_LOG_INFO:
            ESP_LOGI(category.c_str(), "%s", message.c_str());
            break;
        case WISP_LOG_DEBUG:
            ESP_LOGD(category.c_str(), "%s", message.c_str());
            break;
    }
    
    // Update counters
    updateErrorCounters(level);
    
    // Check for error storm (too many errors per second)
    if (!safetyDisabled && level == WISP_LOG_ERROR) {
        checkErrorStorm();
    }
}

void WispDebugSystem::logError(const std::string& category, const std::string& message) {
    logMessage(WISP_LOG_ERROR, category, message);
}

void WispDebugSystem::logWarning(const std::string& category, const std::string& message) {
    logMessage(WISP_LOG_WARNING, category, message);
}

void WispDebugSystem::logInfo(const std::string& category, const std::string& message) {
    logMessage(WISP_LOG_INFO, category, message);
}

void WispDebugSystem::logDebug(const std::string& category, const std::string& message) {
    logMessage(WISP_LOG_DEBUG, category, message);
}

void WispDebugSystem::logQuotaViolation(const std::string& resourceType, uint32_t current, uint32_t max) {
    std::string message = resourceType + " quota exceeded: " + std::to_string(current) + "/" + std::to_string(max);
    logError("QUOTA", message);
}

void WispDebugSystem::logPerformanceWarning(const std::string& operation, uint32_t timeUs, uint32_t limitUs) {
    std::string message = operation + " took " + std::to_string(timeUs) + "μs (limit: " + std::to_string(limitUs) + "μs)";
    logWarning("PERFORMANCE", message);
}

void WispDebugSystem::heartbeat() {
    uint32_t now = millis();
    if (now - lastHeartbeat > 1000) { // Every second
        lastHeartbeat = now;
        
        // Log system stats periodically
        if ((now / 1000) % 30 == 0) { // Every 30 seconds
            logSystemStats();
        }
    }
}

void WispDebugSystem::activateEmergencyMode(const std::string& reason) {
    logError("EMERGENCY", "Emergency mode activated: " + reason);
    
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

void WispDebugSystem::writeToErrorLog(const std::string& message) {
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
    uint32_t currentSecond = millis() / 1000;
    
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
        activateEmergencyMode("Too many errors per second: " + std::to_string(errorsThisSecond));
        
        // In safety mode, force emergency shutdown
        if (!safetyDisabled) {
            logError("EMERGENCY", "Error storm detected - forcing emergency mode");
            delay_ms(1000); // Give time for logs to flush
            // This would trigger the emergency menu in the main loop
        }
    }
}

void WispDebugSystem::logSystemStats() {
    std::string stats = "Heap: " + std::to_string(get_free_heap()) + 
                      " bytes, Errors: " + std::to_string(errorCount) + 
                      ", Warnings: " + std::to_string(warningCount);
    if (!currentAppName.empty()) {
        stats += ", App: " + currentAppName;
    }
    logInfo("STATS", stats);
}

std::string WispDebugSystem::getTimestamp() {
    uint32_t ms = millis();
    uint32_t seconds = ms / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    
    ms %= 1000;
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds << "."
        << std::setfill('0') << std::setw(3) << ms;
    return oss.str();
}

std::string WispDebugSystem::getLevelString(WispLogLevel level) {
    switch (level) {
        case WISP_LOG_ERROR: return "ERROR";
        case WISP_LOG_WARNING: return "WARN ";
        case WISP_LOG_INFO: return "INFO ";
        case WISP_LOG_DEBUG: return "DEBUG";
        default: return "UNKNOWN";
    }
}
