// doc_data_format.cpp - Document Data Format Implementation
#include "doc_data_format.h"
#include <string.h>
#include <cstdio>
#include <algorithm>

// Hash function for index keys
uint32_t DDFDatabase::calculateHash(const void* data, size_t size) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint32_t hash = 2166136261u;  // FNV-1a hash
    
    for (size_t i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    
    return hash;
}

DDFDatabase::DDFDatabase() : data(nullptr), dataSize(0), header(nullptr), schemas(nullptr), initialized(false) {
}

DDFDatabase::~DDFDatabase() {
    shutdown();
}

bool DDFDatabase::initialize(uint8_t* memory, uint32_t size) {
    if (initialized || !memory || size < sizeof(DDFHeader)) {
        return false;
    }
    
    data = memory;
    dataSize = size;
    header = reinterpret_cast<DDFHeader*>(data);
    
    // Validate existing database
    if (header->magic != DDF_MAGIC || header->version != DDF_VERSION) {
        return false;
    }
    
    if (header->totalSize > dataSize) {
        return false;
    }
    
    // Set up schema pointer
    if (header->schemaOffset > 0) {
        schemas = reinterpret_cast<DDFTableSchema*>(data + header->schemaOffset);
    }
    
    initialized = true;
    return true;
}

bool DDFDatabase::create(uint8_t* memory, uint32_t size) {
    if (initialized || !memory || size < sizeof(DDFHeader)) {
        return false;
    }
    
    data = memory;
    dataSize = size;
    
    // Initialize header
    header = reinterpret_cast<DDFHeader*>(data);
    memset(header, 0, sizeof(DDFHeader));
    
    header->magic = DDF_MAGIC;
    header->version = DDF_VERSION;
    header->tableCount = 0;
    header->totalSize = sizeof(DDFHeader);
    header->schemaOffset = sizeof(DDFHeader);
    header->dataOffset = sizeof(DDFHeader) + (DDF_MAX_TABLES * sizeof(DDFTableSchema));
    
    // Initialize schema area
    schemas = reinterpret_cast<DDFTableSchema*>(data + header->schemaOffset);
    memset(schemas, 0, DDF_MAX_TABLES * sizeof(DDFTableSchema));
    
    initialized = true;
    return true;
}

void DDFDatabase::shutdown() {
    data = nullptr;
    dataSize = 0;
    header = nullptr;
    schemas = nullptr;
    initialized = false;
}

bool DDFDatabase::isValid() {
    if (!initialized || !header) {
        return false;
    }
    
    return header->magic == DDF_MAGIC && header->version == DDF_VERSION;
}

uint16_t DDFDatabase::createTable(const char* name, const DDFColumn* columns, uint8_t columnCount, uint16_t maxRows) {
    if (!initialized || !name || !columns || columnCount == 0 || columnCount > DDF_MAX_COLUMNS) {
        return 0;
    }
    
    if (header->tableCount >= DDF_MAX_TABLES) {
        return 0;
    }
    
    // Find free table slot
    uint16_t tableId = 0;
    for (uint16_t i = 0; i < DDF_MAX_TABLES; i++) {
        if (schemas[i].name[0] == 0) {  // Empty slot
            tableId = i;
            break;
        }
    }
    
    if (tableId == 0 && schemas[0].name[0] != 0) {
        return 0;  // No free slots
    }
    
    DDFTableSchema* schema = &schemas[tableId];
    memset(schema, 0, sizeof(DDFTableSchema));
    
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
            case DDF_TYPE_U8:
            case DDF_TYPE_I8:
            case DDF_TYPE_BOOL:
                colSize = 1;
                break;
            case DDF_TYPE_U16:
            case DDF_TYPE_I16:
                colSize = 2;
                break;
            case DDF_TYPE_U32:
            case DDF_TYPE_I32:
            case DDF_TYPE_FLOAT:
                colSize = 4;
                break;
            case DDF_TYPE_STRING:
            case DDF_TYPE_BYTES:
                colSize = columns[i].size;
                break;
            default:
                colSize = 1;
                break;
        }
        
        rowSize += colSize;
        
        // Count indexes
        if (columns[i].indexType != DDF_INDEX_NONE) {
            schema->indexCount++;
        }
    }
    
    schema->rowSize = rowSize;
    
    // Allocate space for table data
    uint32_t tableDataSize = rowSize * maxRows;
    uint32_t indexDataSize = schema->indexCount * maxRows * sizeof(DDFIndexEntry);
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
        if (columns[i].indexType != DDF_INDEX_NONE) {
            schema->indexOffsets[indexId] = tableDataSize + (indexId * maxRows * sizeof(DDFIndexEntry));
            indexId++;
        }
    }
    
    return tableId + 1;  // Return 1-based table ID
}

bool DDFDatabase::dropTable(uint16_t tableId) {
    if (!initialized || tableId == 0 || tableId > DDF_MAX_TABLES) {
        return false;
    }
    
    DDFTableSchema* schema = &schemas[tableId - 1];
    if (schema->name[0] == 0) {
        return false;  // Table doesn't exist
    }
    
    // Clear the schema
    memset(schema, 0, sizeof(DDFTableSchema));
    header->tableCount--;
    
    return true;
}

uint16_t DDFDatabase::getTableId(const char* name) {
    if (!initialized || !name) {
        return 0;
    }
    
    for (uint16_t i = 0; i < DDF_MAX_TABLES; i++) {
        if (schemas[i].name[0] != 0 && strcmp(schemas[i].name, name) == 0) {
            return i + 1;  // Return 1-based ID
        }
    }
    
    return 0;
}

DDFTableSchema* DDFDatabase::getTable(uint16_t tableId) {
    if (!initialized || tableId == 0 || tableId > DDF_MAX_TABLES) {
        return nullptr;
    }
    
    DDFTableSchema* schema = &schemas[tableId - 1];
    return (schema->name[0] != 0) ? schema : nullptr;
}

DDFTableSchema* DDFDatabase::getTableSchema(uint16_t tableId) {
    return getTable(tableId);
}

uint8_t* DDFDatabase::getTableData(uint16_t tableId) {
    if (!initialized || tableId == 0 || tableId > DDF_MAX_TABLES) {
        return nullptr;
    }
    
    return data + header->tableOffsets[tableId - 1];
}

uint16_t DDFDatabase::findNextRowId(uint16_t tableId) {
    DDFTableSchema* schema = getTableSchema(tableId);
    if (!schema) {
        return 0;
    }
    
    if (schema->rowCount >= schema->maxRows) {
        return 0;  // Table is full
    }
    
    return schema->rowCount + 1;  // Return 1-based row ID
}

uint16_t DDFDatabase::insertRow(uint16_t tableId, const void* rowData) {
    if (!initialized || !rowData) {
        return 0;
    }
    
    DDFTableSchema* schema = getTableSchema(tableId);
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
        if (schema->columns[i].indexType != DDF_INDEX_NONE) {
            updateIndex(tableId, indexId, rowId, sourceData + columnOffset);
            indexId++;
        }
        
        // Calculate next column offset
        uint8_t colSize = 0;
        switch (schema->columns[i].type) {
            case DDF_TYPE_U8:
            case DDF_TYPE_I8:
            case DDF_TYPE_BOOL:
                colSize = 1;
                break;
            case DDF_TYPE_U16:
            case DDF_TYPE_I16:
                colSize = 2;
                break;
            case DDF_TYPE_U32:
            case DDF_TYPE_I32:
            case DDF_TYPE_FLOAT:
                colSize = 4;
                break;
            case DDF_TYPE_STRING:
            case DDF_TYPE_BYTES:
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

bool DDFDatabase::updateRow(uint16_t tableId, uint16_t rowId, const void* rowData) {
    if (!initialized || !rowData || rowId == 0) {
        return false;
    }
    
    DDFTableSchema* schema = getTableSchema(tableId);
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

bool DDFDatabase::getRow(uint16_t tableId, uint16_t rowId, void* rowData) {
    if (!initialized || !rowData || rowId == 0) {
        return false;
    }
    
    DDFTableSchema* schema = getTableSchema(tableId);
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

bool DDFDatabase::deleteRow(uint16_t tableId, uint16_t rowId) {
    if (!initialized || rowId == 0) {
        return false;
    }
    
    DDFTableSchema* schema = getTableSchema(tableId);
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

bool DDFDatabase::updateIndex(uint16_t tableId, uint8_t indexId, uint16_t rowId, const void* value) {
    DDFTableSchema* schema = getTableSchema(tableId);
    if (!schema || indexId >= schema->indexCount) {
        return false;
    }
    
    DDFIndexEntry* indexData = getTableIndex(tableId, indexId);
    if (!indexData) {
        return false;
    }
    
    // Simple hash-based index for now
    uint32_t hash = calculateHash(value, 4);  // Simplified - should use actual column size
    
    DDFIndexEntry* entry = &indexData[rowId - 1];
    entry->keyHash = hash;
    entry->rowId = rowId;
    entry->nextEntry = 0;
    
    return true;
}

DDFIndexEntry* DDFDatabase::getTableIndex(uint16_t tableId, uint8_t indexId) {
    DDFTableSchema* schema = getTableSchema(tableId);
    if (!schema || indexId >= schema->indexCount) {
        return nullptr;
    }
    
    uint8_t* tableData = getTableData(tableId);
    if (!tableData) {
        return nullptr;
    }
    
    return reinterpret_cast<DDFIndexEntry*>(tableData + schema->indexOffsets[indexId]);
}

bool DDFDatabase::selectAll(uint16_t tableId, DDFResultSet* results) {
    if (!initialized || !results) {
        return false;
    }
    
    DDFTableSchema* schema = getTableSchema(tableId);
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

bool DDFDatabase::simpleSelect(uint16_t tableId, const char* whereColumn, const void* whereValue, DDFResultSet* results) {
    if (!initialized || !whereColumn || !whereValue || !results) {
        return false;
    }
    
    DDFTableSchema* schema = getTableSchema(tableId);
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
            case DDF_TYPE_U8:
            case DDF_TYPE_I8:
            case DDF_TYPE_BOOL:
                colSize = 1;
                break;
            case DDF_TYPE_U16:
            case DDF_TYPE_I16:
                colSize = 2;
                break;
            case DDF_TYPE_U32:
            case DDF_TYPE_I32:
            case DDF_TYPE_FLOAT:
                colSize = 4;
                break;
            case DDF_TYPE_STRING:
            case DDF_TYPE_BYTES:
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
            case DDF_TYPE_U8:
            case DDF_TYPE_I8:
            case DDF_TYPE_BOOL:
                matches = (*cellData == *static_cast<const uint8_t*>(whereValue));
                break;
            case DDF_TYPE_U16:
            case DDF_TYPE_I16:
                matches = (*reinterpret_cast<uint16_t*>(cellData) == *static_cast<const uint16_t*>(whereValue));
                break;
            case DDF_TYPE_U32:
            case DDF_TYPE_I32:
                matches = (*reinterpret_cast<uint32_t*>(cellData) == *static_cast<const uint32_t*>(whereValue));
                break;
            case DDF_TYPE_STRING:
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

uint32_t DDFDatabase::getUsedMemory() {
    return initialized ? header->totalSize : 0;
}

uint32_t DDFDatabase::getFreeMemory() {
    return initialized ? (dataSize - header->totalSize) : 0;
}

void DDFDatabase::printTableInfo(uint16_t tableId) {
    DDFTableSchema* schema = getTable(tableId);
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
            case DDF_TYPE_U8: typeName = "U8"; break;
            case DDF_TYPE_U16: typeName = "U16"; break;
            case DDF_TYPE_U32: typeName = "U32"; break;
            case DDF_TYPE_STRING: typeName = "String"; break;
            default: typeName = "Other"; break;
        }
        
        printf("    %s: %s", schema->columns[i].name, typeName);
        if (schema->columns[i].indexType != DDF_INDEX_NONE) {
            printf(" (Indexed)");
        }
        printf("\n");
    }
}

void DDFDatabase::printAllTables() {
    if (!initialized) {
        printf("Database not initialized\n");
        return;
    }
    
    printf("DDF Database - %d tables\n", header->tableCount);
    printf("Memory: %lu/%lu bytes used\n", (unsigned long)getUsedMemory(), (unsigned long)dataSize);
    
    for (uint16_t i = 1; i <= DDF_MAX_TABLES; i++) {
        DDFTableSchema* schema = getTable(i);
        if (schema) {
            printTableInfo(i);
            printf("\n");
        }
    }
}

bool DDFDatabase::validate() {
    if (!initialized || !isValid()) {
        return false;
    }
    
    // Basic validation - could be expanded
    return header->totalSize <= dataSize && header->tableCount <= DDF_MAX_TABLES;
}

// Query builder implementation
DDFQueryBuilder::DDFQueryBuilder(uint16_t tableId) : opIndex(0) {
    memset(&query, 0, sizeof(query));
    query.tableId = tableId;
}

DDFQueryBuilder& DDFQueryBuilder::where(const char* column, DDFQueryOp op, const void* value) {
    // Simplified - would need column name lookup
    if (opIndex < DDF_MAX_QUERY_OPS - 1) {
        query.operations[opIndex * 4] = static_cast<uint8_t>(DDF_OP_WHERE);
        query.operations[opIndex * 4 + 1] = static_cast<uint8_t>(op);
        // Store column and value info (simplified)
        opIndex++;
    }
    return *this;
}

DDFQueryBuilder& DDFQueryBuilder::and_() {
    if (opIndex < DDF_MAX_QUERY_OPS) {
        query.operations[opIndex * 4] = static_cast<uint8_t>(DDF_OP_AND);
        opIndex++;
    }
    return *this;
}

DDFQueryBuilder& DDFQueryBuilder::or_() {
    if (opIndex < DDF_MAX_QUERY_OPS) {
        query.operations[opIndex * 4] = static_cast<uint8_t>(DDF_OP_OR);
        opIndex++;
    }
    return *this;
}

DDFQuery DDFQueryBuilder::build() {
    query.opCount = opIndex;
    return query;
}

// Example table definitions
namespace GameTables {
    const DDFColumn ITEM_COLUMNS[] = {
        DDF_PRIMARY_KEY("id", DDF_TYPE_U16),
        DDF_COLUMN("name", DDF_TYPE_STRING, 32),
        DDF_INDEXED_COLUMN("category", DDF_TYPE_U8, 0),
        DDF_COLUMN("rarity", DDF_TYPE_U8, 0),
        DDF_COLUMN("value", DDF_TYPE_U32, 0),
        DDF_COLUMN("stackSize", DDF_TYPE_U16, 0),
        DDF_COLUMN("flags", DDF_TYPE_U8, 0)
    };
    
    const DDFColumn QUEST_COLUMNS[] = {
        DDF_PRIMARY_KEY("id", DDF_TYPE_U16),
        DDF_COLUMN("title", DDF_TYPE_STRING, 48),
        DDF_INDEXED_COLUMN("status", DDF_TYPE_U8, 0),
        DDF_COLUMN("progress", DDF_TYPE_U8, 0),
        DDF_COLUMN("flags", DDF_TYPE_U32, 0),
        DDF_COLUMN("prerequisite", DDF_TYPE_U16, 0),
        DDF_COLUMN("reward_item", DDF_TYPE_U16, 0),
        DDF_COLUMN("reward_exp", DDF_TYPE_U32, 0)
    };
    
    const DDFColumn NPC_COLUMNS[] = {
        DDF_PRIMARY_KEY("id", DDF_TYPE_U16),
        DDF_COLUMN("name", DDF_TYPE_STRING, 24),
        DDF_COLUMN("level", DDF_TYPE_U8, 0),
        DDF_INDEXED_COLUMN("faction", DDF_TYPE_U8, 0),
        DDF_COLUMN("location_x", DDF_TYPE_U16, 0),
        DDF_COLUMN("location_y", DDF_TYPE_U16, 0),
        DDF_COLUMN("flags", DDF_TYPE_U32, 0),
        DDF_COLUMN("dialogue_id", DDF_TYPE_U16, 0)
    };
    
    const uint8_t ITEM_COLUMN_COUNT = sizeof(ITEM_COLUMNS) / sizeof(DDFColumn);
    const uint8_t QUEST_COLUMN_COUNT = sizeof(QUEST_COLUMNS) / sizeof(DDFColumn);
    const uint8_t NPC_COLUMN_COUNT = sizeof(NPC_COLUMNS) / sizeof(DDFColumn);
}
