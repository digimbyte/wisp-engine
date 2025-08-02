# Wisp Database System Documentation

## Overview

The Wisp Database System provides persistent storage for RPG-style game data using the ESP32-C6's 16KB LP-SRAM (Low Power SRAM). This memory persists through power cycles and deep sleep, making it perfect for save game data.

## Key Features

- **Persistent Storage**: Uses ESP32-C6 LP-SRAM that survives power cycles
- **Compact Design**: 8 bytes per entry, supports up to 512 items
- **RPG-Focused**: Built-in support for items, quests, game states, and inventory
- **Memory Efficient**: ~4KB for typical RPG game state
- **Checksum Protection**: Data integrity verification
- **Convenience Macros**: Easy-to-use helper functions

## Memory Layout

```
LP-SRAM (16KB total):
├── WispDBHeader (16 bytes)
│   ├── Magic Number (4 bytes): 'WDBS'
│   ├── Version (2 bytes)
│   ├── Entry Count (2 bytes)
│   ├── Checksum (4 bytes)
│   └── Last Update (4 bytes)
└── WispDBEntry Array (8 bytes each, up to 2000 entries)
    ├── ID (2 bytes)
    ├── Type (1 byte)
    ├── Flags (1 byte)
    └── Data (4 bytes)
```

**Typical Usage**: 512 entries = 4112 bytes (~25% of LP-SRAM)

## Entry Types

| Type | Purpose | ID Range | Usage |
|------|---------|----------|-------|
| `DB_TYPE_ITEM` | Item definitions | 1-999 | Weapons, armor, consumables |
| `DB_TYPE_QUEST` | Quest tracking | 1000-1999 | Story progression, side quests |
| `DB_TYPE_STATE` | Game variables | 2000-2999 | Counters, flags, positions |
| `DB_TYPE_INVENTORY` | Player items | Same as items | Owned items with quantities |
| `DB_TYPE_CONFIG` | Game settings | 3000+ | User preferences |

## Core API

### Initialization
```cpp
#include "engine/wisp_database_system.h"

// Global database instance (automatically created)
extern WispDatabaseSystem wispDB;

// Initialize in your app
bool init() {
    return wispDB.initialize();
}
```

### Item Management
```cpp
// Define items
WispItem sword = {1, 1, 3, 150, 0x01}; // ID, type, rarity, value, properties

// Add item definition
wispDB.addItem(sword);

// Check if item exists
if (wispDB.hasItem(1)) {
    WispItem item = wispDB.getItem(1);
    Serial.printf("Found: %s\n", item.itemType == 1 ? "Weapon" : "Other");
}
```

### Quest System
```cpp
// Create quest
WispQuest quest = {100, 1, 50, 0x00000003}; // ID, active, 50% progress, stage flags

// Add and manage quests
wispDB.addQuest(quest);
wispDB.completeQuest(100);

// Check quest status
if (wispDB.isQuestActive(100)) {
    Serial.println("Quest in progress");
}
```

### Game State Variables
```cpp
// Use convenient macros
WISP_DB_SET_COUNTER(1, 42);        // Player level = 42
WISP_DB_INCREMENT_COUNTER(1, 1);   // Level up
WISP_DB_SET_FLAG(10, true);        // Boss defeated = true
WISP_DB_TOGGLE_FLAG(11);           // Toggle secret found

// Read values
uint32_t level = wispDB.getState(1);
bool bossDefeated = wispDB.getFlag(10);
```

### Inventory Management
```cpp
// Add items to inventory
WISP_DB_ADD_ITEM(1, 5);        // Add 5 swords
WISP_DB_ADD_ITEM(2, 10);       // Add 10 potions

// Check inventory
if (WISP_DB_HAS_ITEM(2, 3)) {  // Has at least 3 potions?
    WISP_DB_USE_ITEM(2, 3);    // Use 3 potions
}

// Get inventory count
uint8_t potions = wispDB.getInventoryCount(2);
```

### Position System
```cpp
// Set player position
WISP_DB_SET_POSITION(mapId, xId, yId, 5, 100, 200);

// Read position
uint16_t map = wispDB.getState(mapId);
uint16_t x = wispDB.getState(xId);
uint16_t y = wispDB.getState(yId);
```

## Convenience Macros

The database provides helpful macros for common operations:

```cpp
// Counters (for levels, scores, etc.)
WISP_DB_SET_COUNTER(id, value)
WISP_DB_INCREMENT_COUNTER(id, amount)  
WISP_DB_DECREMENT_COUNTER(id, amount)

// Flags (for boolean states)
WISP_DB_SET_FLAG(id, value)
WISP_DB_TOGGLE_FLAG(id)

// Inventory operations
WISP_DB_ADD_ITEM(itemId, quantity)
WISP_DB_USE_ITEM(itemId, quantity)
WISP_DB_HAS_ITEM(itemId, quantity)

// Position helpers
WISP_DB_SET_POSITION(mapId, xId, yId, map, x, y)
```

## Example Usage Patterns

### Pokemon-style RPG
```cpp
class PokemonGame : public WispApp {
    enum Items { POKEBALL = 1, POTION = 2, BADGE_1 = 100 };
    enum States { LEVEL = 1, BADGES = 2, POKEMON_COUNT = 3 };
    enum Quests { GYM_1 = 1000, ELITE_FOUR = 1001 };
    
public:
    void catchPokemon() {
        if (WISP_DB_HAS_ITEM(POKEBALL, 1)) {
            WISP_DB_USE_ITEM(POKEBALL, 1);
            WISP_DB_INCREMENT_COUNTER(POKEMON_COUNT, 1);
            Serial.println("Pokemon caught!");
        }
    }
    
    void defeatGym() {
        WISP_DB_INCREMENT_COUNTER(BADGES, 1);
        WISP_DB_ADD_ITEM(BADGE_1, 1);
        wispDB.completeQuest(GYM_1);
    }
};
```

### Classic RPG
```cpp
class FantasyRPG : public WispApp {
    enum Items { SWORD = 1, SHIELD = 2, HEALTH_POTION = 10 };
    enum States { PLAYER_HP = 1, PLAYER_LEVEL = 2, GOLD = 3 };
    enum Flags { DRAGON_DEFEATED = 100, CAVE_EXPLORED = 101 };
    
public:
    void usePotionIfNeeded() {
        uint32_t hp = wispDB.getState(PLAYER_HP);
        if (hp < 50 && WISP_DB_HAS_ITEM(HEALTH_POTION, 1)) {
            WISP_DB_USE_ITEM(HEALTH_POTION, 1);
            WISP_DB_INCREMENT_COUNTER(PLAYER_HP, 30);
            Serial.println("Used health potion!");
        }
    }
    
    void defeatDragon() {
        WISP_DB_SET_FLAG(DRAGON_DEFEATED, true);
        WISP_DB_INCREMENT_COUNTER(GOLD, 1000);
        WISP_DB_INCREMENT_COUNTER(PLAYER_LEVEL, 5);
    }
};
```

## Memory Management

### Capacity Planning
- **Items**: ~50-100 unique items typical
- **Quests**: ~20-50 quests per game
- **States**: ~100-200 variables typical  
- **Inventory**: ~50-100 owned items
- **Total**: ~300-500 entries = 2.4-4KB

### Memory Optimization Tips
1. **Reuse IDs**: Remove completed quests to free entries
2. **Pack Data**: Use the 4-byte data field efficiently
3. **Monitor Usage**: Check `wispDB.getMemoryUsed()`
4. **Batch Updates**: Minimize save operations

### Memory Monitoring
```cpp
void checkMemory() {
    Serial.printf("Entries: %d/%d (%.1f%% full)\n",
                 wispDB.getEntryCount(), WISP_DB_MAX_ITEMS,
                 (float)wispDB.getEntryCount() / WISP_DB_MAX_ITEMS * 100.0f);
    
    Serial.printf("Memory: %d/%d bytes (%.1f%% used)\n",
                 wispDB.getMemoryUsed(), WISP_DB_LP_SRAM_SIZE,
                 (float)wispDB.getMemoryUsed() / WISP_DB_LP_SRAM_SIZE * 100.0f);
}
```

## Debugging and Diagnostics

### Debug Functions
```cpp
// Print comprehensive stats
wispDB.printDatabaseStats();

// Print inventory contents
wispDB.printInventory();

// Print active quests
wispDB.printActiveQuests();

// Validate database integrity
if (!wispDB.validate()) {
    Serial.println("Database corruption detected!");
}
```

### Common Issues

1. **Database Full**: Check entry count vs WISP_DB_MAX_ITEMS
2. **Checksum Failure**: Possible memory corruption
3. **Missing Entries**: Verify correct IDs and types
4. **Performance**: Minimize frequent saves

## Integration with Wisp Apps

### App Integration Pattern
```cpp
class MyGame : public WispApp {
public:
    bool init() override {
        // Always initialize database first
        if (!wispDB.initialize()) {
            return false;
        }
        
        // Setup new game if needed
        if (!wispDB.hasState(PLAYER_LEVEL)) {
            setupNewGame();
        }
        
        return true;
    }
    
    void cleanup() override {
        // Save on exit
        wispDB.save();
    }
    
private:
    void setupNewGame() {
        WISP_DB_SET_COUNTER(PLAYER_LEVEL, 1);
        WISP_DB_ADD_ITEM(STARTER_SWORD, 1);
        // ... more initialization
    }
};
```

## Best Practices

1. **Initialize Early**: Call `wispDB.initialize()` in app `init()`
2. **Save Periodically**: Use auto-save timers for important progress
3. **Validate Data**: Check return values and use `wispDB.validate()`
4. **Plan IDs**: Use consistent ID ranges for different data types
5. **Test Persistence**: Verify data survives power cycles
6. **Monitor Memory**: Track usage to avoid running out of space

## Hardware Requirements

- **ESP32-C6**: 16KB LP-SRAM (primary target)
- **ESP32-S3**: No LP-SRAM (consider NVRAM alternatives)
- **Memory**: Minimum 4KB recommended for typical RPGs

## Limitations

- **LP-SRAM Only**: Currently ESP32-C6 specific
- **Fixed Entry Size**: 8 bytes per entry regardless of data size
- **No Transactions**: Changes are immediate
- **Limited Capacity**: ~500 entries maximum for typical games

## Future Enhancements

- ESP32-S3 NVRAM support
- Compression for large datasets
- Transaction support
- Index optimization for faster lookups
- Cloud save integration
