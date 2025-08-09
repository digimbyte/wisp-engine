// led_controller.h - Generic LED Controller API for Wisp Engine  
// Supports multiple LED types with board-specific implementations
#pragma once

#include "../boards/esp32-c6_config.h"
#include "../boards/esp32-s3_config.h"
#include "esp32_common.h"
#include <cstdint>
#include <vector>

// LED Type Definitions
enum class LEDType {
    NONE = 0,           // No LED
    SIMPLE_GPIO,        // Simple on/off LED connected to GPIO
    PWM_RGB,           // RGB LED via PWM (3 GPIOs)
    WS2812_RGB,        // WS2812/NeoPixel addressable RGB LED
    APA102_RGB         // APA102/DotStar addressable RGB LED
};

// Board-specific LED configuration
#ifdef RGB_LED_TYPE
    #if defined(RGB_LED_PIN) && (RGB_LED_COUNT > 0)
        #define WISP_HAS_LED 1
        // Determine LED type from board config
        #if (RGB_LED_TYPE == "WS2812" || RGB_LED_TYPE == "NEOPIXEL")
            #define WISP_LED_TYPE LEDType::WS2812_RGB
        #elif (RGB_LED_TYPE == "APA102" || RGB_LED_TYPE == "DOTSTAR") 
            #define WISP_LED_TYPE LEDType::APA102_RGB
        #elif (RGB_LED_TYPE == "PWM_RGB")
            #define WISP_LED_TYPE LEDType::PWM_RGB
        #else
            #define WISP_LED_TYPE LEDType::SIMPLE_GPIO
        #endif
    #else
        #define WISP_HAS_LED 0
        #define WISP_LED_TYPE LEDType::NONE
    #endif
#else
    // Check for simple status LED
    #ifdef STATUS_LED_PIN
        #define WISP_HAS_LED 1
        #define WISP_LED_TYPE LEDType::SIMPLE_GPIO
        #define RGB_LED_PIN STATUS_LED_PIN
        #define RGB_LED_COUNT 1
        #define RGB_LED_TYPE "SIMPLE"
    #else
        #define WISP_HAS_LED 0
        #define WISP_LED_TYPE LEDType::NONE
        #define RGB_LED_PIN -1
        #define RGB_LED_COUNT 0
        #define RGB_LED_TYPE "NONE"
    #endif
#endif

// LED Color structure
struct LEDColor {
    uint8_t r, g, b;
    
    LEDColor() : r(0), g(0), b(0) {}
    LEDColor(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
    
    // Common color presets
    static const LEDColor BLACK;
    static const LEDColor WHITE;
    static const LEDColor RED;
    static const LEDColor GREEN;
    static const LEDColor BLUE;
    static const LEDColor YELLOW;
    static const LEDColor CYAN;
    static const LEDColor MAGENTA;
    static const LEDColor ORANGE;
    static const LEDColor PURPLE;
    static const LEDColor PINK;
    
    // Utility functions
    LEDColor scale(float brightness) const {
        return LEDColor(
            (uint8_t)(r * brightness),
            (uint8_t)(g * brightness),
            (uint8_t)(b * brightness)
        );
    }
    
    LEDColor blend(const LEDColor& other, float ratio) const {
        return LEDColor(
            (uint8_t)(r * (1.0f - ratio) + other.r * ratio),
            (uint8_t)(g * (1.0f - ratio) + other.g * ratio),
            (uint8_t)(b * (1.0f - ratio) + other.b * ratio)
        );
    }
    
    bool operator==(const LEDColor& other) const {
        return r == other.r && g == other.g && b == other.b;
    }
    
    bool operator!=(const LEDColor& other) const {
        return !(*this == other);
    }
};

// LED Animation Types
enum LEDAnimationType {
    LED_ANIM_NONE = 0,          // No animation
    LED_ANIM_FADE,              // Fade between colors
    LED_ANIM_PULSE,             // Pulse brightness
    LED_ANIM_BREATHE,           // Breathing effect (smooth pulse)
    LED_ANIM_RAINBOW,           // Rainbow cycle
    LED_ANIM_RAINBOW_CHASE,     // Rainbow chase effect
    LED_ANIM_COLOR_WIPE,        // Color wipe across LEDs
    LED_ANIM_THEATER_CHASE,     // Theater chase effect
    LED_ANIM_FIRE,              // Fire flicker effect
    LED_ANIM_CUSTOM             // Custom user animation
};

// LED Animation Parameters
struct LEDAnimation {
    LEDAnimationType type;
    LEDColor startColor;
    LEDColor endColor;
    uint32_t duration;          // Duration in milliseconds
    uint32_t startTime;         // Start time (internal)
    bool repeat;                // Whether to repeat animation
    float speed;                // Speed multiplier (1.0 = normal)
    bool reverse;               // Whether to reverse on repeat
    
    LEDAnimation() : type(LED_ANIM_NONE), startColor(), endColor(), 
                    duration(1000), startTime(0), repeat(false), 
                    speed(1.0f), reverse(false) {}
};

// LED Implementation Base Class
class LEDImplementation {
public:
    virtual ~LEDImplementation() = default;
    
    // Core interface that all implementations must provide
    virtual bool init(int pin, int count) = 0;
    virtual void shutdown() = 0;
    virtual void setLED(int index, const LEDColor& color) = 0;
    virtual void setAll(const LEDColor& color) = 0;
    virtual void show() = 0;
    virtual void clear() = 0;
    
    // Optional advanced features (default implementations)
    virtual bool supportsRGB() const { return false; }
    virtual bool supportsAnimations() const { return false; }
    virtual bool supportsBrightness() const { return false; }
};

// Forward declarations for specific implementations
class SimpleGPIOLED;
class PWMRGBLed;
class WS2812LED;
class APA102LED;

// LED Controller Class - Generic API
class LEDController {
private:
    static LEDController* instance;
    
    // Hardware configuration
    LEDType ledType;
    int ledPin;
    int ledCount;
    bool initialized;
    
    // LED state
    std::vector<LEDColor> ledColors;
    LEDColor globalColor;
    float globalBrightness;
    bool isDirty;
    
    // Animation state
    LEDAnimation currentAnimation;
    bool animationActive;
    uint32_t lastUpdateTime;
    
    // Implementation handler (board-specific)
    LEDImplementation* implementation;
    
    // Internal methods
    bool initImplementation();
    void shutdownImplementation();
    void updateAnimation();
    LEDColor calculateAnimationColor(int ledIndex, float progress);
    float calculateAnimationProgress();
    
public:
    LEDController();
    ~LEDController();
    
    // Singleton access
    static LEDController& getInstance();
    
    // === INITIALIZATION ===
    bool init();
    void shutdown();
    bool isInitialized() const { return initialized; }
    
    // === BASIC LED CONTROL ===
    
    // Set single LED color
    void setLED(int index, const LEDColor& color);
    void setLED(int index, uint8_t r, uint8_t g, uint8_t b);
    
    // Set all LEDs to same color
    void setAll(const LEDColor& color);
    void setAll(uint8_t r, uint8_t g, uint8_t b);
    
    // Get LED color
    LEDColor getLED(int index) const;
    
    // Clear all LEDs (turn off)
    void clear();
    
    // Update LEDs (send data to hardware)
    void show();
    
    // === BRIGHTNESS CONTROL ===
    
    // Set global brightness (0.0 - 1.0)
    void setBrightness(float brightness);
    float getBrightness() const { return globalBrightness; }
    
    // Fade brightness over time
    void fadeBrightness(float targetBrightness, uint32_t durationMs);
    
    // === COLOR TRANSITIONS ===
    
    // Fade single LED to target color
    void fadeTo(int index, const LEDColor& target, uint32_t durationMs);
    void fadeTo(int index, uint8_t r, uint8_t g, uint8_t b, uint32_t durationMs);
    
    // Fade all LEDs to target color
    void fadeAllTo(const LEDColor& target, uint32_t durationMs);
    void fadeAllTo(uint8_t r, uint8_t g, uint8_t b, uint32_t durationMs);
    
    // Fade from current color to target (smart starting point)
    void fadeToFromCurrent(const LEDColor& target, uint32_t durationMs);
    void fadeToFromCurrent(uint8_t r, uint8_t g, uint8_t b, uint32_t durationMs);
    
    // Cross-fade between two colors
    void crossFade(const LEDColor& colorA, const LEDColor& colorB, uint32_t durationMs);
    
    // Instant transition (no animation)
    void transitionTo(const LEDColor& target);
    void transitionTo(uint8_t r, uint8_t g, uint8_t b);
    
    // === ANIMATIONS ===
    
    // Start animation
    void startAnimation(LEDAnimationType type, uint32_t durationMs, bool repeat = false);
    void startAnimation(const LEDAnimation& animation);
    
    // Stop current animation
    void stopAnimation();
    bool isAnimating() const { return animationActive; }
    
    // Specific animation helpers
    void pulse(const LEDColor& color, uint32_t durationMs, bool repeat = true);
    void breathe(const LEDColor& color, uint32_t durationMs, bool repeat = true);
    void rainbow(uint32_t durationMs, bool repeat = true);
    void colorWipe(const LEDColor& color, uint32_t durationMs);
    void theaterChase(const LEDColor& color, uint32_t durationMs, bool repeat = true);
    void fire(uint32_t durationMs, bool repeat = true);
    
    // === UTILITY FUNCTIONS ===
    
    // Update system (call regularly from main loop)
    void update();
    
    // Reset LED controller to default state
    void reset();
    
    // Current color management
    void setCurrentColor(const LEDColor& color);
    void setCurrentColor(uint8_t r, uint8_t g, uint8_t b);
    LEDColor getCurrentColor() const;
    
    // Get LED count
    int getCount() const { return ledCount; }
    
    // Check if LEDs are available
    static bool isAvailable() { return WISP_HAS_LED; }
    
    // Get LED type
    LEDType getType() const { return ledType; }
    
    // Color utility functions
    static LEDColor HSV(float h, float s, float v);  // HSV to RGB conversion
    static LEDColor wheel(uint8_t pos);              // Color wheel (0-255)
    static LEDColor gamma32(const LEDColor& color);   // Gamma correction
    
    // === EVENT HELPERS ===
    
    // Quick status indicators
    void showStatus(const LEDColor& color, uint32_t durationMs = 1000);
    void showError(uint32_t durationMs = 2000);      // Red flash
    void showWarning(uint32_t durationMs = 1500);    // Orange flash  
    void showSuccess(uint32_t durationMs = 1000);    // Green flash
    void showInfo(uint32_t durationMs = 800);        // Blue flash
    
    // Boot sequence
    void showBootSequence();
    
    // Low battery warning
    void showLowBattery();
    
    // === ADVANCED FEATURES ===
    
    // Set custom animation callback
    typedef LEDColor (*AnimationCallback)(int ledIndex, uint32_t time, void* userData);
    void setCustomAnimation(AnimationCallback callback, void* userData, uint32_t durationMs, bool repeat = false);
    
    // Disable auto-update (for manual control)
    void setAutoUpdate(bool enabled) { /* Not implemented yet */ }
    
private:
    AnimationCallback customCallback;
    void* customUserData;
};

// Global LED controller instance
extern LEDController& ledController;

// === APP/ROM API CONVENIENCE MACROS ===

// Basic LED control
#define LED_INIT()                      ledController.init()
#define LED_SHUTDOWN()                  ledController.shutdown()
#define LED_RESET()                     ledController.reset()
#define LED_UPDATE()                    ledController.update()

// Color setting
#define LED_SET_COLOR(r, g, b)          ledController.setAll(r, g, b)
#define LED_SET_CURRENT(r, g, b)        ledController.setCurrentColor(r, g, b)
#define LED_CLEAR()                     ledController.clear()
#define LED_SHOW()                      ledController.show()

// Brightness control
#define LED_BRIGHTNESS(b)               ledController.setBrightness(b)
#define LED_FADE_BRIGHTNESS(b, ms)      ledController.fadeBrightness(b, ms)

// Color transitions
#define LED_FADE_TO(r, g, b, ms)        ledController.fadeAllTo(r, g, b, ms)
#define LED_FADE_FROM_CURRENT(r, g, b, ms) ledController.fadeToFromCurrent(r, g, b, ms)
#define LED_TRANSITION_TO(r, g, b)      ledController.transitionTo(r, g, b)
#define LED_CROSS_FADE(r1, g1, b1, r2, g2, b2, ms) ledController.crossFade(LEDColor(r1, g1, b1), LEDColor(r2, g2, b2), ms)

// Animations
#define LED_PULSE(r, g, b, ms)          ledController.pulse(LEDColor(r, g, b), ms)
#define LED_BREATHE(r, g, b, ms)        ledController.breathe(LEDColor(r, g, b), ms)
#define LED_RAINBOW(ms)                 ledController.rainbow(ms)
#define LED_COLOR_WIPE(r, g, b, ms)     ledController.colorWipe(LEDColor(r, g, b), ms)
#define LED_THEATER_CHASE(r, g, b, ms)  ledController.theaterChase(LEDColor(r, g, b), ms)
#define LED_FIRE(ms)                    ledController.fire(ms)
#define LED_STOP_ANIMATION()            ledController.stopAnimation()

// Status indication macros
#define LED_ERROR()                     ledController.showError()
#define LED_SUCCESS()                   ledController.showSuccess()
#define LED_WARNING()                   ledController.showWarning()
#define LED_INFO()                      ledController.showInfo()
#define LED_BOOT_SEQUENCE()             ledController.showBootSequence()
#define LED_LOW_BATTERY()               ledController.showLowBattery()

// Query macros
#define LED_IS_AVAILABLE()              LEDController::isAvailable()
#define LED_IS_INITIALIZED()            ledController.isInitialized()
#define LED_IS_ANIMATING()              ledController.isAnimating()
#define LED_GET_COUNT()                 ledController.getCount()
#define LED_GET_BRIGHTNESS()            ledController.getBrightness()
#define LED_GET_CURRENT_COLOR()         ledController.getCurrentColor()

// Utility color macros
#define LED_BLACK                       LEDColor::BLACK
#define LED_WHITE                       LEDColor::WHITE
#define LED_RED                         LEDColor::RED
#define LED_GREEN                       LEDColor::GREEN
#define LED_BLUE                        LEDColor::BLUE
#define LED_YELLOW                      LEDColor::YELLOW
#define LED_CYAN                        LEDColor::CYAN
#define LED_MAGENTA                     LEDColor::MAGENTA
#define LED_ORANGE                      LEDColor::ORANGE
#define LED_PURPLE                      LEDColor::PURPLE
#define LED_PINK                        LEDColor::PINK
