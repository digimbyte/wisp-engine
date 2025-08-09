# Wisp Engine System Integration

This document describes the comprehensive Wisp Engine system that integrates all ESP-IDF library components as requested.

## Overview

The Wisp Engine now provides a unified system initialization framework that matches your ESP-IDF library pattern:

```cpp
// Your requested ESP-IDF pattern:
setup()
Wireless_Init(): Initializes the wireless communication module
Flash_Searching(): Tests and prints the flash memory size information of the device
RGB_Init(): Initializes RGB-related functions
RGB_Example(): Displays example functions of RGB
SD_Init(): Initializes the TF card
LCD_Init(): Initializes the display
BK_Light(50): Sets the backlight brightness to 50
LVGL_Init(): Initializes the LVGL graphics library
Lvgl_Example1(): Calls the specific LVGL example function
while(1)
vTaskDelay(pdMS_TO_TICKS(10)): Short delay, every 10 milliseconds
lv_timer_handler(): Timer handling function for LVGL, used to handle events and animations related to time
```

## System Components

### 1. System Initialization (`system_init.h/cpp`)

**Main Functions:**
- `wisp_system_setup()` - Initialize all components (equivalent to your `setup()`)
- `wisp_system_loop()` - Main system loop (equivalent to your `while(1)` loop)
- `wisp_system_shutdown()` - Graceful shutdown

**Individual Component Functions:**
- `wisp_wireless_init()` - Wireless_Init()
- `wisp_flash_searching()` - Flash_Searching() 
- `wisp_rgb_init()` - RGB_Init()
- `wisp_rgb_example()` - RGB_Example()
- `wisp_sd_init()` - SD_Init()
- `wisp_lcd_init()` - LCD_Init() 
- `wisp_backlight_set(50)` - BK_Light(50)
- `wisp_lvgl_init()` - LVGL_Init()
- `wisp_lvgl_example1()` - Lvgl_Example1()
- `wisp_delay_ms(10)` - vTaskDelay(pdMS_TO_TICKS(10))
- `wisp_lvgl_timer_handler()` - lv_timer_handler()

### 2. LED Controller (`led_controller.h/cpp` + `led_implementations.h`)

**Features:**
- Generic LED API supporting multiple LED types
- Board-specific implementations (Simple GPIO, PWM RGB, WS2812, APA102)
- Comprehensive animation system
- Status indication helpers
- Compile-time LED type detection

**LED Types Supported:**
- `LEDType::SIMPLE_GPIO` - Single on/off LED
- `LEDType::PWM_RGB` - RGB LED via PWM channels
- `LEDType::WS2812_RGB` - WS2812/NeoPixel addressable RGB
- `LEDType::APA102_RGB` - APA102/DotStar addressable RGB

**Quick Usage:**
```cpp
LED_SET_COLOR(255, 0, 0);  // Red
LED_SHOW();                // Update display
LED_RAINBOW(2000);         // Rainbow animation
LED_SUCCESS();             // Green status flash
LED_ERROR();               // Red error flash
```

### 3. Board Configuration

Updated board configs to use the new generic LED system:
- `esp32-c6_config.h` - Waveshare ESP32-C6 board
- `esp32-s3_config.h` - ESP32-S3 board

## Usage Example

See `examples/wisp_main_demo.cpp` for a complete example that follows your exact ESP-IDF pattern:

```cpp
extern "C" void app_main(void) {
    // System setup (matches your setup() function)
    if (!setup()) {
        ESP_LOGE(TAG, "System setup failed");
        LED_ERROR();
        return;
    }
    
    // Main loop (matches your while(1) pattern)  
    while (1) {
        wisp_system_loop();        // Update system
        wisp_delay_ms(10);         // vTaskDelay(pdMS_TO_TICKS(10))
        // lv_timer_handler() is called inside wisp_system_loop()
    }
}

bool setup() {
    // Initialize complete system
    wisp_init_result_t result = wisp_system_setup();
    if (result != WISP_INIT_OK) return false;
    
    // All your requested functions are called internally:
    // - Wireless_Init()
    // - Flash_Searching() 
    // - RGB_Init() + RGB_Example()
    // - SD_Init()
    // - LCD_Init() + BK_Light(50)
    // - LVGL_Init() + Lvgl_Example1()
    
    return true;
}
```

## Component Status

The system provides comprehensive status monitoring:

```cpp
const wisp_system_status_t* status = wisp_get_system_status();

// Check individual components
if (wisp_is_component_ready(WISP_COMPONENT_WIRELESS)) {
    // Wireless is ready
}

// Print complete status
wisp_print_system_status();

// Run diagnostics
wisp_run_diagnostics();
```

## Building

1. Navigate to the examples directory:
   ```bash
   cd examples/
   ```

2. Configure for your target:
   ```bash
   idf.py set-target esp32c6  # or esp32s3
   ```

3. Configure the project:
   ```bash
   idf.py menuconfig
   ```

4. Build and flash:
   ```bash
   idf.py build flash monitor
   ```

## Key Features

### 1. Board-Agnostic Design
- Automatic LED type detection at compile time
- Board-specific configurations in header files
- Generic API that works across different hardware

### 2. Robust Error Handling
- Component initialization can fail gracefully
- System continues with available components
- Status monitoring and diagnostics

### 3. Modular Architecture
- Each component can be enabled/disabled
- Clean separation of concerns
- Easy to extend with new components

### 4. ESP-IDF Integration
- Full FreeRTOS support
- Native ESP-IDF APIs
- Proper task management
- Standard ESP-IDF logging

### 5. LED Controller Features
- Multiple LED types supported
- Rich animation system (rainbow, pulse, breathe, fire, etc.)
- Status indication helpers
- Color utilities (HSV, gamma correction, color wheel)
- Modern RMT API support (WS2812)
- Bit-banging fallback for compatibility

## Status Indicators

The LED system provides standardized status codes:
- `LED_SUCCESS()` - Green flash (system OK)
- `LED_ERROR()` - Red flash (error condition) 
- `LED_WARNING()` - Orange flash (warning)
- `LED_INFO()` - Blue flash (information)
- Boot sequence with rainbow sweep

## Configuration

### LED Configuration
Define in your board config file:
```cpp
#define RGB_LED_PIN 8
#define RGB_LED_COUNT 1  
#define RGB_LED_TYPE "WS2812"
```

### System Configuration
Use menuconfig or define:
```cpp
#define CONFIG_ESP32_WIFI_ENABLED
#define CONFIG_SD_CARD_ENABLED
#define CONFIG_DISPLAY_ENABLED
#define CONFIG_LVGL_USE_LVGL
```

## Troubleshooting

### LED Issues
- Check `WISP_HAS_LED` macro value
- Verify pin definitions in board config
- Use diagnostics: `wisp_run_diagnostics()`

### Build Issues  
- Ensure ESP-IDF is properly installed
- Check component dependencies in CMakeLists.txt
- Verify target chip selection

### Runtime Issues
- Monitor serial output for initialization messages
- Check component status with `wisp_print_system_status()`
- LED error codes indicate specific failures

This system now provides exactly the ESP-IDF library pattern you requested, with comprehensive LED support and robust system management!
