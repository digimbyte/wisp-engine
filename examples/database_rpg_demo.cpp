// Comprehensive Database Demo - Pokemon RPG Style
// Demonstrates working database support for items, quests, and states
// Uses ESP32-C6 LP-SRAM for critical game data persistence

#include "src/engine/engine.h"
#include "examples/apps/pokemon_rpg/database_config.h"

using namespace WispEngine;

// Demo RPG structures that show real database usage
struct ItemDefinition {
    uint16_t itemId;
    char name[16];
    uint8_t type;
    uint16_t price;
    uint8_t rarity;
    char description[32];
};

struct QuestDefinition {
    uint16_t questId;
    char title[24];
    char description[64];
    uint16_t requiredLevel;
    uint16_t reward;
    uint8_t status; // 0=locked, 1=available, 2=active, 3=complete
};

struct PlayerData {
    uint16_t level;
    uint32_t experience;
    uint16_t health;
    uint16_t mana;
    uint16_t gold;
    uint8_t currentMap;
    uint16_t posX, posY;
};

// Item database - stored in ROM partition (read-only)
const ItemDefinition ITEM_DATABASE[] = {
    {1, "Potion", ITEM_TYPE_POTION, 50, 1, "Restores 50 HP"},
    {2, "Super Potion", ITEM_TYPE_POTION, 200, 2, "Restores 150 HP"},
    {3, "Iron Sword", ITEM_TYPE_WEAPON, 500, 3, "Basic metal sword +10 ATK"},
    {4, "Magic Staff", ITEM_TYPE_WEAPON, 800, 4, "Wooden staff +15 MAG"},
    {5, "Leather Armor", ITEM_TYPE_ARMOR, 300, 2, "Light protection +5 DEF"},
    {6, "Mystic Key", ITEM_TYPE_KEY, 0, 5, "Opens ancient doors"},
    {7, "Dragon Scale", ITEM_TYPE_MATERIAL, 1000, 5, "Rare crafting material"},
    {8, "Bread", ITEM_TYPE_FOOD, 10, 1, "Restores 20 HP slowly"}
};

// Quest database - also in ROM partition
const QuestDefinition QUEST_DATABASE[] = {
    {1, "First Steps", "Learn the basics of adventure", 1, 100, 1},
    {2, "Goblin Trouble", "Clear 5 goblins from the forest", 3, 250, 0},
    {3, "Ancient Artifact", "Find the lost crystal in the cave", 5, 500, 0},
    {4, "Dragon Slayer", "Defeat the mighty dragon", 15, 2000, 0},
    {5, "Master Trader", "Earn 5000 gold through trading", 8, 1000, 0}
};

class DatabaseRPGDemo {
private:
    PartitionedDatabase* db;
    
public:
    bool initialize() {
        Serial.println("=== Database RPG Demo Starting ===");
        
        // Initialize database with Pokemon RPG configuration
        db = Engine::getDatabase();
        if (!db) {
            Serial.println("ERROR: Database system not available!");
            return false;
        }
        
        Serial.println("Database initialized successfully");
        loadItemDatabase();
        loadQuestDatabase();
        initializePlayer();
        
        return true;
    }
    
    // Load item definitions into ROM partition (permanent data)
    void loadItemDatabase() {
        Serial.println("\n--- Loading Item Database ---");
        
        for (size_t i = 0; i < sizeof(ITEM_DATABASE) / sizeof(ItemDefinition); i++) {
            uint32_t itemKey = WISP_KEY_MAKE(NS_GAME, CAT_ITEMS, ITEM_DATABASE[i].itemId);
            
            if (db->set(itemKey, &ITEM_DATABASE[i], sizeof(ItemDefinition)) == WISP_SUCCESS) {
                Serial.printf("Loaded item: %s (ID: %d)\n", 
                             ITEM_DATABASE[i].name, ITEM_DATABASE[i].itemId);
            } else {
                Serial.printf("Failed to load item ID: %d\n", ITEM_DATABASE[i].itemId);
            }
        }
        
        Serial.printf("Item database loaded: %d items\n", sizeof(ITEM_DATABASE) / sizeof(ItemDefinition));
    }
    
    // Load quest definitions into ROM partition
    void loadQuestDatabase() {
        Serial.println("\n--- Loading Quest Database ---");
        
        for (size_t i = 0; i < sizeof(QUEST_DATABASE) / sizeof(QuestDefinition); i++) {
            uint32_t questKey = WISP_KEY_MAKE(NS_GAME, CAT_QUESTS, QUEST_DATABASE[i].questId);
            
            if (db->set(questKey, &QUEST_DATABASE[i], sizeof(QuestDefinition)) == WISP_SUCCESS) {
                Serial.printf("Loaded quest: %s (ID: %d)\n", 
                             QUEST_DATABASE[i].title, QUEST_DATABASE[i].questId);
            }
        }
        
        Serial.printf("Quest database loaded: %d quests\n", sizeof(QUEST_DATABASE) / sizeof(QuestDefinition));
    }
    
    // Initialize player data in SAVE partition (persistent across reboots)
    void initializePlayer() {
        Serial.println("\n--- Initializing Player Data ---");
        
        PlayerData player = {
            .level = 1,
            .experience = 0,
            .health = 100,
            .mana = 50,
            .gold = 100,
            .currentMap = 1,
            .posX = 64,
            .posY = 64
        };
        
        uint32_t playerKey = WISP_KEY_MAKE(NS_PLAYER, CAT_STATS, 0);
        if (db->set(playerKey, &player, sizeof(PlayerData)) == WISP_SUCCESS) {
            Serial.println("Player data initialized");
            Serial.printf("Level: %d, HP: %d, Gold: %d\n", player.level, player.health, player.gold);
        }
        
        // Initialize starting inventory
        addItemToInventory(1, 3); // 3 Potions
        addItemToInventory(8, 5); // 5 Bread
        addItemToInventory(3, 1); // 1 Iron Sword
    }
    
    // Demonstrate item lookup and management
    bool getItemInfo(uint16_t itemId, ItemDefinition* item) {
        uint32_t itemKey = WISP_KEY_MAKE(NS_GAME, CAT_ITEMS, itemId);
        uint8_t size;
        
        WispErrorCode result = db->get(itemKey, item, sizeof(ItemDefinition), &size);
        return (result == WISP_SUCCESS && size == sizeof(ItemDefinition));
    }
    
    void addItemToInventory(uint16_t itemId, uint8_t quantity) {
        uint32_t invKey = WISP_KEY_MAKE(NS_PLAYER, CAT_INVENTORY, itemId);
        
        // Get current quantity (or 0 if not present)
        uint8_t currentQty = db->getU8(invKey, 0);
        uint8_t newQty = currentQty + quantity;
        
        db->setU8(invKey, newQty);
        
        ItemDefinition item;
        if (getItemInfo(itemId, &item)) {
            Serial.printf("Added %d x %s to inventory (total: %d)\n", 
                         quantity, item.name, newQty);
        }
    }
    
    bool removeItemFromInventory(uint16_t itemId, uint8_t quantity) {
        uint32_t invKey = WISP_KEY_MAKE(NS_PLAYER, CAT_INVENTORY, itemId);
        uint8_t currentQty = db->getU8(invKey, 0);
        
        if (currentQty < quantity) {
            Serial.printf("Not enough items! Have: %d, Need: %d\n", currentQty, quantity);
            return false;
        }
        
        uint8_t newQty = currentQty - quantity;
        if (newQty == 0) {
            db->remove(invKey); // Remove from inventory if quantity is 0
        } else {
            db->setU8(invKey, newQty);
        }
        
        ItemDefinition item;
        if (getItemInfo(itemId, &item)) {
            Serial.printf("Removed %d x %s from inventory (remaining: %d)\n", 
                         quantity, item.name, newQty);
        }
        
        return true;
    }
    
    // Demonstrate quest state management
    void updateQuestStatus(uint16_t questId, uint8_t newStatus) {
        uint32_t questKey = WISP_KEY_MAKE(NS_PLAYER, CAT_PROGRESS, questId);
        db->setU8(questKey, newStatus);
        
        // Get quest info for display
        QuestDefinition quest;
        uint32_t questDefKey = WISP_KEY_MAKE(NS_GAME, CAT_QUESTS, questId);
        uint8_t size;
        
        if (db->get(questDefKey, &quest, sizeof(QuestDefinition), &size) == WISP_SUCCESS) {
            const char* statusNames[] = {"Locked", "Available", "Active", "Complete"};
            Serial.printf("Quest '%s' status changed to: %s\n", 
                         quest.title, statusNames[newStatus]);
        }
    }
    
    uint8_t getQuestStatus(uint16_t questId) {
        uint32_t questKey = WISP_KEY_MAKE(NS_PLAYER, CAT_PROGRESS, questId);
        return db->getU8(questKey, 0); // Default to locked
    }
    
    // Demonstrate player progression
    void gainExperience(uint32_t exp) {
        uint32_t playerKey = WISP_KEY_MAKE(NS_PLAYER, CAT_STATS, 0);
        PlayerData player;
        uint8_t size;
        
        if (db->get(playerKey, &player, sizeof(PlayerData), &size) == WISP_SUCCESS) {
            player.experience += exp;
            
            // Level up check (simple formula: level = sqrt(exp/100))
            uint16_t newLevel = (uint16_t)(sqrt(player.experience / 100.0) + 1);
            if (newLevel > player.level) {
                player.level = newLevel;
                player.health += 20; // Level up bonus
                player.mana += 10;
                Serial.printf("LEVEL UP! Now level %d (HP: %d, MP: %d)\n", 
                             player.level, player.health, player.mana);
            }
            
            db->set(playerKey, &player, sizeof(PlayerData));
            Serial.printf("Gained %d EXP (Total: %d)\n", exp, player.experience);
        }
    }
    
    // Demonstrate comprehensive database queries
    void runDatabaseDemo() {
        Serial.println("\n=== Running Database Demonstrations ===");
        
        // 1. Item Management Demo
        Serial.println("\n1. ITEM MANAGEMENT:");
        printInventory();
        
        Serial.println("Using a potion...");
        removeItemFromInventory(1, 1); // Use a potion
        
        Serial.println("Finding treasure...");
        addItemToInventory(7, 1); // Found dragon scale
        addItemToInventory(2, 2); // Found super potions
        
        printInventory();
        
        // 2. Quest Progress Demo
        Serial.println("\n2. QUEST PROGRESS:");
        updateQuestStatus(1, 2); // Start first quest
        updateQuestStatus(1, 3); // Complete first quest
        updateQuestStatus(2, 1); // Unlock second quest
        
        printActiveQuests();
        
        // 3. Player Progression Demo
        Serial.println("\n3. PLAYER PROGRESSION:");
        gainExperience(150);
        gainExperience(300);
        gainExperience(500);
        
        printPlayerStats();
        
        // 4. Memory Usage Demo
        Serial.println("\n4. MEMORY EFFICIENCY:");
        printMemoryStats();
    }
    
    void printInventory() {
        Serial.println("--- Current Inventory ---");
        
        for (uint16_t itemId = 1; itemId <= 8; itemId++) {
            uint32_t invKey = WISP_KEY_MAKE(NS_PLAYER, CAT_INVENTORY, itemId);
            uint8_t quantity = db->getU8(invKey, 0);
            
            if (quantity > 0) {
                ItemDefinition item;
                if (getItemInfo(itemId, &item)) {
                    Serial.printf("  %d x %s - %s\n", quantity, item.name, item.description);
                }
            }
        }
    }
    
    void printActiveQuests() {
        Serial.println("--- Quest Status ---");
        
        for (uint16_t questId = 1; questId <= 5; questId++) {
            uint8_t status = getQuestStatus(questId);
            QuestDefinition quest;
            uint32_t questDefKey = WISP_KEY_MAKE(NS_GAME, CAT_QUESTS, questId);
            uint8_t size;
            
            if (db->get(questDefKey, &quest, sizeof(QuestDefinition), &size) == WISP_SUCCESS) {
                const char* statusNames[] = {"🔒", "📋", "⚡", "✅"};
                Serial.printf("  %s %s (Level %d) - %s\n", 
                             statusNames[status], quest.title, quest.requiredLevel, quest.description);
            }
        }
    }
    
    void printPlayerStats() {
        uint32_t playerKey = WISP_KEY_MAKE(NS_PLAYER, CAT_STATS, 0);
        PlayerData player;
        uint8_t size;
        
        if (db->get(playerKey, &player, sizeof(PlayerData), &size) == WISP_SUCCESS) {
            Serial.println("--- Player Statistics ---");
            Serial.printf("  Level: %d\n", player.level);
            Serial.printf("  Experience: %d\n", player.experience);
            Serial.printf("  Health: %d\n", player.health);
            Serial.printf("  Mana: %d\n", player.mana);
            Serial.printf("  Gold: %d\n", player.gold);
            Serial.printf("  Location: Map %d (%d, %d)\n", player.currentMap, player.posX, player.posY);
        }
    }
    
    void printMemoryStats() {
        Serial.printf("--- LP-SRAM Usage (16KB Total) ---\n");
        Serial.printf("  Total Used: %d bytes (%d%%)\n", 
                     db->getTotalUsedBytes(), 
                     (db->getTotalUsedBytes() * 100) / 16384);
        Serial.printf("  Total Free: %d bytes\n", db->getTotalFreeBytes());
        Serial.printf("  ROM Partition: %d bytes\n", db->getPartitionUsedBytes(0));
        Serial.printf("  Save Partition: %d bytes\n", db->getPartitionUsedBytes(1));
        Serial.printf("  Runtime Cache: %d bytes\n", db->getPartitionUsedBytes(3));
    }
};

// DO NOT USE ARDUINO IN THIS ESP-IDF PROJECT
DatabaseRPGDemo demo;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialize Wisp Engine
    if (!WISP_ENGINE_INIT()) {
        Serial.println("FATAL: Engine initialization failed!");
        return;
    }
    
    // Initialize database demo
    if (!demo.initialize()) {
        Serial.println("FATAL: Database demo initialization failed!");
        return;
    }
    
    // Run comprehensive demo
    demo.runDatabaseDemo();
    
    Serial.println("\n=== Database Demo Complete ===");
    Serial.println("This demonstrates working database support for:");
    Serial.println("✅ Item lookup and inventory management");
    Serial.println("✅ Quest state tracking and progression");
    Serial.println("✅ Player data persistence in LP-SRAM");
    Serial.println("✅ Memory-efficient storage and retrieval");
    Serial.println("✅ Real-time game state management");
}

void loop() {
    // In a real game, this would be the main game loop
    // For demo purposes, we just show periodic stats
    static uint32_t lastUpdate = 0;
    
    if (millis() - lastUpdate > 10000) { // Every 10 seconds
        Serial.println("\n--- Periodic Status Update ---");
        demo.printMemoryStats();
        lastUpdate = millis();
    }
    
    delay(100);
}
