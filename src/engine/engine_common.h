// engine_common.h - Common includes and definitions for Wisp Engine
#pragma once

// === CORE ESP32 SYSTEM ===
#include "system/esp32_common.h"
#include "system/definitions.h"
#include "system/display_driver.h"
#include "system/input_controller.h"

// === STANDARD LIBRARIES ===
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <vector>

// === CORE UTILITIES ===
#include "utils/math/math.h"
#include "core/timekeeper.h"

// === DEBUG SYSTEM ===
#include "system/debug_esp32.h"

// === FORWARD DECLARATIONS ===
namespace WispEngine {
    namespace Core { class Engine; class Debug; class Timing; }
    namespace Graphics { class Engine; class Renderer; class LUTSystem; }
    namespace Audio { class Engine; class Mixer; }
    namespace Database { class Engine; }
    namespace App { class Loader; class Interface; }
}

// === COMMON CONSTANTS ===
#ifndef WISP_ENGINE_VERSION
#define WISP_ENGINE_VERSION_MAJOR 1
#define WISP_ENGINE_VERSION_MINOR 0  
#define WISP_ENGINE_VERSION_PATCH 0
#define WISP_ENGINE_VERSION "1.0.0"
#endif

// === CONVENIENCE MACROS ===
#define WISP_ENGINE_INIT() WispEngine::Engine::initialize()
#define WISP_ENGINE_SHUTDOWN() WispEngine::Engine::shutdown()

#ifdef WISP_DEBUG
    #define WISP_PROFILE(name) DebugProfiler profile(name)
#else
    #define WISP_PROFILE(name)
#endif
