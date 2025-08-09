// Extended API Examples
// Demonstrates the new component-based API and script control pipeline

#include "../src/engine/app/curated_api_extended.h"

// ===== EXAMPLE 1: ENHANCED PLATFORMER =====

class ExtendedPlatformerApp : public WispAppBaseExtended {
private:
    EntityHandle player;
    EntityHandle enemies[8];
    uint8_t enemyCount = 0;
    
    // Input sequences for special moves
    WispInputSemantic jumpCombo[3] = {INPUT_UP, INPUT_UP, INPUT_ACCEPT};
    WispInputSemantic dashCombo[3] = {INPUT_LEFT, INPUT_RIGHT, INPUT_ACCEPT};

public:
    bool init(const WispEngine::AppInitData& data) override {
        setAppInfo("Enhanced Platformer", "1.0", "Wisp Dev");
        
        // Create player with components using entity template
        player = createPlayer("player.art", 100, 200);
        
        // Get components for customization
        PhysicsComponent* physics = GET_PHYSICS_COMP(player);
        SpriteComponent* sprite = GET_SPRITE_COMP(player);
        DataComponent* playerData = createData(player);
        
        // Setup platformer physics
        physics->setBodyType(BODY_DYNAMIC);
        physics->enableGravity(true, 1000);
        physics->setFriction(800);
        physics->setBounce(0);
        physics->setCollisionEnterCallback([](uint16_t entity, uint16_t other, CollisionResponse response) {
            // Handle player collisions
            if (response == COLLISION_STOP) {
                // Hit a wall or platform
                EntityHandle playerHandle = entity;
                triggerAudioEvent("landing_sound");
            }
        });
        
        // Setup player data
        playerData->setInt32("health", 100, true);
        playerData->setInt32("lives", 3, true);
        playerData->setFloat("jumpPower", 800.0f);
        playerData->setString("playerName", "Hero", true);
        
        // Register input sequences for special moves
        extendedAPI->registerInputSequence(jumpCombo, 3, "double_jump");
        extendedAPI->registerInputSequence(dashCombo, 3, "dash_attack");
        
        // Load and bind player control script
        if (extendedAPI->loadScript("player_controls", getPlayerControlScript(), sizeof(getPlayerControlScript()))) {
            extendedAPI->bindEntityScript(player, "player_controls");
            
            // Bind specific input events to scripts
            BIND_INPUT_SCRIPT(player, INPUT_ACCEPT, "player_controls");
            BIND_INPUT_SCRIPT(player, INPUT_LEFT, "player_controls");
            BIND_INPUT_SCRIPT(player, INPUT_RIGHT, "player_controls");
        }
        
        // Create enemies with AI scripts
        for (int i = 0; i < 4; i++) {
            enemies[i] = createEnemy("enemy.art", 300 + i * 100, 200, "simple_patrol_ai");
            
            // Each enemy gets its own data
            DataComponent* enemyData = createData(enemies[i]);
            enemyData->setInt32("health", 50);
            enemyData->setFloat("patrolDistance", 64.0f);
            enemyData->setFloat("moveSpeed", 30.0f);
            
            // Bind collision script
            BIND_COLLISION_SCRIPT(enemies[i], "enemy_collision");
            
            enemyCount++;
        }
        
        // Setup audio events
        extendedAPI->registerAudioEvent("jump_sound", extendedAPI->loadAudio("jump.sfx"));
        extendedAPI->registerAudioEvent("landing_sound", extendedAPI->loadAudio("land.sfx"));
        extendedAPI->registerAudioEvent("enemy_hit", extendedAPI->loadAudio("hit.sfx"));
        
        return true;
    }
    
    void update() override {
        // Handle basic movement (script handles complex input)
        DataComponent* playerData = GET_DATA_COMP(player);
        PhysicsComponent* physics = GET_PHYSICS_COMP(player);
        
        float jumpPower = playerData->getFloat("jumpPower");
        int32_t velocityX = 0;
        
        // Basic movement (enhanced by script)
        if (left()) {
            velocityX = -200;
        } else if (right()) {
            velocityX = 200;
        }
        
        // Jump
        if (acceptPressed() && physics->isOnGround()) {
            physics->jump(jumpPower * 65536); // Fixed-point
            extendedAPI->triggerAudioEvent("jump_sound");
        }
        
        // Check for special move combos
        if (WAS_SEQUENCE_TRIGGERED("double_jump")) {
            // Double jump ability
            physics->jump(jumpPower * 0.7f * 65536);
            extendedAPI->triggerScriptEvent("double_jump_performed");
        }
        
        if (WAS_SEQUENCE_TRIGGERED("dash_attack")) {
            // Dash attack
            int32_t dashVelocity = left() ? -400 : 400;
            physics->applyImpulse(dashVelocity * 65536, 0);
            extendedAPI->triggerScriptEvent("dash_attack_performed");
        }
        
        physics->setVelocity(velocityX * 65536, physics->getVelocityY());
        
        // Check player health
        int32_t health = playerData->getInt32("health");
        if (health <= 0) {
            // Handle player death
            fireEvent("player_died");
        }
    }
    
    void render() override {
        // Components handle rendering automatically
        extendedAPI->renderAllEntities();
        
        // UI overlay
        DataComponent* playerData = GET_DATA_COMP(player);
        int32_t health = playerData->getInt32("health");
        int32_t lives = playerData->getInt32("lives");
        
        extendedAPI->drawText("Health: " + std::to_string(health), 10, 10, WispColor(255, 255, 255));
        extendedAPI->drawText("Lives: " + std::to_string(lives), 10, 30, WispColor(255, 255, 255));
        
        // Show input prompts
        extendedAPI->drawText("ARROW KEYS: Move", 10, 200, WispColor(200, 200, 200));
        extendedAPI->drawText("ACCEPT: Jump", 10, 215, WispColor(200, 200, 200));
        extendedAPI->drawText("UP-UP-ACCEPT: Double Jump", 10, 230, WispColor(180, 180, 180));
    }
    
private:
    // Embedded player control script (in real app, loaded from ROM)
    const uint8_t* getPlayerControlScript() {
        static const char script[] = R"(
            // Player control script (.ash bytecode would be here)
            function onInput(input, pressed, value) {
                if (input == INPUT_ACCEPT && pressed) {
                    // Enhanced jump logic
                    if (isOnGround()) {
                        playSound("jump_sound");
                        setAnimation(ANIM_JUMP);
                    }
                }
                
                if (input == INPUT_LEFT && pressed) {
                    setFlip(true, false);
                    setAnimation(ANIM_MOVE);
                } else if (input == INPUT_RIGHT && pressed) {
                    setFlip(false, false);
                    setAnimation(ANIM_MOVE);
                } else if (!input_left && !input_right) {
                    setAnimation(ANIM_IDLE);
                }
            }
            
            function onCollision(otherId, response) {
                string otherTag = getEntityTag(otherId);
                if (otherTag == "enemy") {
                    takeDamage(10);
                    playSound("player_hurt");
                }
            }
        )";
        return (const uint8_t*)script;
    }
};

// ===== EXAMPLE 2: POKEMON-STYLE RPG =====

class ExtendedPokemonRPG : public WispAppBaseExtended {
private:
    EntityHandle player;
    EntityHandle npcs[16];
    EntityHandle pokemon[6];  // Party
    uint8_t npcCount = 0;
    uint8_t pokemonCount = 0;
    
    // UI entities
    EntityHandle menuEntity;
    EntityHandle dialogBox;
    
    // Game state
    enum GameState { OVERWORLD, BATTLE, MENU, DIALOG };
    GameState currentState = OVERWORLD;

public:
    bool init(const WispEngine::AppInitData& data) override {
        setAppInfo("Pokemon RPG", "1.0", "Wisp Dev");
        
        // Create player with RPG-specific components
        player = createPlayer("trainer.art", 160, 120);
        
        // Player data for RPG
        DataComponent* playerData = createData(player);
        playerData->setString("name", "Ash", true);
        playerData->setInt32("badges", 0, true);
        playerData->setInt32("money", 5000, true);
        playerData->setInt32("playtime", 0, true);
        
        // Setup grid-based movement for RPG
        PhysicsComponent* playerPhysics = GET_PHYSICS_COMP(player);
        playerPhysics->setBodyType(BODY_KINEMATIC);
        playerPhysics->enableGravity(false); // Top-down movement
        
        // Create timer for play time tracking
        TimerComponent* playTimer = createTimer(player, 1);
        playTimer->start(TIMER_REPEATING, 1000); // Every second
        playTimer->setCompleteCallback([](uint16_t entity, uint16_t timer) {
            DataComponent* data = GET_DATA_COMP(entity);
            int32_t playtime = data->getInt32("playtime");
            data->setInt32("playtime", playtime + 1, true);
        });
        
        // Create NPCs with dialog scripts
        EntityHandle profOak = createEnemy("prof_oak.art", 200, 100, ""); // No AI, just dialog
        DataComponent* profData = createData(profOak);
        profData->setString("name", "Prof Oak");
        profData->setString("dialog", "Welcome to Pokemon! Take this Pokedex!");
        BIND_INPUT_SCRIPT(profOak, INPUT_ACCEPT, "npc_dialog");
        npcs[npcCount++] = profOak;
        
        // Create Pokemon party
        for (int i = 0; i < 3; i++) {
            pokemon[i] = extendedAPI->createEntity();
            DataComponent* pokemonData = createData(pokemon[i]);
            
            if (i == 0) {
                // Starter Pokemon
                pokemonData->setString("name", "Pikachu", true);
                pokemonData->setString("type", "Electric", true);
                pokemonData->setInt32("level", 5, true);
                pokemonData->setInt32("hp", 25, true);
                pokemonData->setInt32("maxhp", 25, true);
            }
            pokemonCount++;
        }
        
        // Create UI entities
        menuEntity = createUIEntity("menu_bg.art", 0, 0);
        dialogBox = createUIEntity("dialog_box.art", 0, 160);
        
        // Set up UI components
        GET_SPRITE_COMP(menuEntity)->setVisible(false);
        GET_SPRITE_COMP(dialogBox)->setVisible(false);
        
        // Load scripts for different game states
        loadGameScripts();
        
        // Set up input mappings for RPG
        setupRPGControls();
        
        return true;
    }
    
    void update() override {
        DataComponent* playerData = GET_DATA_COMP(player);
        PhysicsComponent* playerPhysics = GET_PHYSICS_COMP(player);
        
        switch (currentState) {
            case OVERWORLD:
                updateOverworldMovement();
                checkNPCInteractions();
                break;
                
            case MENU:
                updateMenuNavigation();
                break;
                
            case DIALOG:
                updateDialog();
                break;
                
            case BATTLE:
                updateBattle();
                break;
        }
        
        // Handle menu toggle
        if (menuPressed()) {
            if (currentState == OVERWORLD) {
                enterMenu();
            } else if (currentState == MENU) {
                exitMenu();
            }
        }
    }
    
    void render() override {
        // Render based on game state
        switch (currentState) {
            case OVERWORLD:
                extendedAPI->renderAllEntities();
                renderHUD();
                break;
                
            case MENU:
                extendedAPI->renderEntity(menuEntity);
                renderMenu();
                break;
                
            case DIALOG:
                extendedAPI->renderAllEntities();
                extendedAPI->renderEntity(dialogBox);
                renderDialog();
                break;
                
            case BATTLE:
                renderBattle();
                break;
        }
    }
    
private:
    void updateOverworldMovement() {
        PhysicsComponent* physics = GET_PHYSICS_COMP(player);
        SpriteComponent* sprite = GET_SPRITE_COMP(player);
        
        // Grid-based movement
        static uint32_t lastMoveTime = 0;
        uint32_t currentTime = extendedAPI->getTime();
        
        if (currentTime - lastMoveTime > 200) { // 200ms movement cooldown
            int32_t moveX = 0, moveY = 0;
            
            if (up()) {
                moveY = -32; // Tile size
                sprite->setFlip(false, false);
                sprite->playAnimation(ANIM_MOVE);
            } else if (down()) {
                moveY = 32;
                sprite->setFlip(false, false);
                sprite->playAnimation(ANIM_MOVE);
            } else if (left()) {
                moveX = -32;
                sprite->setFlip(true, false);
                sprite->playAnimation(ANIM_MOVE);
            } else if (right()) {
                moveX = 32;
                sprite->setFlip(false, false);
                sprite->playAnimation(ANIM_MOVE);
            } else {
                sprite->playAnimation(ANIM_IDLE);
            }
            
            if (moveX != 0 || moveY != 0) {
                physics->move(moveX * 65536, moveY * 65536); // Fixed-point
                lastMoveTime = currentTime;
            }
        }
    }
    
    void checkNPCInteractions() {
        if (acceptPressed()) {
            // Check for nearby NPCs
            EntityHandle nearbyNPCs[4];
            int32_t playerX = GET_PHYSICS_COMP(player)->getX() >> 16; // Convert from fixed-point
            int32_t playerY = GET_PHYSICS_COMP(player)->getY() >> 16;
            
            int count = extendedAPI->getEntitiesInRect(playerX - 48, playerY - 48, 96, 96, nearbyNPCs, 4);
            
            for (int i = 0; i < count; i++) {
                String tag = extendedAPI->getEntityTag(nearbyNPCs[i]);
                if (tag == "npc") {
                    // Start dialog with NPC
                    startDialog(nearbyNPCs[i]);
                    break;
                }
            }
        }
    }
    
    void enterMenu() {
        currentState = MENU;
        GET_SPRITE_COMP(menuEntity)->setVisible(true);
        TRIGGER_EVENT("menu_opened", ScriptValue());
    }
    
    void exitMenu() {
        currentState = OVERWORLD;
        GET_SPRITE_COMP(menuEntity)->setVisible(false);
        TRIGGER_EVENT("menu_closed", ScriptValue());
    }
    
    void startDialog(EntityHandle npc) {
        currentState = DIALOG;
        GET_SPRITE_COMP(dialogBox)->setVisible(true);
        
        // Get NPC dialog text
        DataComponent* npcData = GET_DATA_COMP(npc);
        String dialog = npcData->getString("dialog");
        
        // Trigger dialog script
        extendedAPI->setScriptGlobal("current_dialog", ScriptValue(dialog.c_str()), VALUE_STRING);
        EXECUTE_SCRIPT("dialog_system");
    }
    
    void loadGameScripts() {
        // Load various game scripts (in real app, from ROM)
        extendedAPI->loadScript("dialog_system", getDialogScript(), sizeof(getDialogScript()));
        extendedAPI->loadScript("menu_system", getMenuScript(), sizeof(getMenuScript()));
        extendedAPI->loadScript("battle_system", getBattleScript(), sizeof(getBattleScript()));
        
        // Register script event handlers
        extendedAPI->registerScriptEventHandler("pokemon_fainted", "battle_system", "onPokemonFainted");
        extendedAPI->registerScriptEventHandler("battle_won", "battle_system", "onBattleWon");
        extendedAPI->registerScriptEventHandler("item_used", "menu_system", "onItemUsed");
    }
    
    void setupRPGControls() {
        // Register input sequences for menu shortcuts
        WispInputSemantic pokemonShortcut[2] = {INPUT_ALT, INPUT_ACCEPT};
        WispInputSemantic bagShortcut[2] = {INPUT_ALT, INPUT_BACK};
        
        extendedAPI->registerInputSequence(pokemonShortcut, 2, "pokemon_menu");
        extendedAPI->registerInputSequence(bagShortcut, 2, "bag_menu");
        
        // Bind global input scripts
        BIND_INPUT_SCRIPT(0, INPUT_MENU, "menu_system"); // Entity 0 = global
    }
    
    // Placeholder functions for scripts (real implementations would be bytecode)
    const uint8_t* getDialogScript() {
        static const char script[] = "// Dialog system script bytecode";
        return (const uint8_t*)script;
    }
    
    const uint8_t* getMenuScript() {
        static const char script[] = "// Menu system script bytecode";
        return (const uint8_t*)script;
    }
    
    const uint8_t* getBattleScript() {
        static const char script[] = "// Battle system script bytecode";
        return (const uint8_t*)script;
    }
    
    // Placeholder render functions
    void renderHUD() {
        DataComponent* playerData = GET_DATA_COMP(player);
        String name = playerData->getString("name");
        int32_t badges = playerData->getInt32("badges");
        int32_t money = playerData->getInt32("money");
        
        extendedAPI->drawText("Trainer: " + name, 10, 10, WispColor(255, 255, 255));
        extendedAPI->drawText("Badges: " + std::to_string(badges), 10, 25, WispColor(255, 255, 255));
        extendedAPI->drawText("Money: $" + std::to_string(money), 10, 40, WispColor(255, 255, 255));
    }
    
    void renderMenu() { /* Menu rendering */ }
    void renderDialog() { /* Dialog rendering */ }
    void renderBattle() { /* Battle rendering */ }
    void updateMenuNavigation() { /* Menu logic */ }
    void updateDialog() { /* Dialog logic */ }
    void updateBattle() { /* Battle logic */ }
};

// Register the extended apps
WISP_REGISTER_EXTENDED_APP(ExtendedPlatformerApp);
WISP_REGISTER_EXTENDED_APP(ExtendedPokemonRPG);
