// Wisp Engine Partitioned Database System - ESP32-C6/S3 using ESP-IDF
// Legacy database system using ESP32-C6 LP-SRAM with native ESP-IDF optimization
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include <esp_attr.h>
#include <vector>
#include <unordered_map>

// Database configuration (can be overridden by app)
#define WISP_DB_VERSION 2

// Memory constraints and safety limits
#define WISP_DB_LP_SRAM_SIZE 16384           // ESP32-C6 LP-SRAM: exactly 16KB
#define WISP_DB_MAX_ENTRY_SIZE 1024          // Maximum single entry size (1KB)
#define WISP_DB_MIN_PARTITION_SIZE 256       // Minimum partition size (256B)
#define WISP_DB_HEADER_OVERHEAD 32           // Partition header size
#define WISP_DB_ENTRY_OVERHEAD 8             // Entry header size
#define WISP_DB_SAFETY_MARGIN 64            // Safety buffer per partition
#define WISP_DB_MAX_ENTRIES_PER_PARTITION 255 // Max entries to fit in uint8_t

// Conservative default configuration (total: 8KB, leaves 8KB free)
#ifndef WISP_DB_ROM_PARTITION_SIZE
#define WISP_DB_ROM_PARTITION_SIZE   2048    // 2KB ROM data - conservative
#endif

#ifndef WISP_DB_SAVE_PARTITION_SIZE  
#define WISP_DB_SAVE_PARTITION_SIZE  2048    // 2KB save data
#endif

#ifndef WISP_DB_BACKUP_PARTITION_SIZE
#define WISP_DB_BACKUP_PARTITION_SIZE 1024   // 1KB backup save
#endif

#ifndef WISP_DB_RUNTIME_PARTITION_SIZE
#define WISP_DB_RUNTIME_PARTITION_SIZE 2048  // 2KB runtime cache
#endif

// Compile-time validation with detailed error messages
#define WISP_DB_TOTAL_CONFIGURED (WISP_DB_ROM_PARTITION_SIZE + WISP_DB_SAVE_PARTITION_SIZE + \
                                  WISP_DB_BACKUP_PARTITION_SIZE + WISP_DB_RUNTIME_PARTITION_SIZE)

static_assert(WISP_DB_TOTAL_CONFIGURED <= WISP_DB_LP_SRAM_SIZE,
              "ERROR: Total partition sizes exceed 16KB LP-SRAM!");
static_assert(WISP_DB_ROM_PARTITION_SIZE >= WISP_DB_MIN_PARTITION_SIZE,
              "ERROR: ROM partition too small, minimum 256 bytes required!");
static_assert(WISP_DB_SAVE_PARTITION_SIZE >= WISP_DB_MIN_PARTITION_SIZE,
              "ERROR: Save partition too small, minimum 256 bytes required!");

// Memory efficiency warning
#if WISP_DB_TOTAL_CONFIGURED > (WISP_DB_LP_SRAM_SIZE * 3 / 4)
#warning "Database uses >75% of LP-SRAM - consider reducing partition sizes"
#endif

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

// Database entry types with size optimization (can be extended by apps)
enum WispEntryType : uint8_t {
#ifndef WISP_DB_PARTITION_ROM
#define WISP_DB_PARTITION_ROM 0
#endif

#ifndef WISP_DB_PARTITION_SAVE
#define WISP_DB_PARTITION_SAVE 1
#endif

#ifndef WISP_DB_PARTITION_BACKUP
#define WISP_DB_PARTITION_BACKUP 2
#endif

#ifndef WISP_DB_PARTITION_RUNTIME
#define WISP_DB_PARTITION_RUNTIME 3
#endif

// Error codes for overflow and safety checking
enum WispErrorCode : uint8_t {
    WISP_SUCCESS = 0,
    WISP_ERROR_INVALID_KEY = 1,
    WISP_ERROR_KEY_NOT_FOUND = 2,
    WISP_ERROR_PARTITION_FULL = 3,
    WISP_ERROR_ENTRY_TOO_LARGE = 4,
    WISP_ERROR_INVALID_PARTITION = 5,
    WISP_ERROR_BUFFER_OVERFLOW = 6,
    WISP_ERROR_INDEX_OVERFLOW = 7,
    WISP_ERROR_MEMORY_CORRUPTED = 8,
    WISP_ERROR_SAFETY_VIOLATION = 9,
    WISP_ERROR_READ_ONLY = 10,
    WISP_ERROR_NOT_INITIALIZED = 11,
    WISP_ERROR_INVALID_CONFIG = 12,
    WISP_ERROR_CHECKSUM_FAILED = 13
};
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

// Entry flags (can be extended by apps)
enum WispEntryFlags : uint8_t {
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
struct WispEntryHeader {
    uint32_t key;           // Nested key (namespace.category.id)
    uint8_t type_and_flags; // Upper 4 bits: type, Lower 4 bits: flags
    uint8_t size;           // Data size in bytes (max 255 bytes per entry)
} __attribute__((packed));

// Compact partition header (16 bytes - minimized overhead)
struct WispPartitionHeader {
    uint16_t magic;         // Partition magic (0xDB01)
    uint8_t version;        // Partition version
    uint8_t entryCount;     // Number of entries (max 255)
    uint16_t usedBytes;     // Bytes used for data
    uint16_t totalSize;     // Total partition size
    uint32_t checksum;      // CRC32 for integrity
    uint32_t reserved;      // Future use
} __attribute__((packed));

// Memory-safe partition configuration with bounds checking
struct WispPartitionConfig {
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
#define WISP_BOUNDS_CHECK(ptr, start, size, access_size) \
    ((ptr) >= (start) && ((ptr) + (access_size)) <= ((start) + (size)))

#define WISP_ENTRY_SIZE_VALID(size) \
    ((size) > 0 && (size) <= WISP_DB_MAX_ENTRY_SIZE)

#define WISP_PARTITION_SIZE_VALID(size) \
    ((size) >= WISP_DB_MIN_PARTITION_SIZE && (size) <= (WISP_DB_LP_SRAM_SIZE / 2))

#define WISP_TOTAL_SIZE_VALID(config) \
    (((config)->romSize + (config)->saveSize + (config)->backupSize + (config)->runtimeSize) <= WISP_DB_LP_SRAM_SIZE)

// Safety validation macros
#define WISP_VALIDATE_CONFIG(config) \
    (WISP_PARTITION_SIZE_VALID((config)->romSize) && \
     WISP_PARTITION_SIZE_VALID((config)->saveSize) && \
     WISP_PARTITION_SIZE_VALID((config)->backupSize) && \
     WISP_PARTITION_SIZE_VALID((config)->runtimeSize) && \
     WISP_TOTAL_SIZE_VALID(config))

#define WISP_VALIDATE_WRITE(partition_start, partition_size, write_ptr, write_size) \
    WISP_BOUNDS_CHECK(write_ptr, partition_start, partition_size, write_size)

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
    uint8_t partition;      // Which partition
    uint8_t flags;          // Entry flags cache
    uint16_t offset;        // Offset within partition
} __attribute__((packed));

// Memory-optimized cache entry (12 bytes)
struct WispCacheEntry {
    uint32_t key;           // Cached key
    uint16_t size;          // Data size
    uint16_t partition_offset; // Where data is stored
    uint32_t access_time;   // LRU tracking
} __attribute__((packed));

// Main partitioned database system with overflow protection
class WispPartitionedDB {
private:
    // Memory-safe configuration
    WispPartitionConfig config;
    bool initialized;
    
    // LP-SRAM layout with bounds protection
    RTC_DATA_ATTR static uint8_t lpSramData[WISP_DB_LP_SRAM_SIZE];
    
    // Partition layout (calculated at init, not hardcoded)
    uint8_t* romPartition;
    uint8_t* savePartition;
    uint8_t* backupPartition;
    uint8_t* runtimePartition;
    
    // Partition sizes (for bounds checking)
    uint16_t romSize, saveSize, backupSize, runtimeSize;
    
    // Minimal runtime cache (configurable size)
    WispCacheEntry* cache;
    uint8_t cacheSize;
    uint8_t cacheCount;
    
    // Safety and bounds checking
    WispErrorCode validatePointer(const void* ptr, uint16_t size, uint8_t partition);
    WispErrorCode validateEntry(uint32_t key, uint16_t size);
    WispErrorCode validatePartition(uint8_t partitionId);
    bool isPartitionFull(uint8_t partitionId, uint16_t requiredSpace);
    bool isValidKey(uint32_t key);
    
    // Memory-safe internal operations
    WispErrorCode writeEntryInternal(uint32_t key, const void* data, uint8_t size, 
                                   uint8_t type, uint8_t partition, uint8_t flags);
    WispErrorCode readEntryInternal(uint32_t key, void* buffer, uint8_t maxSize, 
                                  uint8_t* actualSize, uint8_t partition);
    
    // Partition management with overflow protection
    uint8_t* getPartitionStart(uint8_t partitionId);
    uint16_t getPartitionSize(uint8_t partitionId);
    WispPartitionHeader* getPartitionHeader(uint8_t partitionId);
    uint16_t getPartitionFreeSpace(uint8_t partitionId);
    
    // Cache management (LRU eviction)
    void cacheEntry(uint32_t key, uint16_t size, uint16_t partition_offset);
    WispCacheEntry* findInCache(uint32_t key);
    void evictLRU();
    
    // Integrity and corruption detection
    bool verifyPartitionIntegrity(uint8_t partitionId);
    uint32_t calculateChecksum(const void* data, uint16_t size);
    void updatePartitionChecksum(uint8_t partitionId);
    
public:
    // Initialization with strict validation
    WispErrorCode initialize(const WispPartitionConfig* partitionConfig = nullptr);
    WispErrorCode reset(bool preserveROM = true);
    void cleanup();
    
    // Core operations with overflow protection
    WispErrorCode set(uint32_t key, const void* data, uint8_t size, uint8_t type = ENTRY_BYTES);
    WispErrorCode get(uint32_t key, void* buffer, uint8_t maxSize, uint8_t* actualSize = nullptr);
    WispErrorCode remove(uint32_t key);
    bool exists(uint32_t key);
    
    // Type-safe accessors with bounds checking
    WispErrorCode setU8(uint32_t key, uint8_t value);
    WispErrorCode setU16(uint32_t key, uint16_t value);
    WispErrorCode setU32(uint32_t key, uint32_t value);
    
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
    WispErrorCode getLastError();
    
    // Debugging and optimization
    void printMemoryMap();
    void printPartitionStats();
    float getFragmentationLevel(uint8_t partitionId);
};

// Global database instance
extern WispPartitionedDB wispDB;

// Memory-efficient app configuration helpers (all fit within 16KB safely)
#define WISP_DB_CONFIG_TINY() { \
    .romSize = 512, .saveSize = 512, .backupSize = 256, .runtimeSize = 512, \
    .enableCompression = false, .enableEncryption = false, .maxCacheEntries = 4, .safetyLevel = 1 \
}

#define WISP_DB_CONFIG_SMALL() { \
    .romSize = 1024, .saveSize = 1024, .backupSize = 512, .runtimeSize = 1024, \
    .enableCompression = false, .enableEncryption = false, .maxCacheEntries = 8, .safetyLevel = 1 \
}

#define WISP_DB_CONFIG_MEDIUM() { \
    .romSize = 2048, .saveSize = 2048, .backupSize = 1024, .runtimeSize = 2048, \
    .enableCompression = true, .enableEncryption = false, .maxCacheEntries = 16, .safetyLevel = 1 \
}

#define WISP_DB_CONFIG_LARGE() { \
    .romSize = 4096, .saveSize = 4096, .backupSize = 2048, .runtimeSize = 4096, \
    .enableCompression = true, .enableEncryption = true, .maxCacheEntries = 32, .safetyLevel = 1 \
}

#define WISP_DB_CONFIG_SAFE(rom, save, backup, runtime) { \
    .romSize = rom, .saveSize = save, .backupSize = backup, .runtimeSize = runtime, \
    .enableCompression = true, .enableEncryption = false, .maxCacheEntries = 16, .safetyLevel = 2 \
}

// Overflow-safe initialization with validation
#define WISP_DB_INIT_TINY()     wispDB.initialize(&(WispPartitionConfig)WISP_DB_CONFIG_TINY())
#define WISP_DB_INIT_SMALL()    wispDB.initialize(&(WispPartitionConfig)WISP_DB_CONFIG_SMALL())
#define WISP_DB_INIT_MEDIUM()   wispDB.initialize(&(WispPartitionConfig)WISP_DB_CONFIG_MEDIUM())
#define WISP_DB_INIT_LARGE()    wispDB.initialize(&(WispPartitionConfig)WISP_DB_CONFIG_LARGE())
#define WISP_DB_INIT_DEFAULT()  wispDB.initialize()  // Conservative 8KB total

// Memory efficiency helpers
#define WISP_DB_MEMORY_USED()   wispDB.getTotalUsedBytes()
#define WISP_DB_MEMORY_FREE()   wispDB.getTotalFreeBytes()
#define WISP_DB_MEMORY_PERCENT() ((WISP_DB_MEMORY_USED() * 100) / WISP_DB_LP_SRAM_SIZE)

// Safety checking macros for app code
#define WISP_DB_CHECK_SIZE(size) \
    ((size) > 0 && (size) <= 255) ? WISP_SUCCESS : WISP_ERROR_ENTRY_TOO_LARGE

#define WISP_DB_CHECK_SPACE(needed) \
    (wispDB.getTotalFreeBytes() >= (needed)) ? WISP_SUCCESS : WISP_ERROR_PARTITION_FULL

// Bounds-safe memory operations
#define WISP_SAFE_MEMCPY(dst, src, size, max_size) \
    do { \
        size_t copy_size = ((size) > (max_size)) ? (max_size) : (size); \
        memcpy(dst, src, copy_size); \
        if ((size) > (max_size)) return WISP_ERROR_BUFFER_OVERFLOW; \
    } while(0)

// Memory usage tracking for debugging
struct WispMemoryStats {
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

// Common item types
enum WispItemType : uint8_t {
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
enum WispQuestStage : uint32_t {
    QUEST_STAGE_1 = 0x00000001,
    QUEST_STAGE_2 = 0x00000002,
    QUEST_STAGE_3 = 0x00000004,
    QUEST_STAGE_4 = 0x00000008,
    QUEST_STAGE_5 = 0x00000010,
    QUEST_STAGE_6 = 0x00000020,
    QUEST_STAGE_7 = 0x00000040,
    QUEST_STAGE_8 = 0x00000080
};
