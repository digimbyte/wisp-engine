// wbdf_integration.cpp - Implementation of WBDF integration with Wisp Database
#include "wbdf_integration.h"
#include <string.h>

WispDatabaseExtended::WispDatabaseExtended() : WispPartitionedDB(), wbdfInitialized(false), wbdfMemory(nullptr), wbdfSize(0) {
}

WispDatabaseExtended::~WispDatabaseExtended() {
    if (wbdfMemory) {
        delete[] wbdfMemory;
        wbdfMemory = nullptr;
    }
}

WispErrorCode WispDatabaseExtended::initializeStructured(uint32_t structuredMemorySize) {
    // First initialize the base key-value database
    WispErrorCode result = WispPartitionedDB::initialize(nullptr);
    if (result != WISP_SUCCESS) {
        return result;
    }
    
    // Allocate memory for structured database
    wbdfSize = structuredMemorySize;
    wbdfMemory = new uint8_t[wbdfSize];
    if (!wbdfMemory) {
        return WISP_ERROR_OUT_OF_MEMORY;
    }
    
    // Create the WBDF database
    if (!structuredDB.create(wbdfMemory, wbdfSize)) {
        delete[] wbdfMemory;
        wbdfMemory = nullptr;
        return WISP_ERROR_INVALID_CONFIG;
    }
    
    wbdfInitialized = true;
    
    // Store the WBDF database in the key-value store for persistence
    set(WBDF_PARTITION_KEY, wbdfMemory, wbdfSize, 0);
    
    return WISP_SUCCESS;
}

uint16_t WispDatabaseExtended::createGameTable(const char* tableName, const WBDFColumn* columns, uint8_t columnCount, uint16_t maxRows) {
    if (!wbdfInitialized) {
        return 0;
    }
    
    return structuredDB.createTable(tableName, columns, columnCount, maxRows);
}

bool WispDatabaseExtended::dropGameTable(const char* tableName) {
    if (!wbdfInitialized) {
        return false;
    }
    
    uint16_t tableId = structuredDB.getTableId(tableName);
    return tableId > 0 ? structuredDB.dropTable(tableId) : false;
}

uint16_t WispDatabaseExtended::getGameTableId(const char* tableName) {
    if (!wbdfInitialized) {
        return 0;
    }
    
    return structuredDB.getTableId(tableName);
}

bool WispDatabaseExtended::addItem(uint16_t itemId, const char* name, uint8_t category, uint8_t rarity, uint32_t value) {
    if (!wbdfInitialized) {
        return false;
    }
    
    uint16_t tableId = structuredDB.getTableId("items");
    if (tableId == 0) {
        return false;
    }
    
    GameTables::Item item = {};
    item.id = itemId;
    strncpy(item.name, name, sizeof(item.name) - 1);
    item.category = category;
    item.rarity = rarity;
    item.value = value;
    item.stackSize = 1;  // Default
    item.flags = 0;
    
    uint16_t rowId = structuredDB.insertRow(tableId, &item);
    return rowId > 0;
}

bool WispDatabaseExtended::getItem(uint16_t itemId, GameTables::Item* item) {
    if (!wbdfInitialized || !item) {
        return false;
    }
    
    uint16_t tableId = structuredDB.getTableId("items");
    if (tableId == 0) {
        return false;
    }
    
    // Use simple select to find item by ID
    uint16_t results[1];
    uint16_t resultCount = 1;
    
    WBDFResultSet resultSet;
    resultSet.rowIds = results;
    resultSet.maxResults = 1;
    
    if (!structuredDB.simpleSelect(tableId, "id", &itemId, &resultSet)) {
        return false;
    }
    
    if (resultSet.rowCount == 0) {
        return false;
    }
    
    return structuredDB.getRow(tableId, resultSet.rowIds[0], item);
}

bool WispDatabaseExtended::findItemsByCategory(uint8_t category, uint16_t* itemIds, uint16_t* count, uint16_t maxResults) {
    if (!wbdfInitialized || !itemIds || !count) {
        return false;
    }
    
    uint16_t tableId = structuredDB.getTableId("items");
    if (tableId == 0) {
        return false;
    }
    
    uint16_t* results = new uint16_t[maxResults];
    WBDFResultSet resultSet;
    resultSet.rowIds = results;
    resultSet.maxResults = maxResults;
    
    bool success = structuredDB.simpleSelect(tableId, "category", &category, &resultSet);
    
    if (success) {
        // Extract item IDs from results
        *count = resultSet.rowCount;
        for (uint16_t i = 0; i < resultSet.rowCount; i++) {
            GameTables::Item item;
            if (structuredDB.getRow(tableId, resultSet.rowIds[i], &item)) {
                itemIds[i] = item.id;
            }
        }
    } else {
        *count = 0;
    }
    
    delete[] results;
    return success;
}

bool WispDatabaseExtended::addQuest(uint16_t questId, const char* title, uint8_t status, uint16_t prerequisite) {
    if (!wbdfInitialized) {
        return false;
    }
    
    uint16_t tableId = structuredDB.getTableId("quests");
    if (tableId == 0) {
        return false;
    }
    
    GameTables::Quest quest = {};
    quest.id = questId;
    strncpy(quest.title, title, sizeof(quest.title) - 1);
    quest.status = status;
    quest.progress = 0;
    quest.flags = 0;
    quest.prerequisite = prerequisite;
    quest.reward_item = 0;
    quest.reward_exp = 0;
    
    uint16_t rowId = structuredDB.insertRow(tableId, &quest);
    return rowId > 0;
}

bool WispDatabaseExtended::updateQuestStatus(uint16_t questId, uint8_t status, uint8_t progress) {
    if (!wbdfInitialized) {
        return false;
    }
    
    GameTables::Quest quest;
    if (!getQuest(questId, &quest)) {
        return false;
    }
    
    quest.status = status;
    quest.progress = progress;
    
    uint16_t tableId = structuredDB.getTableId("quests");
    if (tableId == 0) {
        return false;
    }
    
    // Find the row ID for this quest
    uint16_t results[1];
    WBDFResultSet resultSet;
    resultSet.rowIds = results;
    resultSet.maxResults = 1;
    
    if (!structuredDB.simpleSelect(tableId, "id", &questId, &resultSet) || resultSet.rowCount == 0) {
        return false;
    }
    
    return structuredDB.updateRow(tableId, resultSet.rowIds[0], &quest);
}

bool WispDatabaseExtended::getQuest(uint16_t questId, GameTables::Quest* quest) {
    if (!wbdfInitialized || !quest) {
        return false;
    }
    
    uint16_t tableId = structuredDB.getTableId("quests");
    if (tableId == 0) {
        return false;
    }
    
    uint16_t results[1];
    WBDFResultSet resultSet;
    resultSet.rowIds = results;
    resultSet.maxResults = 1;
    
    if (!structuredDB.simpleSelect(tableId, "id", &questId, &resultSet)) {
        return false;
    }
    
    if (resultSet.rowCount == 0) {
        return false;
    }
    
    return structuredDB.getRow(tableId, resultSet.rowIds[0], quest);
}

bool WispDatabaseExtended::findQuestsByStatus(uint8_t status, uint16_t* questIds, uint16_t* count, uint16_t maxResults) {
    if (!wbdfInitialized || !questIds || !count) {
        return false;
    }
    
    uint16_t tableId = structuredDB.getTableId("quests");
    if (tableId == 0) {
        return false;
    }
    
    uint16_t* results = new uint16_t[maxResults];
    WBDFResultSet resultSet;
    resultSet.rowIds = results;
    resultSet.maxResults = maxResults;
    
    bool success = structuredDB.simpleSelect(tableId, "status", &status, &resultSet);
    
    if (success) {
        *count = resultSet.rowCount;
        for (uint16_t i = 0; i < resultSet.rowCount; i++) {
            GameTables::Quest quest;
            if (structuredDB.getRow(tableId, resultSet.rowIds[i], &quest)) {
                questIds[i] = quest.id;
            }
        }
    } else {
        *count = 0;
    }
    
    delete[] results;
    return success;
}

bool WispDatabaseExtended::addNPC(uint16_t npcId, const char* name, uint8_t level, uint8_t faction, uint16_t x, uint16_t y) {
    if (!wbdfInitialized) {
        return false;
    }
    
    uint16_t tableId = structuredDB.getTableId("npcs");
    if (tableId == 0) {
        return false;
    }
    
    GameTables::NPC npc = {};
    npc.id = npcId;
    strncpy(npc.name, name, sizeof(npc.name) - 1);
    npc.level = level;
    npc.faction = faction;
    npc.location_x = x;
    npc.location_y = y;
    npc.flags = 0;
    npc.dialogue_id = 0;
    
    uint16_t rowId = structuredDB.insertRow(tableId, &npc);
    return rowId > 0;
}

bool WispDatabaseExtended::getNPC(uint16_t npcId, GameTables::NPC* npc) {
    if (!wbdfInitialized || !npc) {
        return false;
    }
    
    uint16_t tableId = structuredDB.getTableId("npcs");
    if (tableId == 0) {
        return false;
    }
    
    uint16_t results[1];
    WBDFResultSet resultSet;
    resultSet.rowIds = results;
    resultSet.maxResults = 1;
    
    if (!structuredDB.simpleSelect(tableId, "id", &npcId, &resultSet)) {
        return false;
    }
    
    if (resultSet.rowCount == 0) {
        return false;
    }
    
    return structuredDB.getRow(tableId, resultSet.rowIds[0], npc);
}

bool WispDatabaseExtended::findNPCsByFaction(uint8_t faction, uint16_t* npcIds, uint16_t* count, uint16_t maxResults) {
    if (!wbdfInitialized || !npcIds || !count) {
        return false;
    }
    
    uint16_t tableId = structuredDB.getTableId("npcs");
    if (tableId == 0) {
        return false;
    }
    
    uint16_t* results = new uint16_t[maxResults];
    WBDFResultSet resultSet;
    resultSet.rowIds = results;
    resultSet.maxResults = maxResults;
    
    bool success = structuredDB.simpleSelect(tableId, "faction", &faction, &resultSet);
    
    if (success) {
        *count = resultSet.rowCount;
        for (uint16_t i = 0; i < resultSet.rowCount; i++) {
            GameTables::NPC npc;
            if (structuredDB.getRow(tableId, resultSet.rowIds[i], &npc)) {
                npcIds[i] = npc.id;
            }
        }
    } else {
        *count = 0;
    }
    
    delete[] results;
    return success;
}

bool WispDatabaseExtended::executeQuery(const char* tableName, const char* whereColumn, const void* whereValue, uint16_t* resultIds, uint16_t* count, uint16_t maxResults) {
    if (!wbdfInitialized || !tableName || !whereColumn || !whereValue || !resultIds || !count) {
        return false;
    }
    
    uint16_t tableId = structuredDB.getTableId(tableName);
    if (tableId == 0) {
        return false;
    }
    
    uint16_t* results = new uint16_t[maxResults];
    WBDFResultSet resultSet;
    resultSet.rowIds = results;
    resultSet.maxResults = maxResults;
    
    bool success = structuredDB.simpleSelect(tableId, whereColumn, whereValue, &resultSet);
    
    if (success) {
        *count = resultSet.rowCount;
        for (uint16_t i = 0; i < resultSet.rowCount; i++) {
            resultIds[i] = resultSet.rowIds[i];
        }
    } else {
        *count = 0;
    }
    
    delete[] results;
    return success;
}

void WispDatabaseExtended::printStructuredStats() {
    if (!wbdfInitialized) {
        printf("Structured database not initialized\n");
        return;
    }
    
    printf("=== WBDF Structured Database Stats ===\n");
    printf("Memory: %lu/%lu bytes used (%.1f%%)\n", 
           (unsigned long)structuredDB.getUsedMemory(), (unsigned long)wbdfSize,
           (structuredDB.getUsedMemory() * 100.0f) / wbdfSize);
    printf("Free: %lu bytes\n", (unsigned long)structuredDB.getFreeMemory());
    
    structuredDB.printAllTables();
}

void WispDatabaseExtended::printTableData(const char* tableName) {
    if (!wbdfInitialized || !tableName) {
        return;
    }
    
    uint16_t tableId = structuredDB.getTableId(tableName);
    if (tableId == 0) {
        printf("Table '%s' not found\n", tableName);
        return;
    }
    
    structuredDB.printTableInfo(tableId);
    
    // Print sample data based on table type
    if (strcmp(tableName, "items") == 0) {
        printf("Sample items:\n");
        WBDFTableSchema* schema = structuredDB.getTable(tableId);
        if (schema) {
            for (uint16_t i = 1; i <= std::min(schema->rowCount, (uint16_t)5); i++) {
                GameTables::Item item;
                if (structuredDB.getRow(tableId, i, &item)) {
                    printf("  %d: %s (Cat:%d, Rare:%d, Value:%lu)\n", 
                           item.id, item.name, item.category, item.rarity, (unsigned long)item.value);
                }
            }
        }
    } else if (strcmp(tableName, "quests") == 0) {
        printf("Sample quests:\n");
        WBDFTableSchema* schema = structuredDB.getTable(tableId);
        if (schema) {
            for (uint16_t i = 1; i <= std::min(schema->rowCount, (uint16_t)5); i++) {
                GameTables::Quest quest;
                if (structuredDB.getRow(tableId, i, &quest)) {
                    printf("  %d: %s (Status:%d, Progress:%d%%)\n", 
                           quest.id, quest.title, quest.status, quest.progress);
                }
            }
        }
    }
}

bool WispDatabaseExtended::validateStructuredDB() {
    return wbdfInitialized ? structuredDB.validate() : false;
}

// Factory functions implementation
namespace GameTableFactory {
    bool createRPGTables(WispDatabaseExtended* db) {
        if (!db) return false;
        
        // Create items table (max 256 items)
        uint16_t itemTableId = CREATE_ITEM_TABLE(db, 256);
        if (itemTableId == 0) return false;
        
        // Create quests table (max 128 quests)
        uint16_t questTableId = CREATE_QUEST_TABLE(db, 128);
        if (questTableId == 0) return false;
        
        // Create NPCs table (max 128 NPCs)
        uint16_t npcTableId = CREATE_NPC_TABLE(db, 128);
        if (npcTableId == 0) return false;
        
        return true;
    }
    
    bool createInventoryTables(WispDatabaseExtended* db) {
        if (!db) return false;
        
        // Define inventory slot table
        const WBDFColumn inventoryColumns[] = {
            WBDF_PRIMARY_KEY("slot_id", WBDF_TYPE_U16),
            WBDF_INDEXED_COLUMN("item_id", WBDF_TYPE_U16, 0),
            WBDF_COLUMN("quantity", WBDF_TYPE_U8, 0),
            WBDF_COLUMN("condition", WBDF_TYPE_U8, 0),
            WBDF_COLUMN("flags", WBDF_TYPE_U32, 0)
        };
        
        uint16_t invTableId = db->createGameTable("inventory", inventoryColumns, 
                                                  sizeof(inventoryColumns)/sizeof(WBDFColumn), 64);
        
        return invTableId > 0;
    }
    
    bool createWorldTables(WispDatabaseExtended* db) {
        if (!db) return false;
        
        // Define location table
        const WBDFColumn locationColumns[] = {
            WBDF_PRIMARY_KEY("location_id", WBDF_TYPE_U16),
            WBDF_COLUMN("name", WBDF_TYPE_STRING, 32),
            WBDF_INDEXED_COLUMN("zone_id", WBDF_TYPE_U8, 0),
            WBDF_COLUMN("x", WBDF_TYPE_U16, 0),
            WBDF_COLUMN("y", WBDF_TYPE_U16, 0),
            WBDF_COLUMN("flags", WBDF_TYPE_U32, 0)
        };
        
        uint16_t locTableId = db->createGameTable("locations", locationColumns, 
                                                  sizeof(locationColumns)/sizeof(WBDFColumn), 512);
        
        return locTableId > 0;
    }
}
