// Wisp Engine Database System Implementation
#include "legacy_system.h"
#include <esp_crc.h>
#include <iostream>
#include <algorithm>

// Static member definitions for LP-SRAM storage (persistent across power cycles)
RTC_DATA_ATTR bool WispDatabaseSystem::initialized = false;
RTC_DATA_ATTR WispDBHeader WispDatabaseSystem::header = {};
RTC_DATA_ATTR WispDBEntry WispDatabaseSystem::entries[WISP_DB_MAX_ITEMS] = {};

// System initialization
bool WispDatabaseSystem::init() {
    ESP_LOGI("WISPDB", "Initializing Wisp Database System...");
    
    // Check if database exists in LP-SRAM
    if (header.magic == 0x53424457) { // 'WDBS'
        Serial.println("Existing database found in LP-SRAM");
        
        if (!validateChecksum()) {
            Serial.println("WARNING: Database checksum failed - resetting");
            return reset();
        }
        
        // Count entries by type
        itemCount = questCount = stateCount = inventoryCount = 0;
        for (uint16_t i = 0; i < header.entryCount; i++) {
            switch (entries[i].type) {
                case DB_TYPE_ITEM: itemCount++; break;
                case DB_TYPE_QUEST: questCount++; break;
                case DB_TYPE_STATE: stateCount++; break;
                case DB_TYPE_INVENTORY: inventoryCount++; break;
            }
        }
        
        Serial.printf("Database loaded: %d entries (%d items, %d quests, %d states, %d inventory)\n",
                     header.entryCount, itemCount, questCount, stateCount, inventoryCount);
        return true;
        
    } else {
        Serial.println("No existing database - creating new one");
        return reset();
    }
}

bool WispDatabaseSystem::reset() {
    Serial.println("Resetting database to defaults...");
    
    // Clear header
    memset(&header, 0, sizeof(WispDBHeader));
    header.magic = 0x53424457; // 'WDBS'
    header.version = WISP_DB_VERSION;
    header.entryCount = 0;
    header.lastUpdate = millis();
    
    // Clear all entries
    memset(entries, 0, sizeof(entries));
    
    // Reset counters
    itemCount = questCount = stateCount = inventoryCount = 0;
    
    updateChecksum();
    Serial.println("Database reset complete");
    return true;
}

bool WispDatabaseSystem::save() {
    header.lastUpdate = millis();
    updateChecksum();
    
    // Data is automatically saved in LP-SRAM (RTC_DATA_ATTR)
    Serial.printf("Database saved: %d entries, %d bytes used\n", 
                 header.entryCount, getMemoryUsed());
    return true;
}

bool WispDatabaseSystem::validate() {
    return validateChecksum() && header.magic == 0x53424457;
}

bool WispDatabaseSystem::validateChecksum() {
    uint32_t calculatedCRC = esp_crc32_le(0, (uint8_t*)entries, 
                                         header.entryCount * sizeof(WispDBEntry));
    return calculatedCRC == header.checksum;
}

void WispDatabaseSystem::updateChecksum() {
    header.checksum = esp_crc32_le(0, (uint8_t*)entries, 
                                  header.entryCount * sizeof(WispDBEntry));
}

uint16_t WispDatabaseSystem::findEntryIndex(uint16_t id, WispDBType type) {
    for (uint16_t i = 0; i < header.entryCount; i++) {
        if (entries[i].id == id && entries[i].type == type) {
            return i;
        }
    }
    return 0xFFFF; // Not found
}

bool WispDatabaseSystem::isValidEntry(const WispDBEntry& entry) {
    return entry.type >= DB_TYPE_ITEM && entry.type <= DB_TYPE_CONFIG;
}

// Item management implementation
bool WispDatabaseSystem::addItem(const WispItem& item) {
    if (header.entryCount >= WISP_DB_MAX_ITEMS) return false;
    if (hasItem(item.itemId)) return false; // Already exists
    
    WispDBEntry entry = item.toDBEntry();
    entries[header.entryCount] = entry;
    header.entryCount++;
    itemCount++;
    
    save();
    return true;
}

bool WispDatabaseSystem::updateItem(uint16_t itemId, const WispItem& item) {
    uint16_t index = findEntryIndex(itemId, DB_TYPE_ITEM);
    if (index == 0xFFFF) return false;
    
    entries[index] = item.toDBEntry();
    save();
    return true;
}

bool WispDatabaseSystem::removeItem(uint16_t itemId) {
    uint16_t index = findEntryIndex(itemId, DB_TYPE_ITEM);
    if (index == 0xFFFF) return false;
    
    // Shift entries down to fill gap
    for (uint16_t i = index; i < header.entryCount - 1; i++) {
        entries[i] = entries[i + 1];
    }
    
    header.entryCount--;
    itemCount--;
    save();
    return true;
}

WispItem WispDatabaseSystem::getItem(uint16_t itemId) {
    uint16_t index = findEntryIndex(itemId, DB_TYPE_ITEM);
    if (index == 0xFFFF) {
        return {}; // Return empty item
    }
    return WispItem::fromDBEntry(entries[index]);
}

bool WispDatabaseSystem::hasItem(uint16_t itemId) {
    return findEntryIndex(itemId, DB_TYPE_ITEM) != 0xFFFF;
}

// Quest management implementation
bool WispDatabaseSystem::addQuest(const WispQuest& quest) {
    if (header.entryCount >= WISP_DB_MAX_ITEMS) return false;
    
    // Remove existing quest if it exists
    uint16_t existingIndex = findEntryIndex(quest.questId, DB_TYPE_QUEST);
    if (existingIndex != 0xFFFF) {
        entries[existingIndex] = quest.toDBEntry();
    } else {
        entries[header.entryCount] = quest.toDBEntry();
        header.entryCount++;
        questCount++;
    }
    
    save();
    return true;
}

bool WispDatabaseSystem::completeQuest(uint16_t questId) {
    uint16_t index = findEntryIndex(questId, DB_TYPE_QUEST);
    if (index == 0xFFFF) return false;
    
    WispQuest quest = WispQuest::fromDBEntry(entries[index]);
    quest.status = 2; // completed
    quest.progress = 100;
    entries[index] = quest.toDBEntry();
    
    save();
    return true;
}

WispQuest WispDatabaseSystem::getQuest(uint16_t questId) {
    uint16_t index = findEntryIndex(questId, DB_TYPE_QUEST);
    if (index == 0xFFFF) {
        return {}; // Return empty quest
    }
    return WispQuest::fromDBEntry(entries[index]);
}

bool WispDatabaseSystem::isQuestCompleted(uint16_t questId) {
    WispQuest quest = getQuest(questId);
    return quest.status == 2;
}

bool WispDatabaseSystem::isQuestActive(uint16_t questId) {
    WispQuest quest = getQuest(questId);
    return quest.status == 1;
}

// Game state management
bool WispDatabaseSystem::setState(uint16_t stateId, uint32_t value, uint8_t type) {
    uint16_t index = findEntryIndex(stateId, DB_TYPE_STATE);
    
    WispGameState state = {stateId, type, 0, value};
    
    if (index == 0xFFFF) {
        // Add new state
        if (header.entryCount >= WISP_DB_MAX_ITEMS) return false;
        entries[header.entryCount] = state.toDBEntry();
        header.entryCount++;
        stateCount++;
    } else {
        // Update existing state
        entries[index] = state.toDBEntry();
    }
    
    save();
    return true;
}

uint32_t WispDatabaseSystem::getState(uint16_t stateId) {
    uint16_t index = findEntryIndex(stateId, DB_TYPE_STATE);
    if (index == 0xFFFF) return 0;
    
    WispGameState state = WispGameState::fromDBEntry(entries[index]);
    return state.value;
}

bool WispDatabaseSystem::hasState(uint16_t stateId) {
    return findEntryIndex(stateId, DB_TYPE_STATE) != 0xFFFF;
}

bool WispDatabaseSystem::toggleFlag(uint16_t flagId) {
    bool current = getFlag(flagId);
    return setState(flagId, current ? 0 : 1, 1);
}

bool WispDatabaseSystem::getFlag(uint16_t flagId) {
    return getState(flagId) != 0;
}

// Inventory management
bool WispDatabaseSystem::addToInventory(uint16_t itemId, uint8_t quantity) {
    // Check if item already exists in inventory
    for (uint16_t i = 0; i < header.entryCount; i++) {
        if (entries[i].type == DB_TYPE_INVENTORY) {
            WispInventorySlot slot = WispInventorySlot::fromDBEntry(entries[i]);
            if (slot.itemId == itemId) {
                // Add to existing stack
                slot.quantity = min(255, slot.quantity + quantity);
                entries[i] = slot.toDBEntry();
                save();
                return true;
            }
        }
    }
    
    // Add new inventory slot
    if (header.entryCount >= WISP_DB_MAX_ITEMS) return false;
    
    WispInventorySlot slot = {itemId, quantity, 100, 0}; // Perfect condition
    entries[header.entryCount] = slot.toDBEntry();
    header.entryCount++;
    inventoryCount++;
    
    save();
    return true;
}

bool WispDatabaseSystem::hasInInventory(uint16_t itemId, uint8_t quantity) {
    return getInventoryCount(itemId) >= quantity;
}

uint8_t WispDatabaseSystem::getInventoryCount(uint16_t itemId) {
    for (uint16_t i = 0; i < header.entryCount; i++) {
        if (entries[i].type == DB_TYPE_INVENTORY) {
            WispInventorySlot slot = WispInventorySlot::fromDBEntry(entries[i]);
            if (slot.itemId == itemId) {
                return slot.quantity;
            }
        }
    }
    return 0;
}

std::vector<WispInventorySlot> WispDatabaseSystem::getInventory() {
    std::vector<WispInventorySlot> inventory;
    
    for (uint16_t i = 0; i < header.entryCount; i++) {
        if (entries[i].type == DB_TYPE_INVENTORY) {
            inventory.push_back(WispInventorySlot::fromDBEntry(entries[i]));
        }
    }
    
    return inventory;
}

// Debug and diagnostics
void WispDatabaseSystem::printDatabaseStats() {
    Serial.println("=== Wisp Database Statistics ===");
    Serial.printf("Total entries: %d / %d\n", header.entryCount, WISP_DB_MAX_ITEMS);
    Serial.printf("Items: %d, Quests: %d, States: %d, Inventory: %d\n", 
                 itemCount, questCount, stateCount, inventoryCount);
    Serial.printf("Memory used: %d / %d bytes (%.1f%%)\n", 
                 getMemoryUsed(), WISP_DB_LP_SRAM_SIZE, 
                 (float)getMemoryUsed() / WISP_DB_LP_SRAM_SIZE * 100.0f);
    Serial.printf("Last update: %lu ms\n", header.lastUpdate);
    Serial.printf("Checksum valid: %s\n", validateChecksum() ? "YES" : "NO");
}

void WispDatabaseSystem::printInventory() {
    Serial.println("=== Player Inventory ===");
    auto inventory = getInventory();
    
    if (inventory.empty()) {
        Serial.println("Inventory is empty");
        return;
    }
    
    for (const auto& slot : inventory) {
        WispItem item = getItem(slot.itemId);
        Serial.printf("Item %d: %s x%d (condition: %d%%)\n", 
                     slot.itemId, 
                     item.itemId ? "Found" : "Unknown",
                     slot.quantity, 
                     slot.condition);
    }
}

void WispDatabaseSystem::printActiveQuests() {
    Serial.println("=== Active Quests ===");
    bool hasActiveQuests = false;
    
    for (uint16_t i = 0; i < header.entryCount; i++) {
        if (entries[i].type == DB_TYPE_QUEST) {
            WispQuest quest = WispQuest::fromDBEntry(entries[i]);
            if (quest.status == 1) { // Active
                Serial.printf("Quest %d: %d%% complete (stages: 0x%08X)\n",
                             quest.questId, quest.progress, quest.stageFlags);
                hasActiveQuests = true;
            }
        }
    }
    
    if (!hasActiveQuests) {
        Serial.println("No active quests");
    }
}

// Conversion methods for data structures
WispDBEntry WispItem::toDBEntry() const {
    WispDBEntry entry;
    entry.id = itemId;
    entry.type = DB_TYPE_ITEM;
    entry.flags = itemType;
    entry.data = ((uint32_t)rarity << 24) | ((uint32_t)value << 8) | (properties & 0xFF);
    return entry;
}

WispItem WispItem::fromDBEntry(const WispDBEntry& entry) {
    WispItem item;
    item.itemId = entry.id;
    item.itemType = entry.flags;
    item.rarity = (entry.data >> 24) & 0xFF;
    item.value = (entry.data >> 8) & 0xFFFF;
    item.properties = entry.data & 0xFF;
    return item;
}

WispDBEntry WispQuest::toDBEntry() const {
    WispDBEntry entry;
    entry.id = questId;
    entry.type = DB_TYPE_QUEST;
    entry.flags = (status << 4) | (progress >> 4); // Pack status and progress
    entry.data = stageFlags;
    return entry;
}

WispQuest WispQuest::fromDBEntry(const WispDBEntry& entry) {
    WispQuest quest;
    quest.questId = entry.id;
    quest.status = (entry.flags >> 4) & 0x0F;
    quest.progress = (entry.flags & 0x0F) << 4; // Approximate progress
    quest.stageFlags = entry.data;
    return quest;
}

WispDBEntry WispGameState::toDBEntry() const {
    WispDBEntry entry;
    entry.id = stateId;
    entry.type = DB_TYPE_STATE;
    entry.flags = type;
    entry.data = value;
    return entry;
}

WispGameState WispGameState::fromDBEntry(const WispDBEntry& entry) {
    WispGameState state;
    state.stateId = entry.id;
    state.type = entry.flags;
    state.reserved = 0;
    state.value = entry.data;
    return state;
}

WispDBEntry WispInventorySlot::toDBEntry() const {
    WispDBEntry entry;
    entry.id = itemId;
    entry.type = DB_TYPE_INVENTORY;
    entry.flags = quantity;
    entry.data = ((uint32_t)condition << 24) | (modifiers & 0xFFFFFF);
    return entry;
}

WispInventorySlot WispInventorySlot::fromDBEntry(const WispDBEntry& entry) {
    WispInventorySlot slot;
    slot.itemId = entry.id;
    slot.quantity = entry.flags;
    slot.condition = (entry.data >> 24) & 0xFF;
    slot.modifiers = entry.data & 0xFFFFFF;
    return slot;
}
