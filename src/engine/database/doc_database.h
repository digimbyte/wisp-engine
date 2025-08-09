// doc_database.h - Document Database System for WispEngine
// Supports both key-value and structured data with table permissions
#pragma once

#include "../../wisp_engine.h"
#include "doc_data_format.h"
#include <esp_crc.h>
#include <stdint.h>
#include <stddef.h>

// Error code definitions for DocDatabase (using main engine error system)
#define WISP_ERROR_NOT_INITIALIZED      WispEngine::Database::ErrorCode::NOT_INITIALIZED  
#define WISP_ERROR_ALREADY_INITIALIZED  WispEngine::Database::ErrorCode::ALREADY_INITIALIZED
#define WISP_ERROR_INVALID_CONFIG       WispEngine::Database::ErrorCode::INVALID_CONFIG
#define WISP_ERROR_INVALID_PARAMS       WispEngine::Database::ErrorCode::INVALID_PARAM
#define WISP_ERROR_BUFFER_OVERFLOW      WispEngine::Database::ErrorCode::BUFFER_OVERFLOW
#define WISP_ERROR_INVALID_PARTITION    WispEngine::Database::ErrorCode::INVALID_PARTITION
#define WISP_ERROR_INVALID_KEY          WispEngine::Database::ErrorCode::INVALID_KEY
#define WISP_ERROR_ENTRY_TOO_LARGE      WispEngine::Database::ErrorCode::ENTRY_TOO_LARGE
#define WISP_ERROR_PARTITION_FULL       WispEngine::Database::ErrorCode::PARTITION_FULL
#define WISP_ERROR_OUT_OF_MEMORY        WispEngine::Database::ErrorCode::OUT_OF_MEMORY
#define WISP_ERROR_KEY_NOT_FOUND        WispEngine::Database::ErrorCode::KEY_NOT_FOUND

// Table permission flags
#define DDF_TABLE_READABLE   0x01
#define DDF_TABLE_WRITABLE   0x02  
#define DDF_TABLE_READ_ONLY  (DDF_TABLE_READABLE)
#define DDF_TABLE_READ_WRITE (DDF_TABLE_READABLE | DDF_TABLE_WRITABLE)

// Special table IDs for built-in functionality
#define DDF_KV_TABLE_ID      0x0001  // Key-value store table
#define DDF_META_TABLE_ID    0x0002  // Metadata table
#define DDF_CONFIG_TABLE_ID  0x0003  // Configuration table

// Key-value entry structure for unified storage
struct DDFKeyValueEntry {
    uint32_t key;           // 32-bit key
    uint8_t type;           // Data type (U8, U16, U32, STRING, BYTES)
    uint8_t size;           // Data size in bytes
    uint8_t data[58];       // Data payload (max 58 bytes to fit in 64-byte row)
} __attribute__((packed));

// Table metadata structure
struct DDFTableMeta {
    uint16_t tableId;       // Table identifier
    char name[16];          // Table name
    uint8_t permissions;    // Permission flags (read/write)
    uint8_t columnCount;    // Number of columns
    uint16_t maxRows;       // Maximum rows allowed
    uint16_t currentRows;   // Current row count
    uint32_t createdTime;   // Creation timestamp
    uint32_t modifiedTime;  // Last modification time
    uint32_t flags;         // Additional flags
} __attribute__((packed));

// Document database class - uses DDF for structured data with dynamic memory allocation
class DocDatabase {
private:
    DDFDatabase ddfCore;            // Core DDF engine for structured data
    bool initialized;               // Initialization status
    uint8_t* memory;                // Database memory pool
    uint32_t memorySize;            // Total memory size
    
    // Built-in table IDs
    uint16_t kvTableId;             // Key-value store table
    uint16_t metaTableId;           // Table metadata table
    uint16_t configTableId;         // Configuration table
    
    // Permission checking
    bool checkTablePermission(uint16_t tableId, uint8_t requiredPermission);
    bool isBuiltinTable(uint16_t tableId);
    
    // Internal setup
    WispErrorCode createBuiltinTables();
    
    // Key-value store helpers
    uint32_t hashKey(uint32_t key);
    WispErrorCode setKeyValue(uint32_t key, const void* data, uint8_t size, uint8_t type);
    WispErrorCode getKeyValue(uint32_t key, void* buffer, uint8_t maxSize, uint8_t* actualSize);
    
public:
    DocDatabase();
    ~DocDatabase();
    
    // === SYSTEM MANAGEMENT ===
    
    // Initialize the document database
    WispErrorCode initialize(uint32_t memorySize);
    void shutdown();
    bool isInitialized() const { return initialized; }
    
    // Memory management
    uint32_t getUsedMemory() const;
    uint32_t getFreeMemory() const;
    void printStats();
    
    // === KEY-VALUE STORE API ===
    
    // Type-safe key-value operations
    WispErrorCode setU8(uint32_t key, uint8_t value);
    WispErrorCode setU16(uint32_t key, uint16_t value);
    WispErrorCode setU32(uint32_t key, uint32_t value);
    WispErrorCode setFloat(uint32_t key, float value);
    WispErrorCode setString(uint32_t key, const char* value);
    WispErrorCode setBytes(uint32_t key, const void* data, uint8_t size);
    
    uint8_t getU8(uint32_t key, uint8_t defaultValue = 0);
    uint16_t getU16(uint32_t key, uint16_t defaultValue = 0);
    uint32_t getU32(uint32_t key, uint32_t defaultValue = 0);
    float getFloat(uint32_t key, float defaultValue = 0.0f);
    bool getString(uint32_t key, char* buffer, uint8_t bufferSize);
    WispErrorCode getBytes(uint32_t key, void* buffer, uint8_t maxSize, uint8_t* actualSize);
    
    // Key-value utilities
    bool existsKey(uint32_t key);
    WispErrorCode removeKey(uint32_t key);
    
    // === STRUCTURED DATA API ===
    
    // Table management with permissions
    uint16_t createTable(const char* name, const DDFColumn* columns, uint8_t columnCount, 
                        uint16_t maxRows, uint8_t permissions = DDF_TABLE_READ_WRITE);
    WispErrorCode dropTable(uint16_t tableId);
    WispErrorCode setTablePermissions(uint16_t tableId, uint8_t permissions);
    uint8_t getTablePermissions(uint16_t tableId);
    
    // Data operations with permission checking
    uint16_t insertRow(uint16_t tableId, const void* rowData);
    WispErrorCode updateRow(uint16_t tableId, uint16_t rowId, const void* rowData);
    WispErrorCode getRow(uint16_t tableId, uint16_t rowId, void* rowData);
    WispErrorCode deleteRow(uint16_t tableId, uint16_t rowId);
    
    // Query operations
    WispErrorCode selectAll(uint16_t tableId, DDFResultSet* results);
    WispErrorCode simpleSelect(uint16_t tableId, const char* whereColumn, 
                              const void* whereValue, DDFResultSet* results);
    WispErrorCode executeQuery(const DDFQuery* query, DDFResultSet* results);
    
    // Table utilities
    uint16_t getTableId(const char* name);
    DDFTableSchema* getTableSchema(uint16_t tableId);
    bool existsTable(uint16_t tableId);
    void printTableInfo(uint16_t tableId);
    void printAllTables();
    
    // === GAME-SPECIFIC HELPERS ===
    
    // Create common game tables with appropriate permissions
    void createGameTables();
    
    // Items table (writable)
    uint16_t addItem(uint16_t itemId, const char* name, uint8_t category, uint8_t rarity, uint32_t value);
    bool getItem(uint16_t itemId, void* itemData);
    WispErrorCode findItemsByCategory(uint8_t category, uint16_t* results, uint16_t* count, uint16_t maxResults);
    
    // Quests table (writable)
    uint16_t addQuest(uint16_t questId, const char* title, uint8_t status, uint16_t prerequisite);
    bool getQuest(uint16_t questId, void* questData);
    WispErrorCode updateQuestStatus(uint16_t questId, uint8_t status, uint8_t progress);
    WispErrorCode findQuestsByStatus(uint8_t status, uint16_t* results, uint16_t* count, uint16_t maxResults);
    
    // NPCs table (read-only by default, can be made writable)
    uint16_t addNPC(uint16_t npcId, const char* name, uint8_t level, uint8_t faction, uint16_t x, uint16_t y);
    bool getNPC(uint16_t npcId, void* npcData);
    WispErrorCode findNPCsByFaction(uint8_t faction, uint16_t* results, uint16_t* count, uint16_t maxResults);
    
    // === ADVANCED FEATURES ===
    
    // Transaction support (basic)
    WispErrorCode beginTransaction();
    WispErrorCode commitTransaction();
    WispErrorCode rollbackTransaction();
    
    // Validation and integrity
    bool validateDatabase();
    WispErrorCode compactDatabase();
    
    // Configuration management
    WispErrorCode setConfig(const char* key, const char* value);
    bool getConfig(const char* key, char* buffer, uint8_t bufferSize);
    
    // === DIRECT DDF ACCESS ===
    
    // For advanced users who need direct DDF access
    DDFDatabase* getCore() { return initialized ? &ddfCore : nullptr; }
};

// Global document database instance
extern DocDatabase docDB;

// Convenience macros for common operations
#define DOC_SET_VALUE(key, value) docDB.setU32(key, value)
#define DOC_GET_VALUE(key, default_val) docDB.getU32(key, default_val)
#define DOC_SET_CONFIG(key, value) docDB.setConfig(key, value)
#define DOC_GET_CONFIG(key, buffer, size) docDB.getConfig(key, buffer, size)

// Table creation helpers
#define DDF_DEFINE_TABLE(name, ...) \
    static const DDFColumn name##_columns[] = { __VA_ARGS__ }; \
    static const uint8_t name##_column_count = sizeof(name##_columns) / sizeof(DDFColumn);

// Permission checking macros
#define DDF_REQUIRE_READ(tableId) \
    if (!checkTablePermission(tableId, DDF_TABLE_READABLE)) return WISP_ERROR_INVALID_PARTITION;

#define DDF_REQUIRE_WRITE(tableId) \
    if (!checkTablePermission(tableId, DDF_TABLE_WRITABLE)) return WISP_ERROR_INVALID_PARTITION;
