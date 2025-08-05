// wbdf_integration.h - Integration of WBDF with existing Wisp Database System
#pragma once

#include "database_system.h"
#include "wbdf_format.h"

// Enhanced database partition for structured data
#define WBDF_PARTITION_KEY 0xBDF0001  // Special key for WBDF database

class WispDatabaseExtended : public WispPartitionedDB {
protected:  // Changed from private to protected
    WBDFDatabase structuredDB;
    bool wbdfInitialized;
    uint8_t* wbdfMemory;
    uint32_t wbdfSize;
    
public:
    WispDatabaseExtended();
    ~WispDatabaseExtended();
    
    // Initialize structured database alongside key-value store
    WispErrorCode initializeStructured(uint32_t structuredMemorySize = 8192);
    
    // Table management
    uint16_t createGameTable(const char* tableName, const WBDFColumn* columns, uint8_t columnCount, uint16_t maxRows);
    bool dropGameTable(const char* tableName);
    uint16_t getGameTableId(const char* tableName);
    
    // Common game table operations
    bool addItem(uint16_t itemId, const char* name, uint8_t category, uint8_t rarity, uint32_t value);
    bool getItem(uint16_t itemId, GameTables::Item* item);
    bool findItemsByCategory(uint8_t category, uint16_t* itemIds, uint16_t* count, uint16_t maxResults);
    
    bool addQuest(uint16_t questId, const char* title, uint8_t status, uint16_t prerequisite);
    bool updateQuestStatus(uint16_t questId, uint8_t status, uint8_t progress);
    bool getQuest(uint16_t questId, GameTables::Quest* quest);
    bool findQuestsByStatus(uint8_t status, uint16_t* questIds, uint16_t* count, uint16_t maxResults);
    
    bool addNPC(uint16_t npcId, const char* name, uint8_t level, uint8_t faction, uint16_t x, uint16_t y);
    bool getNPC(uint16_t npcId, GameTables::NPC* npc);
    bool findNPCsByFaction(uint8_t faction, uint16_t* npcIds, uint16_t* count, uint16_t maxResults);
    
    // Query interface
    bool executeQuery(const char* tableName, const char* whereColumn, const void* whereValue, uint16_t* resultIds, uint16_t* count, uint16_t maxResults);
    
    // Debug and stats
    void printStructuredStats();
    void printTableData(const char* tableName);
    bool validateStructuredDB();
    
    // Access to underlying WBDF database
    WBDFDatabase* getStructuredDB() { return wbdfInitialized ? &structuredDB : nullptr; }
};

// Factory functions for common table setups
namespace GameTableFactory {
    // Create standard RPG game tables
    bool createRPGTables(WispDatabaseExtended* db);
    
    // Create inventory management tables
    bool createInventoryTables(WispDatabaseExtended* db);
    
    // Create world/map data tables
    bool createWorldTables(WispDatabaseExtended* db);
    
    // Example table definitions
    struct RPGItemData {
        uint16_t id;
        char name[32];
        uint8_t category;    // Weapon=1, Armor=2, Consumable=3, etc.
        uint8_t rarity;      // Common=1, Rare=2, Epic=3, Legendary=4
        uint32_t value;
        uint16_t stackSize;
        uint8_t flags;       // Tradeable=1, Quest=2, etc.
        uint8_t reserved;
    } __attribute__((packed));
    
    struct RPGQuestData {
        uint16_t id;
        char title[48];
        uint8_t status;      // NotStarted=0, Active=1, Complete=2, Failed=3
        uint8_t progress;    // 0-100 percentage
        uint32_t flags;
        uint16_t prerequisite;
        uint16_t reward_item;
        uint32_t reward_exp;
    } __attribute__((packed));
    
    struct NPCData {
        uint16_t id;
        char name[24];
        uint8_t level;
        uint8_t faction;     // Friendly=1, Neutral=2, Hostile=3
        uint16_t location_x;
        uint16_t location_y;
        uint32_t flags;
        uint16_t dialogue_id;
    } __attribute__((packed));
}

// Helper macros for easy table operations
#define CREATE_ITEM_TABLE(db, maxItems) \
    (db)->createGameTable("items", GameTables::ITEM_COLUMNS, GameTables::ITEM_COLUMN_COUNT, maxItems)

#define CREATE_QUEST_TABLE(db, maxQuests) \
    (db)->createGameTable("quests", GameTables::QUEST_COLUMNS, GameTables::QUEST_COLUMN_COUNT, maxQuests)

#define CREATE_NPC_TABLE(db, maxNPCs) \
    (db)->createGameTable("npcs", GameTables::NPC_COLUMNS, GameTables::NPC_COLUMN_COUNT, maxNPCs)

// Query builder for common patterns
class GameQueryBuilder {
private:
    WispDatabaseExtended* db;
    
public:
    GameQueryBuilder(WispDatabaseExtended* database) : db(database) {}
    
    // Find all items of a specific category
    bool findItemsByCategory(uint8_t category, uint16_t* results, uint16_t* count, uint16_t maxResults) {
        return db->findItemsByCategory(category, results, count, maxResults);
    }
    
    // Find all active quests
    bool findActiveQuests(uint16_t* results, uint16_t* count, uint16_t maxResults) {
        uint8_t activeStatus = 1;
        return db->findQuestsByStatus(activeStatus, results, count, maxResults);
    }
    
    // Find all NPCs in a faction
    bool findNPCsByFaction(uint8_t faction, uint16_t* results, uint16_t* count, uint16_t maxResults) {
        return db->findNPCsByFaction(faction, results, count, maxResults);
    }
};
