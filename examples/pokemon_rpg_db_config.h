// Pokemon RPG App Database Configuration - Memory Safe Edition
// Optimized for ESP32-C6 16KB LP-SRAM with safety margins
#pragma once

// App-specific database partition configuration (total: 14KB, leaves 2KB safety margin)
#define WISP_DB_ROM_PARTITION_SIZE   4096    // 4KB ROM - Pokemon/item data (compressed)
#define WISP_DB_SAVE_PARTITION_SIZE  4096    // 4KB save - player progress  
#define WISP_DB_BACKUP_PARTITION_SIZE 2048   // 2KB backup save
#define WISP_DB_RUNTIME_PARTITION_SIZE 3840  // 3.75KB runtime cache

// Safety validation at compile time
static_assert((WISP_DB_ROM_PARTITION_SIZE + WISP_DB_SAVE_PARTITION_SIZE + 
               WISP_DB_BACKUP_PARTITION_SIZE + WISP_DB_RUNTIME_PARTITION_SIZE) <= 14336,
              "Pokemon DB exceeds safe memory limit!");

// Memory usage: ROM=4KB, Save=4KB, Backup=2KB, Runtime=3.75KB = 13.75KB total

// App-specific namespaces
#define NS_POKEMON_DATA    0x10     // Pokemon species data
#define NS_POKEMON_PLAYER  0x11     // Player's Pokemon party
#define NS_POKEMON_STORAGE 0x12     // Pokemon storage system
#define NS_BATTLE_DATA     0x13     // Battle mechanics data

// App-specific categories  
#define CAT_SPECIES        0x01     // Pokemon species definitions
#define CAT_MOVES          0x02     // Move definitions
#define CAT_TYPES          0x03     // Type effectiveness data
#define CAT_PARTY          0x01     // Player's active party
#define CAT_PC_BOXES       0x02     // PC storage boxes
#define CAT_BATTLE_STATE   0x01     // Current battle state

// Pokemon-specific entry types
#define ENTRY_POKEMON      0x80     // Pokemon data structure
#define ENTRY_MOVE         0x81     // Move data structure
#define ENTRY_TRAINER      0x82     // Trainer data structure

// Pokemon-specific flags
#define FLAG_SHINY         0x40     // Pokemon is shiny
#define FLAG_TRADED        0x80     // Pokemon was traded

// Key generation macros for Pokemon data
#define POKEMON_SPECIES_KEY(id)    WISP_KEY_MAKE(NS_POKEMON_DATA, CAT_SPECIES, id)
#define POKEMON_MOVE_KEY(id)       WISP_KEY_MAKE(NS_POKEMON_DATA, CAT_MOVES, id)
#define POKEMON_PARTY_KEY(slot)    WISP_KEY_MAKE(NS_POKEMON_PLAYER, CAT_PARTY, slot)
#define POKEMON_PC_KEY(box, slot)  WISP_KEY_MAKE(NS_POKEMON_STORAGE, CAT_PC_BOXES, (box << 8) | slot)

// Pokemon data structures
struct PokemonSpecies {
    uint16_t id;
    uint8_t type1, type2;
    uint16_t baseHP, baseAttack, baseDefense;
    uint16_t baseSpAttack, baseSpDefense, baseSpeed;
    uint8_t catchRate;
    uint8_t expGroup;
    uint16_t nameOffset;        // Offset in string table
    uint16_t descOffset;        // Description offset
} __attribute__((packed));

struct PokemonInstance {
    uint16_t species;
    uint8_t level;
    uint32_t experience;
    uint16_t currentHP;
    uint8_t ivHP, ivAttack, ivDefense, ivSpAttack, ivSpDefense, ivSpeed;
    uint8_t nature;
    uint8_t ability;
    uint16_t moves[4];          // Move IDs
    uint8_t movePP[4];          // Current PP for each move
    uint32_t personality;       // For shiny, gender, etc.
    uint8_t friendship;
    uint8_t statusCondition;
    uint16_t metLocation;
    uint8_t metLevel;
    uint8_t pokeball;
} __attribute__((packed));

struct PokemonMove {
    uint16_t id;
    uint8_t type;
    uint8_t power;
    uint8_t accuracy;
    uint8_t pp;
    uint8_t priority;
    uint8_t target;
    uint16_t effect;
    uint16_t nameOffset;
    uint16_t descOffset;
} __attribute__((packed));

// Configuration for this specific app (memory-safe)
static const WispPartitionConfig POKEMON_DB_CONFIG = {
    .romSize = WISP_DB_ROM_PARTITION_SIZE,
    .saveSize = WISP_DB_SAVE_PARTITION_SIZE,
    .backupSize = WISP_DB_BACKUP_PARTITION_SIZE,
    .runtimeSize = WISP_DB_RUNTIME_PARTITION_SIZE,
    .enableCompression = true,      // Essential for fitting in 4KB ROM
    .enableEncryption = false,      // Skip encryption to save space
    .maxCacheEntries = 20,          // Conservative cache (20 entries max)
    .safetyLevel = 1               // Standard bounds checking
};

// App-specific convenience macros
#define POKEMON_DB_INIT() wispDB.initialize(&POKEMON_DB_CONFIG)

#define GET_POKEMON_SPECIES(id) ({ \
    PokemonSpecies species; \
    wispDB.get(POKEMON_SPECIES_KEY(id), &species, sizeof(species)); \
    species; \
})

#define SET_POKEMON_PARTY(slot, pokemon) \
    wispDB.set(POKEMON_PARTY_KEY(slot), &pokemon, sizeof(PokemonInstance), ENTRY_POKEMON)

#define GET_POKEMON_PARTY(slot) ({ \
    PokemonInstance pokemon = {}; \
    wispDB.get(POKEMON_PARTY_KEY(slot), &pokemon, sizeof(pokemon)); \
    pokemon; \
})

#define HAS_POKEMON_IN_PARTY(slot) \
    wispDB.has(POKEMON_PARTY_KEY(slot))

#define STORE_POKEMON_PC(box, slot, pokemon) \
    wispDB.set(POKEMON_PC_KEY(box, slot), &pokemon, sizeof(PokemonInstance), ENTRY_POKEMON)

#define GET_POKEMON_PC(box, slot) ({ \
    PokemonInstance pokemon = {}; \
    wispDB.get(POKEMON_PC_KEY(box, slot), &pokemon, sizeof(pokemon)); \
    pokemon; \
})

// ROM data structure for preloaded Pokemon data
struct PokemonROMHeader {
    uint32_t magic;             // 'PKMN'
    uint16_t version;           // ROM version
    uint16_t speciesCount;      // Number of Pokemon species
    uint16_t movesCount;        // Number of moves
    uint16_t stringTableOffset; // Offset to string table
    uint16_t stringTableSize;   // Size of string table
    uint32_t checksum;          // ROM data checksum
} __attribute__((packed));

// Easy access to common Pokemon data
#define POKEMON_BULBASAUR   1
#define POKEMON_IVYSAUR     2  
#define POKEMON_VENUSAUR    3
#define POKEMON_CHARMANDER  4
#define POKEMON_CHARMELEON  5
#define POKEMON_CHARIZARD   6
#define POKEMON_SQUIRTLE    7
#define POKEMON_WARTORTLE   8
#define POKEMON_BLASTOISE   9
#define POKEMON_PIKACHU     25
#define POKEMON_RAICHU      26

// Move IDs
#define MOVE_TACKLE         1
#define MOVE_GROWL          2
#define MOVE_VINE_WHIP      3
#define MOVE_EMBER          4
#define MOVE_WATER_GUN      5
#define MOVE_THUNDERBOLT    6

// Type IDs
#define TYPE_NORMAL         1
#define TYPE_FIGHTING       2
#define TYPE_FLYING         3
#define TYPE_POISON         4
#define TYPE_GROUND         5
#define TYPE_ROCK           6
#define TYPE_BUG            7
#define TYPE_GHOST          8
#define TYPE_STEEL          9
#define TYPE_FIRE           10
#define TYPE_WATER          11
#define TYPE_GRASS          12
#define TYPE_ELECTRIC       13
#define TYPE_PSYCHIC        14
#define TYPE_ICE            15
#define TYPE_DRAGON         16
#define TYPE_DARK           17
