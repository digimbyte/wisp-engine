// engine/core/timing.cpp - Implementation for TimingSystem class
#include "timing.h"
#include "../../../src/core/timekeeper.h" // Existing implementation

// TimingSystem class implementation (global scope, not namespaced)

void TimingSystem::initialize() {
    Time::init();
}

void TimingSystem::update() {
    if (Time::frameReady()) {
        currentTime = Time::getNow();
        frameTime = currentTime - lastTick;
        lastTick = currentTime;
        deltaTime = static_cast<float>(frameTime) / 1000.0f;
        totalFrames++;
        
        updateFrameTimeHistory();
        calculateAverageFrameTime();
    }
}

bool TimingSystem::isFrameReady() {
    return Time::frameReady();
}

void TimingSystem::beginFrame() {
    currentTime = Time::getNow();
}

void TimingSystem::endFrame() {
    Time::frameEnd();
}

void TimingSystem::setTargetFrameRate(uint8_t fps) {
    targetFrameTime = 1000000 / fps; // microseconds
}

void TimingSystem::setFrameRateControl(bool enabled) {
    frameRateControlEnabled = enabled;
}

float TimingSystem::getCurrentFPS() const {
    return frameTime > 0 ? 1000.0f / frameTime : 0.0f;
}

float TimingSystem::getAverageFPS() const {
    return averageFrameTime > 0 ? 1000.0f / averageFrameTime : 0.0f;
}

float TimingSystem::getFrameTimeVariance() const {
    // Calculate variance of frame times
    float variance = 0.0f;
    float avg = static_cast<float>(averageFrameTime);
    
    for (int i = 0; i < 16; i++) {
        float diff = static_cast<float>(frameTimeHistory[i]) - avg;
        variance += diff * diff;
    }
    
    return variance / 16.0f;
}

bool TimingSystem::isPerformanceStable() const {
    return getFrameTimeVariance() < (targetFrameTime * 0.1f); // Within 10%
}

void TimingSystem::printTimingStats() const {
    // Print timing statistics - implementation depends on debug system
}

void TimingSystem::resetStats() {
    totalFrames = 0;
    averageFrameTime = targetFrameTime;
    historyIndex = 0;
    
    for (int i = 0; i < 16; i++) {
        frameTimeHistory[i] = targetFrameTime;
    }
}

void TimingSystem::updateFrameTimeHistory() {
    frameTimeHistory[historyIndex] = frameTime;
    historyIndex = (historyIndex + 1) % 16;
}

void TimingSystem::calculateAverageFrameTime() {
    uint32_t total = 0;
    for (int i = 0; i < 16; i++) {
        total += frameTimeHistory[i];
    }
    averageFrameTime = total / 16;
}
