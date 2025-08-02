// Snake Game Database Configuration
// Ultra-minimal arcade game demonstrating efficient memory usage
#pragma once

#include "../engine/database/partitioned_system.h"

// Ultra-minimal database partition configuration (2.25KB total = 14% of LP-SRAM)
#define ROM_PARTITION_SIZE   512     // 512B ROM - just game constants and settings
#define SAVE_PARTITION_SIZE  768     // 768B save - high scores, player settings
#define BACKUP_PARTITION_SIZE 256    // 256B backup - critical high score backup only
#define RUNTIME_PARTITION_SIZE 768   // 768B runtime - current game state cache

// Ultra-safe memory validation - ensure we don't exceed minimal allocation
static_assert((ROM_PARTITION_SIZE + SAVE_PARTITION_SIZE + 
               BACKUP_PARTITION_SIZE + RUNTIME_PARTITION_SIZE) <= 2304,
              "Snake DB exceeds safe minimal limit of 2.25KB!");

// Memory usage: ROM=512B, Save=768B, Backup=256B, Runtime=768B = 2.25KB total (very conservative)

// Snake-specific namespaces
#define NS_SNAKE_GAME     0x10      // Snake game state and data
#define NS_SNAKE_SCORES   0x11      // High scores and statistics

// Snake-specific categories
#define CAT_HIGH_SCORES   0x01      // High score table
#define CAT_GAME_STATE    0x02      // Current game state
#define CAT_SETTINGS      0x03      // Game settings
#define CAT_SNAKE_DATA    0x01      // Snake segments and position
#define CAT_FOOD_DATA     0x02      // Food position and type

// Snake-specific entry types
#define ENTRY_SNAKE_SEGMENT 0x80    // Snake segment position
#define ENTRY_GAME_STATE    0x81    // Game state structure
#define ENTRY_HIGH_SCORE    0x82    // High score entry

// Key generation macros for Snake data
#define HIGH_SCORE_KEY      MAKE_KEY(NS_SNAKE_SCORES, CAT_HIGH_SCORES, 1)
#define GAME_STATE_KEY      MAKE_KEY(NS_SNAKE_GAME, CAT_GAME_STATE, 1)
#define SNAKE_SEGMENT_KEY(i) MAKE_KEY(NS_SNAKE_GAME, CAT_SNAKE_DATA, i)
#define FOOD_POSITION_KEY   MAKE_KEY(NS_SNAKE_GAME, CAT_FOOD_DATA, 1)
#define SETTINGS_KEY        MAKE_KEY(NS_SNAKE_GAME, CAT_SETTINGS, 1)

// Snake data structures (ultra-compact)
struct SnakeSegment {
    uint8_t x, y;               // Position (fits 20x20 grid)
} __attribute__((packed));

struct GameState {
    uint8_t level;              // Current level
    uint16_t score;             // Current score
    uint8_t snakeLength;        // Snake length
    uint8_t direction;          // Current direction (0=up, 1=right, 2=down, 3=left)
    bool gameActive;            // Game running flag
    bool paused;                // Game paused flag
} __attribute__((packed));

struct FoodPosition {
    uint8_t x, y;               // Food position
    uint8_t type;               // Food type (normal, bonus, etc.)
} __attribute__((packed));

struct GameSettings {
    uint8_t speed;              // Game speed (1-10)
    bool soundEnabled;          // Sound on/off
    uint8_t difficulty;         // Difficulty level
} __attribute__((packed));

// Ultra-minimal configuration for Snake game (2.25KB total)
static const PartitionConfig SNAKE_CONFIG = {
    .romSize = ROM_PARTITION_SIZE,
    .saveSize = SAVE_PARTITION_SIZE,
    .backupSize = BACKUP_PARTITION_SIZE,
    .runtimeSize = RUNTIME_PARTITION_SIZE,
    .enableCompression = false,     // No compression for tiny data
    .enableEncryption = false,      // No encryption needed
    .maxCacheEntries = 4,           // Minimal cache (4 entries)
    .safetyLevel = 1               // Standard bounds checking
};

// Snake-specific convenience macros
#define SNAKE_DB_INIT() database.initialize(&SNAKE_CONFIG)

#define SNAKE_SET_HIGH_SCORE(score) \
    database.setU16(HIGH_SCORE_KEY, score)

#define SNAKE_GET_HIGH_SCORE() \
    database.getU16(HIGH_SCORE_KEY, 0)

#define SNAKE_SET_GAME_STATE(level, score, length, active) do { \
    GameState state = {level, score, length, 0, active, false}; \
    database.set(GAME_STATE_KEY, &state, sizeof(state), ENTRY_GAME_STATE); \
} while(0)

#define SNAKE_GET_GAME_STATE() ({ \
    GameState state = {}; \
    database.get(GAME_STATE_KEY, &state, sizeof(state)); \
    state; \
})

#define SNAKE_SET_SEGMENT(index, x, y) do { \
    SnakeSegment seg = {x, y}; \
    database.set(SNAKE_SEGMENT_KEY(index), &seg, sizeof(seg), ENTRY_SNAKE_SEGMENT); \
} while(0)

#define SNAKE_GET_SEGMENT(index) ({ \
    SnakeSegment seg = {}; \
    database.get(SNAKE_SEGMENT_KEY(index), &seg, sizeof(seg)); \
    seg; \
})

#define SNAKE_SET_FOOD(x, y) do { \
    FoodPosition food = {x, y, 0}; \
    database.set(FOOD_POSITION_KEY, &food, sizeof(food), ENTRY_GAME_STATE); \
} while(0)

#define SNAKE_GET_FOOD() ({ \
    FoodPosition food = {}; \
    database.get(FOOD_POSITION_KEY, &food, sizeof(food)); \
    food; \
})

#define SNAKE_SAVE_SETTINGS(speed, sound, difficulty) do { \
    GameSettings settings = {speed, sound, difficulty}; \
    database.set(SETTINGS_KEY, &settings, sizeof(settings), ENTRY_GAME_STATE); \
} while(0)

#define SNAKE_LOAD_SETTINGS() ({ \
    GameSettings settings = {5, true, 1}; /* defaults */ \
    database.get(SETTINGS_KEY, &settings, sizeof(settings)); \
    settings; \
})

// Snake game constants
#define DIRECTION_UP    0
#define DIRECTION_RIGHT 1
#define DIRECTION_DOWN  2
#define DIRECTION_LEFT  3

#define FOOD_TYPE_NORMAL 0
#define FOOD_TYPE_BONUS  1
#define FOOD_TYPE_POWER  2

#define MAX_SNAKE_LENGTH 50
#define GRID_SIZE 20

// Memory usage analysis for Snake game:
// ROM: ~200B (game constants, default settings)
// Save: ~400B (high scores + game settings + current state)
// Backup: ~100B (just high score backup)
// Runtime: ~300B (snake segments + food position + cache)
// Total: ~1KB actual usage out of 2.25KB allocated = extremely efficient

// This configuration demonstrates that even very simple games can benefit
// from the database system while using minimal memory (14% of LP-SRAM)
