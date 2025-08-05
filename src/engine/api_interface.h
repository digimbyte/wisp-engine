// api_interface.h - Common interface for API wrappers
#pragma once

#include <string>
#include <cstdint>

// Forward declare WispColor
struct WispColor {
    uint8_t r, g, b, a;
    WispColor(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0, uint8_t alpha = 255) 
        : r(red), g(green), b(blue), a(alpha) {}
};

// Common interface that both WispCuratedAPI and minimal wrapper can implement
class IWispAPI {
public:
    virtual ~IWispAPI() = default;
    
    // Sprite management
    virtual uint16_t loadSprite(const std::string& path) = 0;
    virtual void unloadSprite(uint16_t spriteId) = 0;
    virtual bool validateResourceHandle(uint16_t handle) = 0;
    
    // Drawing functions
    virtual void drawSprite(uint16_t spriteId, float x, float y, uint8_t depth) = 0;
    virtual void drawRect(float x, float y, float width, float height, WispColor color, uint8_t depth) = 0;
    virtual void drawText(const std::string& text, float x, float y, WispColor color, uint8_t depth) = 0;
    
    // System functions
    virtual void print(const std::string& message) = 0;
    virtual void setAppPermissions(bool graphics, bool audio, bool network, bool filesystem) = 0;
    virtual void requestAppLaunch(const char* appPath) = 0;
};
