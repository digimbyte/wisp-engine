// diagnostics.h - ESP32-C6/S3 Diagnostics System using ESP-IDF
// Native ESP32 memory and performance monitoring with heap analysis
#pragma once
#include "../system/esp32_common.h"  // Pure ESP-IDF native headers
#include <LovyanGFX.hpp>
#include <esp_heap_caps.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace Diagnostics {

constexpr int DIAG_PIN = 36; // GPIO pin to enable overlay mode

static uint32_t lastFpsCheck = 0;
static uint16_t frameCounter = 0;
static uint8_t currentFps = 0;

inline void init() {
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << DIAG_PIN);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf);
}

inline bool diagnosticsEnabled() {
  return gpio_get_level((gpio_num_t)DIAG_PIN) == 1;
}

inline void updateFPS() {
  frameCounter++;
  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
  if (now - lastFpsCheck >= 1000) {
    currentFps = frameCounter;
    frameCounter = 0;
    lastFpsCheck = now;
  }
}

inline uint8_t getFPS() {
  return currentFps;
}

inline size_t getUsedHeap() {
  return heap_caps_get_total_size(MALLOC_CAP_DEFAULT) - heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
}

inline size_t getTotalHeap() {
  return heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
}

inline uint8_t getHeapUsagePercent() {
  size_t used = getUsedHeap();
  size_t total = getTotalHeap();
  return (uint8_t)((used * 100) / total);
}

inline void renderOverlay(LGFX& display) {
  if (!diagnosticsEnabled()) return;

  updateFPS();

  // Draw FPS in top-left corner (accommodate 2-digit width)
  display.fillRect(0, 0, 16, 8, TFT_BLACK); // clear 2-char area
  display.setTextColor(TFT_WHITE, TFT_BLACK);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print(currentFps);

  // Draw RAM usage bar (1px wide, full height)
  uint8_t usage = getHeapUsagePercent();
  uint16_t color = TFT_GREEN;
  if (usage > 90) color = TFT_RED;
  else if (usage > 75) color = TFT_ORANGE;
  else if (usage > 50) color = TFT_YELLOW;

  int displayHeight = display.height();
  int barHeight = (usage * displayHeight) / 100;
  int y = displayHeight - barHeight;
  display.fillRect(0, y, 1, barHeight, color);
  display.fillRect(0, 0, 1, y, TFT_BLACK);
}

} // namespace Diagnostics
