// minimal_api_wrapper.h - Minimal API wrapper for ESP32-C6
// Provides WispCuratedAPI-compatible interface using minimal engine
#pragma once

#ifdef CONFIG_IDF_TARGET_ESP32C6

#include "minimal_engine.h"
#include "../app/curated_api.h"  // For WispColor definition
#include <string>

namespace WispEngine::Minimal {

// Lightweight API wrapper that provides WispCuratedAPI interface
// but uses minimal engine underneath
class APIWrapper {
private:
    Engine* engine;
    
public:
    APIWrapper(Engine* eng) : engine(eng) {}
    
    // Sprite management - uses actual minimal engine
    uint16_t loadSprite(const std::string& path) {
        printf("loadSprite: %s (minimal)\n", path.c_str());
        
        // Convert path to sprite ID (simple hash for demo)
        uint16_t spriteId = 1;
        if (path.find("player") != std::string::npos) spriteId = 1;
        else if (path.find("enemy") != std::string::npos) spriteId = 2;
        else if (path.find("item") != std::string::npos) spriteId = 3;
        else spriteId = (path.length() % 10) + 1;
        
        // Load into sprite slot system
        uint8_t slotId = engine->graphics().loadSprite(spriteId);
        return spriteId;  // Return sprite ID for API compatibility
    }
    
    void unloadSprite(uint16_t spriteId) {
        printf("unloadSprite: %d (minimal)\n", spriteId);
        // Note: Slot system handles unloading via LRU
    }
    
    bool validateResourceHandle(uint16_t handle) {
        return handle > 0 && handle < 100;  // Valid sprite ID range
    }
    
    // Drawing functions - use actual minimal engine
    void drawSprite(uint16_t spriteId, float x, float y, uint8_t depth) {
        // Use the actual minimal engine sprite system
        engine->graphics().drawSprite(spriteId, (int)x, (int)y, 1);
    }
    
    void drawRect(float x, float y, float width, float height, WispColor color, uint8_t depth) {
        // Use minimal engine drawing
        uint16_t rgb565 = ((color.r >> 3) << 11) | ((color.g >> 2) << 5) | (color.b >> 3);
        engine->graphics().fillRect((int)x, (int)y, (int)width, (int)height, rgb565);
    }
    
    void drawText(const std::string& text, float x, float y, WispColor color, uint8_t depth) {
        // Use minimal engine text rendering
        uint16_t rgb565 = ((color.r >> 3) << 11) | ((color.g >> 2) << 5) | (color.b >> 3);
        extern LGFX display;
        display.setTextColor(rgb565);
        display.setCursor((int)x, (int)y);
        display.print(text.c_str());
    }
    
    // System functions
    void print(const std::string& message) {
        Serial.println(message.c_str());
    }
    
    void setAppPermissions(bool graphics, bool audio, bool network, bool filesystem) {
        printf("setAppPermissions: g=%d a=%d n=%d f=%d\n", graphics, audio, network, filesystem);
    }
    
    void requestAppLaunch(const char* appPath) {
        printf("requestAppLaunch: %s\n", appPath);
    }
};

}  // namespace WispEngine::Minimal

#endif  // CONFIG_IDF_TARGET_ESP32C6
