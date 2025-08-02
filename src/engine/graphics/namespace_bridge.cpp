// engine/graphics/namespace_bridge.cpp - Graphics namespace implementation
#include "../namespaces.h"
#include "../../../src/engine/graphics/engine.h" // Existing GraphicsEngine
#include "../../../src/engine/graphics/optimized_engine.h" // OptimizedGraphicsEngine

namespace WispEngine {
namespace Graphics {

// We'll use the existing GraphicsEngine as the primary implementation
class Engine {
private:
    static GraphicsEngine* instance;
    
public:
    static bool initialize() {
        if (!instance) {
            instance = new GraphicsEngine();
            return instance->initialize();
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
            return Engine::getInstance()->drawSprite(spriteId, x, y, depth);
        }
        return false;
    }
    
    static void clear(uint8_t paletteIndex = 0) {
        if (Engine::getInstance()) {
            Engine::getInstance()->clearScreen(paletteIndex);
        }
    }
    
    static void present() {
        if (Engine::getInstance()) {
            Engine::getInstance()->render();
        }
    }
};

} // namespace Graphics
} // namespace WispEngine
