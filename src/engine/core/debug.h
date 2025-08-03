// debug.h - ESP32-C6/S3 Debug System Declarations (ESP-IDF Native)
// Header file with class declarations and prototypes only
#pragma once

// Use centralized engine header for namespace organization
#include "../../wisp_engine.h"

namespace WispEngine::Core {

enum WispLogLevel {
    WISP_LOG_ERROR = 0,
    WISP_LOG_WARNING = 1,
    WISP_LOG_INFO = 2,
    WISP_LOG_DEBUG = 3
};

class WispDebugSystem {
private:
    static bool debugMode;
    static bool safetyDisabled;
    static uint32_t errorCount;
    static uint32_t warningCount;
    static uint32_t lastHeartbeat;
    static bool pinsInitialized;
    static char currentAppName[32];  // Fixed-size buffer for app name
    
    // Error tracking
    static uint32_t errorsThisSecond;
    static uint32_t lastErrorSecond;
    
    // Private helper methods
    static void initDebugPins();
    static void initErrorLog();
    static void rotateLogFiles();
    static void writeToErrorLog(const char* message);  // Write message to error log
    static void outputDebugSignal(WispLogLevel level);
    static void signalPin(int pin);
    static void signalAllPins(bool active);
    static void updateErrorCounters(WispLogLevel level);
    static void checkErrorStorm();
    static void logSystemStats();
    static const char* getTimestamp();  // Get current timestamp
    static const char* getLevelString(WispLogLevel level);  // Get log level string
    
public:
    // Main interface methods
    static void init(bool enableDebug = true, bool disableSafety = false);
    static void setCurrentApp(const char* appName);  // Set current app name
    static bool isDebugEnabled();
    static bool isSafetyDisabled();
    
    // Safety check function
    static bool checkQuotaLimit(const char* operation, bool withinLimit);  // Check quota limits
    
    // Core logging method
    static void logMessage(WispLogLevel level, const char* category, const char* message);  // Log message with level
    
    // Convenience logging functions
    static void logError(const char* category, const char* message);  // Log error message
    static void logWarning(const char* category, const char* message);  // Log warning message
    static void logInfo(const char* category, const char* message);  // Log info message
    static void logDebug(const char* category, const char* message);  // Log debug message
    
    // Specialized logging
    static void logQuotaViolation(const char* resourceType, uint32_t current, uint32_t max);  // Log quota violations
    static void logPerformanceWarning(const char* operation, uint32_t timeUs, uint32_t limitUs);  // Log performance issues
    
    // System monitoring
    static void heartbeat();
    static void activateEmergencyMode(const char* reason);  // Activate emergency mode
    static void getDebugStats(uint32_t& errors, uint32_t& warnings);
    static void shutdown();
};

// Convenience macros for common debug operations
#define WISP_DEBUG_INIT(debug, safety) WispDebugSystem::init(debug, safety)
#define WISP_DEBUG_ERROR(category, message) WispDebugSystem::logError(category, message)
#define WISP_DEBUG_WARNING(category, message) WispDebugSystem::logWarning(category, message)
#define WISP_DEBUG_INFO(category, message) WispDebugSystem::logInfo(category, message)
#define WISP_DEBUG_CHECK_QUOTA(op, limit) WispDebugSystem::checkQuotaLimit(op, limit)
#define WISP_DEBUG_HEARTBEAT() WispDebugSystem::heartbeat()
#define WISP_DEBUG_SET_APP(name) WispDebugSystem::setCurrentApp(name)

}  // namespace WispEngine::Core
