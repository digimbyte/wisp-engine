// Wisp Engine - Partitioned Database System - ESP32-C6/S3 using ESP-IDF
// Memory-safe database optimized for ESP32-C6 LP-SRAM (16KB) using native ESP-IDF
#pragma once

#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include <esp_attr.h>

// Database configuration
#define DATABASE_VERSION 2

// Memory constraints and safety limits
#define LP_SRAM_SIZE 16384                   // ESP32-C6 LP-SRAM: exactly 16KB
#define MAX_ENTRY_SIZE 255                   // Maximum single entry size (255 bytes)
#define MIN_PARTITION_SIZE 256               // Minimum partition size (256B)
#define PARTITION_HEADER_SIZE 16             // Compact partition header size
#define ENTRY_HEADER_SIZE 6                  // Ultra-compact entry header size
#define SAFETY_MARGIN 64                     // Safety buffer per partition
#define MAX_ENTRIES_PER_PARTITION 255        // Max entries to fit in uint8_t

// Conservative default configuration (total: 8KB, leaves 8KB free)
#ifndef ROM_PARTITION_SIZE
#define ROM_PARTITION_SIZE   2048            // 2KB ROM data - conservative
#endif

#ifndef SAVE_PARTITION_SIZE  
#define SAVE_PARTITION_SIZE  2048            // 2KB save data
#endif

#ifndef BACKUP_PARTITION_SIZE
#define BACKUP_PARTITION_SIZE 1024           // 1KB backup save
#endif

#ifndef RUNTIME_PARTITION_SIZE
#define RUNTIME_PARTITION_SIZE 2048          // 2KB runtime cache
#endif

// Compile-time validation with detailed error messages
#define TOTAL_CONFIGURED (ROM_PARTITION_SIZE + SAVE_PARTITION_SIZE + \
                         BACKUP_PARTITION_SIZE + RUNTIME_PARTITION_SIZE)

static_assert(TOTAL_CONFIGURED <= LP_SRAM_SIZE,
              "ERROR: Total partition sizes exceed 16KB LP-SRAM!");
static_assert(ROM_PARTITION_SIZE >= MIN_PARTITION_SIZE,
              "ERROR: ROM partition too small, minimum 256 bytes required!");
static_assert(SAVE_PARTITION_SIZE >= MIN_PARTITION_SIZE,
              "ERROR: Save partition too small, minimum 256 bytes required!");

// Memory efficiency warning
#if TOTAL_CONFIGURED > (LP_SRAM_SIZE * 3 / 4)
#warning "Database uses >75% of LP-SRAM - consider reducing partition sizes"
#endif

// Nested key system (32-bit keys with hierarchy)
#define KEY_NAMESPACE_BITS  8                // 256 namespaces
#define KEY_CATEGORY_BITS   8                // 256 categories per namespace  
#define KEY_ID_BITS        16                // 65536 IDs per category
#define MAKE_KEY(ns, cat, id) (((uint32_t)(ns) << 24) | ((uint32_t)(cat) << 16) | (uint32_t)(id))
#define GET_NAMESPACE(key) ((key) >> 24)
#define GET_CATEGORY(key) (((key) >> 16) & 0xFF)
#define GET_ID(key) ((key) & 0xFFFF)

// Predefined namespaces for organization
enum Namespace : uint8_t {
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
enum Category : uint8_t {
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

// Partition IDs
#ifndef PARTITION_ROM
#define PARTITION_ROM 0
#endif

#ifndef PARTITION_SAVE
#define PARTITION_SAVE 1
#endif

#ifndef PARTITION_BACKUP
#define PARTITION_BACKUP 2
#endif

#ifndef PARTITION_RUNTIME
#define PARTITION_RUNTIME 3
#endif

// Error codes for overflow and safety checking
enum ErrorCode : uint8_t {
    SUCCESS = 0,
    ERROR_INVALID_KEY = 1,
    ERROR_KEY_NOT_FOUND = 2,
    ERROR_PARTITION_FULL = 3,
    ERROR_ENTRY_TOO_LARGE = 4,
    ERROR_INVALID_PARTITION = 5,
    ERROR_BUFFER_OVERFLOW = 6,
    ERROR_INDEX_OVERFLOW = 7,
    ERROR_MEMORY_CORRUPTED = 8,
    ERROR_SAFETY_VIOLATION = 9,
    ERROR_READ_ONLY = 10,
    ERROR_NOT_INITIALIZED = 11,
    ERROR_INVALID_CONFIG = 12,
    ERROR_CHECKSUM_FAILED = 13
};

// Database entry types with size optimization
enum EntryType : uint8_t {
    ENTRY_U8 = 0x01,        // 1 byte value
    ENTRY_U16 = 0x02,       // 2 byte value  
    ENTRY_U32 = 0x03,       // 4 byte value
    ENTRY_BYTES = 0x04,     // Variable length bytes
    ENTRY_STRING = 0x05,    // Variable length string
    ENTRY_STRUCT = 0x06,    // Structured data
    ENTRY_ARRAY = 0x07,     // Array of values
    ENTRY_INDEX = 0x08,     // Index/reference to other entry
    ENTRY_COMPRESSED = 0x09, // Compressed data
    ENTRY_APP_DEFINED = 0x80, // App can define types >= 0x80
};

// Entry flags
enum EntryFlags : uint8_t {
    FLAG_READ_ONLY = 0x01,      // Cannot be modified (ROM entries)
    FLAG_COMPRESSED = 0x02,     // Data is compressed
    FLAG_ENCRYPTED = 0x04,      // Data is encrypted
    FLAG_CACHED = 0x08,         // Entry is cached in runtime
    FLAG_DIRTY = 0x10,          // Entry needs to be saved
    FLAG_DELETED = 0x20,        // Entry is marked for deletion
    FLAG_APP_DEFINED = 0x40,    // App-specific flag
    FLAG_RESERVED = 0x80,       // Reserved for future use
};

// Ultra-compact entry header (6 bytes - optimized for 16KB)
struct EntryHeader {
    uint32_t key;           // Nested key (namespace.category.id)
    uint8_t type_and_flags; // Upper 4 bits: type, Lower 4 bits: flags
    uint8_t size;           // Data size in bytes (max 255 bytes per entry)
} __attribute__((packed));

// Compact partition header (16 bytes - minimized overhead)
struct PartitionHeader {
    uint16_t magic;         // Partition magic (0xDB01)
    uint8_t version;        // Partition version
    uint8_t entryCount;     // Number of entries (max 255)
    uint16_t usedBytes;     // Bytes used for data
    uint16_t totalSize;     // Total partition size
    uint32_t checksum;      // CRC32 for integrity
    uint32_t reserved;      // Future use
} __attribute__((packed));

// Memory-safe partition configuration with bounds checking
struct PartitionConfig {
    uint16_t romSize;       // Must be >= 256, <= 8192
    uint16_t saveSize;      // Must be >= 256, <= 8192  
    uint16_t backupSize;    // Must be >= 256, <= 4096
    uint16_t runtimeSize;   // Must be >= 256, <= 8192
    bool enableCompression; // LZ4 compression for entries > 32 bytes
    bool enableEncryption;  // XOR encryption for sensitive data
    uint8_t maxCacheEntries;// Runtime cache limit (4-64 entries)
    uint8_t safetyLevel;    // 0=fast, 1=safe, 2=paranoid
} __attribute__((packed));

// Bounds checking helper macros
#define BOUNDS_CHECK(ptr, start, size, access_size) \
    ((ptr) >= (start) && ((ptr) + (access_size)) <= ((start) + (size)))

#define ENTRY_SIZE_VALID(size) \
    ((size) > 0 && (size) <= MAX_ENTRY_SIZE)

#define PARTITION_SIZE_VALID(size) \
    ((size) >= MIN_PARTITION_SIZE && (size) <= (LP_SRAM_SIZE / 2))

#define TOTAL_SIZE_VALID(config) \
    (((config)->romSize + (config)->saveSize + (config)->backupSize + (config)->runtimeSize) <= LP_SRAM_SIZE)

// Safety validation macros
#define VALIDATE_CONFIG(config) \
    (PARTITION_SIZE_VALID((config)->romSize) && \
     PARTITION_SIZE_VALID((config)->saveSize) && \
     PARTITION_SIZE_VALID((config)->backupSize) && \
     PARTITION_SIZE_VALID((config)->runtimeSize) && \
     TOTAL_SIZE_VALID(config))

#define VALIDATE_WRITE(partition_start, partition_size, write_ptr, write_size) \
    BOUNDS_CHECK(write_ptr, partition_start, partition_size, write_size)

// Memory-optimized cache entry (12 bytes)
struct CacheEntry {
    uint32_t key;           // Cached key
    uint16_t size;          // Data size
    uint16_t partition_offset; // Where data is stored
    uint32_t access_time;   // LRU tracking
} __attribute__((packed));

// Memory usage tracking for debugging
struct MemoryStats {
    uint16_t totalUsed;
    uint16_t totalFree;
    uint16_t romUsed;
    uint16_t saveUsed;
    uint16_t backupUsed;
    uint16_t runtimeUsed;
    uint8_t entryCount;
    uint8_t cacheHits;
    uint8_t cacheMisses;
    float fragmentation;
} __attribute__((packed));

// Main partitioned database system with overflow protection
class PartitionedDatabase {
private:
    // Memory-safe configuration
    PartitionConfig config;
    bool initialized;
    
    // LP-SRAM layout with bounds protection
    RTC_DATA_ATTR static uint8_t lpSramData[LP_SRAM_SIZE];
    
    // Partition layout (calculated at init, not hardcoded)
    uint8_t* romPartition;
    uint8_t* savePartition;
    uint8_t* backupPartition;
    uint8_t* runtimePartition;
    
    // Partition sizes (for bounds checking)
    uint16_t romSize, saveSize, backupSize, runtimeSize;
    
    // Minimal runtime cache (configurable size)
    CacheEntry* cache;
    uint8_t cacheSize;
    uint8_t cacheCount;
    
    // Safety and bounds checking
    ErrorCode validatePointer(const void* ptr, uint16_t size, uint8_t partition);
    ErrorCode validateEntry(uint32_t key, uint16_t size);
    ErrorCode validatePartition(uint8_t partitionId);
    bool isPartitionFull(uint8_t partitionId, uint16_t requiredSpace);
    bool isValidKey(uint32_t key);
    
    // Memory-safe internal operations
    ErrorCode writeEntryInternal(uint32_t key, const void* data, uint8_t size, 
                                uint8_t type, uint8_t partition, uint8_t flags);
    ErrorCode readEntryInternal(uint32_t key, void* buffer, uint8_t maxSize, 
                               uint8_t* actualSize, uint8_t partition);
    
    // Partition management with overflow protection
    uint8_t* getPartitionStart(uint8_t partitionId);
    uint16_t getPartitionSize(uint8_t partitionId);
    PartitionHeader* getPartitionHeader(uint8_t partitionId);
    uint16_t getPartitionFreeSpace(uint8_t partitionId);
    ErrorCode setupPartitions();
    ErrorCode initializePartitionHeaders();
    
    // Cache management (LRU eviction)
    void cacheEntry(uint32_t key, uint16_t size, uint16_t partition_offset);
    CacheEntry* findInCache(uint32_t key);
    void evictLRU();
    
    // Integrity and corruption detection
    bool verifyPartitionIntegrity(uint8_t partitionId);
    uint32_t calculateChecksum(const void* data, uint16_t size);
    void updatePartitionChecksum(uint8_t partitionId);
    
public:
    // Initialization with strict validation
    ErrorCode initialize(const PartitionConfig* partitionConfig = nullptr);
    ErrorCode reset(bool preserveROM = true);
    void cleanup();
    
    // Core operations with overflow protection
    ErrorCode set(uint32_t key, const void* data, uint8_t size, uint8_t type = ENTRY_BYTES);
    ErrorCode get(uint32_t key, void* buffer, uint8_t maxSize, uint8_t* actualSize = nullptr);
    ErrorCode remove(uint32_t key);
    bool exists(uint32_t key);
    
    // Type-safe accessors with bounds checking
    ErrorCode setU8(uint32_t key, uint8_t value);
    ErrorCode setU16(uint32_t key, uint16_t value);
    ErrorCode setU32(uint32_t key, uint32_t value);
    
    uint8_t getU8(uint32_t key, uint8_t defaultValue = 0);
    uint16_t getU16(uint32_t key, uint16_t defaultValue = 0);
    uint32_t getU32(uint32_t key, uint32_t defaultValue = 0);
    
    // Memory monitoring and diagnostics
    uint16_t getTotalUsedBytes();
    uint16_t getTotalFreeBytes();
    uint16_t getPartitionUsedBytes(uint8_t partitionId);
    uint16_t getPartitionFreeBytes(uint8_t partitionId);
    uint8_t getEntryCount(uint8_t partitionId);
    
    // Safety and integrity
    bool validateDatabase();
    bool repairCorruption();
    ErrorCode getLastError();
    
    // Debugging and optimization
    void printMemoryMap();
    void printPartitionStats();
    float getFragmentationLevel(uint8_t partitionId);
};

// Global database instance
extern PartitionedDatabase database;

// Memory-efficient app configuration helpers (all fit within 16KB safely)
#define CONFIG_TINY() { \
    .romSize = 512, .saveSize = 512, .backupSize = 256, .runtimeSize = 512, \
    .enableCompression = false, .enableEncryption = false, .maxCacheEntries = 4, .safetyLevel = 1 \
}

#define CONFIG_SMALL() { \
    .romSize = 1024, .saveSize = 1024, .backupSize = 512, .runtimeSize = 1024, \
    .enableCompression = false, .enableEncryption = false, .maxCacheEntries = 8, .safetyLevel = 1 \
}

#define CONFIG_MEDIUM() { \
    .romSize = 2048, .saveSize = 2048, .backupSize = 1024, .runtimeSize = 2048, \
    .enableCompression = true, .enableEncryption = false, .maxCacheEntries = 16, .safetyLevel = 1 \
}

#define CONFIG_LARGE() { \
    .romSize = 4096, .saveSize = 4096, .backupSize = 2048, .runtimeSize = 4096, \
    .enableCompression = true, .enableEncryption = true, .maxCacheEntries = 32, .safetyLevel = 1 \
}

#define CONFIG_SAFE(rom, save, backup, runtime) { \
    .romSize = rom, .saveSize = save, .backupSize = backup, .runtimeSize = runtime, \
    .enableCompression = true, .enableEncryption = false, .maxCacheEntries = 16, .safetyLevel = 2 \
}

// Overflow-safe initialization with validation
#define DB_INIT_TINY()     database.initialize(&(PartitionConfig)CONFIG_TINY())
#define DB_INIT_SMALL()    database.initialize(&(PartitionConfig)CONFIG_SMALL())
#define DB_INIT_MEDIUM()   database.initialize(&(PartitionConfig)CONFIG_MEDIUM())
#define DB_INIT_LARGE()    database.initialize(&(PartitionConfig)CONFIG_LARGE())
#define DB_INIT_DEFAULT()  database.initialize()  // Conservative 8KB total

// Memory efficiency helpers
#define DB_MEMORY_USED()   database.getTotalUsedBytes()
#define DB_MEMORY_FREE()   database.getTotalFreeBytes()
#define DB_MEMORY_PERCENT() ((DB_MEMORY_USED() * 100) / LP_SRAM_SIZE)

// Safety checking macros for app code
#define CHECK_SIZE(size) \
    ((size) > 0 && (size) <= 255) ? SUCCESS : ERROR_ENTRY_TOO_LARGE

#define CHECK_SPACE(needed) \
    (database.getTotalFreeBytes() >= (needed)) ? SUCCESS : ERROR_PARTITION_FULL

// Bounds-safe memory operations
#define SAFE_MEMCPY(dst, src, size, max_size) \
    do { \
        size_t copy_size = ((size) > (max_size)) ? (max_size) : (size); \
        memcpy(dst, src, copy_size); \
        if ((size) > (max_size)) return ERROR_BUFFER_OVERFLOW; \
    } while(0)

// Common item types
enum ItemType : uint8_t {
    ITEM_TYPE_WEAPON = 1,
    ITEM_TYPE_ARMOR = 2,
    ITEM_TYPE_POTION = 3,
    ITEM_TYPE_KEY = 4,
    ITEM_TYPE_MATERIAL = 5,
    ITEM_TYPE_FOOD = 6,
    ITEM_TYPE_SCROLL = 7,
    ITEM_TYPE_MISC = 8
};

// Common quest stages
enum QuestStage : uint32_t {
    QUEST_STAGE_1 = 0x00000001,
    QUEST_STAGE_2 = 0x00000002,
    QUEST_STAGE_3 = 0x00000004,
    QUEST_STAGE_4 = 0x00000008,
    QUEST_STAGE_5 = 0x00000010,
    QUEST_STAGE_6 = 0x00000020,
    QUEST_STAGE_7 = 0x00000040,
    QUEST_STAGE_8 = 0x00000080
};
