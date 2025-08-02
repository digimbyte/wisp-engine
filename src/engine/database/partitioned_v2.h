// Wisp Engine Partitioned Database System V2 - ESP32-C6/S3 using ESP-IDF
// Advanced partitioned database with ROM preloading optimized for ESP32 LP-SRAM
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include <esp_attr.h>
#include <vector>
#include <unordered_map>

// Database configuration
#define WISP_DB_VERSION 2
#define WISP_DB_LP_SRAM_SIZE 16384   // Full 16KB LP-SRAM

// Partition configuration (16KB total)
#define WISP_DB_ROM_PARTITION_SIZE   6144    // 6KB ROM data (read-only)
#define WISP_DB_SAVE_PARTITION_SIZE  4096    // 4KB save data (read-write)
#define WISP_DB_BACKUP_PARTITION_SIZE 2048   // 2KB backup save
#define WISP_DB_RUNTIME_PARTITION_SIZE 4096  // 4KB runtime cache

// Nested key system (32-bit keys with hierarchy)
#define WISP_KEY_NAMESPACE_BITS  8   // 256 namespaces
#define WISP_KEY_CATEGORY_BITS   8   // 256 categories per namespace  
#define WISP_KEY_ID_BITS        16   // 65536 IDs per category
#define WISP_KEY_MAKE(ns, cat, id) (((uint32_t)(ns) << 24) | ((uint32_t)(cat) << 16) | (uint32_t)(id))
#define WISP_KEY_NAMESPACE(key) ((key) >> 24)
#define WISP_KEY_CATEGORY(key) (((key) >> 16) & 0xFF)
#define WISP_KEY_ID(key) ((key) & 0xFFFF)

// Predefined namespaces for organization
enum WispNamespace : uint8_t {
    NS_SYSTEM = 0x00,       // System configuration and metadata
    NS_GAME = 0x01,         // Game-specific data (items, quests, etc.)
    NS_PLAYER = 0x02,       // Player state (inventory, progress, etc.)
    NS_WORLD = 0x03,        // World state (NPCs, locations, etc.)
    NS_APP = 0x04,          // Application-specific data
    NS_USER = 0x05,         // User preferences and settings
    NS_CUSTOM_1 = 0x10,     // Custom namespace 1
    NS_CUSTOM_2 = 0x11,     // Custom namespace 2
    // ... up to 0xFF
};

// Predefined categories for common use cases
enum WispCategory : uint8_t {
    // Game namespace categories
    CAT_ITEMS = 0x01,           // Item definitions
    CAT_QUESTS = 0x02,          // Quest definitions
    CAT_NPCS = 0x03,            // NPC data
    CAT_LOCATIONS = 0x04,       // Location data
    CAT_ABILITIES = 0x05,       // Skills/abilities
    CAT_RECIPES = 0x06,         // Crafting recipes
    
    // Player namespace categories  
    CAT_INVENTORY = 0x01,       // Player inventory
    CAT_STATS = 0x02,           // Player statistics
    CAT_FLAGS = 0x03,           // Player flags/achievements
    CAT_PROGRESS = 0x04,        // Quest/story progress
    CAT_POSITION = 0x05,        // Player position/location
    CAT_SKILLS = 0x06,          // Player skills/levels
    
    // System namespace categories
    CAT_CONFIG = 0x01,          // System configuration
    CAT_METADATA = 0x02,        // Database metadata
    CAT_INDICES = 0x03,         // Index tables
};

// Database entry types with size optimization
enum WispEntryType : uint8_t {
    ENTRY_U8 = 0x01,        // 1 byte value
    ENTRY_U16 = 0x02,       // 2 byte value  
    ENTRY_U32 = 0x03,       // 4 byte value
    ENTRY_BYTES = 0x04,     // Variable length bytes
    ENTRY_STRING = 0x05,    // Variable length string
    ENTRY_STRUCT = 0x06,    // Structured data
    ENTRY_ARRAY = 0x07,     // Array of values
    ENTRY_INDEX = 0x08,     // Index/reference to other entry
    ENTRY_COMPRESSED = 0x09, // Compressed data
};

// Entry flags
enum WispEntryFlags : uint8_t {
    FLAG_READ_ONLY = 0x01,      // Cannot be modified
    FLAG_COMPRESSED = 0x02,     // Data is compressed
    FLAG_ENCRYPTED = 0x04,      // Data is encrypted
    FLAG_CACHED = 0x08,         // Entry is cached in runtime
    FLAG_DIRTY = 0x10,          // Entry needs to be saved
    FLAG_DELETED = 0x20,        // Entry is marked for deletion
};

// Compact entry header (8 bytes)
struct WispEntryHeader {
    uint32_t key;           // Nested key (namespace.category.id)
    uint8_t type;           // Entry type
    uint8_t flags;          // Entry flags
    uint16_t size;          // Data size in bytes
} __attribute__((packed));

// Partition header (32 bytes each)
struct WispPartitionHeader {
    uint32_t magic;         // Partition magic number
    uint16_t version;       // Partition version
    uint16_t entryCount;    // Number of entries
    uint32_t dataSize;      // Total data size
    uint32_t checksum;      // Data integrity checksum
    uint32_t lastModified;  // Last modification timestamp
    uint16_t freeSpace;     // Available space
    uint16_t fragmentation; // Fragmentation level (0-100%)
    uint32_t reserved[2];   // Future use
} __attribute__((packed));

// ROM partition entry (compile-time baked data)
struct WispROMEntry {
    uint32_t key;           // Nested key
    uint16_t offset;        // Offset in ROM data
    uint8_t type;           // Entry type
    uint8_t size;           // Data size (or 0xFF for variable)
} __attribute__((packed));

// Index entry for fast lookups
struct WispIndexEntry {
    uint32_t key;           // Search key
    uint8_t partition;      // Which partition (0=ROM, 1=Save, 2=Backup, 3=Runtime)
    uint8_t flags;          // Entry flags
    uint16_t offset;        // Offset within partition
} __attribute__((packed));

// Cache entry for runtime optimization
struct WispCacheEntry {
    uint32_t key;
    uint32_t lastAccess;
    uint16_t size;
    uint8_t* data;
    bool dirty;
};

// Main partitioned database system
class WispPartitionedDB {
private:
    // LP-SRAM partitions (persistent across power cycles)
    RTC_DATA_ATTR static WispPartitionHeader romHeader;
    RTC_DATA_ATTR static WispPartitionHeader saveHeader;  
    RTC_DATA_ATTR static WispPartitionHeader backupHeader;
    RTC_DATA_ATTR static WispPartitionHeader runtimeHeader;
    
    RTC_DATA_ATTR static uint8_t romPartition[WISP_DB_ROM_PARTITION_SIZE];
    RTC_DATA_ATTR static uint8_t savePartition[WISP_DB_SAVE_PARTITION_SIZE];
    RTC_DATA_ATTR static uint8_t backupPartition[WISP_DB_BACKUP_PARTITION_SIZE];
    RTC_DATA_ATTR static uint8_t runtimePartition[WISP_DB_RUNTIME_PARTITION_SIZE];
    
    // Runtime index for fast lookups (built at startup)
    static std::unordered_map<uint32_t, WispIndexEntry> indexCache;
    static std::unordered_map<uint32_t, WispCacheEntry> dataCache;
    static bool indexBuilt;
    static uint32_t cacheHits, cacheMisses;
    
    // Internal methods
    bool buildIndex();
    bool validatePartition(uint8_t partitionId);
    void updatePartitionChecksum(uint8_t partitionId);
    WispIndexEntry* findEntry(uint32_t key);
    uint8_t* getPartitionData(uint8_t partitionId);
    uint16_t getPartitionSize(uint8_t partitionId);
    WispPartitionHeader* getPartitionHeader(uint8_t partitionId);
    bool writeEntry(uint32_t key, const void* data, uint16_t size, uint8_t type, uint8_t partition, uint8_t flags = 0);
    bool readEntry(uint32_t key, void* data, uint16_t maxSize, uint16_t* actualSize = nullptr);
    bool cacheEntry(uint32_t key, const void* data, uint16_t size);
    void evictOldCache();
    bool compressData(const void* input, uint16_t inputSize, void* output, uint16_t* outputSize);
    bool decompressData(const void* input, uint16_t inputSize, void* output, uint16_t* outputSize);
    
public:
    // Database lifecycle
    bool initialize();
    bool loadROMData(const uint8_t* romData, uint16_t size);
    bool reset(bool preserveROM = true);
    bool save();
    bool backup();
    bool restore();
    bool validate();
    bool compact();
    
    // Core read/write operations
    bool set(uint32_t key, const void* data, uint16_t size, uint8_t type = ENTRY_BYTES);
    bool get(uint32_t key, void* data, uint16_t maxSize, uint16_t* actualSize = nullptr);
    bool has(uint32_t key);
    bool remove(uint32_t key);
    uint16_t getSize(uint32_t key);
    uint8_t getType(uint32_t key);
    
    // Typed accessors with compression support
    bool setU8(uint32_t key, uint8_t value);
    bool setU16(uint32_t key, uint16_t value);
    bool setU32(uint32_t key, uint32_t value);
    bool setString(uint32_t key, const String& value, bool compress = false);
    bool setBytes(uint32_t key, const uint8_t* data, uint16_t size, bool compress = false);
    
    uint8_t getU8(uint32_t key, uint8_t defaultValue = 0);
    uint16_t getU16(uint32_t key, uint16_t defaultValue = 0);
    uint32_t getU32(uint32_t key, uint32_t defaultValue = 0);
    String getString(uint32_t key, const String& defaultValue = "");
    uint16_t getBytes(uint32_t key, uint8_t* data, uint16_t maxSize);
    
    // Convenience methods for common patterns
    bool setFlag(uint32_t key, bool value) { return setU8(key, value ? 1 : 0); }
    bool getFlag(uint32_t key, bool defaultValue = false) { return getU8(key, defaultValue ? 1 : 0) != 0; }
    bool toggleFlag(uint32_t key) { return setFlag(key, !getFlag(key)); }
    
    bool incrementU8(uint32_t key, uint8_t amount = 1);
    bool incrementU16(uint32_t key, uint16_t amount = 1);
    bool incrementU32(uint32_t key, uint32_t amount = 1);
    bool decrementU8(uint32_t key, uint8_t amount = 1);
    bool decrementU16(uint32_t key, uint16_t amount = 1);
    bool decrementU32(uint32_t key, uint32_t amount = 1);
    
    // Batch operations
    bool setBatch(const std::vector<std::pair<uint32_t, uint32_t>>& keyValues);
    std::vector<uint32_t> getBatch(const std::vector<uint32_t>& keys);
    
    // Query operations
    std::vector<uint32_t> getKeysInNamespace(uint8_t ns);
    std::vector<uint32_t> getKeysInCategory(uint8_t ns, uint8_t cat);
    uint16_t countInNamespace(uint8_t ns);
    uint16_t countInCategory(uint8_t ns, uint8_t cat);
    std::vector<uint32_t> findByPattern(uint32_t pattern, uint32_t mask);
    
    // Partition management
    uint16_t getPartitionUsage(uint8_t partitionId);
    uint16_t getPartitionFree(uint8_t partitionId);
    float getPartitionFragmentation(uint8_t partitionId);
    bool compactPartition(uint8_t partitionId);
    bool moveToPartition(uint32_t key, uint8_t targetPartition);
    bool optimizePartitions();
    
    // Cache management
    void clearCache();
    void setCacheSize(uint16_t maxEntries);
    float getCacheHitRatio();
    void printCacheStats();
    
    // Statistics and diagnostics
    void printStats();
    void printIndex();
    void printPartition(uint8_t partitionId);
    void printMemoryMap();
    bool exportPartition(uint8_t partitionId, const String& filename);
    bool importPartition(uint8_t partitionId, const String& filename);
    bool exportDatabase(const String& filename);
    bool importDatabase(const String& filename);
    
    // ROM data generation (for build tools)
    static bool generateROMData(const String& configFile, uint8_t* output, uint16_t maxSize, uint16_t* actualSize);
};

// High-level convenience API for common use cases
class WispGameDB {
private:
    WispPartitionedDB* db;
    
public:
    WispGameDB(WispPartitionedDB* database) : db(database) {}
    
    // Pokemon-style RPG helpers
    struct Pokemon {
        uint16_t species;
        uint8_t level;
        uint8_t happiness;
        uint32_t experience;
        uint16_t hp, attack, defense, speed;
        uint32_t moves; // 4 moves packed into 32 bits (8 bits each)
        uint16_t nature;
        uint16_t ability;
    } __attribute__((packed));
    
    struct Item {
        uint16_t id;
        uint8_t type;
        uint8_t rarity;
        uint16_t value;
        String name;
        String description;
    };
    
    struct Quest {
        uint16_t id;
        uint8_t status; // 0=not_started, 1=active, 2=completed, 3=failed
        uint8_t progress; // 0-100%
        String title;
        String description;
        std::vector<uint16_t> prerequisites;
    };
    
    // Item management
    bool defineItem(uint16_t id, uint8_t type, uint8_t rarity, uint16_t value, const String& name, const String& description = "");
    bool giveItem(uint16_t itemId, uint8_t quantity = 1);
    bool useItem(uint16_t itemId, uint8_t quantity = 1);
    bool hasItem(uint16_t itemId, uint8_t quantity = 1);
    uint8_t getItemCount(uint16_t itemId);
    Item getItemInfo(uint16_t itemId);
    std::vector<uint16_t> getInventoryItems();
    
    // Quest system
    bool defineQuest(uint16_t questId, const String& title, const String& description = "", const std::vector<uint16_t>& prerequisites = {});
    bool startQuest(uint16_t questId);
    bool completeQuest(uint16_t questId);
    bool failQuest(uint16_t questId);
    bool updateQuestProgress(uint16_t questId, uint8_t progress);
    Quest getQuestInfo(uint16_t questId);
    std::vector<uint16_t> getActiveQuests();
    std::vector<uint16_t> getCompletedQuests();
    bool isQuestActive(uint16_t questId);
    bool isQuestCompleted(uint16_t questId);
    
    // Player stats
    bool setPlayerLevel(uint8_t level);
    bool setPlayerXP(uint32_t xp);
    bool addPlayerXP(uint32_t xp);
    uint8_t getPlayerLevel();
    uint32_t getPlayerXP();
    bool setPlayerHP(uint16_t hp);
    uint16_t getPlayerHP();
    bool setPlayerMoney(uint32_t money);
    bool addPlayerMoney(uint32_t money);
    uint32_t getPlayerMoney();
    
    // World state
    bool setMapFlag(uint16_t mapId, uint16_t flagId, bool value);
    bool getMapFlag(uint16_t mapId, uint16_t flagId);
    bool setPlayerPosition(uint16_t mapId, uint16_t x, uint16_t y);
    void getPlayerPosition(uint16_t* mapId, uint16_t* x, uint16_t* y);
    bool unlockLocation(uint16_t locationId);
    bool isLocationUnlocked(uint16_t locationId);
    
    // Pokemon-specific
    bool addPokemon(uint8_t slot, const Pokemon& pokemon);
    bool getPokemon(uint8_t slot, Pokemon* pokemon);
    bool setPokemonLevel(uint8_t slot, uint8_t level);
    bool addPokemonXP(uint8_t slot, uint32_t xp);
    uint8_t getPokemonCount();
    bool releasePokemon(uint8_t slot);
    bool swapPokemon(uint8_t slot1, uint8_t slot2);
    
    // Badges and achievements
    bool earnBadge(uint8_t badgeId);
    bool hasBadge(uint8_t badgeId);
    uint8_t getBadgeCount();
    bool unlockAchievement(uint16_t achievementId);
    bool hasAchievement(uint16_t achievementId);
    std::vector<uint16_t> getUnlockedAchievements();
    
    // Save game management
    bool saveGame();
    bool loadGame();
    bool hasExistingSave();
    bool deleteSave();
    bool backupSave();
    bool restoreSave();
    uint32_t getPlayTime(); // in seconds
    void addPlayTime(uint32_t seconds);
};

// ROM data builder for compile-time optimization
class WispROMBuilder {
public:
    struct ItemDef {
        uint16_t id;
        uint8_t type, rarity;
        uint16_t value;
        String name;
        String description;
        std::unordered_map<String, String> properties;
    };
    
    struct QuestDef {
        uint16_t id;
        String title;
        String description;
        uint16_t requiredLevel;
        std::vector<uint16_t> prerequisites;
        std::unordered_map<String, String> properties;
    };
    
    struct MapDef {
        uint16_t id;
        String name;
        uint16_t width, height;
        std::vector<uint16_t> connections;
        std::unordered_map<String, String> properties;
    };
    
    struct PokemonDef {
        uint16_t id;
        String name;
        uint8_t type1, type2;
        uint16_t baseHP, baseAttack, baseDefense, baseSpeed;
        std::vector<uint16_t> learnableMoves;
    };
    
private:
    std::vector<ItemDef> items;
    std::vector<QuestDef> quests;
    std::vector<MapDef> maps;
    std::vector<PokemonDef> pokemon;
    std::unordered_map<String, String> strings;
    std::unordered_map<String, std::vector<uint8_t>> binaryData;
    
public:
    // Build ROM data from configuration
    bool loadFromYAML(const String& filename);
    bool loadFromJSON(const String& filename);
    bool loadFromDirectory(const String& directory);
    
    // Add data programmatically
    void addItem(const ItemDef& item);
    void addQuest(const QuestDef& quest);
    void addMap(const MapDef& map);
    void addPokemon(const PokemonDef& pokemon);
    void addString(const String& key, const String& value);
    void addBinaryData(const String& key, const std::vector<uint8_t>& data);
    
    // Generate optimized ROM data
    bool generateROM(uint8_t* output, uint16_t maxSize, uint16_t* actualSize);
    bool writeROMFile(const String& filename);
    bool writeHeaderFile(const String& filename); // Generate C header with constants
    void printStats();
    void printMemoryLayout();
    
    // Optimization
    bool optimizeStrings(); // Deduplicate and compress strings
    bool optimizeData();    // Compress and pack data
    void setCompressionLevel(uint8_t level);
    
    // Validation
    bool validate();
    std::vector<String> getValidationErrors();
    std::vector<String> getValidationWarnings();
};

// Convenience macros using nested keys
#define WISP_PLAYER_LEVEL        WISP_KEY_MAKE(NS_PLAYER, CAT_STATS, 1)
#define WISP_PLAYER_XP           WISP_KEY_MAKE(NS_PLAYER, CAT_STATS, 2)
#define WISP_PLAYER_HP           WISP_KEY_MAKE(NS_PLAYER, CAT_STATS, 3)
#define WISP_PLAYER_MONEY        WISP_KEY_MAKE(NS_PLAYER, CAT_STATS, 4)
#define WISP_PLAYER_MAP          WISP_KEY_MAKE(NS_PLAYER, CAT_POSITION, 1)
#define WISP_PLAYER_X            WISP_KEY_MAKE(NS_PLAYER, CAT_POSITION, 2)
#define WISP_PLAYER_Y            WISP_KEY_MAKE(NS_PLAYER, CAT_POSITION, 3)
#define WISP_PLAY_TIME           WISP_KEY_MAKE(NS_PLAYER, CAT_STATS, 10)

#define WISP_ITEM_KEY(id)        WISP_KEY_MAKE(NS_GAME, CAT_ITEMS, id)
#define WISP_QUEST_KEY(id)       WISP_KEY_MAKE(NS_GAME, CAT_QUESTS, id)
#define WISP_INVENTORY_KEY(id)   WISP_KEY_MAKE(NS_PLAYER, CAT_INVENTORY, id)
#define WISP_FLAG_KEY(id)        WISP_KEY_MAKE(NS_PLAYER, CAT_FLAGS, id)
#define WISP_POKEMON_KEY(slot)   WISP_KEY_MAKE(NS_PLAYER, CAT_SKILLS, slot)
#define WISP_BADGE_KEY(id)       WISP_KEY_MAKE(NS_PLAYER, CAT_FLAGS, 100 + id)

// Easy macros for common operations
#define WISP_SET_PLAYER_LEVEL(level)     wispDB.setU8(WISP_PLAYER_LEVEL, level)
#define WISP_GET_PLAYER_LEVEL()          wispDB.getU8(WISP_PLAYER_LEVEL, 1)
#define WISP_ADD_XP(amount)              wispDB.incrementU32(WISP_PLAYER_XP, amount)
#define WISP_GET_XP()                    wispDB.getU32(WISP_PLAYER_XP)
#define WISP_ADD_MONEY(amount)           wispDB.incrementU32(WISP_PLAYER_MONEY, amount)
#define WISP_GET_MONEY()                 wispDB.getU32(WISP_PLAYER_MONEY)

#define WISP_SET_FLAG(id, value)         wispDB.setFlag(WISP_FLAG_KEY(id), value)
#define WISP_GET_FLAG(id)                wispDB.getFlag(WISP_FLAG_KEY(id))
#define WISP_TOGGLE_FLAG(id)             wispDB.toggleFlag(WISP_FLAG_KEY(id))

#define WISP_GIVE_ITEM(id, qty)          wispDB.incrementU8(WISP_INVENTORY_KEY(id), qty)
#define WISP_USE_ITEM(id, qty)           wispDB.decrementU8(WISP_INVENTORY_KEY(id), qty)
#define WISP_HAS_ITEM(id, qty)           (wispDB.getU8(WISP_INVENTORY_KEY(id)) >= qty)
#define WISP_ITEM_COUNT(id)              wispDB.getU8(WISP_INVENTORY_KEY(id))

#define WISP_EARN_BADGE(id)              wispDB.setFlag(WISP_BADGE_KEY(id), true)
#define WISP_HAS_BADGE(id)               wispDB.getFlag(WISP_BADGE_KEY(id))

#define WISP_SET_POSITION(map, x, y)     do { \
    wispDB.setU16(WISP_PLAYER_MAP, map); \
    wispDB.setU16(WISP_PLAYER_X, x); \
    wispDB.setU16(WISP_PLAYER_Y, y); \
} while(0)

// Common item IDs (can be overridden)
#ifndef WISP_CUSTOM_ITEM_IDS
enum WispItemIDs {
    // Pokeballs
    ITEM_POKEBALL = 1,
    ITEM_GREATBALL = 2,
    ITEM_ULTRABALL = 3,
    ITEM_MASTERBALL = 4,
    
    // Healing items
    ITEM_POTION = 10,
    ITEM_SUPER_POTION = 11,
    ITEM_HYPER_POTION = 12,
    ITEM_MAX_POTION = 13,
    ITEM_FULL_RESTORE = 14,
    
    // Status healing
    ITEM_ANTIDOTE = 20,
    ITEM_PARALYZ_HEAL = 21,
    ITEM_AWAKENING = 22,
    ITEM_BURN_HEAL = 23,
    ITEM_ICE_HEAL = 24,
    ITEM_FULL_HEAL = 25,
    
    // Stat boosters
    ITEM_RARE_CANDY = 30,
    ITEM_PROTEIN = 31,
    ITEM_IRON = 32,
    ITEM_CARBOS = 33,
    ITEM_CALCIUM = 34,
    ITEM_HP_UP = 35,
    
    // Key items
    ITEM_BIKE = 50,
    ITEM_SURF_HM = 51,
    ITEM_FLY_HM = 52,
    ITEM_POKEDEX = 53,
    ITEM_TOWN_MAP = 54,
    
    // Badges (use with WISP_BADGE_KEY macro)
    BADGE_BOULDER = 1,
    BADGE_CASCADE = 2,
    BADGE_THUNDER = 3,
    BADGE_RAINBOW = 4,
    BADGE_SOUL = 5,
    BADGE_MARSH = 6,
    BADGE_VOLCANO = 7,
    BADGE_EARTH = 8,
};
#endif

// Common quest IDs
#ifndef WISP_CUSTOM_QUEST_IDS
enum WispQuestIDs {
    // Main story
    QUEST_STARTER_POKEMON = 1000,
    QUEST_FIRST_GYM = 1001,
    QUEST_RIVAL_BATTLE_1 = 1002,
    QUEST_SECOND_GYM = 1003,
    QUEST_TEAM_ROCKET_HIDEOUT = 1004,
    QUEST_ELITE_FOUR = 1005,
    QUEST_CHAMPION = 1006,
    
    // Side quests
    QUEST_FIND_PIKACHU = 2000,
    QUEST_HELP_PROFESSOR = 2001,
    QUEST_DELIVER_PACKAGE = 2002,
    QUEST_CATCH_LEGENDARY = 2003,
    QUEST_COMPLETE_POKEDEX = 2004,
};
#endif

// Common map/location IDs
#ifndef WISP_CUSTOM_MAP_IDS
enum WispMapIDs {
    MAP_PALLET_TOWN = 1,
    MAP_VIRIDIAN_CITY = 2,
    MAP_PEWTER_CITY = 3,
    MAP_CERULEAN_CITY = 4,
    MAP_VERMILION_CITY = 5,
    MAP_CELADON_CITY = 6,
    MAP_FUCHSIA_CITY = 7,
    MAP_SAFFRON_CITY = 8,
    MAP_CINNABAR_ISLAND = 9,
    MAP_INDIGO_PLATEAU = 10,
    
    // Routes
    MAP_ROUTE_1 = 100,
    MAP_ROUTE_2 = 101,
    // ... etc
};
#endif

// Global instances
extern WispPartitionedDB wispDB;
extern WispGameDB gameDB;
extern WispROMBuilder romBuilder;
