// doc_data_format.h - Document Data Format
// Fast binary alternative to JSON for structured game data
#pragma once

#include <stdint.h>
#include <stddef.h>

// DocDataFormat Magic numbers
#define DDF_MAGIC 0x44444646     // "DDFF" (Doc Data Format)
#define DDF_VERSION 1
#define DDF_MAX_TABLES 16
#define DDF_MAX_COLUMNS 32
#define DDF_MAX_INDEXES 8
#define DDF_MAX_QUERY_OPS 16

// Data types for DocDataFormat columns
enum DDFType : uint8_t {
    DDF_TYPE_NULL = 0,
    DDF_TYPE_U8 = 1,
    DDF_TYPE_U16 = 2,
    DDF_TYPE_U32 = 3,
    DDF_TYPE_I8 = 4,
    DDF_TYPE_I16 = 5,
    DDF_TYPE_I32 = 6,
    DDF_TYPE_FLOAT = 7,
    DDF_TYPE_STRING = 8,      // Fixed-length string
    DDF_TYPE_BYTES = 9,       // Binary data
    DDF_TYPE_BOOL = 10
};

// Index types for fast queries
enum DDFIndexType : uint8_t {
    DDF_INDEX_NONE = 0,
    DDF_INDEX_PRIMARY = 1,    // Unique, sorted
    DDF_INDEX_UNIQUE = 2,     // Unique, unsorted
    DDF_INDEX_SORTED = 3,     // Non-unique, sorted
    DDF_INDEX_HASH = 4        // Hash table for exact matches
};

// Query operation codes
enum DDFQueryOp : uint8_t {
    DDF_OP_SELECT = 1,
    DDF_OP_WHERE = 2,
    DDF_OP_EQUALS = 3,
    DDF_OP_NOT_EQUALS = 4,
    DDF_OP_LESS = 5,
    DDF_OP_LESS_EQUAL = 6,
    DDF_OP_GREATER = 7,
    DDF_OP_GREATER_EQUAL = 8,
    DDF_OP_AND = 9,
    DDF_OP_OR = 10,
    DDF_OP_LIMIT = 11,
    DDF_OP_ORDER_BY = 12
};

// Column definition in table schema
struct DDFColumn {
    char name[16];           // Column name
    DDFType type;          // Data type
    uint8_t size;           // Size in bytes (for strings/bytes)
    uint8_t flags;          // Column flags (nullable, etc.)
    DDFIndexType indexType; // Index type for this column
    uint8_t reserved[3];    // Padding to 24 bytes
} __attribute__((packed));

// Index entry for fast lookups
struct DDFIndexEntry {
    uint32_t keyHash;       // Hash of the key value
    uint16_t rowId;         // Row ID in table
    uint16_t nextEntry;     // Next entry in hash chain (0 = end)
} __attribute__((packed));

// Table schema definition
struct DDFTableSchema {
    char name[16];          // Table name
    uint16_t columnCount;   // Number of columns
    uint16_t rowCount;      // Current number of rows
    uint16_t maxRows;       // Maximum rows allowed
    uint16_t rowSize;       // Size of each row in bytes
    uint16_t indexCount;    // Number of indexes
    uint8_t flags;          // Table flags
    uint8_t reserved;       // Padding
    DDFColumn columns[DDF_MAX_COLUMNS];  // Column definitions
    uint16_t indexOffsets[DDF_MAX_INDEXES]; // Offsets to index data
} __attribute__((packed));

// Database header with table registry
struct DDFHeader {
    uint32_t magic;         // DDF magic number
    uint16_t version;       // Format version
    uint16_t tableCount;    // Number of tables
    uint32_t totalSize;     // Total database size
    uint16_t schemaOffset;  // Offset to schema section
    uint16_t dataOffset;    // Offset to data section
    uint32_t checksum;      // Data integrity checksum
    uint16_t tableOffsets[DDF_MAX_TABLES]; // Offsets to each table
} __attribute__((packed));

// Query structure for binary queries
struct DDFQuery {
    uint16_t tableId;       // Target table ID
    uint8_t opCount;        // Number of operations
    uint8_t flags;          // Query flags
    uint8_t operations[DDF_MAX_QUERY_OPS * 4]; // Packed operations
} __attribute__((packed));

// Result set for query results
struct DDFResultSet {
    uint16_t rowCount;      // Number of matching rows
    uint16_t columnMask;    // Bitmask of selected columns
    uint16_t* rowIds;       // Array of matching row IDs
    uint16_t maxResults;    // Maximum results allocated
} __attribute__((packed));

// DDF Database class for structured data
class DDFDatabase {
private:
    uint8_t* data;          // Raw database memory
    uint32_t dataSize;      // Total allocated size
    DDFHeader* header;      // Database header
    DDFTableSchema* schemas; // Table schemas
    bool initialized;
    
    // Internal helpers
    uint32_t calculateHash(const void* data, size_t size);
    DDFTableSchema* getTableSchema(uint16_t tableId);
    uint8_t* getTableData(uint16_t tableId);
    DDFIndexEntry* getTableIndex(uint16_t tableId, uint8_t indexId);
    uint16_t findNextRowId(uint16_t tableId);
    
    // Index management
    bool buildIndex(uint16_t tableId, uint8_t columnId);
    bool updateIndex(uint16_t tableId, uint8_t columnId, uint16_t rowId, const void* value);
    uint16_t findByIndex(uint16_t tableId, uint8_t columnId, const void* value);
    
public:
    DDFDatabase();
    ~DDFDatabase();
    
    // Database lifecycle
    bool initialize(uint8_t* memory, uint32_t size);
    bool create(uint8_t* memory, uint32_t size);
    void shutdown();
    bool isValid();
    
    // Schema management
    uint16_t createTable(const char* name, const DDFColumn* columns, uint8_t columnCount, uint16_t maxRows);
    bool dropTable(uint16_t tableId);
    uint16_t getTableId(const char* name);
    DDFTableSchema* getTable(uint16_t tableId);
    
    // Data operations
    uint16_t insertRow(uint16_t tableId, const void* rowData);
    bool updateRow(uint16_t tableId, uint16_t rowId, const void* rowData);
    bool deleteRow(uint16_t tableId, uint16_t rowId);
    bool getRow(uint16_t tableId, uint16_t rowId, void* rowData);
    
    // Column operations
    bool setValue(uint16_t tableId, uint16_t rowId, uint8_t columnId, const void* value);
    bool getValue(uint16_t tableId, uint16_t rowId, uint8_t columnId, void* value);
    
    // Query operations
    bool executeQuery(const DDFQuery* query, DDFResultSet* results);
    bool simpleSelect(uint16_t tableId, const char* whereColumn, const void* whereValue, DDFResultSet* results);
    bool selectAll(uint16_t tableId, DDFResultSet* results);
    
    // Utility functions
    uint32_t getUsedMemory();
    uint32_t getFreeMemory();
    void printTableInfo(uint16_t tableId);
    void printAllTables();
    bool validate();
};

// Helper macros for common table operations
#define DDF_DEFINE_COLUMN(name, type, size, index) \
    { name, type, size, 0, index, {0} }

#define DDF_PRIMARY_KEY(name, type) \
    DDF_DEFINE_COLUMN(name, type, 0, DDF_INDEX_PRIMARY)

#define DDF_INDEXED_COLUMN(name, type, size) \
    DDF_DEFINE_COLUMN(name, type, size, DDF_INDEX_SORTED)

#define DDF_COLUMN(name, type, size) \
    DDF_DEFINE_COLUMN(name, type, size, DDF_INDEX_NONE)

// Query builder helpers
class DDFQueryBuilder {
private:
    DDFQuery query;
    uint8_t opIndex;
    
public:
    DDFQueryBuilder(uint16_t tableId);
    DDFQueryBuilder& where(const char* column, DDFQueryOp op, const void* value);
    DDFQueryBuilder& and_();
    DDFQueryBuilder& or_();
    DDFQueryBuilder& limit(uint16_t count);
    DDFQueryBuilder& orderBy(const char* column);
    DDFQuery build();
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
    extern const DDFColumn ITEM_COLUMNS[];
    extern const DDFColumn QUEST_COLUMNS[];
    extern const DDFColumn NPC_COLUMNS[];
    
    extern const uint8_t ITEM_COLUMN_COUNT;
    extern const uint8_t QUEST_COLUMN_COUNT;
    extern const uint8_t NPC_COLUMN_COUNT;
}
