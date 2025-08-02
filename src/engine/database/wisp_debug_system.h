// wisp_debug_system.h - Debug system stub for ESP32-C6/S3
#pragma once
#include "../system/esp32_common.h"

// Simple debug macros using ESP_LOG
#define DEBUG_INIT() ESP_LOGI("DEBUG", "Debug system initialized")
#define DEBUG_LOG(msg) ESP_LOGI("DEBUG", "%s", msg)
#define DEBUG_ERROR(msg) ESP_LOGE("DEBUG", "%s", msg)
