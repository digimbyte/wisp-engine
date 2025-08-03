// esp32-c6_config.h
#pragma once

// ESP32-C6 1.47inch Display Development Board Configuration
// Based on ESP32-C6-LCD-1.47 actual hardware specifications
//
// VERIFIED HARDWARE SPECIFICATIONS:
// - ESP32-C6FH4 microcontroller with dual RISC-V cores
// - High-performance 32-bit RISC-V processor: up to 160 MHz  
// - Low-power 32-bit RISC-V processor: up to 20 MHz
// - Built-in 320KB ROM, 512KB HP SRAM, 16KB LP SRAM, 4MB Flash
// - Display: 1.47" LCD, 172×320 resolution, 262K colors (RGB565)
// - Connectivity: 2.4GHz WiFi 6 (802.11ax/b/g/n), Bluetooth 5 LE, Bluetooth Mesh
// - Storage: TF card slot for external storage
// - I/O: Multiple GPIO, full-speed USB port, RGB LED with acrylic panel
// - Power: ME6217C33M5G low dropout regulator, 800mA output max
//
// MEMORY BREAKDOWN:
// - HP SRAM (512KB): Main execution memory for apps, framebuffers, game data
// - LP SRAM (16KB): Ultra-low-power memory for sleep mode persistence  
// - Flash (4MB): Program storage, assets, configuration, save data
// - ROM (320KB): ESP-IDF firmware and bootloader
//
// DISPLAY CAPABILITIES:
// - Native resolution: 172×320 (portrait orientation)
// - Color depth: 16-bit RGB565 (262,144 colors)
// - Frame buffer: ~110KB for full-screen RGB565
// - Perfect for GBA-style gaming with modern enhancements

#ifdef PLATFORM_C6

// === CORE PROCESSOR SPECIFICATIONS ===
#define ESP32_C6_CPU_FREQ_HIGH_HZ    160000000    // 160 MHz high-performance RISC-V core
#define ESP32_C6_CPU_FREQ_LOW_HZ     20000000     // 20 MHz low-power RISC-V core
#define ESP32_C6_DUAL_CORE           true         // Dual core architecture
#define ESP32_C6_ARCHITECTURE        "RISC-V"     // 32-bit RISC-V instruction set

// === MEMORY SPECIFICATIONS ===
#define ESP32_C6_ROM_SIZE_KB         320          // 320KB built-in ROM
#define ESP32_C6_HP_SRAM_SIZE_KB     512          // 512KB high-performance SRAM  
#define ESP32_C6_LP_SRAM_SIZE_KB     16           // 16KB low-power SRAM
#define ESP32_C6_FLASH_SIZE_KB       4096         // 4MB Flash memory
#define ESP32_C6_TOTAL_USABLE_KB     (ESP32_C6_HP_SRAM_SIZE_KB + ESP32_C6_LP_SRAM_SIZE_KB)

// === DISPLAY SPECIFICATIONS ===
#define DISPLAY_DIAGONAL_INCH        1.47         // 1.47" diagonal size
#define DISPLAY_WIDTH_PX             172          // 172 pixels width
#define DISPLAY_HEIGHT_PX            320          // 320 pixels height
#define DISPLAY_COLOR_DEPTH_BITS     16           // RGB565 = 16 bits per pixel

// Graphics engine compatibility aliases (protect against redefinition)
#ifndef DISPLAY_WIDTH
    #define DISPLAY_WIDTH                DISPLAY_WIDTH_PX
#endif
#ifndef DISPLAY_HEIGHT
    #define DISPLAY_HEIGHT               DISPLAY_HEIGHT_PX
#endif
#define DISPLAY_TOTAL_COLORS         262144       // 262K colors (2^18)
#define DISPLAY_NATIVE_ORIENTATION   "PORTRAIT"   // 172×320 portrait layout
#define DISPLAY_PIXEL_FORMAT         "RGB565"     // Native color format

// === FRAMEBUFFER MEMORY CALCULATIONS ===
#define DISPLAY_FRAMEBUFFER_BYTES    (DISPLAY_WIDTH_PX * DISPLAY_HEIGHT_PX * 2)     // ~110KB RGB565
#define DISPLAY_DEPTHBUFFER_BYTES    (DISPLAY_WIDTH_PX * DISPLAY_HEIGHT_PX * 1)     // ~55KB depth
#define DISPLAY_PALETTE_BUFFER_BYTES (DISPLAY_WIDTH_PX * DISPLAY_HEIGHT_PX / 2)     // ~27KB 4-bit indexed
#define DISPLAY_TOTAL_PIXELS         (DISPLAY_WIDTH_PX * DISPLAY_HEIGHT_PX)         // 54,880 pixels

// === CONNECTIVITY SPECIFICATIONS ===
#define WIFI_STANDARD               "WiFi 6"       // 802.11ax support
#define WIFI_BANDS                  "2.4GHz"       // 2.4GHz band only
#define WIFI_BANDWIDTH_MHZ          40             // Up to 40MHz channel bandwidth
#define WIFI_PROTOCOLS              "ax/b/g/n"     // Supported 802.11 protocols
#define BLUETOOTH_VERSION           "5.0 LE"       // Bluetooth 5 Low Energy
#define BLUETOOTH_MESH_SUPPORT      true           // Bluetooth Mesh capable
#define USB_SUPPORT                 "Full-Speed"   // Full-speed USB port

// === STORAGE SPECIFICATIONS ===  
#define EXTERNAL_STORAGE_TYPE       "TF_CARD"      // TF/MicroSD card slot
#define EXTERNAL_STORAGE_FORMAT     "FAT32"        // Recommended format
#define EXTERNAL_STORAGE_MAX_GB     32             // Practical limit for FAT32

// === POWER SPECIFICATIONS ===
#define POWER_REGULATOR_MODEL       "ME6217C33M5G" // Low dropout regulator
#define POWER_OUTPUT_MAX_MA         800            // 800mA maximum output
#define POWER_VOLTAGE_V             3.3            // 3.3V operating voltage

// === PERFORMANCE ESTIMATES FOR GAMING ===
#define ESTIMATED_DRAW_CALLS_60FPS   800          // Conservative sprite count at 60fps
#define ESTIMATED_FILL_RATE_60FPS    2500000      // Pixels fillable per frame at 60fps  
#define ESTIMATED_TRIANGLES_60FPS    300          // 3D wireframe triangles at 60fps
#define ESTIMATED_AUDIO_CHANNELS     6            // Simultaneous audio channels

// === GAMING MEMORY PROFILES ===
// Based on available 512KB HP SRAM minus system overhead
#define GAMEBOY_PROFILE_KB          80            // Minimal: ~400KB available for app
#define GBA_PROFILE_KB              150           // Balanced: ~300KB available for app  
#define MODERN_PROFILE_KB           250           // Enhanced: ~200KB available for app
#define PALETTE_OPTIMIZED_KB        400           // Palette mode: ~450KB available for app

// === RGB LED SPECIFICATIONS ===
#define RGB_LED_TYPE                "WS2812"       // Addressable RGB LED type
#define RGB_LED_COUNT               1              // Single RGB LED
#define RGB_LED_PANEL               "ACRYLIC"      // Clear acrylic sandwich panel
#define RGB_LED_PIN                 8              // GPIO pin for RGB LED

// === AUDIO PIN DEFINITIONS ===
#define AUDIO_PIEZO_PIN             21             // Built-in piezo speaker
#define AUDIO_PWM_LEFT              20             // PWM audio left channel
#define AUDIO_PWM_RIGHT             10             // PWM audio right channel
#define AUDIO_I2S_BCLK              18             // I2S bit clock
#define AUDIO_I2S_LRC               19             // I2S left/right clock  
#define AUDIO_I2S_DIN               22             // I2S data input

// === DISPLAY SPI PIN DEFINITIONS ===
#define DISPLAY_SPI_MOSI_PIN        23             // SPI MOSI pin
#define DISPLAY_SPI_CLK_PIN         18             // SPI Clock pin
#define DISPLAY_SPI_CS_PIN          5              // SPI Chip Select pin
#define DISPLAY_DC_PIN              4              // Display Data/Command pin
#define DISPLAY_RST_PIN             21             // Display Reset pin
#define DISPLAY_BL_PIN              3              // Display Backlight pin

// === BUTTON DEFINITIONS ===
#define BUTTON_BOOT_PIN             9              // BOOT button
#define BUTTON_RESET_PIN            0              // RESET button (GPIO0)

// === INPUT CONTROLLER PIN MAPPING ===
// Standard gaming controller layout for ESP32-C6-LCD-1.47
#define INPUT_LEFT_PIN              4              // Left D-Pad
#define INPUT_RIGHT_PIN             5              // Right D-Pad  
#define INPUT_UP_PIN                6              // Up D-Pad
#define INPUT_DOWN_PIN              7              // Down D-Pad
#define INPUT_A_PIN                 1              // A button (primary action)
#define INPUT_B_PIN                 2              // B button (secondary action)
#define INPUT_C_PIN                 3              // C button (tertiary action)
#define INPUT_SELECT_PIN            11             // SELECT button
#define INPUT_START_PIN             17             // START button

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
#define DEBUG_ERROR_PIN             12             // Error signal output
#define DEBUG_WARNING_PIN           13             // Warning signal output
#define DEBUG_INFO_PIN              14             // Info signal output
#define DEBUG_HEARTBEAT_PIN         15             // System heartbeat

// === AUDIO CAPABILITIES ===
#define AUDIO_HAS_PIEZO             1              // Built-in piezo speaker
#define AUDIO_HAS_I2S               1              // I2S DAC support
#define AUDIO_HAS_BLUETOOTH         1              // Bluetooth audio
#define AUDIO_SAMPLE_RATE_DEFAULT   22050          // Default sample rate
#define AUDIO_CHANNELS_MAX          6              // Maximum audio channels
#define AUDIO_BUFFER_SIZE_DEFAULT   512            // Default buffer size

#endif // PLATFORM_C6
