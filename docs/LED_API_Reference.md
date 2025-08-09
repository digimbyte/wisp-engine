# Wisp Engine LED Manager API Reference

## Overview

The Wisp Engine LED Manager provides a comprehensive, generic API for controlling various types of LEDs across different ESP32 boards. It supports RGB LEDs, addressable LEDs (WS2812, APA102), and simple GPIO LEDs with a unified interface.

## Key Features

- **Universal LED Support**: Works with WS2812, APA102, PWM RGB, and simple GPIO LEDs
- **Advanced Animations**: Built-in support for fades, pulses, rainbow effects, and custom animations
- **State Management**: Tracks current colors, brightness, and animation state
- **App/ROM API**: Simplified macros for easy integration into applications and ROMs
- **Board Agnostic**: Automatically detects and configures based on board definitions

## Quick Start

```cpp
#include "led_controller.h"

// Initialize the LED system
LED_INIT();

// Set a color
LED_SET_COLOR(255, 0, 0);  // Red
LED_SHOW();

// Start an animation
LED_PULSE(0, 255, 0, 2000);  // Green pulse for 2 seconds

// Update in main loop
LED_UPDATE();
```

## Core API Methods

### Initialization
```cpp
bool init()                    // Initialize LED controller
void shutdown()                // Shutdown LED controller
void reset()                   // Reset to default state
bool isInitialized()           // Check initialization status
```

### Basic LED Control
```cpp
void setLED(int index, uint8_t r, uint8_t g, uint8_t b)  // Set single LED
void setAll(uint8_t r, uint8_t g, uint8_t b)             // Set all LEDs
void clear()                                              // Turn off all LEDs
void show()                                               // Update hardware
void update()                                             // Call in main loop
```

### Brightness Control
```cpp
void setBrightness(float brightness)                      // Set brightness (0.0-1.0)
float getBrightness()                                     // Get current brightness
void fadeBrightness(float target, uint32_t duration)     // Animated brightness fade
```

### Color Transitions
```cpp
// Fade all LEDs to target color
void fadeAllTo(uint8_t r, uint8_t g, uint8_t b, uint32_t duration)
void fadeToFromCurrent(uint8_t r, uint8_t g, uint8_t b, uint32_t duration)

// Fade single LED
void fadeTo(int index, uint8_t r, uint8_t g, uint8_t b, uint32_t duration)

// Cross-fade between two colors
void crossFade(LEDColor colorA, LEDColor colorB, uint32_t duration)

// Instant transition (no animation)
void transitionTo(uint8_t r, uint8_t g, uint8_t b)
```

### Built-in Animations
```cpp
void pulse(LEDColor color, uint32_t duration, bool repeat = true)
void breathe(LEDColor color, uint32_t duration, bool repeat = true) 
void rainbow(uint32_t duration, bool repeat = true)
void colorWipe(LEDColor color, uint32_t duration)
void theaterChase(LEDColor color, uint32_t duration, bool repeat = true)
void fire(uint32_t duration, bool repeat = true)
void stopAnimation()
bool isAnimating()
```

### Status Indicators
```cpp
void showError(uint32_t duration = 2000)        // Red pulse
void showWarning(uint32_t duration = 1500)      // Orange pulse
void showSuccess(uint32_t duration = 1000)      // Green pulse
void showInfo(uint32_t duration = 800)          // Blue pulse
void showBootSequence()                         // Rainbow + green pulse
void showLowBattery()                           // Red breathing
```

## App/ROM API Convenience Macros

### Basic Control
```cpp
LED_INIT()                              // Initialize system
LED_SHUTDOWN()                          // Shutdown system
LED_RESET()                             // Reset to defaults
LED_UPDATE()                            // Update in main loop

LED_SET_COLOR(r, g, b)                  // Set all LEDs
LED_SET_CURRENT(r, g, b)                // Set and remember current color
LED_CLEAR()                             // Turn off all LEDs
LED_SHOW()                              // Update hardware display
```

### Brightness & Transitions
```cpp
LED_BRIGHTNESS(brightness)              // Set brightness (0.0-1.0)
LED_FADE_BRIGHTNESS(brightness, ms)     // Fade brightness over time

LED_FADE_TO(r, g, b, ms)               // Fade all to color
LED_FADE_FROM_CURRENT(r, g, b, ms)     // Fade from current to color
LED_TRANSITION_TO(r, g, b)             // Instant color change
LED_CROSS_FADE(r1,g1,b1, r2,g2,b2, ms) // Cross-fade between colors
```

### Animations
```cpp
LED_PULSE(r, g, b, ms)                 // Pulse color
LED_BREATHE(r, g, b, ms)               // Breathing effect  
LED_RAINBOW(ms)                        // Rainbow cycle
LED_COLOR_WIPE(r, g, b, ms)            // Color wipe effect
LED_THEATER_CHASE(r, g, b, ms)         // Theater chase
LED_FIRE(ms)                           // Fire flicker
LED_STOP_ANIMATION()                   // Stop current animation
```

### Status Indicators
```cpp
LED_ERROR()                            // Show error (red pulse)
LED_SUCCESS()                          // Show success (green pulse)
LED_WARNING()                          // Show warning (orange pulse)
LED_INFO()                             // Show info (blue pulse)
LED_BOOT_SEQUENCE()                    // Boot sequence animation
LED_LOW_BATTERY()                      // Low battery indicator
```

### Query Functions
```cpp
LED_IS_AVAILABLE()                     // Check if LEDs are available
LED_IS_INITIALIZED()                   // Check if initialized
LED_IS_ANIMATING()                     // Check if animation is running
LED_GET_COUNT()                        // Get LED count
LED_GET_BRIGHTNESS()                   // Get current brightness
LED_GET_CURRENT_COLOR()                // Get current color
```

### Color Constants
```cpp
LED_BLACK, LED_WHITE, LED_RED, LED_GREEN, LED_BLUE
LED_YELLOW, LED_CYAN, LED_MAGENTA, LED_ORANGE
LED_PURPLE, LED_PINK
```

## Animation Types

The LED Manager supports several built-in animation types:

- **LED_ANIM_FADE**: Smooth fade between colors
- **LED_ANIM_PULSE**: Brightness pulse (sine wave)  
- **LED_ANIM_BREATHE**: Smooth breathing effect
- **LED_ANIM_RAINBOW**: Rainbow color cycle
- **LED_ANIM_RAINBOW_CHASE**: Moving rainbow effect
- **LED_ANIM_COLOR_WIPE**: Progressive color fill
- **LED_ANIM_THEATER_CHASE**: Theater marquee chase
- **LED_ANIM_FIRE**: Flickering fire simulation
- **LED_ANIM_CUSTOM**: User-defined custom animation

## Custom Animations

You can create custom animations using callbacks:

```cpp
LEDColor customAnimation(int ledIndex, uint32_t time, void* userData) {
    // Your custom animation logic here
    return LEDColor(red, green, blue);
}

ledController.setCustomAnimation(customAnimation, userData, duration, repeat);
```

## Board Configuration

The system automatically detects LED configuration from board config files:

```cpp
// In board config (e.g., esp32-c6_config.h)
#define RGB_LED_PIN 8
#define RGB_LED_COUNT 1  
#define RGB_LED_TYPE "WS2812"
#define WISP_HAS_RGB_LED 1
```

Supported LED types:
- `"WS2812"` or `"NEOPIXEL"` → WS2812 addressable LEDs
- `"APA102"` or `"DOTSTAR"` → APA102 addressable LEDs  
- `"PWM_RGB"` → PWM-controlled RGB LEDs
- `"SIMPLE"` → Simple GPIO on/off LEDs

## Usage Examples

### Basic Usage
```cpp
// Initialize and set red color
LED_INIT();
LED_SET_COLOR(255, 0, 0);
LED_SHOW();

// Fade to blue over 2 seconds
LED_FADE_TO(0, 0, 255, 2000);

// Main loop
while (true) {
    LED_UPDATE();  // Handle animations
    delay(10);
}
```

### Status Indication
```cpp
// Boot sequence
LED_BOOT_SEQUENCE();

// Show different statuses
if (error_occurred) {
    LED_ERROR();  // Red pulse
} else if (warning_condition) {
    LED_WARNING();  // Orange pulse
} else {
    LED_SUCCESS();  // Green pulse
}
```

### Custom Patterns
```cpp
// Rainbow cycle
LED_RAINBOW(3000);  // 3-second rainbow

// Fire effect
LED_FIRE(5000);     // 5 seconds of fire

// Custom breathing pattern
LED_BREATHE(128, 0, 255, 4000);  // Purple breathing, 4 seconds
```

## Integration Notes

- Call `LED_UPDATE()` regularly in your main loop (every 10-50ms recommended)
- The system handles board-specific implementation details automatically
- All animations are non-blocking and time-based
- Colors are automatically scaled by global brightness setting
- The singleton pattern ensures consistent state across the application

## Performance Considerations

- LED updates are optimized for each LED type
- Animation calculations are lightweight and efficient
- The system only updates hardware when colors actually change
- Memory usage scales with LED count (one LEDColor struct per LED)

This comprehensive LED API provides everything needed for sophisticated LED control in Wisp Engine applications and ROMs, from simple status indication to complex animated light shows.
