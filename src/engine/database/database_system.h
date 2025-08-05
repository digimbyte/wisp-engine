// database_system.h - ESP32-C6/S3 Database System Declarations
#pragma once

// Use central header for namespace organization  
#include "../../wisp_engine.h"
#include <esp_crc.h>

// RTC memory attribute for persistent data
#ifndef RTC_DATA_ATTR
#define RTC_DATA_ATTR __attribute__((section(".rtc.data")))
#endif

// Implement the Database namespace components
// Forward declarations to avoid namespace conflicts
#include <utility>   // Include before namespace to avoid conflicts
#include <type_traits>

// Note: Avoiding namespace due to C++ stdlib conflicts - using class prefixes instead

// Database configuration constants
#define WISP_DB_MAX_ITEMS 256
#define WISP_DB_LP_SRAM_SIZE 16384
#define WISP_DB_PARTITION_COUNT 4

// Partition index constants (matching the enum in wisp_engine.h)
// Use static const to ensure proper type compatibility with WispPartitionType
static const WispPartitionType WISP_DB_PARTITION_ROM     = static_cast<WispPartitionType>(0);
static const WispPartitionType WISP_DB_PARTITION_SAVE    = static_cast<WispPartitionType>(1);
static const WispPartitionType WISP_DB_PARTITION_BACKUP  = static_cast<WispPartitionType>(2);
static const WispPartitionType WISP_DB_PARTITION_RUNTIME = static_cast<WispPartitionType>(3);

// Partition size constants
#define WISP_DB_ROM_PARTITION_SIZE 4096
#define WISP_DB_SAVE_PARTITION_SIZE 4096
#define WISP_DB_BACKUP_PARTITION_SIZE 2048
#define WISP_DB_RUNTIME_PARTITION_SIZE 6144
#define WISP_DB_MAX_TOTAL_SIZE (WISP_DB_ROM_PARTITION_SIZE + WISP_DB_SAVE_PARTITION_SIZE + WISP_DB_BACKUP_PARTITION_SIZE + WISP_DB_RUNTIME_PARTITION_SIZE)

// Error code constants - map to WispEngine::Database::ErrorCode enum
#define WISP_ERROR_NOT_INITIALIZED      WispEngine::Database::ErrorCode::NOT_INITIALIZED  
#define WISP_ERROR_ALREADY_INITIALIZED  WispEngine::Database::ErrorCode::ALREADY_INITIALIZED
#define WISP_ERROR_INVALID_CONFIG       WispEngine::Database::ErrorCode::INVALID_CONFIG
#define WISP_ERROR_INVALID_PARAMS       WispEngine::Database::ErrorCode::INVALID_PARAM
#define WISP_ERROR_BUFFER_OVERFLOW      WispEngine::Database::ErrorCode::BUFFER_OVERFLOW
#define WISP_ERROR_INVALID_PARTITION    WispEngine::Database::ErrorCode::INVALID_PARTITION
#define WISP_ERROR_INVALID_KEY          WispEngine::Database::ErrorCode::INVALID_KEY
#define WISP_ERROR_ENTRY_TOO_LARGE      WispEngine::Database::ErrorCode::ENTRY_TOO_LARGE
#define WISP_ERROR_PARTITION_FULL       WispEngine::Database::ErrorCode::PARTITION_FULL
#define WISP_ERROR_INDEX_OVERFLOW       WispEngine::Database::ErrorCode::INDEX_OVERFLOW
#define WISP_ERROR_MEMORY_EXCEEDED      WispEngine::Database::ErrorCode::MEMORY_EXCEEDED
#define WISP_ERROR_OUT_OF_MEMORY        WispEngine::Database::ErrorCode::OUT_OF_MEMORY
#define WISP_ERROR_KEY_NOT_FOUND        WispEngine::Database::ErrorCode::KEY_NOT_FOUND

// Additional missing error codes
#define WISP_ERROR_PARTITION_NOT_FOUND  WispEngine::Database::ErrorCode::INVALID_PARTITION
#define WISP_ERROR_READ_ONLY_PARTITION  WispEngine::Database::ErrorCode::INVALID_PARTITION  
#define WISP_ERROR_INSUFFICIENT_SPACE   WispEngine::Database::ErrorCode::PARTITION_FULL
#define WISP_ERROR_BUFFER_TOO_SMALL     WispEngine::Database::ErrorCode::BUFFER_OVERFLOW
#define WISP_ERROR_CHECKSUM_MISMATCH    WispEngine::Database::ErrorCode::CHECKSUM_FAILED
#define WISP_ERROR_MEMORY_CORRUPTED     WispEngine::Database::ErrorCode::STORAGE_FAILURE

// Success constant
#define WISP_SUCCESS WispEngine::Database::ErrorCode::OK

// Database version and constants
#define WISP_DB_VERSION  1
#define WISP_DB_MAGIC    0x57495350  // "WISP"

// Entry validation macros  
#define WISP_ENTRY_SIZE_VALID(size) ((size) > 0 && (size) <= 64)

// Entry type constants
#define ENTRY_U8     1
#define ENTRY_U16    2  
#define ENTRY_U32    3

// Database constants and limits
#define WISP_DB_MAX_ENTRIES_PER_PARTITION 128
// WISP_DB_LP_SRAM_SIZE already defined above (16384)

// Partition flag constants (todo.md #10)
#define PARTITION_FLAG_COMPRESSED  0x01
#define PARTITION_FLAG_ENCRYPTED   0x02
#define PARTITION_FLAG_READ_ONLY   0x04

// Key parsing macros (todo.md #12) - assumes 8-bit category, 8-bit namespace, 16-bit ID
#define WISP_KEY_CATEGORY(k)   ((uint8_t)((k) >> 24))      
#define WISP_KEY_NAMESPACE(k)  ((uint8_t)(((k) >> 16) & 0xFF))  
#define WISP_KEY_ID(k)         ((uint16_t)((k) & 0xFFFF))

// Entry structures for database operations
struct WispEntry {
    uint32_t key;
    uint8_t size;
    uint8_t type;
    uint16_t offset;
    uint8_t flags;
    uint32_t timestamp;
    uint16_t checksum;
    uint8_t data[];  // Variable size data
};

struct WispEntryHeader {
    uint32_t key;
    uint8_t size;
    uint8_t type_and_flags;  // Combined type and flags field
    uint16_t offset;
};

// Error codes and partition types are defined in wisp_engine.h

// Partition header structure (todo.md #9)
struct WispPartitionHeader {
    uint32_t magic;         // identifier (e.g. "WISP")  
    uint16_t version;       // database format version  
    uint8_t  partitionType; // enum value for ROM/SAVE/BACKUP/RUNTIME  
    uint8_t  flags;         // bit flags (compression, encryption, etc.)  
    uint32_t checksum;      // data integrity check
    uint16_t entryCount;    // Number of entries in partition
    uint16_t freeSpace;     // Number of free bytes
    uint16_t usedBytes;     // Number of used bytes (for safe mode compatibility)
    uint16_t totalSize;     // Total partition size (for safe mode compatibility)
    uint32_t reserved;      // Reserved for future use
};

// Magic numbers and constants
#define WISP_PARTITION_MAGIC 0x57495350  // "WISP"

// Database statistics structure
struct WispDBStats {
    uint32_t totalSize;
    uint32_t usedSize;
    uint32_t freeSize;
    uint32_t totalEntries;  // Total number of entries across all partitions
    uint16_t partitionEntries[4];
    uint32_t partitionSizes[4];
    uint32_t partitionUsed[4];
    bool compressionEnabled;
    bool encryptionEnabled;
};

// Cache entry structure for safe mode
struct WispCacheEntry {
    uint32_t key;
    uint16_t size;  // Renamed from dataSize for consistency
    uint16_t partition;
    uint16_t partition_offset;
    uint32_t access_time;
    uint8_t data[64];  // Small cache entry data
};

// Database partition configuration
struct WispPartitionConfig {
    uint16_t romSize;
    uint16_t saveSize;
    uint16_t backupSize;
    uint16_t runtimeSize;
    bool enableSafety;
    bool enableBackup;
    bool enableCompression;
    bool enableEncryption;
    uint8_t maxCacheEntries;  // For safe mode caching
    uint8_t safetyLevel;      // Safety level for database operations
};

// Configuration validation macro
#define WISP_VALIDATE_CONFIG(cfg) ((cfg) && (cfg)->romSize > 0 && (cfg)->saveSize > 0)

// Database types enumeration
enum WispDBType {
    DB_TYPE_ITEM = 1,
    DB_TYPE_QUEST = 2,
    DB_TYPE_STATE = 3,
    DB_TYPE_INVENTORY = 4,
    DB_TYPE_CONFIG = 5
};

// Database header structure
struct WispDBHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t entryCount;
    uint32_t checksum;
    uint32_t reserved[4];
};

// Generic database entry structure
struct WispDBEntry {
    uint16_t id;
    uint8_t type;
    uint8_t flags;
    uint32_t data[4]; // Generic data storage
};

// Item system structures
struct WispItem {
    uint16_t itemId;
    char name[64];
    char description[128];
    uint8_t category;
    uint8_t rarity;
    uint32_t value;
    uint8_t stackable;
    
    WispDBEntry toDBEntry() const;
    static WispItem fromDBEntry(const WispDBEntry& entry);
};

// Quest system structures
struct WispQuest {
    uint16_t questId;
    char title[64];
    char description[256];
    uint8_t status;             // Quest status (0=not started, 1=active, 2=complete, etc.)
    uint8_t progress;
    uint32_t flags;
    
    WispDBEntry toDBEntry() const;
    static WispQuest fromDBEntry(const WispDBEntry& entry);
};

// Game state structures
struct WispGameState {
    uint16_t stateId;
    uint8_t type;
    uint8_t reserved;
    uint32_t value;
    
    WispDBEntry toDBEntry() const;
    static WispGameState fromDBEntry(const WispDBEntry& entry);
};

// Inventory system structures
struct WispInventorySlot {
    uint16_t itemId;
    uint8_t quantity;
    uint8_t condition;
    uint32_t flags;
    
    WispDBEntry toDBEntry() const;
    static WispInventorySlot fromDBEntry(const WispDBEntry& entry);
};

// Main database system class
class WispDatabaseSystem {
private:
    static bool initialized;
    static WispDBHeader header;
    static WispDBEntry entries[WISP_DB_MAX_ITEMS];
    
    // Helper methods
    static void updateChecksum();
    static uint16_t findEntryIndex(uint16_t id, WispDBType type);
    static bool isValidEntry(const WispDBEntry& entry);

public:
    // System management
    static bool init();
    static void shutdown();
    static bool isInitialized();
    static uint32_t getMemoryUsed();
    
    // Item management
    static bool addItem(const WispItem& item);
    static bool updateItem(uint16_t itemId, const WispItem& item);
    static bool removeItem(uint16_t itemId);
    static WispItem getItem(uint16_t itemId);
    static bool hasItem(uint16_t itemId);
    
    // Quest management
    static bool addQuest(const WispQuest& quest);
    static bool completeQuest(uint16_t questId);
    static WispQuest getQuest(uint16_t questId);
    static bool isQuestCompleted(uint16_t questId);
    static bool isQuestActive(uint16_t questId);
    
    // State management
    static bool setState(uint16_t stateId, uint32_t value, uint8_t type = 0);
    static uint32_t getState(uint16_t stateId);
    static bool hasState(uint16_t stateId);
    static bool toggleFlag(uint16_t flagId);
    static bool getFlag(uint16_t flagId);
    
    // Inventory management
    static bool addToInventory(uint16_t itemId, uint8_t quantity = 1);
    static bool hasInInventory(uint16_t itemId, uint8_t quantity = 1);
    static uint8_t getInventoryCount(uint16_t itemId);
    static void getInventory(WispInventorySlot* inventory, uint8_t* numSlots, uint8_t maxSlots);
    
    // Debug and statistics
    static void printDatabaseStats();
    static void printInventory();
    static void printActiveQuests();
};

// Forward declarations for database system
class WispPartitionedDB {
private:
    bool initialized;
    WispPartitionConfig config;  // Was void* config

    // Member variables used in implementation
    size_t cacheSize;
    size_t cacheCount;
    
    // Partition pointers
    uint8_t* romPartition;
    uint8_t* savePartition;
    uint8_t* backupPartition;
    uint8_t* runtimePartition;
    
    // Partition arrays and sizes
    uint8_t* partitions[WISP_DB_PARTITION_COUNT];
    size_t partitionSizes[WISP_DB_PARTITION_COUNT];
    
    // Cache pointer
    WispCacheEntry* cache;
    
    // Partition sizes
    uint16_t romSize;
    uint16_t saveSize;
    uint16_t backupSize;
    uint16_t runtimeSize;

public:
    // Static data member for RTC RAM
    static RTC_DATA_ATTR uint8_t lpSramData[WISP_DB_LP_SRAM_SIZE];

    WispPartitionedDB();
    ~WispPartitionedDB();
    
    // Basic database operations
    bool init();
    void shutdown();
    bool isInitialized() const;
    void cleanup();
    
    // New API functions used in implementation
    WispErrorCode initialize(const WispPartitionConfig* cfg);
    WispErrorCode allocatePartitions();
    WispErrorCode initializePartitions();
    WispErrorCode setupPartitions();
    WispErrorCode initializePartitionHeaders();
    
    // Data operations with new signatures
    WispErrorCode set(uint32_t key, const void* data, size_t size, uint8_t flags, WispPartitionType type, uint8_t extra);
    WispErrorCode get(uint32_t key, void* buffer, size_t bufferSize, WispPartitionType type);
    WispErrorCode searchPartition(uint32_t key, void* buffer, size_t bufferSize, WispPartitionType type);
    WispErrorCode remove(uint32_t key, WispPartitionType partition);
    
    // Alternative set signatures used in implementation
    WispErrorCode set(uint32_t key, const void* data, uint8_t size, uint8_t type);
    WispErrorCode get(uint32_t key, void* buffer, uint8_t maxSize, uint8_t* actualSize);
    
    // Type-specific getters with reference keys (used in safe mode)
    WispErrorCode get(uint32_t& key, uint8_t* buffer, unsigned int bufferSize);
    WispErrorCode get(uint32_t& key, uint16_t* buffer, unsigned int bufferSize);
    WispErrorCode get(uint32_t& key, uint32_t* buffer, unsigned int bufferSize);
    
    // Type-specific setters
    WispErrorCode setU8(uint32_t key, uint8_t value);
    WispErrorCode setU16(uint32_t key, uint16_t value);
    WispErrorCode setU32(uint32_t key, uint32_t value);
    
    // Validation and utility functions
    WispErrorCode validatePointer(const void* ptr, uint16_t size, uint8_t partition);
    WispErrorCode validateEntry(uint32_t key, uint16_t size);
    bool isValidKey(uint32_t key);
    
    // Internal functions
    WispErrorCode writeEntryInternal(uint32_t key, const void* data, uint8_t size, uint8_t type, uint8_t partition, uint8_t flags = 0);
    WispErrorCode readEntryInternal(uint32_t key, void* buffer, uint8_t maxSize, uint8_t* actualSize, uint8_t partition);
    
    // Partition management
    uint8_t* getPartitionStart(uint8_t partitionId);
    uint16_t getPartitionSize(uint8_t partitionId);
    void updatePartitionChecksum(uint8_t partitionId);
    uint16_t calculateChecksum(const void* data, size_t size);
    uint32_t calculateChecksum(const void* data, uint16_t size);  // Alternative signature for safe mode
    
    // Cache management
    void cacheEntry(uint32_t key, uint16_t size, uint16_t partition_offset);
    
    // Timestamp utility
    uint32_t getCurrentTimestamp();
    
    // Missing method declarations from safe mode (safe mode functions)
    bool exists(uint32_t key);
    bool exists(uint32_t key, WispPartitionType partition);
    WispErrorCode getStats(WispDBStats* stats);
    WispErrorCode defragment(WispPartitionType partition);
    
    // Safe mode specific methods
    uint8_t getU8(uint32_t key, uint8_t defaultValue = 0);
    uint16_t getU16(uint32_t key, uint16_t defaultValue = 0);
    uint32_t getU32(uint32_t key, uint32_t defaultValue = 0);
    uint16_t getTotalUsedBytes();
    uint16_t getTotalFreeBytes();
    uint16_t getPartitionUsedBytes(uint8_t partitionId);
    uint16_t getPartitionFreeBytes(uint8_t partitionId);
    bool validateDatabase();
    void printMemoryMap();
    uint8_t getEntryCount(uint8_t partitionId);
    
    // Legacy data operations (for backward compatibility)
    bool store(const char* key, const void* data, uint32_t size);
    bool retrieve(const char* key, void* data, uint32_t maxSize);
    bool remove(const char* key);
    bool exists(const char* key);
};

// Global database instance
extern WispPartitionedDB* g_Database;
