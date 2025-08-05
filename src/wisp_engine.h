// wisp_engine.h - Central Engine Header with Namespace Organization
// 
// WISP ENGINE ARCHITECTURE - ESP32-C6/S3 Native Implementation
// This header provides the main engine namespace and organized includes
// for all Wisp Engine components using proper C++ namespace structure.
//
#pragma once

// === CORE SYSTEM HEADERS ===
#include "system/esp32_common.h"      // ESP-IDF native platform layer
#include "system/definitions.h"       // Hardware and constant definitions

// === ENGINE NAMESPACE ORGANIZATION ===
namespace WispEngine {

    // === CORE SYSTEMS NAMESPACE ===
    namespace Core {
        // Forward declarations for core systems
        class Debug;
        class Timing;
        class Memory;
        class Config;
    }

    // === GRAPHICS ENGINE NAMESPACE ===
    namespace Graphics {
        // Forward declarations for graphics systems
        class Engine;
        class Renderer;
        class LUTSystem;
        class SpriteManager;
        class DisplayDriver;
    }

    // === AUDIO ENGINE NAMESPACE ===
    namespace Audio {
        // Forward declarations for audio systems
        class Engine;
        class Mixer;
        class Synthesizer;
        class Effects;
    }

    // === INPUT SYSTEM NAMESPACE ===
    namespace Input {
        // Forward declarations for input systems
        class Controller;
        class ButtonManager;
        class TouchManager;
    }

    // === DATABASE SYSTEM NAMESPACE ===
    namespace Database {
        // Forward declarations for database systems
        class System;
        class FileSystem;
        class SaveSystem;
        class PartitionManager;
        
        // Database types from database_system.h
        enum ErrorCode {
            OK = 0,
            INVALID_PARAM = 1,
            NOT_INITIALIZED = 2,
            OUT_OF_MEMORY = 3,
            KEY_NOT_FOUND = 4,
            PARTITION_FULL = 5,
            INVALID_PARTITION = 6,
            CHECKSUM_FAILED = 7,
            STORAGE_FAILURE = 8,
            INVALID_CONFIG = 9,
            BUFFER_OVERFLOW = 10,
            INVALID_KEY = 11,
            ENTRY_TOO_LARGE = 12,
            INDEX_OVERFLOW = 13,
            PARTITION_NOT_FOUND = 14,
            ALREADY_INITIALIZED = 15,
            MEMORY_EXCEEDED = 16
        };
        
        enum PartitionType {
            ROM = 0,
            SAVE = 1,
            BACKUP = 2,
            RUNTIME = 3
        };
    }

    // === APPLICATION SYSTEM NAMESPACE ===
    namespace App {
        // Forward declarations for application systems
        class Loader;
        class Manager;
        class CuratedAPI;
        class LoopManager;
    }

    // === UTILITY SYSTEMS NAMESPACE ===
    namespace Utils {
        // Forward declarations for utility systems
        class Math;
        class Crypto;
        class Compression;
    }

    // === GLOBAL ENGINE INTERFACE ===
    class Engine {
    public:
        // Engine lifecycle
        static bool initialize();
        static void shutdown();
        static bool isInitialized();
        
        // Component access
        static Core::Debug& getDebug();
        static Graphics::Engine& getGraphics();
        static Audio::Engine& getAudio();
        static Input::Controller& getInput();
        static Database::System& getDatabase();
        static App::Manager& getAppManager();
        
        // Engine status
        static uint32_t getFrameCount();
        static float getFrameRate();
        static uint32_t getUptime();
    };
}

// === CONVENIENCE ALIASES ===
// These allow shorter syntax while maintaining namespace organization
namespace Wisp = WispEngine;
using WispErrorCode = WispEngine::Database::ErrorCode;
using WispPartitionType = WispEngine::Database::PartitionType;

// Legacy compatibility constants
#define WISP_SUCCESS WispEngine::Database::ErrorCode::OK

// === COMPONENT INCLUDES ===
// Include specific components as needed (optional - can be included separately)
#ifdef WISP_INCLUDE_ALL_COMPONENTS
    #include "engine/core/debug.h"
    #include "engine/core/timing.h"
    #include "engine/graphics/engine.h"
    #include "engine/audio/engine.h"
    #include "engine/database/database_system.h"
    #include "engine/app/manager.h"
#endif

// === GLOBAL ENGINE INSTANCE ===
extern WispEngine::Engine* g_WispEngine;
