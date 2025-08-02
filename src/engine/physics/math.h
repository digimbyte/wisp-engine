// engine/wisp_math.h - ESP32-C6/S3 Math Library using ESP-IDF
// Optimized 2D vector math for ESP32 microcontroller performance
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include <math.h>

// Simple 2D vector for position and calculations
struct WispVec2 {
    float x, y;
    
    WispVec2() : x(0), y(0) {}
    WispVec2(float _x, float _y) : x(_x), y(_y) {}
    
    // Vector operations
    WispVec2 operator+(const WispVec2& other) const {
        return WispVec2(x + other.x, y + other.y);
    }
    
    WispVec2 operator-(const WispVec2& other) const {
        return WispVec2(x - other.x, y - other.y);
    }
    
    WispVec2 operator*(float scalar) const {
        return WispVec2(x * scalar, y * scalar);
    }
    
    WispVec2& operator+=(const WispVec2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    
    WispVec2& operator-=(const WispVec2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    
    WispVec2& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }
    
    // Vector math
    float length() const {
        return sqrt(x * x + y * y);
    }
    
    float lengthSquared() const {
        return x * x + y * y;
    }
    
    WispVec2 normalized() const {
        float len = length();
        if (len > 0) {
            return WispVec2(x / len, y / len);
        }
        return WispVec2(0, 0);
    }
    
    void normalize() {
        float len = length();
        if (len > 0) {
            x /= len;
            y /= len;
        }
    }
    
    float dot(const WispVec2& other) const {
        return x * other.x + y * other.y;
    }
    
    float distance(const WispVec2& other) const {
        return (*this - other).length();
    }
    
    static WispVec2 lerp(const WispVec2& a, const WispVec2& b, float t) {
        return WispVec2(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t
        );
    }
};

// Simple rectangle for bounds checking
struct WispRect {
    float x, y, width, height;
    
    WispRect() : x(0), y(0), width(0), height(0) {}
    WispRect(float _x, float _y, float _w, float _h) : x(_x), y(_y), width(_w), height(_h) {}
    
    // Rectangle operations
    bool contains(float px, float py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
    
    bool contains(const WispVec2& point) const {
        return contains(point.x, point.y);
    }
    
    bool intersects(const WispRect& other) const {
        return !(x >= other.x + other.width || 
                 x + width <= other.x || 
                 y >= other.y + other.height || 
                 y + height <= other.y);
    }
    
    WispVec2 center() const {
        return WispVec2(x + width * 0.5f, y + height * 0.5f);
    }
    
    float left() const { return x; }
    float right() const { return x + width; }
    float top() const { return y; }
    float bottom() const { return y + height; }
};

// Color utilities
struct WispColor {
    uint8_t r, g, b, a;
    
    WispColor() : r(0), g(0), b(0), a(255) {}
    WispColor(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255) : r(_r), g(_g), b(_b), a(_a) {}
    
    // Convert to RGB565
    uint16_t toRGB565() const {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
    
    // Create from RGB565
    static WispColor fromRGB565(uint16_t color) {
        uint8_t r = (color >> 11) & 0x1F;
        uint8_t g = (color >> 5) & 0x3F;
        uint8_t b = color & 0x1F;
        
        // Scale to 8-bit
        r = (r * 255) / 31;
        g = (g * 255) / 63;
        b = (b * 255) / 31;
        
        return WispColor(r, g, b);
    }
    
    // Color operations
    WispColor lerp(const WispColor& other, float t) const {
        return WispColor(
            (uint8_t)(r + (other.r - r) * t),
            (uint8_t)(g + (other.g - g) * t),
            (uint8_t)(b + (other.b - b) * t),
            (uint8_t)(a + (other.a - a) * t)
        );
    }
    
    WispColor multiply(float factor) const {
        return WispColor(
            (uint8_t)constrain(r * factor, 0, 255),
            (uint8_t)constrain(g * factor, 0, 255),
            (uint8_t)constrain(b * factor, 0, 255),
            a
        );
    }
    
    // Common colors
    static const WispColor WHITE;
    static const WispColor BLACK;
    static const WispColor RED;
    static const WispColor GREEN;
    static const WispColor BLUE;
    static const WispColor YELLOW;
    static const WispColor MAGENTA;
    static const WispColor CYAN;
    static const WispColor TRANSPARENT;
};

// Define common colors
const WispColor WispColor::WHITE(255, 255, 255);
const WispColor WispColor::BLACK(0, 0, 0);
const WispColor WispColor::RED(255, 0, 0);
const WispColor WispColor::GREEN(0, 255, 0);
const WispColor WispColor::BLUE(0, 0, 255);
const WispColor WispColor::YELLOW(255, 255, 0);
const WispColor WispColor::MAGENTA(255, 0, 255);
const WispColor WispColor::CYAN(0, 255, 255);
const WispColor WispColor::TRANSPARENT(0, 0, 0, 0);

// Math utilities
namespace WispMath {
    // Convert degrees to radians
    inline float degToRad(float degrees) {
        return degrees * PI / 180.0f;
    }
    
    // Convert radians to degrees
    inline float radToDeg(float radians) {
        return radians * 180.0f / PI;
    }
    
    // Linear interpolation
    inline float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }
    
    // Clamp value between min and max
    inline float clamp(float value, float minVal, float maxVal) {
        if (value < minVal) return minVal;
        if (value > maxVal) return maxVal;
        return value;
    }
    
    // Wrap value between 0 and max
    inline float wrap(float value, float max) {
        while (value < 0) value += max;
        while (value >= max) value -= max;
        return value;
    }
    
    // Simple easing functions
    inline float easeInQuad(float t) {
        return t * t;
    }
    
    inline float easeOutQuad(float t) {
        return 1 - (1 - t) * (1 - t);
    }
    
    inline float easeInOutQuad(float t) {
        if (t < 0.5f) {
            return 2 * t * t;
        } else {
            return 1 - 2 * (1 - t) * (1 - t);
        }
    }
};
