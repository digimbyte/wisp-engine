// wbdf_format.h - Wisp Binary Document Format
// Fast binary alternative to JSON for structured game data
#pragma once

#include <stdint.h>
#include <stddef.h>

// WBDF Magic numbers
#define WBDF_MAGIC 0x57424446     // "WBDF"
#define WBDF_VERSION 1
#define WBDF_MAX_TABLES 16
#define WBDF_MAX_COLUMNS 32
#define WBDF_MAX_INDEXES 8
#define WBDF_MAX_QUERY_OPS 16

// Data types for WBDF columns
enum WBDFType : uint8_t {
    WBDF_TYPE_NULL = 0,
    WBDF_TYPE_U8 = 1,
    WBDF_TYPE_U16 = 2,
    WBDF_TYPE_U32 = 3,
    WBDF_TYPE_I8 = 4,
    WBDF_TYPE_I16 = 5,
    WBDF_TYPE_I32 = 6,
    WBDF_TYPE_FLOAT = 7,
    WBDF_TYPE_STRING = 8,      // Fixed-length string
    WBDF_TYPE_BYTES = 9,       // Binary data
    WBDF_TYPE_BOOL = 10
};

// Index types for fast queries
enum WBDFIndexType : uint8_t {
    WBDF_INDEX_NONE = 0,
    WBDF_INDEX_PRIMARY = 1,    // Unique, sorted
    WBDF_INDEX_UNIQUE = 2,     // Unique, unsorted
    WBDF_INDEX_SORTED = 3,     // Non-unique, sorted
    WBDF_INDEX_HASH = 4        // Hash table for exact matches
};

// Query operation codes
enum WBDFQueryOp : uint8_t {
    WBDF_OP_SELECT = 1,
    WBDF_OP_WHERE = 2,
    WBDF_OP_EQUALS = 3,
    WBDF_OP_NOT_EQUALS = 4,
    WBDF_OP_LESS = 5,
    WBDF_OP_LESS_EQUAL = 6,
    WBDF_OP_GREATER = 7,
    WBDF_OP_GREATER_EQUAL = 8,
    WBDF_OP_AND = 9,
    WBDF_OP_OR = 10,
    WBDF_OP_LIMIT = 11,
    WBDF_OP_ORDER_BY = 12
};

// Column definition in table schema
struct WBDFColumn {
    char name[16];           // Column name
    WBDFType type;          // Data type
    uint8_t size;           // Size in bytes (for strings/bytes)
    uint8_t flags;          // Column flags (nullable, etc.)
    WBDFIndexType indexType; // Index type for this column
    uint8_t reserved[3];    // Padding to 24 bytes
} __attribute__((packed));

// Index entry for fast lookups
struct WBDFIndexEntry {
    uint32_t keyHash;       // Hash of the key value
    uint16_t rowId;         // Row ID in table
    uint16_t nextEntry;     // Next entry in hash chain (0 = end)
} __attribute__((packed));

// Table schema definition
struct WBDFTableSchema {
    char name[16];          // Table name
    uint16_t columnCount;   // Number of columns
    uint16_t rowCount;      // Current number of rows
    uint16_t maxRows;       // Maximum rows allowed
    uint16_t rowSize;       // Size of each row in bytes
    uint16_t indexCount;    // Number of indexes
    uint8_t flags;          // Table flags
    uint8_t reserved;       // Padding
    WBDFColumn columns[WBDF_MAX_COLUMNS];  // Column definitions
    uint16_t indexOffsets[WBDF_MAX_INDEXES]; // Offsets to index data
} __attribute__((packed));

// Database header with table registry
struct WBDFHeader {
    uint32_t magic;         // WBDF magic number
    uint16_t version;       // Format version
    uint16_t tableCount;    // Number of tables
    uint32_t totalSize;     // Total database size
    uint16_t schemaOffset;  // Offset to schema section
    uint16_t dataOffset;    // Offset to data section
    uint32_t checksum;      // Data integrity checksum
    uint16_t tableOffsets[WBDF_MAX_TABLES]; // Offsets to each table
} __attribute__((packed));

// Query structure for binary queries
struct WBDFQuery {
    uint16_t tableId;       // Target table ID
    uint8_t opCount;        // Number of operations
    uint8_t flags;          // Query flags
    uint8_t operations[WBDF_MAX_QUERY_OPS * 4]; // Packed operations
} __attribute__((packed));

// Result set for query results
struct WBDFResultSet {
    uint16_t rowCount;      // Number of matching rows
    uint16_t columnMask;    // Bitmask of selected columns
    uint16_t* rowIds;       // Array of matching row IDs
    uint16_t maxResults;    // Maximum results allocated
} __attribute__((packed));

// WBDF Database class for structured data
class WBDFDatabase {
private:
    uint8_t* data;          // Raw database memory
    uint32_t dataSize;      // Total allocated size
    WBDFHeader* header;     // Database header
    WBDFTableSchema* schemas; // Table schemas
    bool initialized;
    
    // Internal helpers
    uint32_t calculateHash(const void* data, size_t size);
    WBDFTableSchema* getTableSchema(uint16_t tableId);
    uint8_t* getTableData(uint16_t tableId);
    WBDFIndexEntry* getTableIndex(uint16_t tableId, uint8_t indexId);
    uint16_t findNextRowId(uint16_t tableId);
    
    // Index management
    bool buildIndex(uint16_t tableId, uint8_t columnId);
    bool updateIndex(uint16_t tableId, uint8_t columnId, uint16_t rowId, const void* value);
    uint16_t findByIndex(uint16_t tableId, uint8_t columnId, const void* value);
    
public:
    WBDFDatabase();
    ~WBDFDatabase();
    
    // Database lifecycle
    bool initialize(uint8_t* memory, uint32_t size);
    bool create(uint8_t* memory, uint32_t size);
    void shutdown();
    bool isValid();
    
    // Schema management
    uint16_t createTable(const char* name, const WBDFColumn* columns, uint8_t columnCount, uint16_t maxRows);
    bool dropTable(uint16_t tableId);
    uint16_t getTableId(const char* name);
    WBDFTableSchema* getTable(uint16_t tableId);
    
    // Data operations
    uint16_t insertRow(uint16_t tableId, const void* rowData);
    bool updateRow(uint16_t tableId, uint16_t rowId, const void* rowData);
    bool deleteRow(uint16_t tableId, uint16_t rowId);
    bool getRow(uint16_t tableId, uint16_t rowId, void* rowData);
    
    // Column operations
    bool setValue(uint16_t tableId, uint16_t rowId, uint8_t columnId, const void* value);
    bool getValue(uint16_t tableId, uint16_t rowId, uint8_t columnId, void* value);
    
    // Query operations
    bool executeQuery(const WBDFQuery* query, WBDFResultSet* results);
    bool simpleSelect(uint16_t tableId, const char* whereColumn, const void* whereValue, WBDFResultSet* results);
    bool selectAll(uint16_t tableId, WBDFResultSet* results);
    
    // Utility functions
    uint32_t getUsedMemory();
    uint32_t getFreeMemory();
    void printTableInfo(uint16_t tableId);
    void printAllTables();
    bool validate();
};

// Helper macros for common table operations
#define WBDF_DEFINE_COLUMN(name, type, size, index) \
    { name, type, size, 0, index, {0} }

#define WBDF_PRIMARY_KEY(name, type) \
    WBDF_DEFINE_COLUMN(name, type, 0, WBDF_INDEX_PRIMARY)

#define WBDF_INDEXED_COLUMN(name, type, size) \
    WBDF_DEFINE_COLUMN(name, type, size, WBDF_INDEX_SORTED)

#define WBDF_COLUMN(name, type, size) \
    WBDF_DEFINE_COLUMN(name, type, size, WBDF_INDEX_NONE)

// Query builder helpers
class WBDFQueryBuilder {
private:
    WBDFQuery query;
    uint8_t opIndex;
    
public:
    WBDFQueryBuilder(uint16_t tableId);
    WBDFQueryBuilder& where(const char* column, WBDFQueryOp op, const void* value);
    WBDFQueryBuilder& and_();
    WBDFQueryBuilder& or_();
    WBDFQueryBuilder& limit(uint16_t count);
    WBDFQueryBuilder& orderBy(const char* column);
    WBDFQuery build();
};

// Example usage structures for common game data
namespace GameTables {
    // Items table structure
    struct Item {
        uint16_t id;           // Primary key
        char name[32];         // Item name
        uint8_t category;      // Item category
        uint8_t rarity;        // Rarity level
        uint32_t value;        // Base value
        uint16_t stackSize;    // Max stack size
        uint8_t flags;         // Item flags
        uint8_t reserved;
    } __attribute__((packed));
    
    // Quests table structure
    struct Quest {
        uint16_t id;           // Primary key
        char title[48];        // Quest title
        uint8_t status;        // Quest status
        uint8_t progress;      // Progress percentage
        uint32_t flags;        // Quest flags
        uint16_t prerequisite; // Required quest ID
        uint16_t reward_item;  // Reward item ID
        uint32_t reward_exp;   // Experience reward
    } __attribute__((packed));
    
    // NPCs table structure
    struct NPC {
        uint16_t id;           // Primary key
        char name[24];         // NPC name
        uint8_t level;         // NPC level
        uint8_t faction;       // Faction ID
        uint16_t location_x;   // X coordinate
        uint16_t location_y;   // Y coordinate
        uint32_t flags;        // NPC flags
        uint16_t dialogue_id;  // Dialogue tree ID
    } __attribute__((packed));
    
    // Helper functions to define table schemas
    extern const WBDFColumn ITEM_COLUMNS[];
    extern const WBDFColumn QUEST_COLUMNS[];
    extern const WBDFColumn NPC_COLUMNS[];
    
    extern const uint8_t ITEM_COLUMN_COUNT;
    extern const uint8_t QUEST_COLUMN_COUNT;
    extern const uint8_t NPC_COLUMN_COUNT;
}
