// examples/restricted_platformer_game.cpp
// Example of a platformer game using the restricted curated API
// This shows how developers work within the quotas and limitations

#include "../src/engine/wisp_app_interface.h"

class RestrictedPlatformerGame : public WispAppBase {
private:
    // Helper functions for array management
    void addEnemy(EntityHandle enemy) {
        if (enemyCount < MAX_ENEMIES) {
            enemies[enemyCount++] = enemy;
        }
    }
    
    void removeEnemy(int index) {
        if (index < enemyCount) {
            for (int i = index; i < enemyCount - 1; i++) {
                enemies[i] = enemies[i + 1];
            }
            enemyCount--;
        }
    }
    
    void addCoin(EntityHandle coin) {
        if (coinCount < MAX_COINS) {
            coins[coinCount++] = coin;
        }
    }
    
    void removeCoin(int index) {
        if (index < coinCount) {
            for (int i = index; i < coinCount - 1; i++) {
                coins[i] = coins[i + 1];
            }
            coinCount--;
        }
    }

    // Game entities (limited by quota)
    static const int MAX_ENEMIES = 10;
    static const int MAX_COINS = 20;
    
    EntityHandle player;
    EntityHandle enemies[MAX_ENEMIES];
    int enemyCount;
    EntityHandle coins[MAX_COINS];
    int coinCount;
    
    // Resources (limited by quota)
    ResourceHandle playerSprite;
    ResourceHandle enemySprite;
    ResourceHandle coinSprite;
    ResourceHandle tilesetSprite;
    
    // Audio resources (limited by quota)
    ResourceHandle jumpSound;
    ResourceHandle coinSound;
    ResourceHandle backgroundMusic;
    
    // Game state
    float playerX, playerY;
    float playerVX, playerVY;
    bool playerOnGround;
    int score;
    int lives;
    
    // Level management
    int currentLevel;
    float cameraX, cameraY;
    
    // Timers (limited by quota)
    TimerHandle gameTimer;
    TimerHandle enemySpawnTimer;
    
    // Performance monitoring
    bool quotaWarningShown;
    
public:
    RestrictedPlatformerGame() : 
        player(INVALID_ENTITY), playerX(32), playerY(200), 
        playerVX(0), playerVY(0), playerOnGround(false),
        score(0), lives(3), currentLevel(1),
        cameraX(0), cameraY(0),
        gameTimer(INVALID_TIMER), enemySpawnTimer(INVALID_TIMER),
        quotaWarningShown(false), enemyCount(0), coinCount(0) {
        
        setAppInfo("Restricted Platformer", "1.0", "Wisp Demo");
    }
    
    bool init() override {
        print("Initializing Restricted Platformer...");
        
        // Check if we have API access
        if (!api) {
            error("No API access provided!");
            return false;
        }
        
        // Load sprites (quota-limited)
        playerSprite = api->loadSprite("/sprites/player.spr");
        if (playerSprite == INVALID_RESOURCE) {
            error("Failed to load player sprite");
            return false;
        }
        
        enemySprite = api->loadSprite("/sprites/enemy.spr");
        if (enemySprite == INVALID_RESOURCE) {
            warning("Failed to load enemy sprite - continuing without");
        }
        
        coinSprite = api->loadSprite("/sprites/coin.spr");
        tilesetSprite = api->loadSprite("/sprites/tileset.spr");
        
        // Load audio (quota-limited)
        jumpSound = api->loadAudio("/audio/jump.wav");
        coinSound = api->loadAudio("/audio/coin.wav");
        backgroundMusic = api->loadAudio("/audio/bgm.ogg");
        
        // Create player entity (quota-limited)
        player = api->createEntity();
        if (player == INVALID_ENTITY) {
            error("Failed to create player entity - quota exceeded?");
            return false;
        }
        
        // Set up player
        api->setEntityPosition(player, WispVec2(playerX, playerY));
        api->setEntitySprite(player, playerSprite);
        
        // Create initial enemies (respecting quota)
        createInitialEnemies();
        
        // Create initial coins
        createInitialCoins();
        
        // Set up timers (quota-limited)
        gameTimer = api->createTimer(60000, false); // 1 minute game timer
        enemySpawnTimer = api->createTimer(3000, true); // Spawn enemy every 3 seconds
        
        // Start background music
        if (backgroundMusic != INVALID_RESOURCE) {
            WispAudioParams musicParams;
            musicParams.volume = 0.3f;
            musicParams.loop = true;
            musicParams.priority = 255; // Low priority
            api->playAudio(backgroundMusic, musicParams);
        }
        
        // Check quota usage
        checkQuotaUsage();
        
        print("Restricted Platformer initialized successfully!");
        return true;
    }
    
    void update() override {
        // Handle input (input state is read-only)
        handleInput();
        
        // Update player physics
        updatePlayer();
        
        // Update camera
        updateCamera();
        
        // Spawn enemies if timer finished and quota allows
        if (api->isTimerFinished(enemySpawnTimer) && enemyCount < 8) {
            spawnEnemy();
            api->resetTimer(enemySpawnTimer);
        }
        
        // Update enemies
        updateEnemies();
        
        // Check collisions (quota-limited)
        checkCollisions();
        
        // Clean up off-screen entities to free quota
        cleanupOffScreenEntities();
        
        // Check quota warnings
        checkQuotaWarnings();
        
        // Check game timer
        if (api->isTimerFinished(gameTimer)) {
            // Time's up!
            print("Time's up! Final score: " + String(score));
        }
    }
    
    void render() override {
        // Set camera
        api->setCameraPosition(cameraX, cameraY);
        
        // Draw background tiles (quota-limited draw calls)
        renderBackground();
        
        // Draw entities (automatically handled by entity system)
        // Player and entities are rendered automatically based on their sprites
        
        // Draw UI elements (quota-limited)
        renderUI();
        
        // Show quota warnings if needed
        if (quotaWarningShown) {
            renderQuotaWarning();
        }
    }
    
    void cleanup() override {
        print("Cleaning up Restricted Platformer...");
        
        // Destroy entities (frees quota)
        if (player != INVALID_ENTITY) {
            api->destroyEntity(player);
        }
        
        for (EntityHandle enemy : enemies) {
            api->destroyEntity(enemy);
        }
        
        for (EntityHandle coin : coins) {
            api->destroyEntity(coin);
        }
        
        // Destroy timers (frees quota)
        if (gameTimer != INVALID_TIMER) {
            api->destroyTimer(gameTimer);
        }
        if (enemySpawnTimer != INVALID_TIMER) {
            api->destroyTimer(enemySpawnTimer);
        }
        
        // Unload resources (frees quota)
        api->unloadSprite(playerSprite);
        api->unloadSprite(enemySprite);
        api->unloadSprite(coinSprite);
        api->unloadSprite(tilesetSprite);
        
        api->unloadAudio(jumpSound);
        api->unloadAudio(coinSound);
        api->unloadAudio(backgroundMusic);
        
        print("Cleanup complete");
    }
    
    void onLowMemory() override {
        warning("Low memory warning received");
        
        // Emergency cleanup - remove non-essential entities
        while (enemies.size() > 2) {
            api->destroyEntity(enemies.back());
            enemies.pop_back();
        }
        
        // Reduce audio quality
        api->setMasterVolume(0.1f);
        
        print("Emergency cleanup performed");
    }
    
    void onError(const String& error) override {
        print("Game error occurred: " + error);
        
        // Try to recover gracefully
        if (error.indexOf("quota") >= 0) {
            quotaWarningShown = true;
        }
    }
    
private:
    void handleInput() {
        const WispInputState& input = api->getInput();
        
        // Horizontal movement
        if (input.left && !input.right) {
            playerVX = -2.0f;
        } else if (input.right && !input.left) {
            playerVX = 2.0f;
        } else {
            playerVX *= 0.8f; // Friction
        }
        
        // Jumping
        if (input.buttonA && playerOnGround) {
            playerVY = -8.0f;
            playerOnGround = false;
            
            // Play jump sound (quota-limited)
            if (jumpSound != INVALID_RESOURCE) {
                WispAudioParams jumpParams;
                jumpParams.volume = 0.5f;
                jumpParams.priority = 50; // Medium priority
                api->playAudio(jumpSound, jumpParams);
            }
        }
    }
    
    void updatePlayer() {
        // Apply gravity
        if (!playerOnGround) {
            playerVY += 0.5f; // Gravity
        }
        
        // Apply velocity
        playerX += playerVX;
        playerY += playerVY;
        
        // Simple ground collision
        if (playerY >= 200) {
            playerY = 200;
            playerVY = 0;
            playerOnGround = true;
        }
        
        // Keep player in bounds
        if (playerX < 0) playerX = 0;
        if (playerX > 2000) playerX = 2000; // Level width
        
        // Update entity position
        api->setEntityPosition(player, WispVec2(playerX, playerY));
    }
    
    void updateCamera() {
        // Follow player
        float targetCameraX = playerX - 160; // Center player on screen
        cameraX += (targetCameraX - cameraX) * 0.1f; // Smooth following
        
        // Clamp camera to level bounds
        if (cameraX < 0) cameraX = 0;
        if (cameraX > 1680) cameraX = 1680; // Level width - screen width
    }
    
    void createInitialEnemies() {
        // Create a few enemies, but respect quota
        for (int i = 0; i < 4 && enemies.size() < 8; i++) {
            EntityHandle enemy = api->createEntity();
            if (enemy != INVALID_ENTITY) {
                api->setEntityPosition(enemy, WispVec2(200 + i * 100, 200));
                api->setEntitySprite(enemy, enemySprite);
                enemies.push_back(enemy);
            } else {
                warning("Failed to create enemy - quota exceeded");
                break;
            }
        }
    }
    
    void createInitialCoins() {
        // Create coins throughout the level
        for (int i = 0; i < 10 && coins.size() < 16; i++) {
            EntityHandle coin = api->createEntity();
            if (coin != INVALID_ENTITY) {
                api->setEntityPosition(coin, WispVec2(150 + i * 150, 180));
                api->setEntitySprite(coin, coinSprite);
                coins.push_back(coin);
            } else {
                warning("Failed to create coin - quota exceeded");
                break;
            }
        }
    }
    
    void spawnEnemy() {
        if (enemies.size() >= 8) return; // Respect our self-imposed limit
        
        EntityHandle enemy = api->createEntity();
        if (enemy != INVALID_ENTITY) {
            // Spawn ahead of player
            float spawnX = playerX + 400 + api->random(0, 200);
            api->setEntityPosition(enemy, WispVec2(spawnX, 200));
            api->setEntitySprite(enemy, enemySprite);
            enemies.push_back(enemy);
        } else {
            warning("Failed to spawn enemy - quota exceeded");
        }
    }
    
    void updateEnemies() {
        // Simple AI - move enemies left
        for (EntityHandle enemy : enemies) {
            WispVec2 pos = api->getEntityPosition(enemy);
            pos.x -= 1.0f;
            api->setEntityPosition(enemy, pos);
        }
    }
    
    void checkCollisions() {
        // Check player vs enemies (quota-limited collision checks)
        for (auto it = enemies.begin(); it != enemies.end();) {
            WispCollision collision = api->checkCollision(player, *it);
            if (collision.hit) {
                // Player hit enemy
                lives--;
                print("Player hit enemy! Lives left: " + String(lives));
                
                // Remove enemy
                api->destroyEntity(*it);
                it = enemies.erase(it);
                
                if (lives <= 0) {
                    print("Game Over! Final score: " + String(score));
                }
            } else {
                ++it;
            }
        }
        
        // Check player vs coins
        for (auto it = coins.begin(); it != coins.end();) {
            WispCollision collision = api->checkCollision(player, *it);
            if (collision.hit) {
                // Player collected coin
                score += 100;
                
                // Play coin sound
                if (coinSound != INVALID_RESOURCE) {
                    WispAudioParams coinParams;
                    coinParams.volume = 0.7f;
                    coinParams.priority = 30; // High priority
                    api->playAudio(coinSound, coinParams);
                }
                
                // Remove coin
                api->destroyEntity(*it);
                it = coins.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void cleanupOffScreenEntities() {
        // Remove enemies that are too far behind camera
        for (auto it = enemies.begin(); it != enemies.end();) {
            WispVec2 pos = api->getEntityPosition(*it);
            if (pos.x < cameraX - 100) {
                api->destroyEntity(*it);
                it = enemies.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void renderBackground() {
        // Draw simple tile background (quota-limited draw calls)
        if (tilesetSprite != INVALID_RESOURCE) {
            for (int x = (int)cameraX / 32; x < (int)(cameraX + 320) / 32 + 1; x++) {
                for (int y = 12; y < 15; y++) { // Ground tiles
                    if (!api->drawSprite(tilesetSprite, x * 32 - cameraX, y * 32 - cameraY, 10)) {
                        // Draw quota exceeded - stop drawing background
                        return;
                    }
                }
            }
        }
    }
    
    void renderUI() {
        // Draw score (quota-limited)
        api->drawText("Score: " + String(score), 10, 10, WispColor(255, 255, 255), 0);
        
        // Draw lives
        api->drawText("Lives: " + String(lives), 10, 30, WispColor(255, 255, 255), 0);
        
        // Draw timer
        uint32_t timeLeft = api->getTimerRemaining(gameTimer);
        api->drawText("Time: " + String(timeLeft / 1000), 10, 50, WispColor(255, 255, 255), 0);
    }
    
    void renderQuotaWarning() {
        // Draw warning about quota usage
        api->drawRect(50, 100, 220, 60, WispColor(255, 0, 0, 128), 0);
        api->drawText("QUOTA WARNING!", 60, 110, WispColor(255, 255, 255), 0);
        api->drawText("Reducing features", 60, 130, WispColor(255, 255, 255), 0);
    }
    
    void checkQuotaUsage() {
        const WispResourceQuota& quota = api->getQuota();
        
        print("=== Quota Usage Report ===");
        print("Entities: " + String(quota.currentEntities) + "/" + String(quota.maxEntities));
        print("Sprites: " + String(quota.currentSprites) + "/" + String(quota.maxSprites));
        print("Memory: " + String(quota.currentMemoryUsage) + "/" + String(quota.maxMemoryUsage) + " bytes");
        print("=========================");
    }
    
    void checkQuotaWarnings() {
        const WispResourceQuota& quota = api->getQuota();
        
        // Show warning if approaching limits
        if (!quotaWarningShown && (quota.isEntityUsageHigh() || quota.isMemoryUsageHigh())) {
            warning("Approaching quota limits - reducing features");
            quotaWarningShown = true;
            
            // Reduce quality to stay within limits
            if (enemies.size() > 4) {
                for (int i = 0; i < 2; i++) {
                    api->destroyEntity(enemies.back());
                    enemies.pop_back();
                }
            }
        }
    }
};

// Register the app for the engine
WISP_REGISTER_APP(RestrictedPlatformerGame)
