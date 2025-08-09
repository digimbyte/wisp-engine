// esp32_common.h - ESP32-C6/S3 Common Headers for ESP-IDF
// Provides all necessary ESP-IDF includes for the WISP engine
#pragma once

// Standard C libraries
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <math.h>
#include <string>
#include <algorithm>
#include <cctype>
#include <functional>

// ESP-IDF core includes
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_rom_sys.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"

// GPIO and peripheral includes
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"

// Common macros
#define delay_ms(ms) vTaskDelay(pdMS_TO_TICKS(ms))
// Don't define micros/millis macros to avoid LovyanGFX conflicts
// Use get_micros() and get_millis() instead
#define get_micros() (esp_timer_get_time())
#define get_millis() (esp_timer_get_time() / 1000)
#define get_free_heap() esp_get_free_heap_size()

// Arduino-like compatibility macros (for easier porting)
#define PROGMEM
#define F(x) (x)
#define PI 3.14159265359
#define HEX 16
#define DEC 10
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

// Arduino map function as inline function (avoiding macro collision with std::map)
inline long mapValue(long value, long fromLow, long fromHigh, long toLow, long toHigh) {
    return ((value) - (fromLow)) * ((toHigh) - (toLow)) / ((fromHigh) - (fromLow)) + (toLow);
}

// ESP32 yield function for cooperative multitasking
// Arduino delay compatibility function
inline void delay(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

// Arduino yield compatibility function
inline void yield() { vTaskDelay(pdMS_TO_TICKS(1)); }

// Simple random function replacement
#undef random
// Helper functions for random with different parameters
static inline long _wisp_random_0() { return esp_random() % 32768; }
static inline long _wisp_random_1(long max) { return esp_random() % max; }
static inline long _wisp_random_2(long min, long max) { return esp_random() % (max - min) + min; }

// Macro to select the right function based on argument count
#define _GET_RANDOM_MACRO(_1,_2,NAME,...) NAME
#define random(...) _GET_RANDOM_MACRO(__VA_ARGS__, _wisp_random_2, _wisp_random_1, _wisp_random_0)(__VA_ARGS__)

// Audio compatibility functions - simplified versions
// Note: These functions provide basic ESP-IDF LEDC functionality
// Configure LEDC timer and channel for PWM output
inline esp_err_t ledcSetup(uint8_t channel, double freq, uint8_t resolution) {
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = (ledc_timer_bit_t)resolution,
        .timer_num = (ledc_timer_t)(channel / 2),
        .freq_hz = (uint32_t)freq,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false
    };
    esp_err_t ret = ledc_timer_config(&timer_config);
    if (ret != ESP_OK) return ret;
    
    ledc_channel_config_t channel_config = {
        .gpio_num = -1,  // Will be set with ledcAttachPin
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = (ledc_channel_t)channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = (ledc_timer_t)(channel / 2),
        .duty = 0,
        .hpoint = 0,
        .flags = { .output_invert = 0 },
        .sleep_mode = LEDC_SLEEP_MODE_DISABLED,
    };
    return ledc_channel_config(&channel_config);
}

inline esp_err_t ledcAttachPin(uint8_t pin, uint8_t channel) {
    return ledc_set_pin(pin, LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel);
}

inline esp_err_t ledcWrite(uint8_t channel, uint32_t duty) {
    esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel, duty);
    if (ret == ESP_OK) {
        ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel);
    }
    return ret;
}

inline esp_err_t ledcWriteTone(uint8_t channel, double freq) {
    return ledc_set_freq(LEDC_LOW_SPEED_MODE, (ledc_timer_t)(channel / 2), (uint32_t)freq);
}
#define delayMicroseconds(us) esp_rom_delay_us(us)

// Serial compatibility for debugging
class SerialClass {
public:
    void print(const char* str) { printf("%s", str); }
    void print(const std::string& str) { printf("%s", str.c_str()); }
    void print(int value) { printf("%d", value); }
    void print(unsigned int value) { printf("%u", value); }
    void print(long value) { printf("%ld", value); }
    void print(unsigned long value) { printf("%lu", value); }
    void print(unsigned long value, int base) { 
        if (base == 16) printf("%lx", value);
        else printf("%lu", value);
    }
    void println(const char* str) { printf("%s\n", str); }
    void println(const std::string& str) { printf("%s\n", str.c_str()); }
    void println(int value) { printf("%d\n", value); }
    void println(unsigned int value) { printf("%u\n", value); }
    void println(long value) { printf("%ld\n", value); }
    void println(unsigned long value) { printf("%lu\n", value); }
};

// Arduino String compatibility class
class String {
private:
    std::string data;
public:
    String() {}
    String(const char* str) : data(str ? str : "") {}
    String(const char* str, size_t len) : data(str ? std::string(str, len) : "") {}
    String(const std::string& str) : data(str) {}
    String(int value) : data(std::to_string(value)) {}
    String(unsigned int value) : data(std::to_string(value)) {}
    String(long value) : data(std::to_string(value)) {}
    String(unsigned long value) : data(std::to_string(value)) {}
    String(float value) : data(std::to_string(value)) {}
    String(double value) : data(std::to_string(value)) {}
    
    const char* c_str() const { return data.c_str(); }
    size_t length() const { return data.length(); }
    bool empty() const { return data.empty(); }  // Add missing empty() method
    bool operator==(const String& other) const { return data == other.data; }
    bool operator!=(const String& other) const { return data != other.data; }
    
    String operator+(const String& other) const { return String(data + other.data); }
    String operator+(const char* str) const { return String(data + (str ? str : "")); }
    String& operator+=(const String& other) { data += other.data; return *this; }
    String& operator+=(const char* str) { if (str) data += str; return *this; }
    
    // Arduino String compatibility methods
    String substring(size_t start, size_t end = std::string::npos) const {
        if (end == std::string::npos) return String(data.substr(start));
        return String(data.substr(start, end - start));
    }
    
    int lastIndexOf(const char* str) const {
        size_t pos = data.rfind(str);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }
    
    int lastIndexOf(char ch) const {
        size_t pos = data.rfind(ch);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }
    
    int indexOf(char c) const {
        size_t pos = data.find(c);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }
    
    int indexOf(const char* str) const {
        if (!str) return -1;
        size_t pos = data.find(str);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }
    
    int indexOf(char c, size_t fromIndex) const {
        size_t pos = data.find(c, fromIndex);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }
    
    bool endsWith(const char* suffix) const {
        if (!suffix) return false;
        size_t suffixLen = strlen(suffix);
        if (suffixLen > data.length()) return false;
        return data.compare(data.length() - suffixLen, suffixLen, suffix) == 0;
    }
    
    bool startsWith(const char* prefix) const {
        if (!prefix) return false;
        return data.compare(0, strlen(prefix), prefix) == 0;
    }
    
    void trim() {
        // Remove leading whitespace
        data.erase(data.begin(), std::find_if(data.begin(), data.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        // Remove trailing whitespace
        data.erase(std::find_if(data.rbegin(), data.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), data.end());
    }
    
    void reserve(size_t size) {
        data.reserve(size);
    }
    
    char charAt(size_t index) const {
        return (index < data.length()) ? data[index] : '\0';
    }
    
    int toInt() const {
        // Use C-style conversion instead of exceptions
        char* endptr;
        long result = strtol(data.c_str(), &endptr, 10);
        
        // Check if conversion was successful and result fits in int
        if (endptr == data.c_str() || *endptr != '\0') {
            return 0; // Failed to convert
        }
        
        // Check for overflow
        if (result > INT_MAX || result < INT_MIN) {
            return 0;
        }
        
        return static_cast<int>(result);
    }
    
    bool isEmpty() const {
        return data.empty();
    }
    
    operator const char*() const { return data.c_str(); }
    operator std::string() const { return data; }
};

// Global operator for const char* + String
inline String operator+(const char* lhs, const String& rhs) {
    return String(lhs) + rhs;
}

// Arduino Preferences compatibility class
class Preferences {
private:
    nvs_handle_t handle;
    bool opened;
public:
    Preferences() : handle(0), opened(false) {}
    ~Preferences() { if (opened) end(); }
    
    bool begin(const char* name, bool readOnly = false) {
        esp_err_t err = nvs_open(name, readOnly ? NVS_READONLY : NVS_READWRITE, &handle);
        opened = (err == ESP_OK);
        return opened;
    }
    
    void end() {
        if (opened) {
            nvs_close(handle);
            opened = false;
        }
    }
    
    bool putString(const char* key, const std::string& value) {
        if (!opened) return false;
        esp_err_t err = nvs_set_str(handle, key, value.c_str());
        if (err == ESP_OK) nvs_commit(handle);
        return err == ESP_OK;
    }
    
    std::string getString(const char* key, const std::string& defaultValue = "") {
        if (!opened) return defaultValue;
        size_t required_size = 0;
        esp_err_t err = nvs_get_str(handle, key, nullptr, &required_size);
        if (err != ESP_OK) return defaultValue;
        
        char* buffer = (char*)malloc(required_size);
        if (!buffer) return defaultValue;
        
        err = nvs_get_str(handle, key, buffer, &required_size);
        std::string result = (err == ESP_OK) ? std::string(buffer) : defaultValue;
        free(buffer);
        return result;
    }
    
    bool putBool(const char* key, bool value) {
        if (!opened) return false;
        esp_err_t err = nvs_set_u8(handle, key, value ? 1 : 0);
        if (err == ESP_OK) nvs_commit(handle);
        return err == ESP_OK;
    }
    
    bool getBool(const char* key, bool defaultValue = false) {
        if (!opened) return defaultValue;
        uint8_t value;
        esp_err_t err = nvs_get_u8(handle, key, &value);
        return (err == ESP_OK) ? (value != 0) : defaultValue;
    }
    
    bool putInt(const char* key, int value) {
        if (!opened) return false;
        esp_err_t err = nvs_set_i32(handle, key, value);
        if (err == ESP_OK) nvs_commit(handle);
        return err == ESP_OK;
    }
    
    int getInt(const char* key, int defaultValue = 0) {
        if (!opened) return defaultValue;
        int32_t value;
        esp_err_t err = nvs_get_i32(handle, key, &value);
        return (err == ESP_OK) ? value : defaultValue;
    }
};

// Simple SD card compatibility class (placeholder)
class SD_Class {
public:
    bool begin(int pin = -1) { return false; } // Not implemented
    bool exists(const char* path) { return false; }
};

// Simple ESP compatibility class (placeholder) 
class ESP_Class {
public:
    void restart() { esp_restart(); }
    uint32_t getFreeHeap() { return esp_get_free_heap_size(); }
};

extern SerialClass Serial;
extern SD_Class SD;
extern ESP_Class ESP;
