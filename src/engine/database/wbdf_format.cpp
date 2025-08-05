// wbdf_format.cpp - Wisp Binary Document Format Implementation
#include "wbdf_format.h"
#include <string.h>
#include <cstdio>
#include <algorithm>

// Hash function for index keys
uint32_t WBDFDatabase::calculateHash(const void* data, size_t size) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint32_t hash = 2166136261u;  // FNV-1a hash
    
    for (size_t i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    
    return hash;
}

WBDFDatabase::WBDFDatabase() : data(nullptr), dataSize(0), header(nullptr), schemas(nullptr), initialized(false) {
}

WBDFDatabase::~WBDFDatabase() {
    shutdown();
}

bool WBDFDatabase::initialize(uint8_t* memory, uint32_t size) {
    if (initialized || !memory || size < sizeof(WBDFHeader)) {
        return false;
    }
    
    data = memory;
    dataSize = size;
    header = reinterpret_cast<WBDFHeader*>(data);
    
    // Validate existing database
    if (header->magic != WBDF_MAGIC || header->version != WBDF_VERSION) {
        return false;
    }
    
    if (header->totalSize > dataSize) {
        return false;
    }
    
    // Set up schema pointer
    if (header->schemaOffset > 0) {
        schemas = reinterpret_cast<WBDFTableSchema*>(data + header->schemaOffset);
    }
    
    initialized = true;
    return true;
}

bool WBDFDatabase::create(uint8_t* memory, uint32_t size) {
    if (initialized || !memory || size < sizeof(WBDFHeader)) {
        return false;
    }
    
    data = memory;
    dataSize = size;
    
    // Initialize header
    header = reinterpret_cast<WBDFHeader*>(data);
    memset(header, 0, sizeof(WBDFHeader));
    
    header->magic = WBDF_MAGIC;
    header->version = WBDF_VERSION;
    header->tableCount = 0;
    header->totalSize = sizeof(WBDFHeader);
    header->schemaOffset = sizeof(WBDFHeader);
    header->dataOffset = sizeof(WBDFHeader) + (WBDF_MAX_TABLES * sizeof(WBDFTableSchema));
    
    // Initialize schema area
    schemas = reinterpret_cast<WBDFTableSchema*>(data + header->schemaOffset);
    memset(schemas, 0, WBDF_MAX_TABLES * sizeof(WBDFTableSchema));
    
    initialized = true;
    return true;
}

void WBDFDatabase::shutdown() {
    data = nullptr;
    dataSize = 0;
    header = nullptr;
    schemas = nullptr;
    initialized = false;
}

bool WBDFDatabase::isValid() {
    if (!initialized || !header) {
        return false;
    }
    
    return header->magic == WBDF_MAGIC && header->version == WBDF_VERSION;
}

uint16_t WBDFDatabase::createTable(const char* name, const WBDFColumn* columns, uint8_t columnCount, uint16_t maxRows) {
    if (!initialized || !name || !columns || columnCount == 0 || columnCount > WBDF_MAX_COLUMNS) {
        return 0;
    }
    
    if (header->tableCount >= WBDF_MAX_TABLES) {
        return 0;
    }
    
    // Find free table slot
    uint16_t tableId = 0;
    for (uint16_t i = 0; i < WBDF_MAX_TABLES; i++) {
        if (schemas[i].name[0] == 0) {  // Empty slot
            tableId = i;
            break;
        }
    }
    
    if (tableId == 0 && schemas[0].name[0] != 0) {
        return 0;  // No free slots
    }
    
    WBDFTableSchema* schema = &schemas[tableId];
    memset(schema, 0, sizeof(WBDFTableSchema));
    
    // Set table properties
    strncpy(schema->name, name, sizeof(schema->name) - 1);
    schema->columnCount = columnCount;
    schema->rowCount = 0;
    schema->maxRows = maxRows;
    schema->indexCount = 0;
    
    // Calculate row size and set up columns
    uint16_t rowSize = 0;
    for (uint8_t i = 0; i < columnCount; i++) {
        schema->columns[i] = columns[i];
        
        uint8_t colSize = 0;
        switch (columns[i].type) {
            case WBDF_TYPE_U8:
            case WBDF_TYPE_I8:
            case WBDF_TYPE_BOOL:
                colSize = 1;
                break;
            case WBDF_TYPE_U16:
            case WBDF_TYPE_I16:
                colSize = 2;
                break;
            case WBDF_TYPE_U32:
            case WBDF_TYPE_I32:
            case WBDF_TYPE_FLOAT:
                colSize = 4;
                break;
            case WBDF_TYPE_STRING:
            case WBDF_TYPE_BYTES:
                colSize = columns[i].size;
                break;
            default:
                colSize = 1;
                break;
        }
        
        rowSize += colSize;
        
        // Count indexes
        if (columns[i].indexType != WBDF_INDEX_NONE) {
            schema->indexCount++;
        }
    }
    
    schema->rowSize = rowSize;
    
    // Allocate space for table data
    uint32_t tableDataSize = rowSize * maxRows;
    uint32_t indexDataSize = schema->indexCount * maxRows * sizeof(WBDFIndexEntry);
    uint32_t totalTableSize = tableDataSize + indexDataSize;
    
    if (header->totalSize + totalTableSize > dataSize) {
        return 0;  // Not enough space
    }
    
    // Set table offset
    header->tableOffsets[tableId] = header->totalSize;
    header->totalSize += totalTableSize;
    header->tableCount++;
    
    // Build indexes for this table
    uint8_t indexId = 0;
    for (uint8_t i = 0; i < columnCount; i++) {
        if (columns[i].indexType != WBDF_INDEX_NONE) {
            schema->indexOffsets[indexId] = tableDataSize + (indexId * maxRows * sizeof(WBDFIndexEntry));
            indexId++;
        }
    }
    
    return tableId + 1;  // Return 1-based table ID
}

bool WBDFDatabase::dropTable(uint16_t tableId) {
    if (!initialized || tableId == 0 || tableId > WBDF_MAX_TABLES) {
        return false;
    }
    
    WBDFTableSchema* schema = &schemas[tableId - 1];
    if (schema->name[0] == 0) {
        return false;  // Table doesn't exist
    }
    
    // Clear the schema
    memset(schema, 0, sizeof(WBDFTableSchema));
    header->tableCount--;
    
    return true;
}

uint16_t WBDFDatabase::getTableId(const char* name) {
    if (!initialized || !name) {
        return 0;
    }
    
    for (uint16_t i = 0; i < WBDF_MAX_TABLES; i++) {
        if (schemas[i].name[0] != 0 && strcmp(schemas[i].name, name) == 0) {
            return i + 1;  // Return 1-based ID
        }
    }
    
    return 0;
}

WBDFTableSchema* WBDFDatabase::getTable(uint16_t tableId) {
    if (!initialized || tableId == 0 || tableId > WBDF_MAX_TABLES) {
        return nullptr;
    }
    
    WBDFTableSchema* schema = &schemas[tableId - 1];
    return (schema->name[0] != 0) ? schema : nullptr;
}

WBDFTableSchema* WBDFDatabase::getTableSchema(uint16_t tableId) {
    return getTable(tableId);
}

uint8_t* WBDFDatabase::getTableData(uint16_t tableId) {
    if (!initialized || tableId == 0 || tableId > WBDF_MAX_TABLES) {
        return nullptr;
    }
    
    return data + header->tableOffsets[tableId - 1];
}

uint16_t WBDFDatabase::findNextRowId(uint16_t tableId) {
    WBDFTableSchema* schema = getTableSchema(tableId);
    if (!schema) {
        return 0;
    }
    
    if (schema->rowCount >= schema->maxRows) {
        return 0;  // Table is full
    }
    
    return schema->rowCount + 1;  // Return 1-based row ID
}

uint16_t WBDFDatabase::insertRow(uint16_t tableId, const void* rowData) {
    if (!initialized || !rowData) {
        return 0;
    }
    
    WBDFTableSchema* schema = getTableSchema(tableId);
    if (!schema) {
        return 0;
    }
    
    uint16_t rowId = findNextRowId(tableId);
    if (rowId == 0) {
        return 0;
    }
    
    uint8_t* tableData = getTableData(tableId);
    if (!tableData) {
        return 0;
    }
    
    // Copy row data
    uint32_t rowOffset = (rowId - 1) * schema->rowSize;
    memcpy(tableData + rowOffset, rowData, schema->rowSize);
    
    schema->rowCount++;
    
    // Update indexes
    uint8_t indexId = 0;
    const uint8_t* sourceData = static_cast<const uint8_t*>(rowData);
    uint16_t columnOffset = 0;
    
    for (uint8_t i = 0; i < schema->columnCount; i++) {
        if (schema->columns[i].indexType != WBDF_INDEX_NONE) {
            updateIndex(tableId, indexId, rowId, sourceData + columnOffset);
            indexId++;
        }
        
        // Calculate next column offset
        uint8_t colSize = 0;
        switch (schema->columns[i].type) {
            case WBDF_TYPE_U8:
            case WBDF_TYPE_I8:
            case WBDF_TYPE_BOOL:
                colSize = 1;
                break;
            case WBDF_TYPE_U16:
            case WBDF_TYPE_I16:
                colSize = 2;
                break;
            case WBDF_TYPE_U32:
            case WBDF_TYPE_I32:
            case WBDF_TYPE_FLOAT:
                colSize = 4;
                break;
            case WBDF_TYPE_STRING:
            case WBDF_TYPE_BYTES:
                colSize = schema->columns[i].size;
                break;
            default:
                colSize = 1;
                break;
        }
        columnOffset += colSize;
    }
    
    return rowId;
}

bool WBDFDatabase::updateRow(uint16_t tableId, uint16_t rowId, const void* rowData) {
    if (!initialized || !rowData || rowId == 0) {
        return false;
    }
    
    WBDFTableSchema* schema = getTableSchema(tableId);
    if (!schema || rowId > schema->rowCount) {
        return false;
    }
    
    uint8_t* tableData = getTableData(tableId);
    if (!tableData) {
        return false;
    }
    
    uint32_t rowOffset = (rowId - 1) * schema->rowSize;
    memcpy(tableData + rowOffset, rowData, schema->rowSize);
    
    return true;
}

bool WBDFDatabase::getRow(uint16_t tableId, uint16_t rowId, void* rowData) {
    if (!initialized || !rowData || rowId == 0) {
        return false;
    }
    
    WBDFTableSchema* schema = getTableSchema(tableId);
    if (!schema || rowId > schema->rowCount) {
        return false;
    }
    
    uint8_t* tableData = getTableData(tableId);
    if (!tableData) {
        return false;
    }
    
    uint32_t rowOffset = (rowId - 1) * schema->rowSize;
    memcpy(rowData, tableData + rowOffset, schema->rowSize);
    
    return true;
}

bool WBDFDatabase::deleteRow(uint16_t tableId, uint16_t rowId) {
    if (!initialized || rowId == 0) {
        return false;
    }
    
    WBDFTableSchema* schema = getTableSchema(tableId);
    if (!schema || rowId > schema->rowCount) {
        return false;
    }
    
    uint8_t* tableData = getTableData(tableId);
    if (!tableData) {
        return false;
    }
    
    // Move last row to deleted position (simple approach)
    if (rowId < schema->rowCount) {
        uint32_t deletedOffset = (rowId - 1) * schema->rowSize;
        uint32_t lastOffset = (schema->rowCount - 1) * schema->rowSize;
        memcpy(tableData + deletedOffset, tableData + lastOffset, schema->rowSize);
    }
    
    schema->rowCount--;
    return true;
}

bool WBDFDatabase::updateIndex(uint16_t tableId, uint8_t indexId, uint16_t rowId, const void* value) {
    WBDFTableSchema* schema = getTableSchema(tableId);
    if (!schema || indexId >= schema->indexCount) {
        return false;
    }
    
    WBDFIndexEntry* indexData = getTableIndex(tableId, indexId);
    if (!indexData) {
        return false;
    }
    
    // Simple hash-based index for now
    uint32_t hash = calculateHash(value, 4);  // Simplified - should use actual column size
    
    WBDFIndexEntry* entry = &indexData[rowId - 1];
    entry->keyHash = hash;
    entry->rowId = rowId;
    entry->nextEntry = 0;
    
    return true;
}

WBDFIndexEntry* WBDFDatabase::getTableIndex(uint16_t tableId, uint8_t indexId) {
    WBDFTableSchema* schema = getTableSchema(tableId);
    if (!schema || indexId >= schema->indexCount) {
        return nullptr;
    }
    
    uint8_t* tableData = getTableData(tableId);
    if (!tableData) {
        return nullptr;
    }
    
    return reinterpret_cast<WBDFIndexEntry*>(tableData + schema->indexOffsets[indexId]);
}

bool WBDFDatabase::selectAll(uint16_t tableId, WBDFResultSet* results) {
    if (!initialized || !results) {
        return false;
    }
    
    WBDFTableSchema* schema = getTableSchema(tableId);
    if (!schema) {
        return false;
    }
    
    uint16_t rowCount = std::min(schema->rowCount, results->maxResults);
    
    for (uint16_t i = 0; i < rowCount; i++) {
        results->rowIds[i] = i + 1;
    }
    
    results->rowCount = rowCount;
    results->columnMask = 0xFFFF;  // All columns selected
    
    return true;
}

bool WBDFDatabase::simpleSelect(uint16_t tableId, const char* whereColumn, const void* whereValue, WBDFResultSet* results) {
    if (!initialized || !whereColumn || !whereValue || !results) {
        return false;
    }
    
    WBDFTableSchema* schema = getTableSchema(tableId);
    if (!schema) {
        return false;
    }
    
    // Find column by name
    uint8_t columnId = 0xFF;
    uint16_t columnOffset = 0;
    for (uint8_t i = 0; i < schema->columnCount; i++) {
        if (strcmp(schema->columns[i].name, whereColumn) == 0) {
            columnId = i;
            break;
        }
        
        // Calculate column offset for when we find it
        uint8_t colSize = 0;
        switch (schema->columns[i].type) {
            case WBDF_TYPE_U8:
            case WBDF_TYPE_I8:
            case WBDF_TYPE_BOOL:
                colSize = 1;
                break;
            case WBDF_TYPE_U16:
            case WBDF_TYPE_I16:
                colSize = 2;
                break;
            case WBDF_TYPE_U32:
            case WBDF_TYPE_I32:
            case WBDF_TYPE_FLOAT:
                colSize = 4;
                break;
            case WBDF_TYPE_STRING:
            case WBDF_TYPE_BYTES:
                colSize = schema->columns[i].size;
                break;
            default:
                colSize = 1;
                break;
        }
        
        if (columnId == 0xFF) {
            columnOffset += colSize;
        }
    }
    
    if (columnId == 0xFF) {
        return false;  // Column not found
    }
    
    uint8_t* tableData = getTableData(tableId);
    if (!tableData) {
        return false;
    }
    
    // Linear scan for matching rows
    uint16_t matchCount = 0;
    for (uint16_t rowId = 1; rowId <= schema->rowCount && matchCount < results->maxResults; rowId++) {
        uint32_t rowOffset = (rowId - 1) * schema->rowSize;
        uint8_t* cellData = tableData + rowOffset + columnOffset;
        
        // Simple equality comparison based on column type
        bool matches = false;
        switch (schema->columns[columnId].type) {
            case WBDF_TYPE_U8:
            case WBDF_TYPE_I8:
            case WBDF_TYPE_BOOL:
                matches = (*cellData == *static_cast<const uint8_t*>(whereValue));
                break;
            case WBDF_TYPE_U16:
            case WBDF_TYPE_I16:
                matches = (*reinterpret_cast<uint16_t*>(cellData) == *static_cast<const uint16_t*>(whereValue));
                break;
            case WBDF_TYPE_U32:
            case WBDF_TYPE_I32:
                matches = (*reinterpret_cast<uint32_t*>(cellData) == *static_cast<const uint32_t*>(whereValue));
                break;
            case WBDF_TYPE_STRING:
                matches = (strncmp(reinterpret_cast<char*>(cellData), static_cast<const char*>(whereValue), schema->columns[columnId].size) == 0);
                break;
            default:
                matches = (memcmp(cellData, whereValue, schema->columns[columnId].size) == 0);
                break;
        }
        
        if (matches) {
            results->rowIds[matchCount] = rowId;
            matchCount++;
        }
    }
    
    results->rowCount = matchCount;
    results->columnMask = 0xFFFF;  // All columns selected
    
    return true;
}

uint32_t WBDFDatabase::getUsedMemory() {
    return initialized ? header->totalSize : 0;
}

uint32_t WBDFDatabase::getFreeMemory() {
    return initialized ? (dataSize - header->totalSize) : 0;
}

void WBDFDatabase::printTableInfo(uint16_t tableId) {
    WBDFTableSchema* schema = getTable(tableId);
    if (!schema) {
        return;
    }
    
    printf("Table: %s (ID: %d)\n", schema->name, tableId);
    printf("  Rows: %d/%d\n", schema->rowCount, schema->maxRows);
    printf("  Row Size: %d bytes\n", schema->rowSize);
    printf("  Columns: %d\n", schema->columnCount);
    
    for (uint8_t i = 0; i < schema->columnCount; i++) {
        const char* typeName = "Unknown";
        switch (schema->columns[i].type) {
            case WBDF_TYPE_U8: typeName = "U8"; break;
            case WBDF_TYPE_U16: typeName = "U16"; break;
            case WBDF_TYPE_U32: typeName = "U32"; break;
            case WBDF_TYPE_STRING: typeName = "String"; break;
            default: typeName = "Other"; break;
        }
        
        printf("    %s: %s", schema->columns[i].name, typeName);
        if (schema->columns[i].indexType != WBDF_INDEX_NONE) {
            printf(" (Indexed)");
        }
        printf("\n");
    }
}

void WBDFDatabase::printAllTables() {
    if (!initialized) {
        printf("Database not initialized\n");
        return;
    }
    
    printf("WBDF Database - %d tables\n", header->tableCount);
    printf("Memory: %lu/%lu bytes used\n", (unsigned long)getUsedMemory(), (unsigned long)dataSize);
    
    for (uint16_t i = 1; i <= WBDF_MAX_TABLES; i++) {
        WBDFTableSchema* schema = getTable(i);
        if (schema) {
            printTableInfo(i);
            printf("\n");
        }
    }
}

bool WBDFDatabase::validate() {
    if (!initialized || !isValid()) {
        return false;
    }
    
    // Basic validation - could be expanded
    return header->totalSize <= dataSize && header->tableCount <= WBDF_MAX_TABLES;
}

// Query builder implementation
WBDFQueryBuilder::WBDFQueryBuilder(uint16_t tableId) : opIndex(0) {
    memset(&query, 0, sizeof(query));
    query.tableId = tableId;
}

WBDFQueryBuilder& WBDFQueryBuilder::where(const char* column, WBDFQueryOp op, const void* value) {
    // Simplified - would need column name lookup
    if (opIndex < WBDF_MAX_QUERY_OPS - 1) {
        query.operations[opIndex * 4] = static_cast<uint8_t>(WBDF_OP_WHERE);
        query.operations[opIndex * 4 + 1] = static_cast<uint8_t>(op);
        // Store column and value info (simplified)
        opIndex++;
    }
    return *this;
}

WBDFQueryBuilder& WBDFQueryBuilder::and_() {
    if (opIndex < WBDF_MAX_QUERY_OPS) {
        query.operations[opIndex * 4] = static_cast<uint8_t>(WBDF_OP_AND);
        opIndex++;
    }
    return *this;
}

WBDFQueryBuilder& WBDFQueryBuilder::or_() {
    if (opIndex < WBDF_MAX_QUERY_OPS) {
        query.operations[opIndex * 4] = static_cast<uint8_t>(WBDF_OP_OR);
        opIndex++;
    }
    return *this;
}

WBDFQuery WBDFQueryBuilder::build() {
    query.opCount = opIndex;
    return query;
}

// Example table definitions
namespace GameTables {
    const WBDFColumn ITEM_COLUMNS[] = {
        WBDF_PRIMARY_KEY("id", WBDF_TYPE_U16),
        WBDF_COLUMN("name", WBDF_TYPE_STRING, 32),
        WBDF_INDEXED_COLUMN("category", WBDF_TYPE_U8, 0),
        WBDF_COLUMN("rarity", WBDF_TYPE_U8, 0),
        WBDF_COLUMN("value", WBDF_TYPE_U32, 0),
        WBDF_COLUMN("stackSize", WBDF_TYPE_U16, 0),
        WBDF_COLUMN("flags", WBDF_TYPE_U8, 0)
    };
    
    const WBDFColumn QUEST_COLUMNS[] = {
        WBDF_PRIMARY_KEY("id", WBDF_TYPE_U16),
        WBDF_COLUMN("title", WBDF_TYPE_STRING, 48),
        WBDF_INDEXED_COLUMN("status", WBDF_TYPE_U8, 0),
        WBDF_COLUMN("progress", WBDF_TYPE_U8, 0),
        WBDF_COLUMN("flags", WBDF_TYPE_U32, 0),
        WBDF_COLUMN("prerequisite", WBDF_TYPE_U16, 0),
        WBDF_COLUMN("reward_item", WBDF_TYPE_U16, 0),
        WBDF_COLUMN("reward_exp", WBDF_TYPE_U32, 0)
    };
    
    const WBDFColumn NPC_COLUMNS[] = {
        WBDF_PRIMARY_KEY("id", WBDF_TYPE_U16),
        WBDF_COLUMN("name", WBDF_TYPE_STRING, 24),
        WBDF_COLUMN("level", WBDF_TYPE_U8, 0),
        WBDF_INDEXED_COLUMN("faction", WBDF_TYPE_U8, 0),
        WBDF_COLUMN("location_x", WBDF_TYPE_U16, 0),
        WBDF_COLUMN("location_y", WBDF_TYPE_U16, 0),
        WBDF_COLUMN("flags", WBDF_TYPE_U32, 0),
        WBDF_COLUMN("dialogue_id", WBDF_TYPE_U16, 0)
    };
    
    const uint8_t ITEM_COLUMN_COUNT = sizeof(ITEM_COLUMNS) / sizeof(WBDFColumn);
    const uint8_t QUEST_COLUMN_COUNT = sizeof(QUEST_COLUMNS) / sizeof(WBDFColumn);
    const uint8_t NPC_COLUMN_COUNT = sizeof(NPC_COLUMNS) / sizeof(WBDFColumn);
}
