// Engine Namespace Definitions
// Clean organization with inline bridges to existing implementations
#pragma once

// Include ESP-IDF compatibility layer
#include "../system/esp32_common.h"

// Include the actual working implementations
#include "../core/timekeeper.h"
#include "../system/debug_esp32.h"
#include "graphics/engine.h" 
#include "audio/engine.h"

namespace WispEngine {
    // Forward declarations for clean separation
    class Engine;
    
    namespace Core {
        class Engine;
        class Config;
        class ResourceManager;
        class Timing;
        
        namespace Debug {
            enum DebugMode {
                DEBUG_MODE_DISABLED = 0,
                DEBUG_MODE_ENABLED = 1,
                DEBUG_MODE_VERBOSE = 2
            };
            
            enum SafetyMode {
                SAFETY_DISABLED = 0,
                SAFETY_ENABLED = 1
            };
            
            // Inline bridges to existing DebugSystem
            inline void init(DebugMode mode, SafetyMode safety) {
                bool enableDebug = (mode != DEBUG_MODE_DISABLED);
                bool disableSafety = (safety == SAFETY_DISABLED);
                DebugSystem::init(enableDebug, disableSafety);
            }
            
            inline void info(const char* category, const char* message) {
                DebugSystem::logInfo(category, message);
            }
            
            inline void warning(const char* category, const char* message) {
                DebugSystem::logWarning(category, message);
            }
            
            inline void error(const char* category, const char* message) {
                DebugSystem::logError(category, message);
            }
            
            inline void heartbeat() {
                DebugSystem::heartbeat();
            }
            
            inline void activateEmergencyMode(const String& error) {
                DebugSystem::activateEmergencyMode(error);
            }
            
            inline void shutdown() {
                DebugSystem::shutdown();
            }
        }
        
        namespace Timing {
            // Inline bridges to existing Time:: namespace
            inline void init() {
                Time::init();
            }
            
            inline bool frameReady() {
                return Time::frameReady();
            }
            
            inline void tick() {
                Time::tick();
            }
            
            inline uint32_t getFrameTime() {
                return Time::getDelta();
            }
            
            inline float getFPS() {
                return Time::getCurrentFPS();
            }
        }
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
            return getEngine()->initialize();
        }
        
        inline void cleanup() {
            GraphicsEngine* engine = getEngine();
            if (engine) {
                delete engine;
            }
        }
    }
    
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
                engine->cleanup();
                delete engine;
            }
        }
    }
    
    namespace Database {
        class PartitionedDatabase;
        class SaveSystem;
        
        namespace SaveSystem {
            void setGlobalInstance(SaveSystem* instance);
            SaveSystem* getGlobalInstance();
        }
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
        class LoopManager;
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
            STAGE_AUDIO_UPDATE,
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
