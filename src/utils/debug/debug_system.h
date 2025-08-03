// debug_system.h - Debug system for ESP32-C6/S3
#pragma once

// Use the centralized engine header for proper namespace organization
#include "../../wisp_engine.h"

// Declare this as part of the WispEngine::Utils namespace
namespace WispEngine::Utils {
    
    class DebugSystem {
    public:
        // Simple debug initialization using ESP_LOG
        static void initialize() {
            ESP_LOGI("DEBUG", "Debug system initialized");
        }
        
        static void log(const char* tag, const char* message) {
            ESP_LOGI(tag, "%s", message);
        }
        
        static void error(const char* tag, const char* message) {
            ESP_LOGE(tag, "%s", message);
        }
        
        static void warning(const char* tag, const char* message) {
            ESP_LOGW(tag, "%s", message);
        }
        
        // Support for std::string
        static void log(const char* tag, const std::string& message) {
            ESP_LOGI(tag, "%s", message.c_str());
        }
        
        static void error(const char* tag, const std::string& message) {
            ESP_LOGE(tag, "%s", message.c_str());
        }
        
        static void warning(const char* tag, const std::string& message) {
            ESP_LOGW(tag, "%s", message.c_str());
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
