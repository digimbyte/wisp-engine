// esp32_common.h - ESP-IDF Native Headers for ESP32-C6/S3
// 
// PURE ESP-IDF IMPLEMENTATION - NO ARDUINO SUPPORT
// This is the primary include for all Wisp Engine files on ESP32-C6/S3
// Provides ONLY native ESP-IDF functionality for maximum performance
// 
// ARCHITECTURE: Native ESP32-C6/S3 using ESP-IDF framework
// - All timing functions use esp_timer (microsecond precision)
// - All GPIO uses native ESP-IDF drivers (fastest possible)
// - All logging uses ESP_LOG system (structured logging)
// - NO Arduino compatibility - pure ESP32 native code only
//
#pragma once

// ESP-IDF Core Headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_efuse.h"
#include "esp_random.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
// Note: I2S headers removed - use driver/i2s_std.h when needed
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_spiffs.h"
#include "esp_heap_caps.h"
#include "esp_attr.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
// Note: esp_bt.h removed - Bluetooth not needed for basic engine

// SD Card Support (ESP-IDF native)
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"

// Standard C++ Headers (native)
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <fstream>
#include <iostream>
#include <cstdarg>

// ESP32-native type definitions
using String = std::string;  // Use std::string as String type

// Native ESP32 timing functions (microsecond precision)
inline uint64_t micros() {
    return esp_timer_get_time();
}

inline uint32_t millis() {
    return esp_timer_get_time() / 1000;
}

inline void delay_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

inline void delay_us(uint32_t us) {
    esp_rom_delay_us(us);
}

// Native ESP32 memory functions
inline uint32_t get_free_heap() {
    return esp_get_free_heap_size();
}

inline uint32_t get_min_free_heap() {
    return esp_get_minimum_free_heap_size();
}

// Native ESP32 system functions
inline void restart_system() {
    esp_restart();
}

inline uint64_t get_chip_id() {
    uint64_t chipid = 0;
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    memcpy(&chipid, mac, 6);
    return chipid;
}

// Native ESP32 random functions
inline uint32_t random_uint32() {
    return esp_random();
}

inline int32_t random_range(int32_t min, int32_t max) {
    return min + (esp_random() % (max - min));
}

// Native ESP32 math functions
template<typename T>
constexpr T clamp(T value, T min_val, T max_val) {
    return (value < min_val) ? min_val : (value > max_val) ? max_val : value;
}

template<typename T>
constexpr T map_range(T x, T in_min, T in_max, T out_min, T out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// GPIO definitions (ESP-IDF native)
#define GPIO_HIGH 1
#define GPIO_LOW 0

// Arduino compatibility macros for Serial (maps to ESP_LOG)
#define HEX 16  // For println/print hex formatting
class SerialClass {
public:
    void println(const char* msg) { ESP_LOGI("WISP", "%s", msg); }
    void print(const char* msg) { ESP_LOGI("WISP", "%s", msg); }
    void print(int val) { ESP_LOGI("WISP", "%d", val); }
    void print(uint16_t val) { ESP_LOGI("WISP", "%u", val); }
    void print(uint32_t val) { ESP_LOGI("WISP", "%u", val); }
    void print(float val, int decimals = 2) { ESP_LOGI("WISP", "%.*f", decimals, val); }
    void println(uint32_t val, int base = 10) { 
        if (base == 16) ESP_LOGI("WISP", "0x%X", val);
        else ESP_LOGI("WISP", "%u", val);
    }
    void printf(const char* format, ...) { 
        va_list args; va_start(args, format); 
        esp_log_writev(ESP_LOG_INFO, "WISP", format, args); 
        va_end(args); 
    }
};
extern SerialClass Serial;
