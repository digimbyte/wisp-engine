// wbdf_compatibility.h - Compatibility layer for existing database code
#pragma once

#include "wbdf_integration.h"
#include <cstring>

// Compatibility layer that makes WBDF work with existing code patterns
class WispDatabaseCompatible : public WispDatabaseExtended {
private:
    // Legacy key mapping tables
    struct LegacyKeyMapping {
        uint32_t key;
        uint16_t tableId;
        uint16_t rowId;
        uint8_t category;
    };
    
    static const uint8_t MAX_LEGACY_MAPPINGS = 32;
    LegacyKeyMapping legacyMappings[MAX_LEGACY_MAPPINGS];
    uint8_t mappingCount;
    
    // Key category constants for legacy compatibility
    static const uint8_t KEY_CATEGORY_TRAINER = 1;
    static const uint8_t KEY_CATEGORY_POKEMON = 2;
    static const uint8_t KEY_CATEGORY_ITEM = 3;
    static const uint8_t KEY_CATEGORY_QUEST = 4;
    static const uint8_t KEY_CATEGORY_GAME_STATE = 5;
    static const uint8_t KEY_CATEGORY_SNAKE_SEGMENT = 6;
    static const uint8_t KEY_CATEGORY_FOOD = 7;
    static const uint8_t KEY_CATEGORY_SETTINGS = 8;
    static const uint8_t KEY_CATEGORY_SENSOR = 9;
    static const uint8_t KEY_CATEGORY_DEVICE = 10;
    
public:
    WispDatabaseCompatible() : WispDatabaseExtended(), mappingCount(0) {
        memset(legacyMappings, 0, sizeof(legacyMappings));
    }
    
    // Legacy initialization interface
    WispErrorCode initialize(const WispPartitionConfig* config) {
        // Calculate total memory from partition configuration
        uint32_t totalMemory = 8192; // Default 8KB
        if (config) {
            totalMemory = config->romSize + config->saveSize + 
                         config->backupSize + config->runtimeSize;
            // Ensure minimum size for WBDF
            if (totalMemory < 4096) totalMemory = 4096;
        }
        
        // Initialize structured database
        WispErrorCode result = initializeStructured(totalMemory);
        if (result != WISP_SUCCESS) {
            return result;
        }
        
        // Create default tables for common legacy patterns
        createLegacyTables();
        
        return WISP_SUCCESS;
    }
    
    // Legacy set interface with automatic table mapping
    WispErrorCode set(uint32_t key, const void* data, uint8_t size, uint8_t type) {
        if (!data || size == 0) {
            return WISP_ERROR_INVALID_PARAMS;
        }
        
        // Parse legacy key format
        uint8_t category = WISP_KEY_CATEGORY(key);
        uint16_t id = WISP_KEY_ID(key);
        
        // Map to appropriate WBDF table
        switch (category) {
            case KEY_CATEGORY_TRAINER:
                return setTrainerData(id, data, size);
                
            case KEY_CATEGORY_POKEMON:
                return setPokemonData(id, data, size);
                
            case KEY_CATEGORY_ITEM:
                return setItemData(id, data, size);
                
            case KEY_CATEGORY_QUEST:
                return setQuestData(id, data, size);
                
            case KEY_CATEGORY_GAME_STATE:
                return setGameStateData(id, data, size);
                
            case KEY_CATEGORY_SNAKE_SEGMENT:
                return setSnakeSegmentData(id, data, size);
                
            case KEY_CATEGORY_FOOD:
                return setFoodData(id, data, size);
                
            case KEY_CATEGORY_SETTINGS:
                return setSettingsData(id, data, size);
                
            case KEY_CATEGORY_SENSOR:
                return setSensorData(id, data, size);
                
            case KEY_CATEGORY_DEVICE:
                return setDeviceData(id, data, size);
                
            default:
                // Fallback to key-value store for unknown categories
                return WispPartitionedDB::set(key, data, size, type);
        }
    }
    
    // Legacy get interface with automatic table mapping
    WispErrorCode get(uint32_t key, void* buffer, uint8_t maxSize, uint8_t* actualSize = nullptr) {
        if (!buffer || maxSize == 0) {
            return WISP_ERROR_INVALID_PARAMS;
        }
        
        // Parse legacy key format
        uint8_t category = WISP_KEY_CATEGORY(key);
        uint16_t id = WISP_KEY_ID(key);
        
        // Map to appropriate WBDF table
        switch (category) {
            case KEY_CATEGORY_TRAINER:
                return getTrainerData(id, buffer, maxSize, actualSize);
                
            case KEY_CATEGORY_POKEMON:
                return getPokemonData(id, buffer, maxSize, actualSize);
                
            case KEY_CATEGORY_ITEM:
                return getItemData(id, buffer, maxSize, actualSize);
                
            case KEY_CATEGORY_QUEST:
                return getQuestData(id, buffer, maxSize, actualSize);
                
            case KEY_CATEGORY_GAME_STATE:
                return getGameStateData(id, buffer, maxSize, actualSize);
                
            case KEY_CATEGORY_SNAKE_SEGMENT:
                return getSnakeSegmentData(id, buffer, maxSize, actualSize);
                
            case KEY_CATEGORY_FOOD:
                return getFoodData(id, buffer, maxSize, actualSize);
                
            case KEY_CATEGORY_SETTINGS:
                return getSettingsData(id, buffer, maxSize, actualSize);
                
            case KEY_CATEGORY_SENSOR:
                return getSensorData(id, buffer, maxSize, actualSize);
                
            case KEY_CATEGORY_DEVICE:
                return getDeviceData(id, buffer, maxSize, actualSize);
                
            default:
                // Fallback to key-value store for unknown categories
                return WispPartitionedDB::get(key, buffer, maxSize, actualSize);
        }
    }
    
    // Legacy exists interface
    bool exists(uint32_t key) {
        uint8_t dummy;
        return get(key, &dummy, sizeof(dummy)) == WISP_SUCCESS;
    }

private:
    void createLegacyTables() {
        if (!wbdfInitialized) return;
        
        // Create common legacy tables automatically
        
        // Generic data table for miscellaneous legacy data
        const WBDFColumn legacyColumns[] = {
            WBDF_PRIMARY_KEY("legacy_id", WBDF_TYPE_U16),
            WBDF_COLUMN("data", WBDF_TYPE_BYTES, 64),
            WBDF_COLUMN("size", WBDF_TYPE_U8, 0),
            WBDF_COLUMN("type", WBDF_TYPE_U8, 0)
        };
        createGameTable("legacy_data", legacyColumns, 4, 128);
        
        // Create standard game tables if they don't exist
        GameTableFactory::createRPGTables(this);
    }
    
    // Helper methods for different data types
    WispErrorCode setTrainerData(uint16_t id, const void* data, uint8_t size) {
        // Store in legacy data table
        return setLegacyData(KEY_CATEGORY_TRAINER, id, data, size);
    }
    
    WispErrorCode getTrainerData(uint16_t id, void* buffer, uint8_t maxSize, uint8_t* actualSize) {
        return getLegacyData(KEY_CATEGORY_TRAINER, id, buffer, maxSize, actualSize);
    }
    
    WispErrorCode setPokemonData(uint16_t id, const void* data, uint8_t size) {
        return setLegacyData(KEY_CATEGORY_POKEMON, id, data, size);
    }
    
    WispErrorCode getPokemonData(uint16_t id, void* buffer, uint8_t maxSize, uint8_t* actualSize) {
        return getLegacyData(KEY_CATEGORY_POKEMON, id, buffer, maxSize, actualSize);
    }
    
    WispErrorCode setItemData(uint16_t id, const void* data, uint8_t size) {
        // Try to parse as item data and store in items table if possible
        if (size >= sizeof(GameTables::Item)) {
            const GameTables::Item* item = static_cast<const GameTables::Item*>(data);
            if (addItem(item->id, item->name, item->category, item->rarity, item->value)) {
                return WISP_SUCCESS;
            }
        }
        // Fallback to legacy data table
        return setLegacyData(KEY_CATEGORY_ITEM, id, data, size);
    }
    
    WispErrorCode getItemData(uint16_t id, void* buffer, uint8_t maxSize, uint8_t* actualSize) {
        // Try to get from items table first
        GameTables::Item item;
        if (getItem(id, &item)) {
            if (maxSize >= sizeof(item)) {
                memcpy(buffer, &item, sizeof(item));
                if (actualSize) *actualSize = sizeof(item);
                return WISP_SUCCESS;
            } else {
                return WISP_ERROR_BUFFER_OVERFLOW;
            }
        }
        // Fallback to legacy data table
        return getLegacyData(KEY_CATEGORY_ITEM, id, buffer, maxSize, actualSize);
    }
    
    WispErrorCode setQuestData(uint16_t id, const void* data, uint8_t size) {
        // Try to parse as quest data and store in quests table if possible
        if (size >= sizeof(GameTables::Quest)) {
            const GameTables::Quest* quest = static_cast<const GameTables::Quest*>(data);
            if (addQuest(quest->id, quest->title, quest->status, quest->prerequisite)) {
                return WISP_SUCCESS;
            }
        }
        // Fallback to legacy data table
        return setLegacyData(KEY_CATEGORY_QUEST, id, data, size);
    }
    
    WispErrorCode getQuestData(uint16_t id, void* buffer, uint8_t maxSize, uint8_t* actualSize) {
        // Try to get from quests table first
        GameTables::Quest quest;
        if (getQuest(id, &quest)) {
            if (maxSize >= sizeof(quest)) {
                memcpy(buffer, &quest, sizeof(quest));
                if (actualSize) *actualSize = sizeof(quest);
                return WISP_SUCCESS;
            } else {
                return WISP_ERROR_BUFFER_OVERFLOW;
            }
        }
        // Fallback to legacy data table
        return getLegacyData(KEY_CATEGORY_QUEST, id, buffer, maxSize, actualSize);
    }
    
    WispErrorCode setGameStateData(uint16_t id, const void* data, uint8_t size) {
        return setLegacyData(KEY_CATEGORY_GAME_STATE, id, data, size);
    }
    
    WispErrorCode getGameStateData(uint16_t id, void* buffer, uint8_t maxSize, uint8_t* actualSize) {
        return getLegacyData(KEY_CATEGORY_GAME_STATE, id, buffer, maxSize, actualSize);
    }
    
    WispErrorCode setSnakeSegmentData(uint16_t id, const void* data, uint8_t size) {
        return setLegacyData(KEY_CATEGORY_SNAKE_SEGMENT, id, data, size);
    }
    
    WispErrorCode getSnakeSegmentData(uint16_t id, void* buffer, uint8_t maxSize, uint8_t* actualSize) {
        return getLegacyData(KEY_CATEGORY_SNAKE_SEGMENT, id, buffer, maxSize, actualSize);
    }
    
    WispErrorCode setFoodData(uint16_t id, const void* data, uint8_t size) {
        return setLegacyData(KEY_CATEGORY_FOOD, id, data, size);
    }
    
    WispErrorCode getFoodData(uint16_t id, void* buffer, uint8_t maxSize, uint8_t* actualSize) {
        return getLegacyData(KEY_CATEGORY_FOOD, id, buffer, maxSize, actualSize);
    }
    
    WispErrorCode setSettingsData(uint16_t id, const void* data, uint8_t size) {
        return setLegacyData(KEY_CATEGORY_SETTINGS, id, data, size);
    }
    
    WispErrorCode getSettingsData(uint16_t id, void* buffer, uint8_t maxSize, uint8_t* actualSize) {
        return getLegacyData(KEY_CATEGORY_SETTINGS, id, buffer, maxSize, actualSize);
    }
    
    WispErrorCode setSensorData(uint16_t id, const void* data, uint8_t size) {
        return setLegacyData(KEY_CATEGORY_SENSOR, id, data, size);
    }
    
    WispErrorCode getSensorData(uint16_t id, void* buffer, uint8_t maxSize, uint8_t* actualSize) {
        return getLegacyData(KEY_CATEGORY_SENSOR, id, buffer, maxSize, actualSize);
    }
    
    WispErrorCode setDeviceData(uint16_t id, const void* data, uint8_t size) {
        return setLegacyData(KEY_CATEGORY_DEVICE, id, data, size);
    }
    
    WispErrorCode getDeviceData(uint16_t id, void* buffer, uint8_t maxSize, uint8_t* actualSize) {
        return getLegacyData(KEY_CATEGORY_DEVICE, id, buffer, maxSize, actualSize);
    }
    
    // Generic legacy data storage in WBDF format
    WispErrorCode setLegacyData(uint8_t category, uint16_t id, const void* data, uint8_t size) {
        if (!wbdfInitialized) {
            return WISP_ERROR_NOT_INITIALIZED;
        }
        
        // Find legacy data table
        uint16_t tableId = getGameTableId("legacy_data");
        if (tableId == 0) {
            return WISP_ERROR_INVALID_PARTITION;
        }
        
        // Create a composite key based on category and id
        uint16_t compositeId = (category << 8) | (id & 0xFF);
        
        // Create row data
        struct LegacyRow {
            uint16_t legacy_id;
            uint8_t data[64];
            uint8_t size;
            uint8_t type;
        } __attribute__((packed));
        
        LegacyRow row;
        row.legacy_id = compositeId;
        row.size = (size > 64) ? 64 : size;
        row.type = category;
        memset(row.data, 0, 64);
        memcpy(row.data, data, row.size);
        
        // Insert into WBDF table
        WBDFDatabase* wbdf = getStructuredDB();
        if (wbdf) {
            uint16_t rowId = wbdf->insertRow(tableId, &row);
            return (rowId > 0) ? WISP_SUCCESS : WISP_ERROR_PARTITION_FULL;
        }
        
        return WISP_ERROR_NOT_INITIALIZED;
    }
    
    WispErrorCode getLegacyData(uint8_t category, uint16_t id, void* buffer, uint8_t maxSize, uint8_t* actualSize) {
        if (!wbdfInitialized) {
            return WISP_ERROR_NOT_INITIALIZED;
        }
        
        // Find legacy data table
        uint16_t tableId = getGameTableId("legacy_data");
        if (tableId == 0) {
            return WISP_ERROR_INVALID_PARTITION;
        }
        
        // Create composite key
        uint16_t compositeId = (category << 8) | (id & 0xFF);
        
        // Search for the data using WBDF simple select
        WBDFDatabase* wbdf = getStructuredDB();
        if (wbdf) {
            WBDFResultSet results;
            if (wbdf->simpleSelect(tableId, "legacy_id", &compositeId, &results)) {
                if (results.rowCount > 0) {
                    // Get the first matching row
                    struct LegacyRow {
                        uint16_t legacy_id;
                        uint8_t data[64];
                        uint8_t size;
                        uint8_t type;
                    } __attribute__((packed));
                    
                    LegacyRow row;
                    if (wbdf->getRow(tableId, results.rowIds[0], &row)) {
                        uint8_t copySize = (row.size > maxSize) ? maxSize : row.size;
                        memcpy(buffer, row.data, copySize);
                        if (actualSize) *actualSize = row.size;
                        return (row.size <= maxSize) ? WISP_SUCCESS : WISP_ERROR_BUFFER_OVERFLOW;
                    }
                }
            }
        }
        
        return WISP_ERROR_KEY_NOT_FOUND;
    }
};

// Global compatibility instance - drop-in replacement for existing wispDB
extern WispDatabaseCompatible wispCompatDB;
