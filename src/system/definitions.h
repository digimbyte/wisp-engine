// definitions.h - Hardware definitions for ESP32-C6 1.47" Display Board
#pragma once
#include <stdint.h>
#include "driver/gpio.h"  // For GPIO_NUM_* constants

// Include board-specific configuration
#ifdef PLATFORM_C6  
  #include "../boards/esp32-c6_config.h"  
#elif defined(PLATFORM_S3)  
  #include "../boards/esp32-s3_config.h"  
#endif

// --- HARDWARE CONFIGURATION ---
// Updated for ESP32-C6-LCD-1.47 actual specifications
#ifndef MAX_BUTTONS
#define MAX_BUTTONS 6
#endif
#ifndef MAX_PALETTES
#define MAX_PALETTES 4
#endif
#ifndef PALETTE_SIZE
#define PALETTE_SIZE 256
#endif
#ifndef MAX_PARTICLES
#define MAX_PARTICLES 64
#endif
#ifndef MAX_PHYSICS_QUEUE
#define MAX_PHYSICS_QUEUE 128
#endif
#ifndef MAX_SHAPES
#define MAX_SHAPES 4
#endif
// MAX_SPRITES and SPRITE_LUT_SIZE now defined as constexpr in graphics engine
#ifndef MAX_DEPTH_LAYERS
#define MAX_DEPTH_LAYERS 13
#endif

// --- BUTTON INPUT CONFIGURATION ---
// Updated for ESP32-C6 board layout
enum Button {
  BTN_UP,
  BTN_DOWN,
  BTN_LEFT,
  BTN_RIGHT,
  BTN_A,       // Main action button (was SELECT)
  BTN_B,       // Secondary button (was BACK)
  BTN_COUNT
};

// Button pin assignments now defined in board-specific configs
// See boards/esp32-c6_config.h or boards/esp32-s3_config.h

// --- BUILT-IN BUTTONS ---
#ifndef BUTTON_BOOT_PIN
#define BUTTON_BOOT_PIN   9   // Default BOOT button (can be overridden by board config)
#endif
#ifndef BUTTON_RESET_PIN
#define BUTTON_RESET_PIN  0   // Default RESET button (can be overridden by board config)
#endif

// --- AUDIO CONFIGURATION ---
// Verified for ESP32-C6-LCD-1.47 board
#ifndef AUDIO_PIEZO_PIN
#define AUDIO_PIEZO_PIN 21
#endif

// I2S DAC pins for external audio (optional) - guard against board config redefinition
#ifndef AUDIO_I2S_BCLK
#define AUDIO_I2S_BCLK  GPIO_NUM_18
#endif
#ifndef AUDIO_I2S_LRC
#define AUDIO_I2S_LRC   GPIO_NUM_19
#endif  
#ifndef AUDIO_I2S_DIN
#define AUDIO_I2S_DIN   GPIO_NUM_22
#endif

// --- SCREEN SPECIFICATIONS ---
// SCREEN_WIDTH and SCREEN_HEIGHT now defined as constexpr in graphics engine
// Remove macro definitions to avoid conflicts

// --- C++ APP CONFIGURATION ---
#define APP_MAIN_FUNCTION "main"  // C++ applications use main() function

// --- RENDER ENGINE DEFAULTS ---
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static uint8_t DUMMY_HEIGHT_MAP[172] __attribute__((unused)) = {0};
#pragma GCC diagnostic pop

// --- SYSTEM TIMING CONFIGURATION ---
#define SYSTEM_FPS 24    // Engine heartbeat/frame pacing (not app speed)
#define FRAME_TIME_MS 16 // Legacy 60 FPS cap, can be deprecated or used for static delay fallback

// --- SAFETY LIMITS ---
#define MAX_ENTITIES 64
// MAX_SPRITES already defined above (256)

// --- COLOR DEFINITIONS ---
#define COLOR_TRANSPARENT 0x0000  // RGB565 black (assumed transparent)
