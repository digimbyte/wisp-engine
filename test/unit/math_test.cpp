// test/math_test.cpp - Quick test to validate unified math library
#include "../src/utils/math/math.h"

// Simple test to verify math structures compile correctly
void testMathLibrary() {
    // Test Vec2
    Vec2 a(1.0f, 2.0f);
    Vec2 b(3.0f, 4.0f);
    Vec2 c = a + b;
    float distance = a.distance(b);
    
    // Test Rect
    Rect rect(0, 0, 100, 50);
    bool contains = rect.contains(25.0f, 25.0f);
    Vec2 center = rect.center();
    
    // Test Color
    Color red(255, 0, 0);
    uint16_t rgb565 = red.toRGB565();
    Color blended = red.lerp(Color::BLUE, 0.5f);
    
    // Test Math utilities
    float lerped = Math::lerp(0.0f, 100.0f, 0.5f);
    float clamped = Math::clamp(150.0f, 0.0f, 100.0f);
    float eased = Math::easeInQuad(0.5f);
    
    // Test backward compatibility aliases
    WispVec2 oldVec(1.0f, 2.0f);
    WispColor oldColor(255, 128, 0);
    float oldLerp = WispMath::lerp(0.0f, 1.0f, 0.3f);
}
