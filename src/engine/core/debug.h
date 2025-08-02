// debug.h - ESP32-C6/S3 Debug System Declarations (ESP-IDF Native)
// Header file with class declarations and prototypes only
#pragma once

#include "../../system/esp32_common.h"
#include <string>

// Include board-specific debug pin definitions
#ifdef PLATFORM_S3
    #include "../../boards/esp32-s3_config.h"
#elif defined(PLATFORM_C6)
    #include "../../boards/esp32-c6_config.h"
#endif

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
    static std::string currentAppName;
    
    // Error tracking
    static uint32_t errorsThisSecond;
    static uint32_t lastErrorSecond;
    
    // Private helper methods
    static void initDebugPins();
    static void initErrorLog();
    static void rotateLogFiles();
    static void writeToErrorLog(const std::string& message);
    static void outputDebugSignal(WispLogLevel level);
    static void signalPin(int pin);
    static void signalAllPins(bool active);
    static void updateErrorCounters(WispLogLevel level);
    static void checkErrorStorm();
    static void logSystemStats();
    static std::string getTimestamp();
    static std::string getLevelString(WispLogLevel level);
    
public:
    // Main interface methods
    static void init(bool enableDebug = true, bool disableSafety = false);
    static void setCurrentApp(const std::string& appName);
    static bool isDebugEnabled();
    static bool isSafetyDisabled();
    
    // Safety check function
    static bool checkQuotaLimit(const std::string& operation, bool withinLimit);
    
    // Core logging method
    static void logMessage(WispLogLevel level, const std::string& category, const std::string& message);
    
    // Convenience logging functions
    static void logError(const std::string& category, const std::string& message);
    static void logWarning(const std::string& category, const std::string& message);
    static void logInfo(const std::string& category, const std::string& message);
    static void logDebug(const std::string& category, const std::string& message);
    
    // Specialized logging
    static void logQuotaViolation(const std::string& resourceType, uint32_t current, uint32_t max);
    static void logPerformanceWarning(const std::string& operation, uint32_t timeUs, uint32_t limitUs);
    
    // System monitoring
    static void heartbeat();
    static void activateEmergencyMode(const std::string& reason);
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
