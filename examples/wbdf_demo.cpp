// wbdf_demo.cpp - Demonstration of WBDF Structured Database System
#include "wbdf_integration.h"
#include <stdio.h>

// Demo function showing how to use the structured database
void demonstrateWBDFDatabase() {
    printf("=== WBDF Structured Database Demo ===\n\n");
    
    // Create extended database instance
    WispDatabaseExtended db;
    
    // Initialize with 12KB of structured memory
    WispErrorCode result = db.initializeStructured(12288);
    if (result != WISP_SUCCESS) {
        printf("Failed to initialize database: %d\n", result);
        return;
    }
    
    printf("✓ Database initialized successfully\n");
    
    // Create standard RPG tables
    if (!GameTableFactory::createRPGTables(&db)) {
        printf("Failed to create RPG tables\n");
        return;
    }
    
    printf("✓ Created Items, Quests, and NPCs tables\n\n");
    
    // === ITEMS DEMO ===
    printf("--- Items Table Demo ---\n");
    
    // Add some sample items
    db.addItem(1, "Iron Sword", 1, 1, 100);      // Weapon, Common
    db.addItem(2, "Leather Armor", 2, 1, 50);    // Armor, Common  
    db.addItem(3, "Health Potion", 3, 1, 25);    // Consumable, Common
    db.addItem(4, "Magic Staff", 1, 3, 500);     // Weapon, Epic
    db.addItem(5, "Dragon Scale", 2, 4, 1000);   // Armor, Legendary
    
    printf("Added 5 items to database\n");
    
    // Query items by category
    uint16_t weaponIds[10];
    uint16_t weaponCount;
    uint8_t weaponCategory = 1;
    
    if (db.findItemsByCategory(weaponCategory, weaponIds, &weaponCount, 10)) {
        printf("Found %d weapons:\n", weaponCount);
        for (uint16_t i = 0; i < weaponCount; i++) {
            GameTables::Item item;
            if (db.getItem(weaponIds[i], &item)) {
                printf("  - %s (ID:%d, Value:%d)\n", item.name, item.id, item.value);
            }
        }
    }
    
    // === QUESTS DEMO ===
    printf("\n--- Quests Table Demo ---\n");
    
    // Add sample quests
    db.addQuest(1, "Kill 10 Goblins", 1, 0);         // Active, no prerequisite
    db.addQuest(2, "Find the Lost Sword", 0, 1);     // Not started, requires quest 1
    db.addQuest(3, "Deliver Message", 2, 0);         // Complete
    db.addQuest(4, "Collect Herbs", 1, 0);           // Active
    
    printf("Added 4 quests to database\n");
    
    // Update quest progress
    db.updateQuestStatus(1, 1, 75);  // 75% complete
    db.updateQuestStatus(4, 2, 100); // Mark as complete
    
    // Find active quests
    uint16_t activeQuestIds[10];
    uint16_t activeQuestCount;
    uint8_t activeStatus = 1;
    
    if (db.findQuestsByStatus(activeStatus, activeQuestIds, &activeQuestCount, 10)) {
        printf("Found %d active quests:\n", activeQuestCount);
        for (uint16_t i = 0; i < activeQuestCount; i++) {
            GameTables::Quest quest;
            if (db.getQuest(activeQuestIds[i], &quest)) {
                printf("  - %s (Progress: %d%%)\n", quest.title, quest.progress);
            }
        }
    }
    
    // === NPCS DEMO ===
    printf("\n--- NPCs Table Demo ---\n");
    
    // Add sample NPCs
    db.addNPC(1, "Village Elder", 50, 1, 100, 200);    // Friendly faction
    db.addNPC(2, "Goblin Warrior", 15, 3, 300, 150);   // Hostile faction
    db.addNPC(3, "Merchant", 25, 1, 120, 180);         // Friendly faction
    db.addNPC(4, "Orc Chief", 35, 3, 450, 200);        // Hostile faction
    
    printf("Added 4 NPCs to database\n");
    
    // Find hostile NPCs
    uint16_t hostileNPCIds[10];
    uint16_t hostileNPCCount;
    uint8_t hostileFaction = 3;
    
    if (db.findNPCsByFaction(hostileFaction, hostileNPCIds, &hostileNPCCount, 10)) {
        printf("Found %d hostile NPCs:\n", hostileNPCCount);
        for (uint16_t i = 0; i < hostileNPCCount; i++) {
            GameTables::NPC npc;
            if (db.getNPC(hostileNPCIds[i], &npc)) {
                printf("  - %s (Level %d at %d,%d)\n", npc.name, npc.level, npc.location_x, npc.location_y);
            }
        }
    }
    
    // === ADVANCED QUERIES ===
    printf("\n--- Advanced Query Demo ---\n");
    
    // Generic query interface
    uint16_t results[10];
    uint16_t resultCount;
    uint8_t rareItems = 3;  // Epic rarity
    
    if (db.executeQuery("items", "rarity", &rareItems, results, &resultCount, 10)) {
        printf("Found %d epic items:\n", resultCount);
        for (uint16_t i = 0; i < resultCount; i++) {
            GameTables::Item item;
            if (db.getItem(results[i], &item)) {
                printf("  - %s\n", item.name);
            }
        }
    }
    
    // === PERFORMANCE AND STATS ===
    printf("\n--- Database Statistics ---\n");
    db.printStructuredStats();
    
    printf("\n--- Table Details ---\n");
    db.printTableData("items");
    printf("\n");
    db.printTableData("quests");
    
    // === VALIDATION ===
    printf("\n--- Validation ---\n");
    if (db.validateStructuredDB()) {
        printf("✓ Database validation passed\n");
    } else {
        printf("✗ Database validation failed\n");
    }
    
    // === QUERY BUILDER DEMO ===
    printf("\n--- Query Builder Demo ---\n");
    GameQueryBuilder queryBuilder(&db);
    
    uint16_t builderResults[10];
    uint16_t builderCount;
    
    // Find all weapons
    if (queryBuilder.findItemsByCategory(1, builderResults, &builderCount, 10)) {
        printf("Query builder found %d weapons\n", builderCount);
    }
    
    // Find active quests
    if (queryBuilder.findActiveQuests(builderResults, &builderCount, 10)) {
        printf("Query builder found %d active quests\n", builderCount);
    }
    
    printf("\n=== Demo Complete ===\n");
}

// Example of how to integrate with existing game systems
void gameSystemIntegration() {
    printf("\n=== Game System Integration Example ===\n");
    
    WispDatabaseExtended gameDB;
    gameDB.initializeStructured(8192);
    
    // Create custom tables for a specific game
    const WBDFColumn skillColumns[] = {
        WBDF_PRIMARY_KEY("skill_id", WBDF_TYPE_U16),
        WBDF_COLUMN("name", WBDF_TYPE_STRING, 24),
        WBDF_INDEXED_COLUMN("category", WBDF_TYPE_U8, 0),  // Combat=1, Magic=2, etc.
        WBDF_COLUMN("level", WBDF_TYPE_U8, 0),
        WBDF_COLUMN("experience", WBDF_TYPE_U32, 0),
        WBDF_COLUMN("max_level", WBDF_TYPE_U8, 0)
    };
    
    uint16_t skillTableId = gameDB.createGameTable("skills", skillColumns, 
                                                   sizeof(skillColumns)/sizeof(WBDFColumn), 64);
    
    if (skillTableId > 0) {
        printf("✓ Created custom skills table\n");
        
        // Add some skills
        struct Skill {
            uint16_t skill_id;
            char name[24];
            uint8_t category;
            uint8_t level;
            uint32_t experience;
            uint8_t max_level;
        } __attribute__((packed));
        
        Skill swordSkill = {1, "Sword Fighting", 1, 15, 2500, 100};
        Skill magicSkill = {2, "Fire Magic", 2, 8, 800, 50};
        
        WBDFDatabase* wbdf = gameDB.getStructuredDB();
        if (wbdf) {
            wbdf->insertRow(skillTableId, &swordSkill);
            wbdf->insertRow(skillTableId, &magicSkill);
            printf("✓ Added skills to custom table\n");
        }
        
        gameDB.printTableData("skills");
    }
}

// Function to demonstrate performance characteristics
void performanceDemo() {
    printf("\n=== Performance Characteristics ===\n");
    
    WispDatabaseExtended perfDB;
    perfDB.initializeStructured(16384);  // 16KB
    
    // Create a table for performance testing
    const WBDFColumn perfColumns[] = {
        WBDF_PRIMARY_KEY("id", WBDF_TYPE_U32),
        WBDF_INDEXED_COLUMN("category", WBDF_TYPE_U8, 0),
        WBDF_COLUMN("value", WBDF_TYPE_U32, 0),
        WBDF_COLUMN("data", WBDF_TYPE_BYTES, 16)
    };
    
    uint16_t perfTableId = perfDB.createGameTable("perf_test", perfColumns, 4, 200);
    
    if (perfTableId > 0) {
        printf("Performance table created, adding test data...\n");
        
        struct PerfData {
            uint32_t id;
            uint8_t category;
            uint32_t value;
            uint8_t data[16];
        } __attribute__((packed));
        
        // Add test data
        WBDFDatabase* wbdf = perfDB.getStructuredDB();
        for (uint32_t i = 1; i <= 50; i++) {
            PerfData entry;
            entry.id = i;
            entry.category = (i % 5) + 1;  // Categories 1-5
            entry.value = i * 100;
            memset(entry.data, i & 0xFF, sizeof(entry.data));
            
            wbdf->insertRow(perfTableId, &entry);
        }
        
        printf("Added 50 test records\n");
        
        // Test query performance
        uint8_t testCategory = 3;
        uint16_t results[50];
        uint16_t resultCount;
        
        if (perfDB.executeQuery("perf_test", "category", &testCategory, results, &resultCount, 50)) {
            printf("Query for category 3 found %d records\n", resultCount);
        }
        
        perfDB.printStructuredStats();
    }
}

// Main demo function that can be called from your application
extern "C" void runWBDFDemo() {
    demonstrateWBDFDatabase();
    gameSystemIntegration();
    performanceDemo();
}
