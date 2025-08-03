// database_system.h - ESP32-C6/S3 Database System Declarations
#pragma once

// Use central header for namespace organization  
#include "../../wisp_engine.h"
#include <esp_crc.h>

// Implement the Database namespace components
// Forward declarations to avoid namespace conflicts
#include <utility>   // Include before namespace to avoid conflicts
#include <type_traits>

// Note: Avoiding namespace due to C++ stdlib conflicts - using class prefixes instead

// Database configuration constants
#define WISP_DB_MAX_ITEMS 256
#define WISP_DB_LP_SRAM_SIZE 16384
#define WISP_DB_PARTITION_COUNT 4

// Database error codes
enum WispErrorCode {
    WISP_OK = 0,
    WISP_ERROR_INVALID_PARAM = 1,
    WISP_ERROR_NOT_INITIALIZED = 2,
    WISP_ERROR_OUT_OF_MEMORY = 3,
    WISP_ERROR_KEY_NOT_FOUND = 4,
    WISP_ERROR_PARTITION_FULL = 5,
    WISP_ERROR_INVALID_PARTITION = 6,
    WISP_ERROR_CHECKSUM_FAILED = 7,
    WISP_ERROR_STORAGE_FAILURE = 8
};

// Database partition types
enum WispPartitionType {
    WISP_PARTITION_ROM = 0,
    WISP_PARTITION_SAVE = 1,
    WISP_PARTITION_BACKUP = 2,
    WISP_PARTITION_RUNTIME = 3
};

// Database partition configuration
struct WispPartitionConfig {
    uint16_t romSize;
    uint16_t saveSize;
    uint16_t backupSize;
    uint16_t runtimeSize;
    bool enableSafety;
    bool enableBackup;
};

// Database statistics
struct WispDBStats {
    uint32_t totalEntries;
    uint32_t usedBytes;
    uint32_t freeBytes;
    uint32_t fragmentedBytes;
};

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
    void* config;

public:
    WispPartitionedDB();
    ~WispPartitionedDB();
    
    // Basic database operations
    bool init();
    void shutdown();
    bool isInitialized() const;
    
    // Data operations (to be implemented)
    bool store(const char* key, const void* data, uint32_t size);
    bool retrieve(const char* key, void* data, uint32_t maxSize);
    bool remove(const char* key);
    bool exists(const char* key);
};

// Global database instance
extern WispPartitionedDB* g_Database;
