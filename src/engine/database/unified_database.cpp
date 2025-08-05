// unified_database.cpp - Implementation of Unified Database System
#include "unified_database.h"
#include "database_system.h"
#include <string.h>
#include <esp_log.h>

// Global unified database instance
WispUnifiedDatabase wispDB;

WispUnifiedDatabase::WispUnifiedDatabase() 
    : initialized(false), memory(nullptr), memorySize(0), 
      kvTableId(0), metaTableId(0), configTableId(0) {
}

WispUnifiedDatabase::~WispUnifiedDatabase() {
    shutdown();
}

WispErrorCode WispUnifiedDatabase::initialize(uint32_t memSize) {
    if (initialized) {
        return WISP_ERROR_ALREADY_INITIALIZED;
    }
    
    if (memSize < 4096) {
        ESP_LOGE("DB", "Memory size too small, minimum 4KB required");
        return WISP_ERROR_INVALID_CONFIG;
    }
    
    // Store memory info
    memorySize = memSize;
    
    // Allocate memory from LP-SRAM or HP-SRAM
    if (memSize <= 16384) {
        // Use LP-SRAM for small databases
        static uint8_t lpSramPool[16384];
        memory = lpSramPool;
    } else {
        ESP_LOGE("DB", "Memory size too large, maximum 16KB supported");
        return WISP_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize core WBDF engine
    if (!wbdfCore.create(memory, memSize)) {
        ESP_LOGE("DB", "Failed to initialize WBDF core");
        return WISP_ERROR_NOT_INITIALIZED;
    }
    
    // Create built-in tables
    WispErrorCode result = createBuiltinTables();
    if (result != WISP_SUCCESS) {
        ESP_LOGE("DB", "Failed to create builtin tables: %d", (int)result);
        wbdfCore.shutdown();
        return result;
    }
    
    initialized = true;
    ESP_LOGI("DB", "Unified database initialized with %d bytes", (int)memSize);
    
    return WISP_SUCCESS;
}

void WispUnifiedDatabase::shutdown() {
    if (initialized) {
        wbdfCore.shutdown();
        initialized = false;
        memory = nullptr;
        memorySize = 0;
        kvTableId = metaTableId = configTableId = 0;
        ESP_LOGI("DB", "Unified database shutdown");
    }
}

WispErrorCode WispUnifiedDatabase::createBuiltinTables() {
    // Create key-value table
    const WBDFColumn kvColumns[] = {
        WBDF_PRIMARY_KEY("key", WBDF_TYPE_U32),
        WBDF_COLUMN("type", WBDF_TYPE_U8, 0),
        WBDF_COLUMN("size", WBDF_TYPE_U8, 0),
        WBDF_COLUMN("data", WBDF_TYPE_BYTES, 58)
    };
    kvTableId = wbdfCore.createTable("kv_store", kvColumns, 4, 256);
    if (kvTableId == 0) {
        return WISP_ERROR_PARTITION_FULL;
    }
    
    // Create table metadata table
    const WBDFColumn metaColumns[] = {
        WBDF_PRIMARY_KEY("table_id", WBDF_TYPE_U16),
        WBDF_COLUMN("name", WBDF_TYPE_STRING, 16),
        WBDF_COLUMN("permissions", WBDF_TYPE_U8, 0),
        WBDF_COLUMN("column_count", WBDF_TYPE_U8, 0),
        WBDF_COLUMN("max_rows", WBDF_TYPE_U16, 0),
        WBDF_COLUMN("current_rows", WBDF_TYPE_U16, 0),
        WBDF_COLUMN("created_time", WBDF_TYPE_U32, 0),
        WBDF_COLUMN("modified_time", WBDF_TYPE_U32, 0),
        WBDF_COLUMN("flags", WBDF_TYPE_U32, 0)
    };
    metaTableId = wbdfCore.createTable("table_meta", metaColumns, 9, 64);
    if (metaTableId == 0) {
        return WISP_ERROR_PARTITION_FULL;
    }
    
    // Create configuration table
    const WBDFColumn configColumns[] = {
        WBDF_PRIMARY_KEY("config_id", WBDF_TYPE_U16),
        WBDF_COLUMN("key", WBDF_TYPE_STRING, 16),
        WBDF_COLUMN("value", WBDF_TYPE_STRING, 32),
        WBDF_COLUMN("type", WBDF_TYPE_U8, 0)
    };
    configTableId = wbdfCore.createTable("config", configColumns, 4, 32);
    if (configTableId == 0) {
        return WISP_ERROR_PARTITION_FULL;
    }
    
    // Register built-in table metadata
    uint32_t currentTime = esp_log_timestamp();
    
    WBDFTableMeta kvMeta = {kvTableId, "kv_store", WBDF_TABLE_READ_WRITE, 4, 256, 0, currentTime, currentTime, 0};
    wbdfCore.insertRow(metaTableId, &kvMeta);
    
    WBDFTableMeta metaMeta = {metaTableId, "table_meta", WBDF_TABLE_READ_ONLY, 9, 64, 0, currentTime, currentTime, 0};
    wbdfCore.insertRow(metaTableId, &metaMeta);
    
    WBDFTableMeta configMeta = {configTableId, "config", WBDF_TABLE_READ_WRITE, 4, 32, 0, currentTime, currentTime, 0};
    wbdfCore.insertRow(metaTableId, &configMeta);
    
    return WISP_SUCCESS;
}

bool WispUnifiedDatabase::checkTablePermission(uint16_t tableId, uint8_t requiredPermission) {
    if (!initialized) return false;
    
    // Built-in tables have fixed permissions
    if (isBuiltinTable(tableId)) {
        if (tableId == kvTableId || tableId == configTableId) {
            return true; // KV and config tables are always read-write
        } else if (tableId == metaTableId) {
            return (requiredPermission == WBDF_TABLE_READABLE); // Meta table is read-only
        }
    }
    
    // Look up permissions in metadata table
    WBDFResultSet results;
    if (wbdfCore.simpleSelect(metaTableId, "table_id", &tableId, &results)) {
        if (results.rowCount > 0) {
            WBDFTableMeta meta;
            if (wbdfCore.getRow(metaTableId, results.rowIds[0], &meta)) {
                return (meta.permissions & requiredPermission) != 0;
            }
        }
    }
    
    return false; // Default deny
}

bool WispUnifiedDatabase::isBuiltinTable(uint16_t tableId) {
    return (tableId == kvTableId || tableId == metaTableId || tableId == configTableId);
}

uint32_t WispUnifiedDatabase::hashKey(uint32_t key) {
    // Simple hash function for key-value store
    key ^= key >> 16;
    key *= 0x85ebca6b;
    key ^= key >> 13;
    key *= 0xc2b2ae35;
    key ^= key >> 16;
    return key;
}

// === KEY-VALUE STORE IMPLEMENTATION ===

WispErrorCode WispUnifiedDatabase::setKeyValue(uint32_t key, const void* data, uint8_t size, uint8_t type) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    if (!data || size == 0 || size > 58) return WISP_ERROR_INVALID_PARAMS;
    
    // Check if key already exists and update or insert
    WBDFResultSet results;
    if (wbdfCore.simpleSelect(kvTableId, "key", &key, &results)) {
        if (results.rowCount > 0) {
            // Update existing entry
            WBDFKeyValueEntry entry;
            entry.key = key;
            entry.type = type;
            entry.size = size;
            memset(entry.data, 0, 58);
            memcpy(entry.data, data, size);
            
            return wbdfCore.updateRow(kvTableId, results.rowIds[0], &entry) ? 
                   WISP_SUCCESS : WISP_ERROR_PARTITION_FULL;
        }
    }
    
    // Insert new entry
    WBDFKeyValueEntry entry;
    entry.key = key;
    entry.type = type;
    entry.size = size;
    memset(entry.data, 0, 58);
    memcpy(entry.data, data, size);
    
    uint16_t rowId = wbdfCore.insertRow(kvTableId, &entry);
    return (rowId > 0) ? WISP_SUCCESS : WISP_ERROR_PARTITION_FULL;
}

WispErrorCode WispUnifiedDatabase::getKeyValue(uint32_t key, void* buffer, uint8_t maxSize, uint8_t* actualSize) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    if (!buffer || maxSize == 0) return WISP_ERROR_INVALID_PARAMS;
    
    WBDFResultSet results;
    if (wbdfCore.simpleSelect(kvTableId, "key", &key, &results)) {
        if (results.rowCount > 0) {
            WBDFKeyValueEntry entry;
            if (wbdfCore.getRow(kvTableId, results.rowIds[0], &entry)) {
                if (actualSize) *actualSize = entry.size;
                
                if (entry.size > maxSize) {
                    return WISP_ERROR_BUFFER_OVERFLOW;
                }
                
                memcpy(buffer, entry.data, entry.size);
                return WISP_SUCCESS;
            }
        }
    }
    
    return WISP_ERROR_KEY_NOT_FOUND;
}

WispErrorCode WispUnifiedDatabase::setU8(uint32_t key, uint8_t value) {
    return setKeyValue(key, &value, sizeof(value), WBDF_TYPE_U8);
}

WispErrorCode WispUnifiedDatabase::setU16(uint32_t key, uint16_t value) {
    return setKeyValue(key, &value, sizeof(value), WBDF_TYPE_U16);
}

WispErrorCode WispUnifiedDatabase::setU32(uint32_t key, uint32_t value) {
    return setKeyValue(key, &value, sizeof(value), WBDF_TYPE_U32);
}

WispErrorCode WispUnifiedDatabase::setFloat(uint32_t key, float value) {
    return setKeyValue(key, &value, sizeof(value), WBDF_TYPE_FLOAT);
}

WispErrorCode WispUnifiedDatabase::setString(uint32_t key, const char* value) {
    if (!value) return WISP_ERROR_INVALID_PARAMS;
    uint8_t len = strlen(value);
    if (len > 57) len = 57; // Leave space for null terminator
    
    char buffer[58];
    memset(buffer, 0, 58);
    memcpy(buffer, value, len);
    buffer[len] = '\0';
    
    return setKeyValue(key, buffer, len + 1, WBDF_TYPE_STRING);
}

WispErrorCode WispUnifiedDatabase::setBytes(uint32_t key, const void* data, uint8_t size) {
    return setKeyValue(key, data, size, WBDF_TYPE_BYTES);
}

uint8_t WispUnifiedDatabase::getU8(uint32_t key, uint8_t defaultValue) {
    uint8_t value = defaultValue;
    uint8_t actualSize;
    getKeyValue(key, &value, sizeof(value), &actualSize);
    return value;
}

uint16_t WispUnifiedDatabase::getU16(uint32_t key, uint16_t defaultValue) {
    uint16_t value = defaultValue;
    uint8_t actualSize;
    getKeyValue(key, &value, sizeof(value), &actualSize);
    return value;
}

uint32_t WispUnifiedDatabase::getU32(uint32_t key, uint32_t defaultValue) {
    uint32_t value = defaultValue;
    uint8_t actualSize;
    getKeyValue(key, &value, sizeof(value), &actualSize);
    return value;
}

float WispUnifiedDatabase::getFloat(uint32_t key, float defaultValue) {
    float value = defaultValue;
    uint8_t actualSize;
    getKeyValue(key, &value, sizeof(value), &actualSize);
    return value;
}

bool WispUnifiedDatabase::getString(uint32_t key, char* buffer, uint8_t bufferSize) {
    if (!buffer || bufferSize == 0) return false;
    
    uint8_t actualSize;
    WispErrorCode result = getKeyValue(key, buffer, bufferSize - 1, &actualSize);
    if (result == WISP_SUCCESS) {
        buffer[actualSize] = '\0'; // Ensure null termination
        return true;
    }
    
    buffer[0] = '\0';
    return false;
}

WispErrorCode WispUnifiedDatabase::getBytes(uint32_t key, void* buffer, uint8_t maxSize, uint8_t* actualSize) {
    return getKeyValue(key, buffer, maxSize, actualSize);
}

bool WispUnifiedDatabase::existsKey(uint32_t key) {
    uint8_t dummy;
    uint8_t actualSize;
    return getKeyValue(key, &dummy, sizeof(dummy), &actualSize) == WISP_SUCCESS;
}

WispErrorCode WispUnifiedDatabase::removeKey(uint32_t key) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    
    WBDFResultSet results;
    if (wbdfCore.simpleSelect(kvTableId, "key", &key, &results)) {
        if (results.rowCount > 0) {
            return wbdfCore.deleteRow(kvTableId, results.rowIds[0]) ? 
                   WISP_SUCCESS : WISP_ERROR_KEY_NOT_FOUND;
        }
    }
    
    return WISP_ERROR_KEY_NOT_FOUND;
}

// === STRUCTURED DATA IMPLEMENTATION ===

uint16_t WispUnifiedDatabase::createTable(const char* name, const WBDFColumn* columns, 
                                         uint8_t columnCount, uint16_t maxRows, uint8_t permissions) {
    if (!initialized) return 0;
    if (!name || !columns || columnCount == 0) return 0;
    
    // Create table in WBDF core
    uint16_t tableId = wbdfCore.createTable(name, columns, columnCount, maxRows);
    if (tableId == 0) return 0;
    
    // Register table metadata
    uint32_t currentTime = esp_log_timestamp();
    WBDFTableMeta meta = {
        tableId, "", permissions, columnCount, maxRows, 0, currentTime, currentTime, 0
    };
    strncpy(meta.name, name, 15);
    meta.name[15] = '\0';
    
    uint16_t metaRowId = wbdfCore.insertRow(metaTableId, &meta);
    if (metaRowId == 0) {
        // Failed to insert metadata, clean up table
        wbdfCore.dropTable(tableId);
        return 0;
    }
    
    ESP_LOGI("DB", "Created table '%s' (ID: %d) with permissions 0x%02X", name, tableId, permissions);
    return tableId;
}

WispErrorCode WispUnifiedDatabase::setTablePermissions(uint16_t tableId, uint8_t permissions) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    if (isBuiltinTable(tableId)) return WISP_ERROR_INVALID_PARAMS; // Can't change builtin table permissions
    
    WBDFResultSet results;
    if (wbdfCore.simpleSelect(metaTableId, "table_id", &tableId, &results)) {
        if (results.rowCount > 0) {
            WBDFTableMeta meta;
            if (wbdfCore.getRow(metaTableId, results.rowIds[0], &meta)) {
                meta.permissions = permissions;
                meta.modifiedTime = esp_log_timestamp();
                
                return wbdfCore.updateRow(metaTableId, results.rowIds[0], &meta) ? 
                       WISP_SUCCESS : WISP_ERROR_PARTITION_FULL;
            }
        }
    }
    
    return WISP_ERROR_INVALID_PARTITION;
}

uint8_t WispUnifiedDatabase::getTablePermissions(uint16_t tableId) {
    if (!initialized) return 0;
    
    if (isBuiltinTable(tableId)) {
        if (tableId == kvTableId || tableId == configTableId) {
            return WBDF_TABLE_READ_WRITE;
        } else if (tableId == metaTableId) {
            return WBDF_TABLE_READ_ONLY;
        }
    }
    
    WBDFResultSet results;
    if (wbdfCore.simpleSelect(metaTableId, "table_id", &tableId, &results)) {
        if (results.rowCount > 0) {
            WBDFTableMeta meta;
            if (wbdfCore.getRow(metaTableId, results.rowIds[0], &meta)) {
                return meta.permissions;
            }
        }
    }
    
    return 0; // No permissions found
}

uint16_t WispUnifiedDatabase::insertRow(uint16_t tableId, const void* rowData) {
    if (!initialized) return 0;
    if (!checkTablePermission(tableId, WBDF_TABLE_WRITABLE)) return 0;
    
    return wbdfCore.insertRow(tableId, rowData);
}

WispErrorCode WispUnifiedDatabase::updateRow(uint16_t tableId, uint16_t rowId, const void* rowData) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    if (!checkTablePermission(tableId, WBDF_TABLE_WRITABLE)) return WISP_ERROR_INVALID_PARTITION;
    
    return wbdfCore.updateRow(tableId, rowId, rowData) ? WISP_SUCCESS : WISP_ERROR_KEY_NOT_FOUND;
}

WispErrorCode WispUnifiedDatabase::getRow(uint16_t tableId, uint16_t rowId, void* rowData) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    if (!checkTablePermission(tableId, WBDF_TABLE_READABLE)) return WISP_ERROR_INVALID_PARTITION;
    
    return wbdfCore.getRow(tableId, rowId, rowData) ? WISP_SUCCESS : WISP_ERROR_KEY_NOT_FOUND;
}

WispErrorCode WispUnifiedDatabase::deleteRow(uint16_t tableId, uint16_t rowId) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    if (!checkTablePermission(tableId, WBDF_TABLE_WRITABLE)) return WISP_ERROR_INVALID_PARTITION;
    
    return wbdfCore.deleteRow(tableId, rowId) ? WISP_SUCCESS : WISP_ERROR_KEY_NOT_FOUND;
}

WispErrorCode WispUnifiedDatabase::selectAll(uint16_t tableId, WBDFResultSet* results) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    if (!checkTablePermission(tableId, WBDF_TABLE_READABLE)) return WISP_ERROR_INVALID_PARTITION;
    
    return wbdfCore.selectAll(tableId, results) ? WISP_SUCCESS : WISP_ERROR_KEY_NOT_FOUND;
}

WispErrorCode WispUnifiedDatabase::simpleSelect(uint16_t tableId, const char* whereColumn, 
                                               const void* whereValue, WBDFResultSet* results) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    if (!checkTablePermission(tableId, WBDF_TABLE_READABLE)) return WISP_ERROR_INVALID_PARTITION;
    
    return wbdfCore.simpleSelect(tableId, whereColumn, whereValue, results) ? 
           WISP_SUCCESS : WISP_ERROR_KEY_NOT_FOUND;
}

uint16_t WispUnifiedDatabase::getTableId(const char* name) {
    if (!initialized || !name) return 0;
    return wbdfCore.getTableId(name);
}

uint32_t WispUnifiedDatabase::getUsedMemory() const {
    return initialized ? const_cast<WBDFDatabase&>(wbdfCore).getUsedMemory() : 0;
}

uint32_t WispUnifiedDatabase::getFreeMemory() const {
    return initialized ? const_cast<WBDFDatabase&>(wbdfCore).getFreeMemory() : 0;
}

void WispUnifiedDatabase::printStats() {
    if (!initialized) {
        ESP_LOGI("DB", "Database not initialized");
        return;
    }
    
    ESP_LOGI("DB", "=== Unified Database Statistics ===");
    ESP_LOGI("DB", "Memory: %d/%d bytes used (%.1f%%)", 
             (int)getUsedMemory(), (int)memorySize,
             (getUsedMemory() * 100.0f) / memorySize);
    
    wbdfCore.printAllTables();
}

bool WispUnifiedDatabase::validateDatabase() {
    return initialized ? wbdfCore.validate() : false;
}
