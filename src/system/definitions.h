// definitions.h - Hardware definitions for ESP32-C6 1.47" Display Board
#pragma once
#include <stdint.h>

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
#ifndef MAX_SPRITES
#define MAX_SPRITES 256
#endif
#ifndef SPRITE_LUT_SIZE
#define SPRITE_LUT_SIZE 64
#endif
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
#define BUTTON_BOOT_PIN   9   // Built-in BOOT button
#define BUTTON_RESET_PIN  0   // Built-in RESET button

// --- AUDIO CONFIGURATION ---
// Verified for ESP32-C6-LCD-1.47 board
#define AUDIO_PIEZO_PIN 21

// I2S DAC pins for external audio (optional)
#define AUDIO_I2S_BCLK  18
#define AUDIO_I2S_LRC   19  
#define AUDIO_I2S_DIN   22

// --- SCREEN SPECIFICATIONS ---
// Updated for actual 1.47" display: 172×320 portrait
// Protect against redefinition - board configs may define these
#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH  172    // 172 pixels wide
#endif
#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 320    // 320 pixels tall
#endif

// --- C++ APP CONFIGURATION ---
#define APP_MAIN_FUNCTION "main"  // C++ applications use main() function

// --- RENDER ENGINE DEFAULTS ---
static uint8_t DUMMY_HEIGHT_MAP[SCREEN_WIDTH] = {0};

// --- SYSTEM TIMING CONFIGURATION ---
#define SYSTEM_FPS 24    // Engine heartbeat/frame pacing (not app speed)
#define FRAME_TIME_MS 16 // Legacy 60 FPS cap, can be deprecated or used for static delay fallback

// --- SAFETY LIMITS ---
#define MAX_ENTITIES 64
// MAX_SPRITES already defined above (256)

// --- COLOR DEFINITIONS ---
#define COLOR_TRANSPARENT 0x0000  // RGB565 black (assumed transparent)
