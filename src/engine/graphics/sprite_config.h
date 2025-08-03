// sprite_system_config.h - ESP32-C6/S3 Sprite Configuration using ESP-IDF
// Native ESP32 memory configuration for game development (Game Boy/GBA inspired)
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers

// ESP32-C6 Memory Configuration for Game Development
// Inspired by Game Boy / Game Boy Advance architecture
// Choose configuration based on your game's needs

// Memory profile selection
enum MemoryProfile {
    PROFILE_MINIMAL,     // Maximum memory for game logic (like Game Boy)
    PROFILE_BALANCED,    // Good balance of features and memory (like GBA)
    PROFILE_FULL         // Maximum visual features (modern indie games)
};

// Set your desired profile here
#ifndef WISP_MEMORY_PROFILE
#define WISP_MEMORY_PROFILE PROFILE_BALANCED
#endif

// ESP32-C6 Hardware Constants
#define ESP32_C6_HP_SRAM_TOTAL   (512 * 1024)  // 512KB total HP SRAM
#define ESP32_C6_LP_SRAM_TOTAL   (16 * 1024)   // 16KB LP SRAM (sleep data only)
#define ESP32_C6_FLASH_TOTAL     (4 * 1024 * 1024)  // 4MB Flash
#define DISPLAY_WIDTH            172
#define DISPLAY_HEIGHT           320
#define DISPLAY_PIXELS           (DISPLAY_WIDTH * DISPLAY_HEIGHT)  // 55,040 pixels

// System overhead estimates (conservative)
#define ESP_IDF_FRAMEWORK_KB     40    // ESP-IDF core, heap management
#define WIFI_STACK_KB           40    // WiFi 6 stack when active
#define BLUETOOTH_STACK_KB      20    // BLE 5 stack when active  
#define SYSTEM_BUFFERS_KB       16    // Serial, interrupts, etc.
#define SAFETY_MARGIN_KB        16    // Emergency reserve

// Profile Configurations
#if WISP_MEMORY_PROFILE == PROFILE_MINIMAL
    // Like original Game Boy: Minimal graphics, maximum game logic
    #define GRAPHICS_MODE           "TILE_BASED"
    #define MAX_SPRITES_ACTIVE      32
    #define SPRITE_LAYERS           4
    #define FRAMEBUFFER_MODE        "NONE"         // Tile-based rendering
    #define DEPTH_BUFFER_ENABLED    false
    #define COLOR_LUT_SIZE          16             // 16 colors per palette
    #define ENABLE_WIFI            false          // Save 40KB
    #define ENABLE_BLUETOOTH       false          // Save 20KB
    #define AUDIO_CHANNELS         2
    #define AUDIO_BUFFER_KB        4
    
    // Memory allocation
    #define SYSTEM_OVERHEAD_KB     (ESP_IDF_FRAMEWORK_KB + SYSTEM_BUFFERS_KB + SAFETY_MARGIN_KB)
    #define PALETTE_MEMORY_KB      1               // 16 colors × 4 palettes = 128 bytes
    #define GRAPHICS_MEMORY_KB     (8 + PALETTE_MEMORY_KB)  // Tile buffers + tiny palettes
    #define SPRITE_MEMORY_KB       4               // Minimal sprite management
    #define AUDIO_MEMORY_KB        AUDIO_BUFFER_KB
    #define GAME_LOGIC_MEMORY_KB   (512 - SYSTEM_OVERHEAD_KB - GRAPHICS_MEMORY_KB - SPRITE_MEMORY_KB - AUDIO_MEMORY_KB)
    
    // Game Boy-like constraints
    #define TILE_SIZE              16
    #define TILES_X                (DISPLAY_WIDTH / TILE_SIZE + 1)   // 11 tiles
    #define TILES_Y                (DISPLAY_HEIGHT / TILE_SIZE + 1)  // 21 tiles
    
#elif WISP_MEMORY_PROFILE == PROFILE_BALANCED
    // Like Game Boy Advance: Good balance of features and memory
    #define GRAPHICS_MODE           "PARTIAL_FRAMEBUFFER"
    #define MAX_SPRITES_ACTIVE      64
    #define SPRITE_LAYERS           6
    #define FRAMEBUFFER_MODE        "STRIP"        // 32-line strips
    #define DEPTH_BUFFER_ENABLED    false
    #define COLOR_LUT_SIZE          64             // 64 colors per palette
    #define ENABLE_WIFI            true           // Optional
    #define ENABLE_BLUETOOTH       false          // Save 20KB
    #define AUDIO_CHANNELS         4
    #define AUDIO_BUFFER_KB        8
    
    // Memory allocation
    #define WIFI_MEMORY_KB         (ENABLE_WIFI ? WIFI_STACK_KB : 0)
    #define SYSTEM_OVERHEAD_KB     (ESP_IDF_FRAMEWORK_KB + SYSTEM_BUFFERS_KB + WIFI_MEMORY_KB + SAFETY_MARGIN_KB)
    #define STRIP_BUFFER_KB        (DISPLAY_WIDTH * 32 * 2 / 1024)  // ~22KB for 32-line strips
    #define PALETTE_MEMORY_KB      1               // 64 colors × 4 palettes = 512 bytes
    #define GRAPHICS_MEMORY_KB     (STRIP_BUFFER_KB + PALETTE_MEMORY_KB)  // Strip buffer + tiny palettes
    #define SPRITE_MEMORY_KB       12              // Moderate sprite management
    #define AUDIO_MEMORY_KB        AUDIO_BUFFER_KB
    #define GAME_LOGIC_MEMORY_KB   (512 - SYSTEM_OVERHEAD_KB - GRAPHICS_MEMORY_KB - SPRITE_MEMORY_KB - AUDIO_MEMORY_KB)
    
    // Strip rendering parameters
    #define STRIP_HEIGHT           32
    #define STRIPS_TOTAL           (DISPLAY_HEIGHT / STRIP_HEIGHT + 1)
    
#elif WISP_MEMORY_PROFILE == PROFILE_FULL
    // Modern indie game: Full features, less game logic memory
    #define GRAPHICS_MODE           "FULL_FRAMEBUFFER"
    #define MAX_SPRITES_ACTIVE      128
    #define SPRITE_LAYERS           8
    #define FRAMEBUFFER_MODE        "DOUBLE"       // Double buffering
    #define DEPTH_BUFFER_ENABLED    true
    #define COLOR_LUT_SIZE          256            // 256 colors per palette  
    #define ENABLE_WIFI            true
    #define ENABLE_BLUETOOTH       true
    #define AUDIO_CHANNELS         6
    #define AUDIO_BUFFER_KB        16
    
    // Memory allocation
    #define WIFI_MEMORY_KB         (ENABLE_WIFI ? WIFI_STACK_KB : 0)
    #define BT_MEMORY_KB           (ENABLE_BLUETOOTH ? BLUETOOTH_STACK_KB : 0)
    #define SYSTEM_OVERHEAD_KB     (ARDUINO_FRAMEWORK_KB + SYSTEM_BUFFERS_KB + WIFI_MEMORY_KB + BT_MEMORY_KB + SAFETY_MARGIN_KB)
    #define FRAMEBUFFER_KB         (DISPLAY_PIXELS * 2 / 1024)      // ~108KB RGB565
    #define DEPTH_BUFFER_KB        (DEPTH_BUFFER_ENABLED ? (DISPLAY_PIXELS / 1024) : 0)  // ~54KB
    #define PALETTE_MEMORY_KB      2               // 256 colors × 4 palettes = 2KB (vs 32KB LUT!)
    #define GRAPHICS_MEMORY_KB     (FRAMEBUFFER_KB + DEPTH_BUFFER_KB + PALETTE_MEMORY_KB)
    #define SPRITE_MEMORY_KB       24              // Full sprite management
    #define AUDIO_MEMORY_KB        AUDIO_BUFFER_KB
    #define GAME_LOGIC_MEMORY_KB   (512 - SYSTEM_OVERHEAD_KB - GRAPHICS_MEMORY_KB - SPRITE_MEMORY_KB - AUDIO_MEMORY_KB)
    
#endif

// Derived constants
#define LAYER_COUNT            SPRITE_LAYERS
#define MAX_SPRITES_PER_LAYER  (MAX_SPRITES_ACTIVE / SPRITE_LAYERS)

// Memory usage validation at compile time
#define TOTAL_USED_MEMORY_KB   (SYSTEM_OVERHEAD_KB + GRAPHICS_MEMORY_KB + SPRITE_MEMORY_KB + AUDIO_MEMORY_KB)

#if TOTAL_USED_MEMORY_KB > 512
#error "Memory configuration exceeds ESP32-C6 HP SRAM capacity!"
#endif

#if GAME_LOGIC_MEMORY_KB < 32
#warning "Very little memory available for game logic - consider PROFILE_MINIMAL"
#endif

// Configuration summary for debugging
static inline void printMemoryConfig() {
    Serial.println("=== WISP Engine Memory Configuration ===");
    Serial.print("Profile: ");
    #if WISP_MEMORY_PROFILE == PROFILE_MINIMAL
    Serial.println("MINIMAL (Game Boy style)");
    #elif WISP_MEMORY_PROFILE == PROFILE_BALANCED  
    Serial.println("BALANCED (Game Boy Advance style)");
    #elif WISP_MEMORY_PROFILE == PROFILE_FULL
    Serial.println("FULL (Modern indie game style)");
    #endif
    
    Serial.print("Graphics mode: ");
    Serial.println(GRAPHICS_MODE);
    Serial.print("Max sprites: ");
    Serial.println(MAX_SPRITES_ACTIVE);
    Serial.print("Sprite layers: ");
    Serial.println(SPRITE_LAYERS);
    
    Serial.println("\n--- Memory Allocation (KB) ---");
    Serial.print("System overhead: ");
    Serial.println(SYSTEM_OVERHEAD_KB);
    Serial.print("Graphics engine: ");
    Serial.println(GRAPHICS_MEMORY_KB);
    Serial.print("Sprite system: ");
    Serial.println(SPRITE_MEMORY_KB);
    Serial.print("Audio system: ");
    Serial.println(AUDIO_MEMORY_KB);
    Serial.print("Game logic: ");
    Serial.println(GAME_LOGIC_MEMORY_KB);
    Serial.print("TOTAL USED: ");
    Serial.print(TOTAL_USED_MEMORY_KB);
    Serial.println(" / 512 KB");
    
    #if GAME_LOGIC_MEMORY_KB > 200
    Serial.println("✓ Plenty of memory for complex games");
    #elif GAME_LOGIC_MEMORY_KB > 100
    Serial.println("✓ Good memory for most games");
    #elif GAME_LOGIC_MEMORY_KB > 50
    Serial.println("⚠ Limited memory - keep games simple");
    #else
    Serial.println("⚠ Very tight memory - minimal games only");
    #endif
    
    Serial.println("========================================");
}

// Feature availability macros
#define HAS_FULL_FRAMEBUFFER   (WISP_MEMORY_PROFILE == PROFILE_FULL)
#define HAS_DEPTH_BUFFER       (DEPTH_BUFFER_ENABLED)
#define HAS_WIFI               (ENABLE_WIFI)
#define HAS_BLUETOOTH          (ENABLE_BLUETOOTH)
#define IS_MEMORY_CONSTRAINED  (GAME_LOGIC_MEMORY_KB < 100)

// Layer definitions based on profile
enum LayerType {
    LAYER_BACKGROUND = 0,
    LAYER_GAME_BG,
    LAYER_GAME_MAIN,
    #if SPRITE_LAYERS > 3
    LAYER_GAME_FG,
    #endif
    #if SPRITE_LAYERS > 4
    LAYER_PARTICLES,
    LAYER_UI_BG,
    #endif
    #if SPRITE_LAYERS > 6
    LAYER_UI_MAIN,
    LAYER_UI_TEXT,
    #endif
    LAYER_COUNT_ENUM = SPRITE_LAYERS
};
