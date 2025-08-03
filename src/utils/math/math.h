// utils/math/math.h - Unified Math Library for ESP32-C6/S3 Wisp Engine
// Optimized 2D vector math, geometry, and color utilities for ESP32 performance
#pragma once
#include "../../system/esp32_common.h"
#include <math.h>

// Simple 2D vector for position and calculations
struct Vec2 {
    float x, y;
    
    Vec2() : x(0), y(0) {}
    Vec2(float _x, float _y) : x(_x), y(_y) {}
    
    // Vector operations
    Vec2 operator+(const Vec2& other) const {
        return Vec2(x + other.x, y + other.y);
    }
    
    Vec2 operator-(const Vec2& other) const {
        return Vec2(x - other.x, y - other.y);
    }
    
    Vec2 operator*(float scalar) const {
        return Vec2(x * scalar, y * scalar);
    }
    
    Vec2& operator+=(const Vec2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    
    Vec2& operator-=(const Vec2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    
    Vec2& operator*=(float scalar) {
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
    
    Vec2 normalized() const {
        float len = length();
        if (len > 0) {
            return Vec2(x / len, y / len);
        }
        return Vec2(0, 0);
    }
    
    void normalize() {
        float len = length();
        if (len > 0) {
            x /= len;
            y /= len;
        }
    }
    
    float dot(const Vec2& other) const {
        return x * other.x + y * other.y;
    }
    
    float distance(const Vec2& other) const {
        return (*this - other).length();
    }
    
    static Vec2 lerp(const Vec2& a, const Vec2& b, float t) {
        return Vec2(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t
        );
    }
};

// Simple rectangle for bounds checking
struct Rect {
    float x, y, width, height;
    
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(float _x, float _y, float _w, float _h) : x(_x), y(_y), width(_w), height(_h) {}
    
    // Rectangle operations
    bool contains(float px, float py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
    
    bool contains(const Vec2& point) const {
        return contains(point.x, point.y);
    }
    
    bool intersects(const Rect& other) const {
        return !(x >= other.x + other.width || 
                 x + width <= other.x || 
                 y >= other.y + other.height || 
                 y + height <= other.y);
    }
    
    Vec2 center() const {
        return Vec2(x + width * 0.5f, y + height * 0.5f);
    }
    
    float left() const { return x; }
    float right() const { return x + width; }
    float top() const { return y; }
    float bottom() const { return y + height; }
};

// Color utilities
struct Color {
    uint8_t r, g, b, a;
    
    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255) : r(_r), g(_g), b(_b), a(_a) {}
    
    // Convert to RGB565
    uint16_t toRGB565() const {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
    
    // Create from RGB565
    static Color fromRGB565(uint16_t color) {
        uint8_t r = (color >> 11) & 0x1F;
        uint8_t g = (color >> 5) & 0x3F;
        uint8_t b = color & 0x1F;
        
        // Scale to 8-bit
        r = (r * 255) / 31;
        g = (g * 255) / 63;
        b = (b * 255) / 31;
        
        return Color(r, g, b);
    }
    
    // Color operations
    Color lerp(const Color& other, float t) const {
        return Color(
            (uint8_t)(r + (other.r - r) * t),
            (uint8_t)(g + (other.g - g) * t),
            (uint8_t)(b + (other.b - b) * t),
            (uint8_t)(a + (other.a - a) * t)
        );
    }
    
    Color multiply(float factor) const {
        return Color(
            (uint8_t)std::min(255.0f, std::max(0.0f, r * factor)),
            (uint8_t)std::min(255.0f, std::max(0.0f, g * factor)),
            (uint8_t)std::min(255.0f, std::max(0.0f, b * factor)),
            a
        );
    }
    
    // Common colors
    static const Color WHITE;
    static const Color BLACK;
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
    static const Color YELLOW;
    static const Color MAGENTA;
    static const Color CYAN;
    static const Color TRANSPARENT;
};

// Define common colors
inline const Color Color::WHITE(255, 255, 255);
inline const Color Color::BLACK(0, 0, 0);
inline const Color Color::RED(255, 0, 0);
inline const Color Color::GREEN(0, 255, 0);
inline const Color Color::BLUE(0, 0, 255);
inline const Color Color::YELLOW(255, 255, 0);
inline const Color Color::MAGENTA(255, 0, 255);
inline const Color Color::CYAN(0, 255, 255);
inline const Color Color::TRANSPARENT(0, 0, 0, 0);

// Math utilities namespace
namespace Math {
    // Convert degrees to radians
    inline float degToRad(float degrees) {
        return degrees * M_PI / 180.0f;
    }
    
    // Convert radians to degrees
    inline float radToDeg(float radians) {
        return radians * 180.0f / M_PI;
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
    
    // Clamp integer value between min and max
    inline int clamp_int(int value, int min_val, int max_val) { 
        return (value < min_val) ? min_val : (value > max_val) ? max_val : value; 
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
}

// Type aliases for backward compatibility
using WispVec2 = Vec2;
using WispRect = Rect;
using WispColor = Color;
namespace WispMath = Math;
