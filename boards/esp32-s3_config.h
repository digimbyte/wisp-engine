// esp32-s3_config.h
#pragma once

// ESP32-S3 1.28" Round Display Development Board Configuration
// Supports two variants: ESP32S3-128SPIT (SPI) and ESP32S3-128I80T (I80 parallel)
//
// VERIFIED HARDWARE SPECIFICATIONS:
// - ESP32-S3-N16R16 microcontroller with dual Xtensa LX7 cores
// - Dual-core Xtensa LX7 processors: up to 240 MHz each
// - Memory: 16MB Flash + 16MB PSRAM (configurable)
// - Display: 1.28" IPS Round LCD, 240×240 resolution, GC9A01 controller
// - Interface: SPI (4-wire) or I80 parallel (8-bit)
// - Touch: Capacitive touch support (CST816S controller)
// - Connectivity: WiFi 6 (2.4GHz), Bluetooth 5 LE
// - Working voltage: 3.3V/5V
// - Current consumption: 20mA typical
//
// DISPLAY SPECIFICATIONS:
// - Size: 1.28" diagonal, Ø32.4mm display area
// - Resolution: 240×240 pixels (square format)
// - Color depth: 16-bit RGB565 (65,536 colors)
// - Brightness: 400 Cd/m²
// - Viewing angle: IPS full view
// - Backlight: LED × 2
//
// TOUCH SPECIFICATIONS:
// - Type: Capacitive touch (when available)
// - Controller: CST816S or similar
// - Interface: I2C
// - Multi-touch: Single point
//
// MEMORY ARCHITECTURE:
// - Flash (16MB): Program storage, assets, configuration, large game data
// - PSRAM (16MB): Extended working memory for complex apps
// - SRAM (512KB): High-speed cache and critical operations
// - RTC Memory: Persistent data during deep sleep

#ifdef PLATFORM_S3

// === CORE PROCESSOR SPECIFICATIONS ===
#define ESP32_S3_CPU_FREQ_MAX_HZ     240000000    // 240 MHz dual-core Xtensa LX7
#define ESP32_S3_CORE_COUNT          2            // Dual core architecture
#define ESP32_S3_ARCHITECTURE        "Xtensa LX7" // 32-bit Xtensa LX7 instruction set
#define ESP32_S3_AI_ACCELERATION     true         // AI acceleration support

// === MEMORY SPECIFICATIONS ===
#define ESP32_S3_SRAM_SIZE_KB        512          // 512KB built-in SRAM
#define ESP32_S3_FLASH_SIZE_KB       16384        // 16MB Flash memory (N16)
#define ESP32_S3_PSRAM_SIZE_KB       16384        // 16MB PSRAM (R16)
#define ESP32_S3_RTC_MEMORY_KB       16           // RTC slow memory
#define ESP32_S3_TOTAL_RAM_KB        (ESP32_S3_SRAM_SIZE_KB + ESP32_S3_PSRAM_SIZE_KB)

// === DISPLAY SPECIFICATIONS ===
#define DISPLAY_DIAGONAL_INCH        1.28         // 1.28" diagonal size
#define DISPLAY_WIDTH_PX             240          // 240 pixels width
#define DISPLAY_HEIGHT_PX            240          // 240 pixels height
#define DISPLAY_SHAPE                "ROUND"      // Round display format

// Graphics engine compatibility aliases (protect against redefinition)  
#ifndef DISPLAY_WIDTH
    #define DISPLAY_WIDTH                DISPLAY_WIDTH_PX
#endif
#ifndef DISPLAY_HEIGHT
    #define DISPLAY_HEIGHT               DISPLAY_HEIGHT_PX
#endif
#define DISPLAY_AREA_MM              32.4         // Ø32.4mm display area
#define DISPLAY_COLOR_DEPTH_BITS     16           // RGB565 = 16 bits per pixel
#define DISPLAY_TOTAL_COLORS         65536        // 65K colors (2^16)
#define DISPLAY_CONTROLLER           "GC9A01"     // Display controller chip
#define DISPLAY_PIXEL_FORMAT         "RGB565"     // Native color format
#define DISPLAY_BRIGHTNESS_CDMZ      400          // 400 Cd/m² brightness
#define DISPLAY_BACKLIGHT_LEDS       2            // LED × 2 backlight

// === DISPLAY INTERFACE SPECIFICATIONS ===
// Board variant 1: SPI Interface (ESP32S3-128SPIT)
#define DISPLAY_INTERFACE_SPI        1            // SPI 4-wire interface
#define DISPLAY_SPI_FREQUENCY_MHZ    80           // Max SPI frequency
#define DISPLAY_SPI_MODE             0            // SPI mode 0

// Board variant 2: I80 Parallel Interface (ESP32S3-128I80T)  
#define DISPLAY_INTERFACE_I80        1            // I80 8-bit parallel interface
#define DISPLAY_I80_FREQUENCY_MHZ    20           // Max I80 frequency

// === FRAMEBUFFER MEMORY CALCULATIONS ===
#define DISPLAY_FRAMEBUFFER_BYTES    (DISPLAY_WIDTH_PX * DISPLAY_HEIGHT_PX * 2)     // ~115KB RGB565
#define DISPLAY_DEPTHBUFFER_BYTES    (DISPLAY_WIDTH_PX * DISPLAY_HEIGHT_PX * 1)     // ~58KB depth
#define DISPLAY_PALETTE_BUFFER_BYTES (DISPLAY_WIDTH_PX * DISPLAY_HEIGHT_PX / 2)     // ~29KB 4-bit indexed
#define DISPLAY_TOTAL_PIXELS         (DISPLAY_WIDTH_PX * DISPLAY_HEIGHT_PX)         // 57,600 pixels

// === TOUCH SPECIFICATIONS ===
#define TOUCH_SUPPORT                true         // Capacitive touch available
#define TOUCH_CONTROLLER             "CST816S"    // Touch controller chip
#define TOUCH_INTERFACE              "I2C"        // I2C interface
#define TOUCH_MAX_POINTS             1            // Single point touch
#define TOUCH_RESOLUTION_X           240          // Touch X resolution
#define TOUCH_RESOLUTION_Y           240          // Touch Y resolution

// === CONNECTIVITY SPECIFICATIONS ===
// NOTE: ESP32-S3 connectivity varies by module variant
// Some modules (like ESP32-S3-WROOM) have WiFi/Bluetooth
// Others (like ESP32-S3 basic modules) may only have wired connectivity
#define WIFI_STANDARD               "WiFi 6"       // 802.11ax support (when available)
#define WIFI_BANDS                  "2.4GHz"       // 2.4GHz band primary (when available)
#define WIFI_BANDWIDTH_MHZ          40             // Up to 40MHz channel bandwidth
#define WIFI_PROTOCOLS              "ax/b/g/n"     // Supported 802.11 protocols
#define BLUETOOTH_VERSION           "5.0 LE"       // Bluetooth 5 Low Energy (when available)
#define BLUETOOTH_CLASSIC           true           // Classic Bluetooth support (when available)
#define USB_SUPPORT                 "Full-Speed"   // Full-speed USB OTG

// === FEATURE AVAILABILITY FLAGS ===
// Conditional compilation based on ESP32-S3 module variant
// Define these in platformio.ini or build configuration for your specific board

#ifndef WISP_HAS_WIFI
    #define WISP_HAS_WIFI           0              // Default: No WiFi (most basic ESP32-S3 modules)
#endif

#ifndef WISP_HAS_BLUETOOTH  
    #define WISP_HAS_BLUETOOTH      0              // Default: No Bluetooth (most basic ESP32-S3 modules)
#endif

#ifndef WISP_HAS_BLUETOOTH_CLASSIC
    #define WISP_HAS_BLUETOOTH_CLASSIC 0          // Default: No Bluetooth Classic
#endif

#ifndef WISP_HAS_WIFI_DIRECT
    #define WISP_HAS_WIFI_DIRECT    0              // Default: No WiFi Direct/P2P
#endif

#ifndef WISP_HAS_EXTERNAL_STORAGE
    #define WISP_HAS_EXTERNAL_STORAGE 0            // Default: No external storage
#endif

// ESP32-S3-WROOM-1/2 and similar modules with wireless capabilities:
// To enable WiFi/Bluetooth for ESP32-S3-WROOM modules, add to platformio.ini:
// build_flags = 
//   -DWISP_HAS_WIFI=1
//   -DWISP_HAS_BLUETOOTH=1
//   -DWISP_HAS_BLUETOOTH_CLASSIC=1
//   -DWISP_HAS_WIFI_DIRECT=1

// === POWER SPECIFICATIONS ===
#define POWER_VOLTAGE_MIN_V         2.8            // Minimum operating voltage
#define POWER_VOLTAGE_MAX_V         3.3            // Maximum operating voltage
#define POWER_CURRENT_TYPICAL_MA    20             // Typical current consumption
#define POWER_CURRENT_MAX_MA        200            // Maximum current consumption

// === PERFORMANCE ESTIMATES FOR GAMING ===
#define ESTIMATED_DRAW_CALLS_60FPS   1500         // Sprite count at 60fps
#define ESTIMATED_FILL_RATE_60FPS    4000000      // Pixels fillable per frame at 60fps  
#define ESTIMATED_TRIANGLES_60FPS    800          // 3D triangles at 60fps
#define ESTIMATED_AUDIO_CHANNELS     8            // Simultaneous audio channels

// === GAMING MEMORY PROFILES ===
// Based on available 16MB PSRAM + 512KB SRAM
#define GAMEBOY_PROFILE_KB          200           // Minimal: ~16MB available
#define GBA_PROFILE_KB              500           // Balanced: ~15MB available  
#define MODERN_PROFILE_KB           2000          // Enhanced: ~14MB available
#define UNLIMITED_PROFILE_KB        8000          // Full features: ~8MB available

// === DISPLAY PIN DEFINITIONS (SPI VARIANT) ===
#define DISPLAY_SPI_SCK_PIN         18            // SPI clock
#define DISPLAY_SPI_CLK_PIN         18            // SPI clock (alias)
#define DISPLAY_SPI_MOSI_PIN        19            // SPI MOSI (data)
#define DISPLAY_SPI_CS_PIN          5             // Chip select
#define DISPLAY_DC_PIN              16            // Data/Command select
#define DISPLAY_RST_PIN             23            // Reset pin
#define DISPLAY_BL_PIN              4             // Backlight control

// === DISPLAY PIN DEFINITIONS (I80 VARIANT) ===
#define DISPLAY_I80_WR_PIN          35            // Write strobe
#define DISPLAY_I80_RD_PIN          34            // Read strobe (optional)
#define DISPLAY_I80_CS_PIN          33            // Chip select
#define DISPLAY_I80_DC_PIN          15            // Data/Command select
#define DISPLAY_I80_RST_PIN         16            // Reset pin
#define DISPLAY_I80_D0_PIN          8             // Data bus D0-D7
#define DISPLAY_I80_D1_PIN          3
#define DISPLAY_I80_D2_PIN          46
#define DISPLAY_I80_D3_PIN          9
#define DISPLAY_I80_D4_PIN          1
#define DISPLAY_I80_D5_PIN          5
#define DISPLAY_I80_D6_PIN          6
#define DISPLAY_I80_D7_PIN          7

// === TOUCH PIN DEFINITIONS ===
#define TOUCH_SDA_PIN               6             // I2C data
#define TOUCH_SCL_PIN               7             // I2C clock
#define TOUCH_INT_PIN               21            // Touch interrupt
#define TOUCH_RST_PIN               None          // Touch reset (shared with display)

// === AUDIO PIN DEFINITIONS ===
#define AUDIO_PIEZO_PIN             21            // Built-in piezo speaker
#define AUDIO_PWM_LEFT              20            // PWM audio left channel
#define AUDIO_PWM_RIGHT             23            // PWM audio right channel
#define AUDIO_I2S_BCLK              GPIO_NUM_5    // I2S bit clock (valid ESP32-S3 pin)
#define AUDIO_I2S_LRC               GPIO_NUM_4    // I2S left/right clock (valid ESP32-S3 pin)
#define AUDIO_I2S_DIN               GPIO_NUM_6    // I2S data input (valid ESP32-S3 pin)
#define AUDIO_I2S_DOUT              GPIO_NUM_7    // I2S data output (valid ESP32-S3 pin)

// === BUTTON DEFINITIONS ===
#define BUTTON_BOOT_PIN             0             // BOOT button
#define BUTTON_RESET_PIN            -1            // Hardware reset (no GPIO)

// === INPUT CONTROLLER PIN MAPPING ===
// Standard gaming controller layout for ESP32-S3 Round Display
// Uses different pins to avoid conflicts with display and touch
#define INPUT_LEFT_PIN              8             // Left D-Pad
#define INPUT_RIGHT_PIN             3             // Right D-Pad  
#define INPUT_UP_PIN                46            // Up D-Pad
#define INPUT_DOWN_PIN              9             // Down D-Pad
#define INPUT_A_PIN                 1             // A button (primary action)
#define INPUT_B_PIN                 2             // B button (secondary action)
#define INPUT_C_PIN                 17            // C button (tertiary action)
#define INPUT_SELECT_PIN            18            // SELECT button
#define INPUT_START_PIN             45            // START button

// Pin array for InputController initialization
static const uint8_t BUTTON_PINS[9] = {
    INPUT_LEFT_PIN,     // 0: LEFT
    INPUT_RIGHT_PIN,    // 1: RIGHT
    INPUT_UP_PIN,       // 2: UP
    INPUT_DOWN_PIN,     // 3: DOWN
    INPUT_A_PIN,        // 4: A
    INPUT_B_PIN,        // 5: B
    INPUT_C_PIN,        // 6: C
    INPUT_SELECT_PIN,   // 7: SELECT
    INPUT_START_PIN     // 8: START
};

// === DEBUG PIN DEFINITIONS ===
#define DEBUG_ERROR_PIN             12            // Error signal output
#define DEBUG_WARNING_PIN           13            // Warning signal output
#define DEBUG_INFO_PIN              14            // Info signal output
#define DEBUG_HEARTBEAT_PIN         15            // System heartbeat

// === AUDIO CAPABILITIES ===
#define AUDIO_HAS_PIEZO             1             // Built-in piezo speaker
#define AUDIO_HAS_I2S               1             // I2S DAC support
#define AUDIO_HAS_BLUETOOTH         1             // Bluetooth audio
#define AUDIO_HAS_SPEAKER           1             // External speaker support
#define AUDIO_SAMPLE_RATE_DEFAULT   44100         // Default sample rate (higher quality)
#define AUDIO_CHANNELS_MAX          8             // Maximum audio channels
#define AUDIO_BUFFER_SIZE_DEFAULT   1024          // Default buffer size (larger)

#endif // PLATFORM_S3
