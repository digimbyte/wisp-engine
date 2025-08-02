// wisp_math.h - Math utilities for ESP32-C6/S3
#pragma once
#include <cmath>

// Simple 2D vector struct
struct Vec2 {
    float x, y;
    Vec2(float x = 0, float y = 0) : x(x), y(y) {}
    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
    Vec2 operator*(float scalar) const { return Vec2(x * scalar, y * scalar); }
};

// Math helper functions
inline float lerp(float a, float b, float t) { return a + t * (b - a); }
inline int clamp_int(int value, int min_val, int max_val) { 
    return (value < min_val) ? min_val : (value > max_val) ? max_val : value; 
}
