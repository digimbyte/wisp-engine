// examples/platformer_game_lazy.cpp
// Complete example of a platformer game using lazy loading system

#include "../src/engine/game_loop_manager.h"
#include "../src/engine/lazy_resource_manager.h"
#include "../src/engine/wisp_app_interface.h"
#include "../graphics_engine.h"

// Resource IDs for our platformer game
enum PlatformerResources {
    // Sprites
    SPRITE_PLAYER_IDLE = 1,
    SPRITE_PLAYER_RUN = 2,
    SPRITE_PLAYER_JUMP = 3,
    SPRITE_ENEMY_GOOMBA = 10,
    SPRITE_ENEMY_KOOPA = 11,
    SPRITE_TILESET_GRASS = 20,
    SPRITE_TILESET_STONE = 21,
    SPRITE_TILESET_PIPES = 22,
    
    // Audio
    AUDIO_JUMP = 100,
    AUDIO_COIN = 101,
    AUDIO_POWERUP = 102,
    AUDIO_BGM_OVERWORLD = 110,
    
    // Level data
    LEVEL_WORLD_1_1 = 200,
    LEVEL_WORLD_1_2 = 201
};

// Chunk IDs for World 1-1 (each chunk is 320x240 pixels = 1 screen)
enum World1_1_Chunks {
    CHUNK_1_1_START = 1000,     // x:0-319, y:0-239
    CHUNK_1_1_PIPE1 = 1001,     // x:320-639, y:0-239
    CHUNK_1_1_UNDERGROUND = 1002, // x:640-959, y:0-239
    CHUNK_1_1_CASTLE = 1003     // x:960-1279, y:0-239
};

// Game entities for platformer
struct PlatformerEntity {
    uint16_t spriteId;
    float x, y;
    float vx, vy;       // Velocity
    bool onGround;
    uint8_t health;
    uint8_t entityType; // 0=player, 1=enemy, 2=powerup, 3=coin
    uint8_t frame;      // Current animation frame
    uint32_t animTimer; // Animation timing
};

// Platformer game implementation using lazy loading
class PlatformerGame : public WispAppBase {
private:
    LazyResourceManager* resourceManager;
    GameLoopManager* gameLoop;
    GraphicsEngine* graphics;
    
    // Game state
    static const int MAX_ENTITIES = 100;
    PlatformerEntity player;
    PlatformerEntity entities[MAX_ENTITIES];
    int entityCount;
    
    // Camera system
    float cameraX, cameraY;
    float cameraTargetX, cameraTargetY;
    
    // Input state
    bool leftPressed, rightPressed, jumpPressed;
    
    // Level data
    uint16_t currentLevel;
    int16_t levelWidth, levelHeight;
    
    // Performance monitoring
    uint32_t lastEntityUpdate;
    uint32_t entityUpdateInterval;
    
public:
    PlatformerGame(LazyResourceManager* resMgr, GameLoopManager* loop, GraphicsEngine* gfx) :
        resourceManager(resMgr), gameLoop(loop), graphics(gfx),
        cameraX(0), cameraY(0), cameraTargetX(0), cameraTargetY(0),
        leftPressed(false), rightPressed(false), jumpPressed(false),
        currentLevel(0), levelWidth(1280), levelHeight(240),
        lastEntityUpdate(0), entityUpdateInterval(16), entityCount(0) {} // 60fps entity updates
    
    bool init() override {
        Serial.println("Initializing Platformer Game...");
        
        // Register all game resources
        if (!registerGameResources()) {
            Serial.println("ERROR: Failed to register game resources");
            return false;
        }
        
        // Initialize player
        player.spriteId = SPRITE_PLAYER_IDLE;
        player.x = 32;  // Start position
        player.y = 200;
        player.vx = 0;
        player.vy = 0;
        player.onGround = false;
        player.health = 1;
        player.entityType = 0; // Player
        player.frame = 0;
        player.animTimer = 0;
        
        // Set camera to follow player
        cameraTargetX = player.x - 160; // Center player on screen
        cameraTargetY = player.y - 120;
        cameraX = cameraTargetX;
        cameraY = cameraTargetY;
        
        // Load initial level
        if (!loadLevel(LEVEL_WORLD_1_1)) {
            Serial.println("ERROR: Failed to load initial level");
            return false;
        }
        
        Serial.println("Platformer Game initialized successfully");
        return true;
    }
    
    void update() override {
        uint32_t currentTime = millis();
        
        // Update input (simulate input for demo)
        updateInput();
        
        // Update entities at controlled rate
        if (currentTime - lastEntityUpdate >= entityUpdateInterval) {
            updateEntities();
            lastEntityUpdate = currentTime;
        }
        
        // Update camera
        updateCamera();
        
        // Update resource manager with player position
        gameLoop->updatePlayerPosition((int16_t)player.x, (int16_t)player.y);
        
        // Handle entity spawning/despawning based on camera
        manageEntityLoading();
    }
    
    void render() override {
        // Clear screen
        graphics->clearBuffers(0x001F); // Dark blue sky
        
        // Render visible chunks
        renderVisibleChunks();
        
        // Render entities
        renderEntities();
        
        // Render UI
        renderUI();
        
        // Present frame
        graphics->present();
    }
    
    void cleanup() override {
        Serial.println("Cleaning up Platformer Game");
        entityCount = 0; 
    }
    
private:
    bool registerGameResources() {
        bool success = true;
        
        // Register sprites (these would be real file paths in actual implementation)
        success &= resourceManager->registerResource(SPRITE_PLAYER_IDLE, RESOURCE_SPRITE, 
                                                    "/sprites/player_idle.spr", 0, 2048);
        success &= resourceManager->registerResource(SPRITE_PLAYER_RUN, RESOURCE_SPRITE,
                                                    "/sprites/player_run.spr", 0, 4096);
        success &= resourceManager->registerResource(SPRITE_PLAYER_JUMP, RESOURCE_SPRITE,
                                                    "/sprites/player_jump.spr", 0, 2048);
        
        success &= resourceManager->registerResource(SPRITE_ENEMY_GOOMBA, RESOURCE_SPRITE,
                                                    "/sprites/goomba.spr", 0, 1024);
        success &= resourceManager->registerResource(SPRITE_ENEMY_KOOPA, RESOURCE_SPRITE,
                                                    "/sprites/koopa.spr", 0, 2048);
        
        success &= resourceManager->registerResource(SPRITE_TILESET_GRASS, RESOURCE_SPRITE,
                                                    "/tiles/grass_tiles.spr", 0, 8192);
        success &= resourceManager->registerResource(SPRITE_TILESET_STONE, RESOURCE_SPRITE,
                                                    "/tiles/stone_tiles.spr", 0, 8192);
        
        // Register audio
        success &= resourceManager->registerResource(AUDIO_JUMP, RESOURCE_AUDIO,
                                                    "/audio/jump.wav", 0, 4096);
        success &= resourceManager->registerResource(AUDIO_BGM_OVERWORLD, RESOURCE_AUDIO,
                                                    "/audio/bgm_overworld.ogg", 0, 32768);
        
        return success;
    }
    
    bool loadLevel(uint16_t levelId) {
        currentLevel = levelId;
        
        // Register level chunks
        bool success = true;
        
        if (levelId == LEVEL_WORLD_1_1) {
            // Register chunks for World 1-1
            success &= resourceManager->registerLevelChunk(CHUNK_1_1_START, 0, 0, 320, 240);
            success &= resourceManager->registerLevelChunk(CHUNK_1_1_PIPE1, 320, 0, 320, 240);
            success &= resourceManager->registerLevelChunk(CHUNK_1_1_UNDERGROUND, 640, 0, 320, 240);
            success &= resourceManager->registerLevelChunk(CHUNK_1_1_CASTLE, 960, 0, 320, 240);
            
            // Load starting chunk immediately
            success &= resourceManager->loadChunk(CHUNK_1_1_START);
            
            // Preload adjacent chunk
            resourceManager->preloadResource(SPRITE_TILESET_GRASS, 0); // High priority
            resourceManager->preloadResource(SPRITE_PLAYER_RUN, 50);   // Medium priority
        }
        
        return success;
    }
    
    void updateInput() {
        // Simulate platformer input (in real game, read from input controller)
        static uint32_t inputTimer = 0;
        uint32_t currentTime = millis();
        
        if (currentTime - inputTimer > 3000) { // Change input every 3 seconds for demo
            leftPressed = !leftPressed;
            inputTimer = currentTime;
        }
        
        // Simulate jump input occasionally
        if ((currentTime % 5000) < 100) { // Jump for 100ms every 5 seconds
            jumpPressed = true;
        } else {
            jumpPressed = false;
        }
    }
    
    void updateEntities() {
        // Update player physics
        updatePlayerPhysics();
        
        // Update player animation
        updatePlayerAnimation();
        
        // Update other entities
        for (auto& entity : entities) {
            updateEntityPhysics(entity);
        }
        
        // Check collisions
        checkCollisions();
    }
    
    void updatePlayerPhysics() {
        const float GRAVITY = 0.5f;
        const float MOVE_SPEED = 2.0f;
        const float JUMP_POWER = -8.0f;
        
        // Horizontal movement
        if (leftPressed && !rightPressed) {
            player.vx = -MOVE_SPEED;
        } else if (rightPressed && !leftPressed) {
            player.vx = MOVE_SPEED;
        } else {
            player.vx *= 0.8f; // Friction
        }
        
        // Jumping
        if (jumpPressed && player.onGround) {
            player.vy = JUMP_POWER;
            player.onGround = false;
            // TODO: Play jump sound
        }
        
        // Apply gravity
        if (!player.onGround) {
            player.vy += GRAVITY;
        }
        
        // Apply velocity
        player.x += player.vx;
        player.y += player.vy;
        
        // Simple ground collision (at y=200)
        if (player.y >= 200) {
            player.y = 200;
            player.vy = 0;
            player.onGround = true;
        }
        
        // Keep player in level bounds
        if (player.x < 0) player.x = 0;
        if (player.x > levelWidth - 16) player.x = levelWidth - 16;
    }
    
    void updatePlayerAnimation() {
        uint32_t currentTime = millis();
        
        // Choose sprite based on state
        uint16_t targetSprite = SPRITE_PLAYER_IDLE;
        
        if (!player.onGround) {
            targetSprite = SPRITE_PLAYER_JUMP;
        } else if (abs(player.vx) > 0.1f) {
            targetSprite = SPRITE_PLAYER_RUN;
        }
        
        // Change sprite if needed
        if (player.spriteId != targetSprite) {
            player.spriteId = targetSprite;
            player.frame = 0;
            player.animTimer = currentTime;
        }
        
        // Animate frames
        if (currentTime - player.animTimer > 150) { // 150ms per frame
            player.frame = (player.frame + 1) % 4; // Assume 4 frames per animation
            player.animTimer = currentTime;
        }
    }
    
    void updateEntityPhysics(PlatformerEntity& entity) {
        // Simple AI for enemies
        if (entity.entityType == 1) { // Enemy
            entity.vx = -1.0f; // Move left
            entity.x += entity.vx;
            
            // Simple ground collision
            if (entity.y >= 200) {
                entity.y = 200;
                entity.onGround = true;
            }
        }
    }
    
    void checkCollisions() {
        // Simple collision between player and enemies
        for (auto& entity : entities) {
            if (entity.entityType == 1) { // Enemy
                float dx = player.x - entity.x;
                float dy = player.y - entity.y;
                float distance = sqrt(dx*dx + dy*dy);
                
                if (distance < 16) { // Collision
                    // Handle collision (damage player, etc.)
                    Serial.println("Player hit enemy!");
                }
            }
        }
    }
    
    void updateCamera() {
        // Smooth camera following
        const float CAMERA_SPEED = 0.1f;
        
        cameraTargetX = player.x - 160; // Center player
        cameraTargetY = player.y - 120;
        
        // Clamp camera to level bounds
        if (cameraTargetX < 0) cameraTargetX = 0;
        if (cameraTargetX > levelWidth - 320) cameraTargetX = levelWidth - 320;
        if (cameraTargetY < 0) cameraTargetY = 0;
        if (cameraTargetY > levelHeight - 240) cameraTargetY = levelHeight - 240;
        
        // Smooth movement
        cameraX += (cameraTargetX - cameraX) * CAMERA_SPEED;
        cameraY += (cameraTargetY - cameraY) * CAMERA_SPEED;
    }
    
    void manageEntityLoading() {
        // Spawn/despawn entities based on camera position
        // This is where the lazy loading really shines - only entities
        // near the camera are active in memory
        
        float leftEdge = cameraX - 64;   // Load entities 64 pixels off-screen
        float rightEdge = cameraX + 320 + 64;
        
        // Remove entities that are too far away using manual C-style loop
        int writeIndex = 0;
        for (int readIndex = 0; readIndex < entityCount; readIndex++) {
            if (!(entities[readIndex].x < leftEdge || entities[readIndex].x > rightEdge)) {
                if (writeIndex != readIndex) {
                    entities[writeIndex] = entities[readIndex];
                }
                writeIndex++;
            }
        }
        entityCount = writeIndex;
        
        // Add entities that should be visible
        // (In real game, this would load from chunk data)
        static bool spawnedTestEnemy = false;
        if (!spawnedTestEnemy && cameraX > 200) {
            PlatformerEntity enemy;
            enemy.spriteId = SPRITE_ENEMY_GOOMBA;
            enemy.x = 400;
            enemy.y = 200;
            enemy.vx = -1;
            enemy.vy = 0;
            enemy.onGround = true;
            enemy.health = 1;
            enemy.entityType = 1; // Enemy
            enemy.frame = 0;
            enemy.animTimer = 0;
            
            entities.push_back(enemy);
            spawnedTestEnemy = true;
            
            Serial.println("Spawned enemy at x=400");
        }
    }
    
    void renderVisibleChunks() {
        // Determine which chunks are visible
        uint16_t startChunkX = (uint16_t)(cameraX / 320);
        uint16_t endChunkX = (uint16_t)((cameraX + 320) / 320);
        
        // Render each visible chunk
        for (uint16_t chunkX = startChunkX; chunkX <= endChunkX; chunkX++) {
            uint16_t chunkId = CHUNK_1_1_START + chunkX; // Simple mapping for demo
            
            LevelChunk* chunk = resourceManager->getChunk(chunkId);
            if (chunk && chunk->loaded) {
                renderChunk(chunk, chunkX * 320, 0);
            } else {
                // Chunk not loaded - render placeholder or loading indicator
                graphics->drawRect((chunkX * 320) - (int16_t)cameraX, 
                                  0 - (int16_t)cameraY,
                                  320, 240, 0xF800, 10); // Red placeholder
            }
        }
    }
    
    void renderChunk(LevelChunk* chunk, int16_t worldX, int16_t worldY) {
        // Render background tiles
        uint16_t* tilesetSprite = (uint16_t*)resourceManager->getResource(SPRITE_TILESET_GRASS);
        
        if (tilesetSprite) {
            // Simple tile rendering (16x16 tiles)
            for (int ty = 0; ty < 15; ty++) { // 240 / 16 = 15 tiles high
                for (int tx = 0; tx < 20; tx++) { // 320 / 16 = 20 tiles wide
                    int16_t screenX = worldX + tx * 16 - (int16_t)cameraX;
                    int16_t screenY = worldY + ty * 16 - (int16_t)cameraY;
                    
                    // Only render if on screen
                    if (screenX > -16 && screenX < 320 && screenY > -16 && screenY < 240) {
                        // Simple grass pattern
                        if (ty >= 13) { // Ground level
                            graphics->drawRect(screenX, screenY, 16, 16, 0x07E0, 5); // Green ground
                        }
                    }
                }
            }
        }
    }
    
    void renderEntities() {
        // Render player
        renderEntity(player);
        
        // Render other entities
        for (const auto& entity : entities) {
            renderEntity(entity);
        }
    }
    
    void renderEntity(const PlatformerEntity& entity) {
        int16_t screenX = (int16_t)entity.x - (int16_t)cameraX;
        int16_t screenY = (int16_t)entity.y - (int16_t)cameraY;
        
        // Only render if on screen
        if (screenX > -32 && screenX < 320 && screenY > -32 && screenY < 240) {
            void* spriteData = resourceManager->getResource(entity.spriteId);
            
            if (spriteData) {
                // In real implementation, this would use graphics->drawSpriteFrame()
                // For demo, just draw colored rectangles
                uint16_t color = 0xFFFF; // White
                if (entity.entityType == 1) color = 0xF800; // Red for enemies
                
                graphics->drawRect(screenX, screenY, 16, 16, color, 3);
            } else {
                // Sprite not loaded - draw placeholder
                graphics->drawRect(screenX, screenY, 16, 16, 0x07FF, 8); // Cyan placeholder
            }
        }
    }
    
    void renderUI() {
        // Render HUD elements
        graphics->drawRect(10, 10, 100, 20, 0x0000, 0); // Health bar background
        graphics->drawRect(10, 10, player.health * 100, 20, 0xF800, 0); // Health bar
        
        // Render debug info if needed
        if (gameLoop->getMetrics().memoryPressure > 80) {
            graphics->drawRect(220, 10, 80, 20, 0xF800, 0); // Memory warning
        }
    }
};

// Usage example in main.cpp
void setupPlatformerGame() {
    static LazyResourceManager resourceManager;
    static GraphicsEngine graphics;
    static GameLoopManager gameLoop(&resourceManager, &graphics);
    static PlatformerGame game(&resourceManager, &gameLoop, &graphics);
    
    // Set memory budget appropriate for ESP32
    resourceManager.setMemoryBudget(96 * 1024); // 96KB for resources
    
    // Configure performance settings
    gameLoop.setTargetFPS(60.0f);
    gameLoop.setLoadStrategy(LOAD_ADJACENT);
    gameLoop.setAdaptiveLoading(true);
    gameLoop.setPerformanceBudget(8000); // 8ms per frame for loading
    
    // Load the game
    if (gameLoop.loadLevel(LEVEL_WORLD_1_1, &game)) {
        Serial.println("Platformer game loaded successfully!");
        
        // Main game loop
        while (true) {
            gameLoop.tick();
            
            // Print performance stats every 5 seconds
            static uint32_t lastStats = 0;
            if (millis() - lastStats > 5000) {
                gameLoop.printPerformanceStats();
                resourceManager.printMemoryStatus();
                lastStats = millis();
            }
            
            delay(1); // Small delay to prevent watchdog issues
        }
    } else {
        Serial.println("ERROR: Failed to load platformer game");
    }
}
