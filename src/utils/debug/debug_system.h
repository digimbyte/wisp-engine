// debug_system.h - Debug system for ESP32-C6/S3
#pragma once

// Use pure ESP-IDF headers only
#include "esp_log.h"
#include <stdarg.h>

// Declare this as part of the WispEngine::Utils namespace
namespace WispEngine::Utils {
    
    class DebugSystem {
    public:
        // Simple debug initialization using pure ESP-IDF
        static void initialize() {
            esp_log_level_set("WISP", ESP_LOG_INFO);
        }
        
        static void log(const char* tag, const char* message) {
            esp_log_write(ESP_LOG_INFO, tag, "%s\n", message);
        }
        
        static void error(const char* tag, const char* message) {
            esp_log_write(ESP_LOG_ERROR, tag, "%s\n", message);
        }
        
        static void warning(const char* tag, const char* message) {
            esp_log_write(ESP_LOG_WARN, tag, "%s\n", message);
        }
        
        // ESP-IDF compatible logging with printf-style formatting
        static void logf(const char* tag, const char* format, ...) {
            va_list args;
            va_start(args, format);
            esp_log_writev(ESP_LOG_INFO, tag, format, args);
            va_end(args);
        }
        
        static void errorf(const char* tag, const char* format, ...) {
            va_list args;
            va_start(args, format);
            esp_log_writev(ESP_LOG_ERROR, tag, format, args);
            va_end(args);
        }
        
        static void warningf(const char* tag, const char* format, ...) {
            va_list args;
            va_start(args, format);
            esp_log_writev(ESP_LOG_WARN, tag, format, args);
            va_end(args);
        }
    };
}

// Convenience macros that use the namespace
#define WISP_DEBUG_INIT() WispEngine::Utils::DebugSystem::initialize()
#define WISP_DEBUG_LOG(tag, msg) WispEngine::Utils::DebugSystem::log(tag, msg)
#define WISP_DEBUG_ERROR(tag, msg) WispEngine::Utils::DebugSystem::error(tag, msg)
#define WISP_DEBUG_WARNING(tag, msg) WispEngine::Utils::DebugSystem::warning(tag, msg)

// Legacy compatibility macros (can be phased out)
#define DEBUG_INIT() WISP_DEBUG_INIT()
#define DEBUG_LOG(msg) WISP_DEBUG_LOG("DEBUG", msg)
#define DEBUG_ERROR(msg) WISP_DEBUG_ERROR("DEBUG", msg)
#define DEBUG_INFO(tag, msg) WISP_DEBUG_LOG(tag, msg)
#define DEBUG_WARNING(tag, msg) WISP_DEBUG_WARNING(tag, msg)
#define DEBUG_ERROR(tag, msg) WISP_DEBUG_ERROR(tag, msg)
#define DEBUG_INFO_STR(tag, msg) WISP_DEBUG_LOG(tag, msg)
#define DEBUG_WARNING_STR(tag, msg) WISP_DEBUG_WARNING(tag, msg)
#define DEBUG_ERROR_STR(tag, msg) WISP_DEBUG_ERROR(tag, msg)
