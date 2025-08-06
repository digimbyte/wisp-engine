// Wisp Engine - Main Header
// Organized micro-folder architecture for ESP32-C6 embedded gaming
#pragma once

// Common engine includes and definitions
#include "engine_common.h"

// Clean namespace definitions first
#include "namespaces.h"

// Core engine systems
#include "core/engine.h"
#include "core/config.h"
#include "core/timing.h"
#include "core/resource_manager.h"

// Graphics subsystem
#include "graphics/engine.h"
#include "graphics/palette_system.h"
#include "graphics/sprite_system.h"
#include "graphics/renderer.h"
#include "graphics/framebuffer.h"
#include "graphics/lut_system.h"
#include "graphics/sprite_layers.h"
#include "graphics/particles.h"

// Audio subsystem
#include "audio/audio_engine.h"

// Physics and math
#include "physics/engine.h"

// Entity management
#include "entities/system.h"

// Database and persistence
#include "database/partitioned_system.h"
#include "database/save_system.h"

// Application framework
#include "app/interface.h"
#include "app/config.h"
#include "app/loader.h"
#include "app/loop.h"
#include "app/loop_manager.h"
#include "app/curated_api.h"

// Namespace integration bridge
#include "namespace_integration.h"

// Unified Engine API
#include "wisp_engine_api.h"
#include "app/loader.h"
#include "app/loop.h"
#include "app/loop_manager.h"
#include "app/curated_api.h"

// Namespace integration bridge
#include "namespace_integration.h"

// Convenience aliases for common types (avoiding conflicts with unified API)
namespace WispEngine {
    using Vec2 = WispVec2;
    using PaletteColor = uint8_t;  // Palette index
    using SpriteID = uint16_t;
    using EntityID = uint16_t;
    using DatabaseKey = uint32_t;
}

// Performance monitoring
#ifdef WISP_DEBUG
    #define WISP_PROFILE(name) DebugProfiler profile(name)
#else
    #define WISP_PROFILE(name)
#endif
