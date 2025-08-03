// Example Pokemon-style RPG App using Wisp Database System
#include "../../include/wisp_app_interface.h"
#include "../src/engine/database/database_system.h"

class PokemonRPGApp : public WispApp {
private:
    uint8_t gameState;
    uint16_t playerX, playerY;
    uint16_t currentMap;
    uint32_t lastUpdate;
    
    // Game states
    enum GameState {
        STATE_MENU = 0,
        STATE_OVERWORLD = 1,
        STATE_BATTLE = 2,
        STATE_INVENTORY = 3,
        STATE_SAVE = 4
    };
    
    // Item IDs
    enum ItemIDs {
        ITEM_POKEBALL = 1,
        ITEM_GREATBALL = 2,
        ITEM_ULTRABALL = 3,
        ITEM_POTION = 10,
        ITEM_SUPER_POTION = 11,
        ITEM_HYPER_POTION = 12,
        ITEM_RARE_CANDY = 20,
        ITEM_TM_SURF = 30,
        ITEM_GYM_BADGE_1 = 100,
        ITEM_GYM_BADGE_2 = 101
    };
    
    // Quest IDs  
    enum QuestIDs {
        QUEST_STARTER_POKEMON = 1,
        QUEST_FIRST_GYM = 2,
        QUEST_SURF_HM = 3,
        QUEST_ELITE_FOUR = 4,
        QUEST_CHAMPION = 5
    };
    
    // Game state IDs
    enum StateIDs {
        STATE_PLAYER_LEVEL = 1,
        STATE_BADGES_EARNED = 2,
        STATE_POKEMON_CAUGHT = 3,
        STATE_CURRENT_MAP = 4,
        STATE_PLAYER_X = 5,
        STATE_PLAYER_Y = 6,
        STATE_RIVAL_DEFEATED = 10,
        STATE_ELITE_FOUR_BEATEN = 11,
        STATE_CHAMPION_DEFEATED = 12
    };

public:
    PokemonRPGApp() : gameState(STATE_MENU), playerX(10), playerY(10), 
                      currentMap(1), lastUpdate(0) {}
    
    const char* getName() override { return "Pokemon RPG Demo"; }
    const char* getVersion() override { return "1.0.0"; }
    uint8_t getTargetFPS() override { return 10; }
    uint32_t getMemoryRequirement() override { return 32768; }
    
    bool init() override {
        Serial.println("=== Pokemon RPG Demo Starting ===");
        
        // Initialize database
        if (!wispDB.initialize()) {
            Serial.println("ERROR: Failed to initialize database");
            return false;
        }
        
        // Check if this is a new game
        if (!wispDB.hasState(STATE_PLAYER_LEVEL)) {
            setupNewGame();
        } else {
            loadGameState();
        }
        
        return true;
    }
    
    void setupNewGame() {
        Serial.println("Setting up new Pokemon RPG game...");
        
        // Initialize player stats
        WISP_DB_SET_COUNTER(STATE_PLAYER_LEVEL, 5);
        WISP_DB_SET_COUNTER(STATE_BADGES_EARNED, 0);
        WISP_DB_SET_COUNTER(STATE_POKEMON_CAUGHT, 1); // Starter pokemon
        WISP_DB_SET_POSITION(STATE_CURRENT_MAP, STATE_PLAYER_X, STATE_PLAYER_Y, 1, 10, 10);
        
        // Add starter items
        WISP_DB_ADD_ITEM(ITEM_POKEBALL, 5);
        WISP_DB_ADD_ITEM(ITEM_POTION, 3);
        
        // Setup initial quest
        WispQuest starterQuest = {QUEST_STARTER_POKEMON, 1, 100, 0x00000001};
        wispDB.addQuest(starterQuest);
        wispDB.completeQuest(QUEST_STARTER_POKEMON);
        
        // Start first gym quest
        WispQuest gymQuest = {QUEST_FIRST_GYM, 1, 0, 0x00000000};
        wispDB.addQuest(gymQuest);
        
        Serial.println("New game setup complete!");
        wispDB.printDatabaseStats();
    }
    
    void loadGameState() {
        Serial.println("Loading existing game state...");
        
        // Load player position
        currentMap = wispDB.getState(STATE_CURRENT_MAP);
        playerX = wispDB.getState(STATE_PLAYER_X);
        playerY = wispDB.getState(STATE_PLAYER_Y);
        
        uint8_t playerLevel = wispDB.getState(STATE_PLAYER_LEVEL);
        uint8_t badges = wispDB.getState(STATE_BADGES_EARNED);
        uint8_t pokemonCaught = wispDB.getState(STATE_POKEMON_CAUGHT);
        
        Serial.printf("Player Level: %d, Badges: %d, Pokemon: %d\n", 
                     playerLevel, badges, pokemonCaught);
        Serial.printf("Position: Map %d at (%d, %d)\n", currentMap, playerX, playerY);
        
        wispDB.printInventory();
        wispDB.printActiveQuests();
    }
    
    void update(uint32_t deltaTime) override {
        lastUpdate += deltaTime;
        
        // Simple state machine
        switch (gameState) {
            case STATE_MENU:
                updateMenu();
                break;
            case STATE_OVERWORLD:
                updateOverworld();
                break;
            case STATE_INVENTORY:
                updateInventory();
                break;
            case STATE_SAVE:
                saveGame();
                gameState = STATE_OVERWORLD;
                break;
        }
        
        // Auto-save every 30 seconds
        if (lastUpdate > 30000) {
            saveGame();
            lastUpdate = 0;
        }
    }
    
    void updateMenu() {
        // Simulate menu interaction - go to overworld
        gameState = STATE_OVERWORLD;
    }
    
    void updateOverworld() {
        // Simulate random encounters and progression
        static uint32_t lastAction = 0;
        
        if (millis() - lastAction > 5000) { // Every 5 seconds
            simulateGameplayEvent();
            lastAction = millis();
        }
    }
    
    void simulateGameplayEvent() {
        uint8_t event = random(1, 6);
        
        switch (event) {
            case 1: // Find item
                findRandomItem();
                break;
            case 2: // Catch Pokemon
                catchPokemon();
                break;
            case 3: // Gym battle
                attemptGymBattle();
                break;
            case 4: // Use item
                useRandomItem();
                break;
            case 5: // Move to new area
                moveToNewArea();
                break;
        }
    }
    
    void findRandomItem() {
        uint16_t items[] = {ITEM_POKEBALL, ITEM_POTION, ITEM_RARE_CANDY};
        uint16_t itemId = items[random(0, 3)];
        
        WISP_DB_ADD_ITEM(itemId, 1);
        Serial.printf("Found item %d!\n", itemId);
    }
    
    void catchPokemon() {
        if (WISP_DB_HAS_ITEM(ITEM_POKEBALL, 1)) {
            WISP_DB_USE_ITEM(ITEM_POKEBALL, 1);
            WISP_DB_INCREMENT_COUNTER(STATE_POKEMON_CAUGHT, 1);
            Serial.println("Caught a wild Pokemon!");
        } else {
            Serial.println("No Pokeballs available!");
        }
    }
    
    void attemptGymBattle() {
        uint8_t badges = wispDB.getState(STATE_BADGES_EARNED);
        uint8_t playerLevel = wispDB.getState(STATE_PLAYER_LEVEL);
        
        if (badges < 8 && playerLevel >= (badges + 1) * 10) {
            // Win gym battle
            badges++;
            WISP_DB_SET_COUNTER(STATE_BADGES_EARNED, badges);
            WISP_DB_ADD_ITEM(ITEM_GYM_BADGE_1 + badges - 1, 1);
            
            Serial.printf("Won gym battle! Badge %d earned!\n", badges);
            
            // Progress gym quest
            if (badges == 1 && wispDB.isQuestActive(QUEST_FIRST_GYM)) {
                wispDB.completeQuest(QUEST_FIRST_GYM);
                Serial.println("First Gym quest completed!");
            }
        }
    }
    
    void useRandomItem() {
        if (WISP_DB_HAS_ITEM(ITEM_POTION, 1)) {
            WISP_DB_USE_ITEM(ITEM_POTION, 1);
            Serial.println("Used a Potion!");
        }
    }
    
    void moveToNewArea() {
        playerX = random(1, 50);
        playerY = random(1, 50);
        currentMap = random(1, 10);
        
        WISP_DB_SET_POSITION(STATE_CURRENT_MAP, STATE_PLAYER_X, STATE_PLAYER_Y,
                           currentMap, playerX, playerY);
        
        Serial.printf("Moved to Map %d at (%d, %d)\n", currentMap, playerX, playerY);
    }
    
    void updateInventory() {
        // Display inventory
        wispDB.printInventory();
        gameState = STATE_OVERWORLD;
    }
    
    void saveGame() {
        // Update current position
        WISP_DB_SET_POSITION(STATE_CURRENT_MAP, STATE_PLAYER_X, STATE_PLAYER_Y,
                           currentMap, playerX, playerY);
        
        wispDB.save();
        Serial.println("Game saved to LP-SRAM!");
    }
    
    void render() override {
        // Simplified render - just print game state
        static uint32_t lastPrint = 0;
        
        if (millis() - lastPrint > 10000) { // Every 10 seconds
            Serial.println("\n=== Pokemon RPG Status ===");
            Serial.printf("State: %s\n", getStateName());
            Serial.printf("Level: %d, Badges: %d, Pokemon: %d\n",
                         wispDB.getState(STATE_PLAYER_LEVEL),
                         wispDB.getState(STATE_BADGES_EARNED),
                         wispDB.getState(STATE_POKEMON_CAUGHT));
            Serial.printf("Position: Map %d at (%d, %d)\n", currentMap, playerX, playerY);
            Serial.printf("Pokeballs: %d, Potions: %d\n",
                         wispDB.getInventoryCount(ITEM_POKEBALL),
                         wispDB.getInventoryCount(ITEM_POTION));
            lastPrint = millis();
        }
    }
    
    const char* getStateName() {
        switch (gameState) {
            case STATE_MENU: return "Menu";
            case STATE_OVERWORLD: return "Overworld";
            case STATE_BATTLE: return "Battle";
            case STATE_INVENTORY: return "Inventory";
            case STATE_SAVE: return "Saving";
            default: return "Unknown";
        }
    }
    
    void handleInput(uint8_t button, bool pressed) override {
        if (!pressed) return;
        
        switch (button) {
            case 0: // Menu
                gameState = STATE_MENU;
                break;
            case 1: // Inventory
                gameState = STATE_INVENTORY;
                break;
            case 2: // Save
                gameState = STATE_SAVE;
                break;
            case 3: // Debug stats
                wispDB.printDatabaseStats();
                break;
        }
    }
    
    void cleanup() override {
        saveGame();
        Serial.println("Pokemon RPG Demo shutting down...");
    }
};

// App factory function
extern "C" WispApp* createWispApp() {
    return new PokemonRPGApp();
}
