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
#include <math.h>

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

// GPIO and peripheral includes
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"

// Common macros
#define delay_ms(ms) vTaskDelay(pdMS_TO_TICKS(ms))
#define micros() (esp_timer_get_time())
#define millis() (esp_timer_get_time() / 1000)
#define get_free_heap() esp_get_free_heap_size()

// Arduino-like compatibility macros (for easier porting)
#define PROGMEM
#define F(x) (x)
#define PI 3.14159265359
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

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
#define ledcSetup(channel, freq, resolution) /* TODO: Implement */
#define ledcAttachPin(pin, channel) /* TODO: Implement */
#define ledcWrite(channel, duty) /* TODO: Implement */
#define ledcWriteTone(channel, freq) /* TODO: Implement */
#define delayMicroseconds(us) esp_rom_delay_us(us)

// Serial compatibility for debugging
class SerialClass {
public:
    void print(const char* str) { printf("%s", str); }
    void print(int value) { printf("%d", value); }
    void print(unsigned int value) { printf("%u", value); }
    void print(long value) { printf("%ld", value); }
    void print(unsigned long value) { printf("%lu", value); }
    void println(const char* str) { printf("%s\n", str); }
    void println(int value) { printf("%d\n", value); }
    void println(unsigned int value) { printf("%u\n", value); }
    void println(long value) { printf("%ld\n", value); }
    void println(unsigned long value) { printf("%lu\n", value); }
};

extern SerialClass Serial;