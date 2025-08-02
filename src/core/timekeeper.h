// timekeeper.h
// timekeeper.h - ESP32-C6/S3 Timekeeper using ESP-IDF
// High-precision timing system using ESP32 native esp_timer for deterministic performance
#pragma once
#include "../system/esp32_common.h"  // Pure ESP-IDF native headers
#include "definitions.h" // Assumes SYSTEM_FPS is defined here
#include "frame_rate_manager.h"

namespace Time {

// --- Time State ---
static uint32_t lastTick = 0;
static uint32_t now = 0;
static float delta = 0.0f;
static uint32_t totalTime = 0;

// --- Frame Rate Management ---
static FrameRateManager* frameRateManager = nullptr;
static uint8_t targetFPS = SYSTEM_FPS; // Default to system FPS (fallback)
static uint32_t frameDuration = 1000 / SYSTEM_FPS;
static uint32_t nextFrameTarget = 0;

// --- Timer Queue ---
constexpr int MAX_TIMERS = 8;
typedef void (*TimerCallback)();

struct TimerEntry {
  uint32_t targetTime;
  TimerCallback callback;
  bool active;
};

static TimerEntry timerQueue[MAX_TIMERS];

// --- Init ---
inline void init(uint8_t fps = SYSTEM_FPS) {
  lastTick = millis();
  now = lastTick;
  delta = 0.0f;
  totalTime = 0;
  targetFPS = fps;
  frameDuration = 1000 / fps;
  nextFrameTarget = now + frameDuration;
  for (int i = 0; i < MAX_TIMERS; ++i) timerQueue[i].active = false;
}

// --- Init with Frame Rate Manager ---
inline void initWithFrameRateManager(FrameRateManager* manager) {
  frameRateManager = manager;
  lastTick = millis();
  now = lastTick;
  delta = 0.0f;
  totalTime = 0;
  
  if (frameRateManager) {
    targetFPS = frameRateManager->getTargetFPS();
    frameDuration = frameRateManager->getTargetFrameTime() / 1000; // Convert μs to ms
  } else {
    targetFPS = SYSTEM_FPS;
    frameDuration = 1000 / SYSTEM_FPS;
  }
  
  nextFrameTarget = now + frameDuration;
  for (int i = 0; i < MAX_TIMERS; ++i) timerQueue[i].active = false;
}

// --- Frame limiter ---
inline bool frameReady() {
  now = millis();
  
  // Use frame rate manager if available
  if (frameRateManager) {
    bool ready = frameRateManager->isFrameReady();
    if (ready) {
      frameRateManager->frameStart();
      delta = (now - lastTick);
      totalTime += delta;
      lastTick = now;
      
      // Update local values from frame rate manager
      targetFPS = frameRateManager->getTargetFPS();
      frameDuration = frameRateManager->getTargetFrameTime() / 1000;
    }
    return ready;
  }
  
  // Fallback to legacy timing
  if (now >= nextFrameTarget) {
    delta = (now - lastTick);
    totalTime += delta;
    lastTick = now;
    nextFrameTarget = now + frameDuration;
    return true;
  }
  return false;
}

// --- Tick logic and scheduler ---
inline void tick() {
  // Process scheduled callbacks
  for (int i = 0; i < MAX_TIMERS; ++i) {
    if (timerQueue[i].active && now >= timerQueue[i].targetTime) {
      if (timerQueue[i].callback) timerQueue[i].callback();
      timerQueue[i].active = false;
    }
  }
}

// --- End of frame processing ---
inline void frameEnd() {
  if (frameRateManager) {
    frameRateManager->frameEnd();
  }
}

// --- Schedule a future callback ---
inline bool schedule(uint32_t delayMs, TimerCallback cb) {
  for (int i = 0; i < MAX_TIMERS; ++i) {
    if (!timerQueue[i].active) {
      timerQueue[i].targetTime = now + delayMs;
      timerQueue[i].callback = cb;
      timerQueue[i].active = true;
      return true;
    }
  }
  return false; // No space
}

// --- Accessors ---
inline float getDelta() { return delta; }
inline uint32_t getNow() { return now; }
inline uint32_t getTotalTime() { return totalTime; }
inline uint8_t getTargetFPS() { return targetFPS; }
inline FrameRateManager* getFrameRateManager() { return frameRateManager; }

// --- Performance monitoring ---
inline uint32_t getCurrentFPS() {
  if (frameRateManager) {
    return frameRateManager->getCurrentFPS();
  }
  return targetFPS; // Fallback
}

inline float getFrameDropPercentage() {
  if (frameRateManager) {
    return frameRateManager->getFrameDropPercentage();
  }
  return 0.0f;
}

inline void printPerformanceReport() {
  if (frameRateManager) {
    frameRateManager->printPerformanceReport();
  } else {
    Serial.println("Frame Rate Manager not available");
  }
}

} // namespace Time
