// engine/core/timing.cpp - Implementation for namespace bridge
#include "timing.h"
#include "../../../src/core/timekeeper.h" // Existing implementation

namespace WispEngine {
namespace Core {
namespace Timing {

void init() {
    Time::init();
}

bool frameReady() {
    return Time::frameReady();
}

void tick() {
    Time::tick();
}

uint32_t getFrameTime() {
    return Time::getDelta();
}

float getFPS() {
    return Time::getCurrentFPS();
}

} // namespace Timing
} // namespace Core
} // namespace WispEngine
