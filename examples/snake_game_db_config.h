// Simple Snake Game Database Configuration - Ultra-Safe Edition
// Demonstrates minimal viable database usage within safety limits
#pragma once

// Ultra-minimal database partition configuration (total: 2.25KB = 14% of LP-SRAM)
#define WISP_DB_ROM_PARTITION_SIZE   512     // 512B ROM - just constants
#define WISP_DB_SAVE_PARTITION_SIZE  768     // 768B save - scores & settings
#define WISP_DB_BACKUP_PARTITION_SIZE 256    // 256B backup - critical data only
#define WISP_DB_RUNTIME_PARTITION_SIZE 768   // 768B runtime - current state

// Safety validation - ensure we don't exceed minimal allocation
static_assert((WISP_DB_ROM_PARTITION_SIZE + WISP_DB_SAVE_PARTITION_SIZE + 
               WISP_DB_BACKUP_PARTITION_SIZE + WISP_DB_RUNTIME_PARTITION_SIZE) <= 2304,
              "Snake DB exceeds safe minimal limit of 2.25KB!");

// Total memory usage: 2.25KB out of 16KB = very conservative

// Snake-specific namespaces
#define NS_SNAKE_GAME     0x10      // Snake game data
#define NS_SNAKE_SCORES   0x11      // High scores

// Snake-specific categories
#define CAT_HIGH_SCORES   0x01      // High score table
#define CAT_GAME_STATE    0x02      // Current game state
#define CAT_SETTINGS      0x03      // Game settings

// Snake game keys
#define SNAKE_HIGH_SCORE_KEY(rank)    WISP_KEY_MAKE(NS_SNAKE_SCORES, CAT_HIGH_SCORES, rank)
#define SNAKE_CURRENT_SCORE_KEY       WISP_KEY_MAKE(NS_SNAKE_GAME, CAT_GAME_STATE, 1)
#define SNAKE_CURRENT_LEVEL_KEY       WISP_KEY_MAKE(NS_SNAKE_GAME, CAT_GAME_STATE, 2)
#define SNAKE_GAME_SPEED_KEY          WISP_KEY_MAKE(NS_SNAKE_GAME, CAT_SETTINGS, 1)
#define SNAKE_SOUND_ENABLED_KEY       WISP_KEY_MAKE(NS_SNAKE_GAME, CAT_SETTINGS, 2)

// Snake game data structures
struct SnakeHighScore {
    uint32_t score;
    uint16_t level;
    uint8_t nameLength;
    char name[8];               // Player name (7 chars + null)
} __attribute__((packed));

struct SnakeGameState {
    uint32_t currentScore;
    uint16_t currentLevel;
    uint8_t snakeLength;
    uint8_t direction;
    uint16_t snakeBody[100];    // Snake segments (x,y packed into 16-bit)
    uint16_t foodPosition;      // Food position (x,y packed)
    bool gameActive;
} __attribute__((packed));

// Ultra-minimal configuration for Snake game (2.25KB total)
static const WispPartitionConfig SNAKE_DB_CONFIG = {
    .romSize = WISP_DB_ROM_PARTITION_SIZE,
    .saveSize = WISP_DB_SAVE_PARTITION_SIZE,
    .backupSize = WISP_DB_BACKUP_PARTITION_SIZE,
    .runtimeSize = WISP_DB_RUNTIME_PARTITION_SIZE,
    .enableCompression = false,     // No compression for tiny data
    .enableEncryption = false,      // No encryption needed
    .maxCacheEntries = 4,           // Minimal cache (4 entries)
    .safetyLevel = 1               // Standard bounds checking
};

// Snake-specific convenience macros
#define SNAKE_DB_INIT() wispDB.initialize(&SNAKE_DB_CONFIG)

#define SNAKE_SET_HIGH_SCORE(rank, score, level, name) do { \
    SnakeHighScore hs = {score, level, strlen(name)}; \
    strncpy(hs.name, name, 7); \
    hs.name[7] = '\0'; \
    wispDB.set(SNAKE_HIGH_SCORE_KEY(rank), &hs, sizeof(hs), ENTRY_STRUCT); \
} while(0)

#define SNAKE_GET_HIGH_SCORE(rank) ({ \
    SnakeHighScore hs = {}; \
    wispDB.get(SNAKE_HIGH_SCORE_KEY(rank), &hs, sizeof(hs)); \
    hs; \
})

#define SNAKE_SAVE_GAME_STATE(gameState) \
    wispDB.set(WISP_KEY_MAKE(NS_SNAKE_GAME, CAT_GAME_STATE, 0), &gameState, sizeof(gameState), ENTRY_STRUCT)

#define SNAKE_LOAD_GAME_STATE() ({ \
    SnakeGameState state = {}; \
    wispDB.get(WISP_KEY_MAKE(NS_SNAKE_GAME, CAT_GAME_STATE, 0), &state, sizeof(state)); \
    state; \
})

#define SNAKE_SET_SETTING(key, value) wispDB.setU8(key, value)
#define SNAKE_GET_SETTING(key, defaultVal) wispDB.getU8(key, defaultVal)

// ROM data for Snake game (minimal - just default high scores)
const uint8_t SNAKE_ROM_DATA[] = {
    // ROM header
    0x53, 0x4E, 0x4B, 0x01,  // 'SNK' + version 1
    0x05, 0x00,              // 5 high score entries
    0x00, 0x00,              // No other ROM data
    
    // Default high scores (5 entries)
    // High score 1: 1000 points, level 5, "PLAYER1"
    0xE8, 0x03, 0x00, 0x00,  // score: 1000
    0x05, 0x00,              // level: 5
    0x07,                    // name length: 7
    'P', 'L', 'A', 'Y', 'E', 'R', '1', 0x00,
    
    // High score 2: 800 points, level 4, "PLAYER2"
    0x20, 0x03, 0x00, 0x00,  // score: 800
    0x04, 0x00,              // level: 4
    0x07,                    // name length: 7  
    'P', 'L', 'A', 'Y', 'E', 'R', '2', 0x00,
    
    // High score 3: 600 points, level 3, "PLAYER3"
    0x58, 0x02, 0x00, 0x00,  // score: 600
    0x03, 0x00,              // level: 3
    0x07,                    // name length: 7
    'P', 'L', 'A', 'Y', 'E', 'R', '3', 0x00,
    
    // High score 4: 400 points, level 2, "PLAYER4"
    0x90, 0x01, 0x00, 0x00,  // score: 400
    0x02, 0x00,              // level: 2
    0x07,                    // name length: 7
    'P', 'L', 'A', 'Y', 'E', 'R', '4', 0x00,
    
    // High score 5: 200 points, level 1, "PLAYER5"
    0xC8, 0x00, 0x00, 0x00,  // score: 200
    0x01, 0x00,              // level: 1
    0x07,                    // name length: 7
    'P', 'L', 'A', 'Y', 'E', 'R', '5', 0x00,
};

// Total memory usage for Snake game:
// ROM: ~150 bytes (high score defaults)
// Save: ~250 bytes (game state + current scores)
// Backup: ~100 bytes (critical save backup)
// Runtime: ~100 bytes (minimal cache)
// Total: ~600 bytes out of 16KB = 96% free!
