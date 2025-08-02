// definitions.h - Hardware definitions for ESP32-C6 1.47" Display Board
#pragma once
#include <stdint.h>

// --- HARDWARE CONFIGURATION ---
// Updated for ESP32-C6-LCD-1.47 actual specifications
#define MAX_BUTTONS 6
#define MAX_PALETTES 4
#define PALETTE_SIZE 256
#define MAX_PARTICLES 64
#define MAX_PHYSICS_QUEUE 128
#define MAX_SHAPES 4
#define MAX_SPRITES 256
#define SPRITE_LUT_SIZE 64
#define MAX_DEPTH_LAYERS 13

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

// Button pin assignments for ESP32-C6 board
static const int BUTTON_PINS[BTN_COUNT] = {
  32, // UP (external buttons via GPIO)
  33, // DOWN
  25, // LEFT
  26, // RIGHT
  27, // A (main action)
  9   // B (BOOT button can be used as secondary)
};

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
#define SCREEN_WIDTH  172    // 172 pixels wide
#define SCREEN_HEIGHT 320    // 320 pixels tall

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
