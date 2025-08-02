// Pokemon RPG Database Configuration
// Complex RPG game with extensive data needs (memory optimized)
#pragma once

#include "../engine/database/partitioned_system.h"

// Pokemon-specific safe database partition configuration (13.75KB total, 2.25KB safety margin)
#define ROM_PARTITION_SIZE   4096    // 4KB ROM - Pokemon species, moves, items (compressed)
#define SAVE_PARTITION_SIZE  4096    // 4KB save - trainers, captured Pokemon, progress
#define BACKUP_PARTITION_SIZE 2048   // 2KB backup - critical save data backup
#define RUNTIME_PARTITION_SIZE 3840  // 3.75KB runtime - battle cache, temporary data

// Memory usage validation at compile time
static_assert((ROM_PARTITION_SIZE + SAVE_PARTITION_SIZE + 
               BACKUP_PARTITION_SIZE + RUNTIME_PARTITION_SIZE) <= 14336,
              "Pokemon DB config exceeds safe 14KB limit!");

// Memory usage: ROM=4KB, Save=4KB, Backup=2KB, Runtime=3.75KB = 13.75KB total (86% of LP-SRAM)

// Pokemon-specific namespaces
#define NS_POKEMON_DATA    0x10     // Pokemon species data
#define NS_POKEMON_PLAYER  0x11     // Player's Pokemon party/storage
#define NS_POKEMON_BATTLE  0x12     // Battle mechanics and state
#define NS_POKEMON_WORLD   0x13     // World state, NPCs, locations

// Pokemon-specific categories  
#define CAT_SPECIES        0x01     // Pokemon species definitions
#define CAT_MOVES          0x02     // Move definitions and data
#define CAT_ITEMS          0x03     // Item definitions
#define CAT_PARTY          0x01     // Player's active party
#define CAT_PC_STORAGE     0x02     // PC storage boxes
#define CAT_TRAINER_DATA   0x03     // Trainer information
#define CAT_BATTLE_STATE   0x01     // Current battle state
#define CAT_BATTLE_CACHE   0x02     // Battle calculations cache

// Pokemon-specific entry types
#define ENTRY_POKEMON      0x80     // Pokemon instance data
#define ENTRY_MOVE         0x81     // Move data structure
#define ENTRY_TRAINER      0x82     // Trainer data structure
#define ENTRY_SPECIES      0x83     // Pokemon species definition

// Key generation macros for Pokemon data
#define SPECIES_KEY(id)         MAKE_KEY(NS_POKEMON_DATA, CAT_SPECIES, id)
#define MOVE_KEY(id)            MAKE_KEY(NS_POKEMON_DATA, CAT_MOVES, id)
#define ITEM_KEY(id)            MAKE_KEY(NS_POKEMON_DATA, CAT_ITEMS, id)
#define TRAINER_KEY(id)         MAKE_KEY(NS_POKEMON_PLAYER, CAT_TRAINER_DATA, id)
#define PARTY_POKEMON_KEY(slot) MAKE_KEY(NS_POKEMON_PLAYER, CAT_PARTY, slot)
#define PC_POKEMON_KEY(box, slot) MAKE_KEY(NS_POKEMON_PLAYER, CAT_PC_STORAGE, ((box << 8) | slot))
#define BATTLE_STATE_KEY        MAKE_KEY(NS_POKEMON_BATTLE, CAT_BATTLE_STATE, 1)

// Pokemon data structures (memory optimized)
struct PokemonSpecies {
    uint8_t id;                 // Species ID (1-255)
    uint8_t type1, type2;       // Pokemon types
    uint8_t baseStats[6];       // HP, Att, Def, SpA, SpD, Spe
    uint16_t baseExp;           // Base experience yield
    uint8_t nameOffset;         // Offset in string table
} __attribute__((packed));

struct PokemonInstance {
    uint8_t speciesId;          // Which species
    uint8_t level;              // Current level (1-100)
    uint16_t currentHP;         // Current HP
    uint16_t experience;        // Current EXP
    uint8_t moves[4];           // Move IDs
    uint8_t ivs[6];             // Individual values
    uint8_t nature;             // Nature ID
    char nickname[12];          // Pokemon nickname
} __attribute__((packed));

struct TrainerData {
    uint8_t id;                 // Trainer ID
    char name[16];              // Trainer name
    uint8_t badges;             // Badge count
    uint8_t partyCount;         // Pokemon in party
    uint32_t money;             // Current money
    uint8_t partySlots[6];      // Party Pokemon slots
} __attribute__((packed));

struct MoveData {
    uint8_t id;                 // Move ID
    uint8_t type;               // Move type
    uint8_t power;              // Base power
    uint8_t accuracy;           // Accuracy (0-100)
    uint8_t pp;                 // Power points
    uint8_t category;           // Physical/Special/Status
    uint16_t effect;            // Effect flags
} __attribute__((packed));

// Pokemon configuration with compression and safety focus
static const PartitionConfig POKEMON_CONFIG = {
    .romSize = ROM_PARTITION_SIZE,
    .saveSize = SAVE_PARTITION_SIZE,
    .backupSize = BACKUP_PARTITION_SIZE,
    .runtimeSize = RUNTIME_PARTITION_SIZE,
    .enableCompression = true,      // Essential for fitting data in 4KB ROM
    .enableEncryption = false,      // Skip encryption to save space and performance
    .maxCacheEntries = 20,          // Conservative cache size for battle data
    .safetyLevel = 1               // Standard bounds checking
};

// Pokemon-specific convenience macros
#define POKEMON_DB_INIT() database.initialize(&POKEMON_CONFIG)

#define POKEMON_DEFINE_SPECIES(id, name, type1, hp, att, def, spa, spd, spe) do { \
    PokemonSpecies species = {id, type1, 0, {hp, att, def, spa, spd, spe}, 100, 0}; \
    database.set(SPECIES_KEY(id), &species, sizeof(species), ENTRY_SPECIES); \
} while(0)

#define POKEMON_CAPTURE(slot, pokemon) \
    database.set(PARTY_POKEMON_KEY(slot), &pokemon, sizeof(pokemon), ENTRY_POKEMON)

#define POKEMON_GET_SPECIES(id) ({ \
    PokemonSpecies species = {}; \
    database.get(SPECIES_KEY(id), &species, sizeof(species)); \
    species; \
})

#define POKEMON_GET_CAPTURED(slot) ({ \
    PokemonInstance pokemon = {}; \
    database.get(PARTY_POKEMON_KEY(slot), &pokemon, sizeof(pokemon)); \
    pokemon; \
})

#define POKEMON_GET_TRAINER(id) ({ \
    TrainerData trainer = {}; \
    database.get(TRAINER_KEY(id), &trainer, sizeof(trainer)); \
    trainer; \
})

#define POKEMON_SET_TRAINER_MONEY(id, amount) \
    database.setU32(MAKE_KEY(NS_POKEMON_PLAYER, CAT_TRAINER_DATA, (id << 8) | 0x01), amount)

#define POKEMON_GET_TRAINER_MONEY(id) \
    database.getU32(MAKE_KEY(NS_POKEMON_PLAYER, CAT_TRAINER_DATA, (id << 8) | 0x01), 0)

// Pokemon type constants
#define TYPE_NORMAL     1
#define TYPE_FIRE       2
#define TYPE_WATER      3
#define TYPE_ELECTRIC   4
#define TYPE_GRASS      5
#define TYPE_ICE        6
#define TYPE_FIGHTING   7
#define TYPE_POISON     8
#define TYPE_GROUND     9
#define TYPE_FLYING     10
#define TYPE_PSYCHIC    11
#define TYPE_BUG        12
#define TYPE_ROCK       13
#define TYPE_GHOST      14
#define TYPE_DRAGON     15
#define TYPE_DARK       16
#define TYPE_STEEL      17
#define TYPE_FAIRY      18

// Memory usage analysis for Pokemon app:
// ROM: ~3.5KB (150 species * 8 bytes + 50 moves * 8 bytes + compression)
// Save: ~2KB (trainer data + 6 party Pokemon + progress flags)
// Backup: ~1KB (critical trainer data backup)
// Runtime: ~2KB (battle state + move cache + calculations)
// Total: ~8.5KB out of 13.75KB allocated = efficient use with room for expansion
