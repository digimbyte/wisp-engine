// Engine Namespace Definitions
// Clean organization with inline bridges to existing implementations
#pragma once

// Include ESP-IDF compatibility layer
#include "../system/esp32_common.h"
#include <string>

// Include the actual working implementations
#include "../core/timekeeper.h"
#include "../system/debug_esp32.h"
#include "graphics/engine.h" 
// #include "audio/engine.h"  // Temporarily disabled due to GPIO conflicts
#include "app/loop_manager.h"

// Forward declarations
class GameLoopManager;

namespace WispEngine {
    // Forward declarations for clean separation
    class Engine;
    
    namespace Core {
        class Engine;
        class Config;
        class ResourceManager;
        class Timing;
        
        class Debug {
        public:
            enum DebugMode {
                DEBUG_MODE_DISABLED = 0,
                DEBUG_MODE_ON = 1,
                DEBUG_MODE_VERBOSE = 2
            };
            
            enum SafetyMode {
                SAFETY_MODE_DISABLED = 0,
                SAFETY_MODE_ENABLED = 1
            };
            
            // Inline bridges to existing DebugSystem
            static void init(DebugMode mode, SafetyMode safety) {
                bool enableDebug = (mode != DEBUG_MODE_DISABLED);
                bool disableSafety = (safety == SAFETY_MODE_DISABLED);
                DebugSystem::init(enableDebug, disableSafety);
            }
            
            static void info(const char* category, const char* message) {
                DebugSystem::logInfo(category, message);
            }
            
            static void warning(const char* category, const char* message) {
                DebugSystem::logWarning(category, message);
            }
            
            static void error(const char* category, const char* message) {
                DebugSystem::logError(category, message);
            }
            
            static void heartbeat() {
                DebugSystem::heartbeat();
            }
            
            static void activateEmergencyMode(const std::string& error) {
                DebugSystem::activateEmergencyMode(error);
            }
            
            static void shutdown() {
                DebugSystem::shutdown();
            }
        };
        
        class Timing {
        public:
            // Inline bridges to existing Time:: namespace
            static void init() {
                Time::init();
            }
            
            static bool frameReady() {
                return Time::frameReady();
            }
            
            static void tick() {
                Time::tick();
            }
            
            static uint32_t getFrameTime() {
                return Time::getDelta();
            }
            
            static float getFPS() {
                return Time::getCurrentFPS();
            }
        };
    }
    
    namespace Graphics {
        class Engine;
        class Renderer;
        class PaletteSystem;
        class SpriteSystem;
        class FrameBuffer;
        class LUTSystem;
        
        // Simple bridge to existing GraphicsEngine
        // For now, we'll just provide basic access
        inline GraphicsEngine* getEngine() {
            static GraphicsEngine* instance = nullptr;
            if (!instance) {
                instance = new GraphicsEngine();
            }
            return instance;
        }
        
        inline bool initialize() {
            getEngine()->init(nullptr, nullptr);
            return true;
        }
        
        inline void cleanup() {
            GraphicsEngine* engine = getEngine();
            if (engine) {
                delete engine;
            }
        }
    }
    
    /*
    namespace Audio {
        class Engine;
        class SynthEngine;
        class SamplePlayer;
        
        // Simple bridge to existing AudioEngine  
        inline AudioEngine* getEngine() {
            static AudioEngine* instance = nullptr;
            if (!instance) {
                instance = new AudioEngine();
            }
            return instance;
        }
        
        inline bool initialize(uint8_t outputs = AUDIO_ALL, uint32_t sampleRate = AUDIO_SAMPLE_RATE) {
            getEngine()->init(outputs, sampleRate);
            return true;
        }
        
        inline void cleanup() {
            AudioEngine* engine = getEngine();
            if (engine) {
                delete engine;
            }
        }
    }
    */
    
    namespace Database {
        class PartitionedDatabase;
        class SaveSystem {
        public:
            static void setGlobalInstance(SaveSystem* instance);
            static SaveSystem* getGlobalInstance();
        };
    }
    
    namespace Physics {
        class Engine;
        struct Vec2;
        struct BoundingBox;
    }
    
    namespace Entities {
        class System;
        using EntityID = uint16_t;
        using ComponentMask = uint32_t;
    }
    
    namespace App {
        class Interface;
        class Loader;
        
        // Bridge class for GameLoopManager
        class LoopManager {
        private:
            GameLoopManager* impl;
        public:
            LoopManager() : impl(nullptr) {}
            
            // Bridge methods to GameLoopManager
            void setImplementation(GameLoopManager* gameLoopImpl) { impl = gameLoopImpl; }
            GameLoopManager* getAppLoop() { return impl; }
            
            // Add other methods as needed to bridge to GameLoopManager
        };
        
        class CuratedAPI;
        
        // App loop stages
        enum Stage {
            STAGE_INPUT_COLLECTION,
            STAGE_HEARTBEAT,
            STAGE_LOGIC_UPDATE,
            STAGE_PHYSICS_PREDICTION,
            STAGE_COLLISION_DETECTION,
            STAGE_PHYSICS_RESOLUTION,
            STAGE_TRIGGER_PROCESSING,
            // STAGE_AUDIO_UPDATE,  // Temporarily disabled due to audio system conflicts
            STAGE_RENDER_PREPARE,
            STAGE_RENDER_EXECUTE,
            STAGE_RENDER_PRESENT,
            STAGE_COUNT
        };
    }
}

// Input system (not engine-specific)
struct InputState {
    bool left, right, up, down;
    bool buttonA, buttonB, buttonC;
    bool select, start;
    int16_t analogX, analogY;
    bool touched;
    uint16_t touchX, touchY;
};

// Menu system (utility)
namespace Menu {
    bool init(void* curatedAPI);
    void activate();
    bool isActive();
    void update(const InputState& input);
    void render();
}

// Type aliases for cleaner code
using AppLoopManager = WispEngine::App::LoopManager;
using CuratedAPI = WispEngine::App::CuratedAPI;
using AppLoader = WispEngine::App::Loader;
