// Wisp Database System Test Program
#include "../src/engine/wisp_database_system.h"
#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== Wisp Database System Test ===");
    
    // Test 1: Initialize database
    Serial.println("\n--- Test 1: Database Initialization ---");
    if (wispDB.initialize()) {
        Serial.println("✓ Database initialized successfully");
    } else {
        Serial.println("✗ Database initialization failed");
        return;
    }
    
    // Test 2: Reset database
    Serial.println("\n--- Test 2: Database Reset ---");
    if (wispDB.reset()) {
        Serial.println("✓ Database reset successfully");
        wispDB.printDatabaseStats();
    }
    
    // Test 3: Add items
    Serial.println("\n--- Test 3: Item Management ---");
    
    WispItem sword = {1, 1, 3, 150, 0x01}; // Epic sword
    WispItem potion = {2, 2, 1, 25, 0x02}; // Common potion
    WispItem shield = {3, 3, 2, 75, 0x04}; // Rare shield
    
    Serial.printf("Adding sword (ID:%d): %s\n", sword.itemId, 
                 wispDB.addItem(sword) ? "✓" : "✗");
    Serial.printf("Adding potion (ID:%d): %s\n", potion.itemId,
                 wispDB.addItem(potion) ? "✓" : "✗");
    Serial.printf("Adding shield (ID:%d): %s\n", shield.itemId,
                 wispDB.addItem(shield) ? "✓" : "✗");
    
    // Test item retrieval
    WispItem retrievedSword = wispDB.getItem(1);
    Serial.printf("Retrieved sword - Type:%d, Rarity:%d, Value:%d\n",
                 retrievedSword.itemType, retrievedSword.rarity, retrievedSword.value);
    
    // Test 4: Quest management
    Serial.println("\n--- Test 4: Quest Management ---");
    
    WispQuest tutorialQuest = {100, 1, 25, 0x00000003}; // Active, 25% complete
    WispQuest dragonQuest = {101, 0, 0, 0x00000000};   // Not started
    
    Serial.printf("Adding tutorial quest: %s\n", 
                 wispDB.addQuest(tutorialQuest) ? "✓" : "✗");
    Serial.printf("Adding dragon quest: %s\n",
                 wispDB.addQuest(dragonQuest) ? "✓" : "✗");
    
    // Progress and complete quest
    Serial.printf("Tutorial quest active: %s\n",
                 wispDB.isQuestActive(100) ? "✓" : "✗");
    Serial.printf("Completing tutorial quest: %s\n",
                 wispDB.completeQuest(100) ? "✓" : "✗");
    Serial.printf("Tutorial quest completed: %s\n",
                 wispDB.isQuestCompleted(100) ? "✓" : "✗");
    
    // Test 5: Game state management
    Serial.println("\n--- Test 5: Game State Management ---");
    
    // Set various game states
    WISP_DB_SET_COUNTER(1, 42);        // Player level
    WISP_DB_SET_COUNTER(2, 1337);      // Player score
    WISP_DB_SET_FLAG(10, true);        // Boss defeated
    WISP_DB_SET_FLAG(11, false);       // Secret found
    
    Serial.printf("Player level: %d (expected: 42)\n", wispDB.getState(1));
    Serial.printf("Player score: %d (expected: 1337)\n", wispDB.getState(2));
    Serial.printf("Boss defeated: %s (expected: true)\n", 
                 wispDB.getFlag(10) ? "true" : "false");
    Serial.printf("Secret found: %s (expected: false)\n",
                 wispDB.getFlag(11) ? "true" : "false");
    
    // Test counter operations
    WISP_DB_INCREMENT_COUNTER(1, 3);   // Level up by 3
    Serial.printf("After level up: %d (expected: 45)\n", wispDB.getState(1));
    
    WISP_DB_DECREMENT_COUNTER(2, 37);  // Lose some score
    Serial.printf("After score loss: %d (expected: 1300)\n", wispDB.getState(2));
    
    // Test flag toggle
    WISP_DB_TOGGLE_FLAG(11);           // Find secret
    Serial.printf("After toggle: %s (expected: true)\n",
                 wispDB.getFlag(11) ? "true" : "false");
    
    // Test 6: Inventory management
    Serial.println("\n--- Test 6: Inventory Management ---");
    
    // Add items to inventory
    WISP_DB_ADD_ITEM(1, 1);    // Add 1 sword
    WISP_DB_ADD_ITEM(2, 5);    // Add 5 potions
    WISP_DB_ADD_ITEM(2, 3);    // Add 3 more potions (should stack)
    WISP_DB_ADD_ITEM(3, 1);    // Add 1 shield
    
    Serial.printf("Sword count: %d (expected: 1)\n", wispDB.getInventoryCount(1));
    Serial.printf("Potion count: %d (expected: 8)\n", wispDB.getInventoryCount(2));
    Serial.printf("Shield count: %d (expected: 1)\n", wispDB.getInventoryCount(3));
    
    // Test item usage
    Serial.printf("Has 3 potions: %s\n", 
                 WISP_DB_HAS_ITEM(2, 3) ? "✓" : "✗");
    Serial.printf("Has 10 potions: %s\n",
                 WISP_DB_HAS_ITEM(2, 10) ? "✓" : "✗");
    
    // Use some potions
    Serial.println("Using 3 potions...");
    WISP_DB_USE_ITEM(2, 3);
    Serial.printf("Potions after use: %d (expected: 5)\n", wispDB.getInventoryCount(2));
    
    // Test 7: Position system
    Serial.println("\n--- Test 7: Position System ---");
    
    WISP_DB_SET_POSITION(20, 21, 22, 5, 100, 200); // Map 5, pos (100, 200)
    
    uint16_t map = wispDB.getState(20);
    uint16_t x = wispDB.getState(21);
    uint16_t y = wispDB.getState(22);
    
    Serial.printf("Position: Map %d at (%d, %d)\n", map, x, y);
    
    // Test 8: Memory and performance
    Serial.println("\n--- Test 8: Memory Analysis ---");
    wispDB.printDatabaseStats();
    
    Serial.printf("Memory efficiency: %.1f bytes per entry\n",
                 (float)wispDB.getMemoryUsed() / wispDB.getEntryCount());
    
    // Test 9: Database persistence simulation
    Serial.println("\n--- Test 9: Persistence Simulation ---");
    
    Serial.println("Saving database...");
    wispDB.save();
    
    Serial.printf("Database validation: %s\n", 
                 wispDB.validate() ? "✓ Valid" : "✗ Invalid");
    
    // Test 10: Print final state
    Serial.println("\n--- Test 10: Final Database State ---");
    wispDB.printInventory();
    wispDB.printActiveQuests();
    
    // Test 11: Stress test
    Serial.println("\n--- Test 11: Capacity Stress Test ---");
    
    uint16_t initialEntries = wispDB.getEntryCount();
    uint16_t addedEntries = 0;
    
    // Try to fill remaining capacity
    for (uint16_t i = 200; i < 400 && addedEntries < 50; i++) {
        if (wispDB.setState(i, i * 2, 0)) {
            addedEntries++;
        } else {
            break;
        }
    }
    
    Serial.printf("Added %d stress test entries\n", addedEntries);
    Serial.printf("Total entries: %d / %d (%.1f%% full)\n",
                 wispDB.getEntryCount(), WISP_DB_MAX_ITEMS,
                 (float)wispDB.getEntryCount() / WISP_DB_MAX_ITEMS * 100.0f);
    
    Serial.println("\n=== All Tests Completed ===");
    wispDB.printDatabaseStats();
}

void loop() {
    // Optional: Add runtime tests here
    delay(1000);
    
    static uint32_t lastMemCheck = 0;
    if (millis() - lastMemCheck > 30000) { // Every 30 seconds
        Serial.println("\n--- Runtime Memory Check ---");
        Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
        Serial.printf("Database memory: %d bytes\n", wispDB.getMemoryUsed());
        Serial.printf("Database entries: %d\n", wispDB.getEntryCount());
        lastMemCheck = millis();
    }
}
