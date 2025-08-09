// board_auto_config.h - Automatic Board Configuration
// Automatically configured by build script based on board config files
#pragma once

#include "esp_system.h"

// =============================================================================
// BUILD-TIME BOARD CONFIGURATION
// =============================================================================
// This file is automatically configured by the build script when you run:
// .\tools\build.ps1 -board "board-name"
// 
// The build script reads configs/boards/board-name.config and sets all the
// defines below via compiler flags, so no manual configuration needed!

// Board identification (set by build script)
#ifndef WISP_BOARD_NAME
    #define WISP_BOARD_NAME "unknown"
#endif

#ifndef WISP_TARGET
    #define WISP_TARGET "ESP32"
#endif

// =============================================================================
// PLATFORM DETECTION
// =============================================================================
// These are set by the build script based on the board config

#if defined(WISP_PLATFORM_ESP32C6)
    #define WISP_PLATFORM_NAME "ESP32-C6"
    #define WISP_PLATFORM_C6 1
#elif defined(WISP_PLATFORM_ESP32S3)
    #define WISP_PLATFORM_NAME "ESP32-S3" 
    #define WISP_PLATFORM_S3 1
#elif defined(WISP_PLATFORM_ESP32)
    #define WISP_PLATFORM_NAME "ESP32"
    #define WISP_PLATFORM_CLASSIC 1
#else
    #define WISP_PLATFORM_NAME "Generic ESP32"
    #warning "Platform not specified - using generic configuration"
#endif

// =============================================================================
// FEATURE FLAG DEFAULTS
// =============================================================================
// If not specified in board config, use sensible defaults

#ifndef HAS_WIFI
    #define HAS_WIFI 1  // Most ESP32s have WiFi
#endif

#ifndef HAS_BLUETOOTH
    #define HAS_BLUETOOTH 1  // Most ESP32s have Bluetooth
#endif

#ifndef HAS_DISPLAY
    #define HAS_DISPLAY 0  // Not all boards have displays
#endif

#ifndef HAS_TOUCH
    #define HAS_TOUCH 0  // Touch depends on display
#endif

#ifndef HAS_AUDIO
    #define HAS_AUDIO 0  // Audio is optional
#endif

#ifndef HAS_CAMERA
    #define HAS_CAMERA 0  // Camera is optional
#endif

#ifndef HAS_SD_CARD
    #define HAS_SD_CARD 0  // SD card is optional
#endif

#ifndef HAS_IMU
    #define HAS_IMU 0  // IMU is optional
#endif

#ifndef HAS_RTC
    #define HAS_RTC 0  // RTC is optional
#endif

#ifndef HAS_RGB_LED
    #define HAS_RGB_LED 0  // RGB LED is optional
#endif

// =============================================================================
// HARDWARE DEFAULTS
// =============================================================================

#ifndef CPU_FREQ_MHZ
    #define CPU_FREQ_MHZ 240  // Default ESP32 frequency
#endif

#ifndef FLASH_SIZE_MB
    #define FLASH_SIZE_MB 4  // Common ESP32 flash size
#endif

#ifndef DISPLAY_WIDTH
    #define DISPLAY_WIDTH 0  // No display by default
#endif

#ifndef DISPLAY_HEIGHT
    #define DISPLAY_HEIGHT 0  // No display by default
#endif

// =============================================================================
// BOARD INFORMATION STRUCTURE
// =============================================================================

typedef struct {
    const char* board_name;
    const char* platform_name;
    const char* target;
    
    // Hardware specs
    uint16_t cpu_freq_mhz;
    uint8_t cpu_cores;
    uint16_t flash_size_mb;
    uint16_t sram_kb;
    uint16_t psram_size_mb;
    
    // Display info
    uint16_t display_width;
    uint16_t display_height;
    uint8_t display_color_depth;
    const char* display_driver;
    
    // Feature flags
    struct {
        uint8_t wifi : 1;
        uint8_t bluetooth : 1;
        uint8_t display : 1;
        uint8_t touch : 1;
        uint8_t audio : 1;
        uint8_t camera : 1;
        uint8_t sd_card : 1;
        uint8_t imu : 1;
        uint8_t rtc : 1;
        uint8_t rgb_led : 1;
    } features;
} wisp_board_info_t;

// =============================================================================
// BOARD INFO GETTER
// =============================================================================

static inline wisp_board_info_t wisp_get_board_info() {
    wisp_board_info_t info = {
        .board_name = WISP_BOARD_NAME,
        .platform_name = WISP_PLATFORM_NAME,
        .target = WISP_TARGET,
        
        .cpu_freq_mhz = CPU_FREQ_MHZ,
        #ifdef CPU_CORES
            .cpu_cores = CPU_CORES,
        #else
            .cpu_cores = 2,  // Most ESP32s are dual core
        #endif
        .flash_size_mb = FLASH_SIZE_MB,
        #ifdef SRAM_KB
            .sram_kb = SRAM_KB,
        #elif defined(SRAM_HP_KB)
            .sram_kb = SRAM_HP_KB,
        #else
            .sram_kb = 520,  // ESP32 default
        #endif
        #ifdef PSRAM_SIZE_MB
            .psram_size_mb = PSRAM_SIZE_MB,
        #else
            .psram_size_mb = 0,
        #endif
        
        .display_width = DISPLAY_WIDTH,
        .display_height = DISPLAY_HEIGHT,
        #ifdef DISPLAY_COLOR_DEPTH
            .display_color_depth = DISPLAY_COLOR_DEPTH,
        #else
            .display_color_depth = 16,
        #endif
        #ifdef DISPLAY_DRIVER
            .display_driver = DISPLAY_DRIVER,
        #else
            .display_driver = "None",
        #endif
        
        .features = {
            .wifi = HAS_WIFI,
            .bluetooth = HAS_BLUETOOTH,  
            .display = HAS_DISPLAY,
            .touch = HAS_TOUCH,
            .audio = HAS_AUDIO,
            .camera = HAS_CAMERA,
            .sd_card = HAS_SD_CARD,
            .imu = HAS_IMU,
            .rtc = HAS_RTC,
            .rgb_led = HAS_RGB_LED
        }
    };
    return info;
}

// =============================================================================
// CONVENIENCE MACROS
// =============================================================================

// Easy feature checking
#define WISP_HAS_WIFI           HAS_WIFI
#define WISP_HAS_BLUETOOTH      HAS_BLUETOOTH
#define WISP_HAS_DISPLAY        HAS_DISPLAY
#define WISP_HAS_TOUCH          HAS_TOUCH
#define WISP_HAS_AUDIO          HAS_AUDIO
#define WISP_HAS_CAMERA         HAS_CAMERA
#define WISP_HAS_SD_CARD        HAS_SD_CARD
#define WISP_HAS_IMU            HAS_IMU
#define WISP_HAS_RTC            HAS_RTC
#define WISP_HAS_RGB_LED        HAS_RGB_LED

// Feature requirement checking
#define WISP_REQUIRE_WIFI       static_assert(WISP_HAS_WIFI, "This code requires WiFi support")
#define WISP_REQUIRE_DISPLAY    static_assert(WISP_HAS_DISPLAY, "This code requires display support")
#define WISP_REQUIRE_TOUCH      static_assert(WISP_HAS_TOUCH, "This code requires touch support")
#define WISP_REQUIRE_AUDIO      static_assert(WISP_HAS_AUDIO, "This code requires audio support")

// Debug information
static inline void wisp_print_board_info() {
    wisp_board_info_t info = wisp_get_board_info();
    
    printf("\n=== Wisp Engine Board Information ===\n");
    printf("Board: %s\n", info.board_name);
    printf("Platform: %s\n", info.platform_name);
    printf("Target: %s\n", info.target);
    printf("CPU: %d cores @ %d MHz\n", info.cpu_cores, info.cpu_freq_mhz);
    printf("Flash: %d MB\n", info.flash_size_mb);
    printf("SRAM: %d KB\n", info.sram_kb);
    if (info.psram_size_mb > 0) {
        printf("PSRAM: %d MB\n", info.psram_size_mb);
    }
    
    if (info.features.display) {
        printf("Display: %dx%d (%d-bit) - %s\n", 
               info.display_width, info.display_height, 
               info.display_color_depth, info.display_driver);
    }
    
    printf("\nFeatures:\n");
    printf("  WiFi: %s\n", info.features.wifi ? "Yes" : "No");
    printf("  Bluetooth: %s\n", info.features.bluetooth ? "Yes" : "No");
    printf("  Display: %s\n", info.features.display ? "Yes" : "No");
    printf("  Touch: %s\n", info.features.touch ? "Yes" : "No");
    printf("  Audio: %s\n", info.features.audio ? "Yes" : "No");
    printf("  Camera: %s\n", info.features.camera ? "Yes" : "No");
    printf("  SD Card: %s\n", info.features.sd_card ? "Yes" : "No");
    printf("  IMU: %s\n", info.features.imu ? "Yes" : "No");
    printf("  RTC: %s\n", info.features.rtc ? "Yes" : "No");
    printf("  RGB LED: %s\n", info.features.rgb_led ? "Yes" : "No");
    printf("====================================\n");
}
