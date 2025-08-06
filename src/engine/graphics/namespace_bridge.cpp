// engine/graphics/namespace_bridge.cpp - Graphics namespace implementation
#include "../engine_common.h"
#include "engine.h" // Existing GraphicsEngine
#include "optimized_engine.h" // OptimizedGraphicsEngine

namespace WispEngine {
namespace Graphics {

// We'll use the existing GraphicsEngine as the primary implementation
class Engine {
private:
    static GraphicsEngine* instance;
    
public:
    static bool initialize() {
        if (!instance) {
            // TODO: Large GraphicsEngine disabled for ESP32-C6 memory constraints
            // Uncomment when memory optimization is complete
            #if !defined(PLATFORM_C6) || defined(WISP_MEMORY_PROFILE) && WISP_MEMORY_PROFILE >= 1
            instance = new GraphicsEngine();
            // GraphicsEngine likely has init() method, not initialize()
            return true; // Placeholder - implement actual initialization
            #else
            // Return false on ESP32-C6 with minimal memory profile - no graphics engine
            return false;
            #endif
        }
        return true;
    }
    
    static void shutdown() {
        if (instance) {
            delete instance;
            instance = nullptr;
        }
    }
    
    static GraphicsEngine* getInstance() {
        return instance;
    }
    
    static bool isInitialized() {
        return instance != nullptr;
    }
};

// Static member definition
GraphicsEngine* Engine::instance = nullptr;

// Renderer class bridge
class Renderer {
public:
    static bool drawSprite(uint16_t spriteId, int16_t x, int16_t y, uint8_t depth = 0) {
        if (Engine::getInstance()) {
            // GraphicsEngine likely has different method signature
            // Engine::getInstance()->drawSprite(spriteId, x, y, depth);
            return true; // Placeholder
        }
        return false;
    }
    
    static void clear(uint8_t paletteIndex = 0) {
        if (Engine::getInstance()) {
            // Engine::getInstance()->clearScreen(paletteIndex);
            // Placeholder - implement actual clear method
        }
    }
    
    static void present() {
        if (Engine::getInstance()) {
            // Engine::getInstance()->render();
            // Placeholder - implement actual render method
        }
    }
};

} // namespace Graphics
} // namespace WispEngine
