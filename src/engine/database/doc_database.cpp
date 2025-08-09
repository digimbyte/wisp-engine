// doc_database.cpp - Implementation of Document Database System
#include "doc_database.h"
#include <string.h>
#include <esp_log.h>
#include <esp_heap_caps.h>

// Global document database instance  
DocDatabase docDB;

DocDatabase::DocDatabase()
    : initialized(false), memory(nullptr), memorySize(0), 
      kvTableId(0), metaTableId(0), configTableId(0) {
}

DocDatabase::~DocDatabase() {
    shutdown();
}

WispErrorCode DocDatabase::initialize(uint32_t memSize) {
    if (initialized) {
        return WISP_ERROR_ALREADY_INITIALIZED;
    }
    
    // Minimum initial allocation for header and essential tables
    uint32_t minInitialSize = 1024; // 1KB minimum for headers/index
    if (memSize < minInitialSize) {
        ESP_LOGE("DB", "Memory size too small, minimum %d bytes required", minInitialSize);
        return WISP_ERROR_INVALID_CONFIG;
    }
    
    // Store requested size but we'll allocate the full LP_SRAM reservation
    memorySize = memSize;
    
    // Reserve full LP_SRAM space but initialize minimally for app_state usage
    // Always reserve the full LP-SRAM allocation for maximum compatibility
#ifdef PLATFORM_C6
    uint32_t reservedSize = WISP_DB_LP_SRAM_SIZE_BYTES;  // Reserve full LP-SRAM (16KB)
    uint32_t initialSize = (memSize < reservedSize) ? memSize : reservedSize;
    
    #ifdef WISP_DB_USE_HP_SRAM
        // When HP-SRAM is defined, allocate from end of HP-SRAM
        memory = (uint8_t*)heap_caps_malloc(reservedSize, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
        if (!memory) {
            ESP_LOGE("DB", "Failed to reserve %d bytes from HP-SRAM", reservedSize);
            return WISP_ERROR_OUT_OF_MEMORY;
        }
        ESP_LOGI("DB", "Reserved %d bytes from HP-SRAM (using %d initially)", reservedSize, initialSize);
    #else
        // Use LP-SRAM directly for persistence across deep sleep
        memory = (uint8_t*)heap_caps_malloc(reservedSize, MALLOC_CAP_8BIT | MALLOC_CAP_RTCRAM);
        if (!memory) {
            ESP_LOGE("DB", "Failed to reserve %d bytes from LP-SRAM", reservedSize);
            return WISP_ERROR_OUT_OF_MEMORY;
        }
        ESP_LOGI("DB", "Reserved %d bytes from LP-SRAM (using %d initially, persistent)", reservedSize, initialSize);
    #endif
    
    // Zero out the reserved space and set actual working size
    memset(memory, 0, reservedSize);
    uint32_t databaseSize = initialSize;
    
#elif defined(PLATFORM_S3)
    uint32_t databaseSize = WISP_DB_RTC_SRAM_SIZE_BYTES;  // Use RTC memory size (16KB)
    
    #ifdef WISP_DB_USE_SRAM
        // When SRAM is defined, allocate from end of SRAM
        // SRAM total: 512KB, database at end: (512KB - 16KB) = start at 496KB offset
        uint32_t sramTotalBytes = ESP32_S3_SRAM_SIZE_KB * 1024;
        uint32_t databaseStartOffset = sramTotalBytes - databaseSize;
        
        memory = (uint8_t*)heap_caps_malloc(databaseSize, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
        if (!memory) {
            ESP_LOGE("DB", "Failed to allocate %d bytes from end of SRAM", databaseSize);
            return WISP_ERROR_OUT_OF_MEMORY;
        }
        ESP_LOGI("DB", "Allocated %d bytes from SRAM end (offset %d, total %d available)", 
                 databaseSize, databaseStartOffset, sramTotalBytes - databaseSize);
    #elif defined(WISP_DB_USE_PSRAM)
        // When PSRAM is defined, allocate from end of PSRAM
        // PSRAM total: 16MB, database at end: (16MB - 16KB) = start at ~16MB-16KB offset
        uint32_t psramTotalBytes = ESP32_S3_PSRAM_SIZE_KB * 1024;
        uint32_t databaseStartOffset = psramTotalBytes - databaseSize;
        
        memory = (uint8_t*)heap_caps_malloc(databaseSize, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
        if (!memory) {
            ESP_LOGE("DB", "Failed to allocate %d bytes from end of PSRAM", databaseSize);
            return WISP_ERROR_OUT_OF_MEMORY;
        }
        ESP_LOGI("DB", "Allocated %d bytes from PSRAM end (offset %d, total %d available)", 
                 databaseSize, databaseStartOffset, psramTotalBytes - databaseSize);
    #else
        // Use RTC memory directly for persistence across deep sleep
        memory = (uint8_t*)heap_caps_malloc(databaseSize, MALLOC_CAP_8BIT | MALLOC_CAP_RTCRAM);
        if (!memory) {
            ESP_LOGE("DB", "Failed to allocate %d bytes from RTC memory", databaseSize);
            return WISP_ERROR_OUT_OF_MEMORY;
        }
        ESP_LOGI("DB", "Allocated %d bytes from RTC memory (persistent)", databaseSize);
    #endif
    
#else
    // Fallback for unknown platforms - use fixed 16KB database
    uint32_t databaseSize = 16 * 1024;  // 16KB fixed database size
    ESP_LOGW("DB", "Unknown platform, using 16KB database from generic heap");
    memory = (uint8_t*)malloc(databaseSize);
    if (!memory) {
        ESP_LOGE("DB", "Failed to allocate %d bytes for database", databaseSize);
        return WISP_ERROR_OUT_OF_MEMORY;
    }
    ESP_LOGI("DB", "Allocated %d bytes from generic heap", databaseSize);
#endif
    
    // Update memorySize to actual allocated size
    memorySize = databaseSize;
    
    // Initialize core DDF engine with actual allocated database size
    if (!ddfCore.create(memory, databaseSize)) {
        ESP_LOGE("DB", "Failed to initialize DDF core with %d bytes", databaseSize);
        return WISP_ERROR_OUT_OF_MEMORY;
    }
    
    // Create built-in tables
    WispErrorCode result = createBuiltinTables();
    if (result != WISP_SUCCESS) {
        ESP_LOGE("DB", "Failed to create builtin tables: %d", (int)result);
        ddfCore.shutdown();
        return result;
    }
    
    initialized = true;
    ESP_LOGI("DB", "Document database initialized with %d bytes", (int)memSize);
    
    return WISP_SUCCESS;
}

void DocDatabase::shutdown() {
    if (initialized) {
        ddfCore.shutdown();
        initialized = false;
        memory = nullptr;
        memorySize = 0;
        kvTableId = metaTableId = configTableId = 0;
        ESP_LOGI("DB", "Document database shutdown");
    }
}

WispErrorCode DocDatabase::createBuiltinTables() {
    // Minimal app_state database - create essential tables with small footprint
    
    // Create key-value table for app state (reduced size)
    const DDFColumn kvColumns[] = {
        DDF_PRIMARY_KEY("key", DDF_TYPE_U32),
        DDF_COLUMN("type", DDF_TYPE_U8, 0),
        DDF_COLUMN("size", DDF_TYPE_U8, 0),
        DDF_COLUMN("data", DDF_TYPE_BYTES, 32)  // Reduced from 58 to 32 bytes
    };
    kvTableId = ddfCore.createTable("app_state", kvColumns, 4, 16);  // Reduced from 256 to 16 rows
    if (kvTableId == 0) {
        return WISP_ERROR_PARTITION_FULL;
    }
    
    // Create table metadata table (minimal)
    const DDFColumn metaColumns[] = {
        DDF_PRIMARY_KEY("table_id", DDF_TYPE_U16),
        DDF_COLUMN("name", DDF_TYPE_STRING, 16),
        DDF_COLUMN("permissions", DDF_TYPE_U8, 0),
        DDF_COLUMN("max_rows", DDF_TYPE_U16, 0),
        DDF_COLUMN("flags", DDF_TYPE_U8, 0)  // Removed extra columns
    };
    metaTableId = ddfCore.createTable("meta", metaColumns, 5, 8);  // Reduced from 64 to 8 rows
    if (metaTableId == 0) {
        return WISP_ERROR_PARTITION_FULL;
    }
    
    // Create minimal configuration table
    const DDFColumn configColumns[] = {
        DDF_PRIMARY_KEY("config_id", DDF_TYPE_U16),
        DDF_COLUMN("key", DDF_TYPE_STRING, 8),   // Reduced from 16 to 8
        DDF_COLUMN("value", DDF_TYPE_STRING, 16) // Reduced from 32 to 16
    };
    configTableId = ddfCore.createTable("config", configColumns, 3, 8);  // Reduced from 32 to 8 rows
    if (configTableId == 0) {
        return WISP_ERROR_PARTITION_FULL;
    }
    
    // Register built-in table metadata (minimal structure)
    struct MinimalTableMeta {
        uint16_t table_id;
        char name[16];
        uint8_t permissions;
        uint16_t max_rows;
        uint8_t flags;
    };
    
    MinimalTableMeta appStateMeta = {kvTableId, "app_state", DDF_TABLE_READ_WRITE, 16, 0};
    ddfCore.insertRow(metaTableId, &appStateMeta);
    
    MinimalTableMeta metaMeta = {metaTableId, "meta", DDF_TABLE_READ_ONLY, 8, 0};
    ddfCore.insertRow(metaTableId, &metaMeta);
    
    MinimalTableMeta configMeta = {configTableId, "config", DDF_TABLE_READ_WRITE, 8, 0};
    ddfCore.insertRow(metaTableId, &configMeta);
    
    return WISP_SUCCESS;
}

bool DocDatabase::checkTablePermission(uint16_t tableId, uint8_t requiredPermission) {
    if (!initialized) return false;
    
    // Built-in tables have fixed permissions
    if (isBuiltinTable(tableId)) {
        if (tableId == kvTableId || tableId == configTableId) {
            return true; // KV and config tables are always read-write
        } else if (tableId == metaTableId) {
            return (requiredPermission == DDF_TABLE_READABLE); // Meta table is read-only
        }
    }
    
    // Look up permissions in metadata table
    DDFResultSet results;
    if (ddfCore.simpleSelect(metaTableId, "table_id", &tableId, &results)) {
        if (results.rowCount > 0) {
            DDFTableMeta meta;
            if (ddfCore.getRow(metaTableId, results.rowIds[0], &meta)) {
                return (meta.permissions & requiredPermission) != 0;
            }
        }
    }
    
    return false; // Default deny
}

bool DocDatabase::isBuiltinTable(uint16_t tableId) {
    return (tableId == kvTableId || tableId == metaTableId || tableId == configTableId);
}

uint32_t DocDatabase::hashKey(uint32_t key) {
    // Simple hash function for key-value store
    key ^= key >> 16;
    key *= 0x85ebca6b;
    key ^= key >> 13;
    key *= 0xc2b2ae35;
    key ^= key >> 16;
    return key;
}

// === KEY-VALUE STORE IMPLEMENTATION ===

WispErrorCode DocDatabase::setKeyValue(uint32_t key, const void* data, uint8_t size, uint8_t type) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    if (!data || size == 0 || size > 32) return WISP_ERROR_INVALID_PARAMS;  // Reduced from 58 to 32
    
    // Check if key already exists and update or insert
    DDFResultSet results;
    if (ddfCore.simpleSelect(kvTableId, "key", &key, &results)) {
        if (results.rowCount > 0) {
            // Update existing entry (using minimal structure)
            struct MinimalKVEntry {
                uint32_t key;
                uint8_t type;
                uint8_t size;
                uint8_t data[32];  // Reduced to 32 bytes
            } entry;
            
            entry.key = key;
            entry.type = type;
            entry.size = size;
            memset(entry.data, 0, 32);
            memcpy(entry.data, data, size);
            
            return ddfCore.updateRow(kvTableId, results.rowIds[0], &entry) ? 
                   WISP_SUCCESS : WISP_ERROR_PARTITION_FULL;
        }
    }
    
    // Insert new entry (using minimal structure)
    struct MinimalKVEntry {
        uint32_t key;
        uint8_t type;
        uint8_t size;
        uint8_t data[32];  // Reduced to 32 bytes
    } entry;
    
    entry.key = key;
    entry.type = type;
    entry.size = size;
    memset(entry.data, 0, 32);
    memcpy(entry.data, data, size);
    
    uint16_t rowId = ddfCore.insertRow(kvTableId, &entry);
    return (rowId > 0) ? WISP_SUCCESS : WISP_ERROR_PARTITION_FULL;
}

WispErrorCode DocDatabase::getKeyValue(uint32_t key, void* buffer, uint8_t maxSize, uint8_t* actualSize) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    if (!buffer || maxSize == 0) return WISP_ERROR_INVALID_PARAMS;
    
    DDFResultSet results;
    if (ddfCore.simpleSelect(kvTableId, "key", &key, &results)) {
        if (results.rowCount > 0) {
            DDFKeyValueEntry entry;
            if (ddfCore.getRow(kvTableId, results.rowIds[0], &entry)) {
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

WispErrorCode DocDatabase::setU8(uint32_t key, uint8_t value) {
    return setKeyValue(key, &value, sizeof(value), DDF_TYPE_U8);
}

WispErrorCode DocDatabase::setU16(uint32_t key, uint16_t value) {
    return setKeyValue(key, &value, sizeof(value), DDF_TYPE_U16);
}

WispErrorCode DocDatabase::setU32(uint32_t key, uint32_t value) {
    return setKeyValue(key, &value, sizeof(value), DDF_TYPE_U32);
}

WispErrorCode DocDatabase::setFloat(uint32_t key, float value) {
    return setKeyValue(key, &value, sizeof(value), DDF_TYPE_FLOAT);
}

WispErrorCode DocDatabase::setString(uint32_t key, const char* value) {
    if (!value) return WISP_ERROR_INVALID_PARAMS;
    uint8_t len = strlen(value);
    if (len > 31) len = 31; // Leave space for null terminator (reduced from 57 to 31)
    
    char buffer[32];  // Reduced from 58 to 32
    memset(buffer, 0, 32);
    memcpy(buffer, value, len);
    buffer[len] = '\0';
    
    return setKeyValue(key, buffer, len + 1, DDF_TYPE_STRING);
}

WispErrorCode DocDatabase::setBytes(uint32_t key, const void* data, uint8_t size) {
    return setKeyValue(key, data, size, DDF_TYPE_BYTES);
}

uint8_t DocDatabase::getU8(uint32_t key, uint8_t defaultValue) {
    uint8_t value = defaultValue;
    uint8_t actualSize;
    getKeyValue(key, &value, sizeof(value), &actualSize);
    return value;
}

uint16_t DocDatabase::getU16(uint32_t key, uint16_t defaultValue) {
    uint16_t value = defaultValue;
    uint8_t actualSize;
    getKeyValue(key, &value, sizeof(value), &actualSize);
    return value;
}

uint32_t DocDatabase::getU32(uint32_t key, uint32_t defaultValue) {
    uint32_t value = defaultValue;
    uint8_t actualSize;
    getKeyValue(key, &value, sizeof(value), &actualSize);
    return value;
}

float DocDatabase::getFloat(uint32_t key, float defaultValue) {
    float value = defaultValue;
    uint8_t actualSize;
    getKeyValue(key, &value, sizeof(value), &actualSize);
    return value;
}

bool DocDatabase::getString(uint32_t key, char* buffer, uint8_t bufferSize) {
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

WispErrorCode DocDatabase::getBytes(uint32_t key, void* buffer, uint8_t maxSize, uint8_t* actualSize) {
    return getKeyValue(key, buffer, maxSize, actualSize);
}

bool DocDatabase::existsKey(uint32_t key) {
    uint8_t dummy;
    uint8_t actualSize;
    return getKeyValue(key, &dummy, sizeof(dummy), &actualSize) == WISP_SUCCESS;
}

WispErrorCode DocDatabase::removeKey(uint32_t key) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    
    DDFResultSet results;
    if (ddfCore.simpleSelect(kvTableId, "key", &key, &results)) {
        if (results.rowCount > 0) {
            return ddfCore.deleteRow(kvTableId, results.rowIds[0]) ? 
                   WISP_SUCCESS : WISP_ERROR_KEY_NOT_FOUND;
        }
    }
    
    return WISP_ERROR_KEY_NOT_FOUND;
}

// === STRUCTURED DATA IMPLEMENTATION ===

uint16_t DocDatabase::createTable(const char* name, const DDFColumn* columns, 
                                         uint8_t columnCount, uint16_t maxRows, uint8_t permissions) {
    if (!initialized) return 0;
    if (!name || !columns || columnCount == 0) return 0;
    
    // Create table in DDF core
    uint16_t tableId = ddfCore.createTable(name, columns, columnCount, maxRows);
    if (tableId == 0) return 0;
    
    // Register table metadata
    uint32_t currentTime = esp_log_timestamp();
    DDFTableMeta meta = {
        tableId, "", permissions, columnCount, maxRows, 0, currentTime, currentTime, 0
    };
    strncpy(meta.name, name, 15);
    meta.name[15] = '\0';
    
    uint16_t metaRowId = ddfCore.insertRow(metaTableId, &meta);
    if (metaRowId == 0) {
        // Failed to insert metadata, clean up table
        ddfCore.dropTable(tableId);
        return 0;
    }
    
    ESP_LOGI("DB", "Created table '%s' (ID: %d) with permissions 0x%02X", name, tableId, permissions);
    return tableId;
}

WispErrorCode DocDatabase::setTablePermissions(uint16_t tableId, uint8_t permissions) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    if (isBuiltinTable(tableId)) return WISP_ERROR_INVALID_PARAMS; // Can't change builtin table permissions
    
    DDFResultSet results;
    if (ddfCore.simpleSelect(metaTableId, "table_id", &tableId, &results)) {
        if (results.rowCount > 0) {
            DDFTableMeta meta;
            if (ddfCore.getRow(metaTableId, results.rowIds[0], &meta)) {
                meta.permissions = permissions;
                meta.modifiedTime = esp_log_timestamp();
                
                return ddfCore.updateRow(metaTableId, results.rowIds[0], &meta) ? 
                       WISP_SUCCESS : WISP_ERROR_PARTITION_FULL;
            }
        }
    }
    
    return WISP_ERROR_INVALID_PARTITION;
}

uint8_t DocDatabase::getTablePermissions(uint16_t tableId) {
    if (!initialized) return 0;
    
    if (isBuiltinTable(tableId)) {
        if (tableId == kvTableId || tableId == configTableId) {
            return DDF_TABLE_READ_WRITE;
        } else if (tableId == metaTableId) {
            return DDF_TABLE_READ_ONLY;
        }
    }
    
    DDFResultSet results;
    if (ddfCore.simpleSelect(metaTableId, "table_id", &tableId, &results)) {
        if (results.rowCount > 0) {
            DDFTableMeta meta;
            if (ddfCore.getRow(metaTableId, results.rowIds[0], &meta)) {
                return meta.permissions;
            }
        }
    }
    
    return 0; // No permissions found
}

uint16_t DocDatabase::insertRow(uint16_t tableId, const void* rowData) {
    if (!initialized) return 0;
    if (!checkTablePermission(tableId, DDF_TABLE_WRITABLE)) return 0;
    
    return ddfCore.insertRow(tableId, rowData);
}

WispErrorCode DocDatabase::updateRow(uint16_t tableId, uint16_t rowId, const void* rowData) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    if (!checkTablePermission(tableId, DDF_TABLE_WRITABLE)) return WISP_ERROR_INVALID_PARTITION;
    
    return ddfCore.updateRow(tableId, rowId, rowData) ? WISP_SUCCESS : WISP_ERROR_KEY_NOT_FOUND;
}

WispErrorCode DocDatabase::getRow(uint16_t tableId, uint16_t rowId, void* rowData) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    if (!checkTablePermission(tableId, DDF_TABLE_READABLE)) return WISP_ERROR_INVALID_PARTITION;
    
    return ddfCore.getRow(tableId, rowId, rowData) ? WISP_SUCCESS : WISP_ERROR_KEY_NOT_FOUND;
}

WispErrorCode DocDatabase::deleteRow(uint16_t tableId, uint16_t rowId) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    if (!checkTablePermission(tableId, DDF_TABLE_WRITABLE)) return WISP_ERROR_INVALID_PARTITION;
    
    return ddfCore.deleteRow(tableId, rowId) ? WISP_SUCCESS : WISP_ERROR_KEY_NOT_FOUND;
}

WispErrorCode DocDatabase::selectAll(uint16_t tableId, DDFResultSet* results) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    if (!checkTablePermission(tableId, DDF_TABLE_READABLE)) return WISP_ERROR_INVALID_PARTITION;
    
    return ddfCore.selectAll(tableId, results) ? WISP_SUCCESS : WISP_ERROR_KEY_NOT_FOUND;
}

WispErrorCode DocDatabase::simpleSelect(uint16_t tableId, const char* whereColumn, 
                                               const void* whereValue, DDFResultSet* results) {
    if (!initialized) return WISP_ERROR_NOT_INITIALIZED;
    if (!checkTablePermission(tableId, DDF_TABLE_READABLE)) return WISP_ERROR_INVALID_PARTITION;
    
    return ddfCore.simpleSelect(tableId, whereColumn, whereValue, results) ? 
           WISP_SUCCESS : WISP_ERROR_KEY_NOT_FOUND;
}

uint16_t DocDatabase::getTableId(const char* name) {
    if (!initialized || !name) return 0;
    return ddfCore.getTableId(name);
}

uint32_t DocDatabase::getUsedMemory() const {
    return initialized ? const_cast<DDFDatabase&>(ddfCore).getUsedMemory() : 0;
}

uint32_t DocDatabase::getFreeMemory() const {
    return initialized ? const_cast<DDFDatabase&>(ddfCore).getFreeMemory() : 0;
}

void DocDatabase::printStats() {
    if (!initialized) {
        ESP_LOGI("DB", "Database not initialized");
        return;
    }
    
    ESP_LOGI("DB", "=== Document Database Statistics ===");
    ESP_LOGI("DB", "Memory: %d/%d bytes used (%.1f%%)", 
             (int)getUsedMemory(), (int)memorySize,
             (getUsedMemory() * 100.0f) / memorySize);
    
    ddfCore.printAllTables();
}

bool DocDatabase::validateDatabase() {
    return initialized ? ddfCore.validate() : false;
}
