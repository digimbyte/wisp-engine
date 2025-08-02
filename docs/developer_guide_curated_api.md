# Wisp Engine Developer Guide: Curated API

## Overview

The Wisp Engine uses a **curated, quota-limited API** to ensure games run reliably on ESP32 hardware. This prevents system crashes, memory exhaustion, and performance issues by enforcing strict limits on what apps can do.

## Why Restrictions?

**ESP32 has limited resources:**
- 320KB RAM total
- 240MHz single-core processing
- No virtual memory or swap

**Without limits, games could:**
- Allocate too much memory → system crash
- Create too many entities → frame drops
- Make too many draw calls → stuttering
- Infinite loops → watchdog reset
- File system corruption → device brick

## Core Philosophy

**Apps get curated access, not full system access:**
- ✅ **Allowed**: Pre-approved, safe operations through WispCuratedAPI
- ❌ **Blocked**: Direct hardware access, unlimited allocations, system calls
- 🛡️ **Protected**: File system, memory management, hardware GPIO

## Quota System

### Entity Limits
```cpp
#define WISP_MAX_ENTITIES           64      // Total entities per app
#define WISP_MAX_SPRITES            32      // Sprites loaded simultaneously  
#define WISP_MAX_AUDIO_CHANNELS     4       // Concurrent audio streams
#define WISP_MAX_PARTICLES          128     // Particles in system
```

### Memory Limits
```cpp
#define WISP_MAX_APP_MEMORY         (64 * 1024)    // 64KB for app data
#define WISP_MAX_SPRITE_SIZE        (8 * 1024)     // 8KB per sprite
#define WISP_MAX_AUDIO_SIZE         (16 * 1024)    // 16KB per audio file
```

### Performance Limits
```cpp
#define WISP_MAX_FRAME_TIME_US      16667   // Must complete in 16.67ms (60fps)
#define WISP_MAX_DRAW_CALLS         256     // Max draw operations per frame
#define WISP_MAX_COLLISION_CHECKS   512     // Max collision checks per frame
```

## Game Development Pattern

### 1. App Structure
```cpp
class MyGame : public WispAppBase {
private:
    // Game state
    EntityHandle player;
    ResourceHandle playerSprite;
    std::vector<EntityHandle> enemies;
    
public:
    bool init() override {
        // Load resources (quota-limited)
        playerSprite = api->loadSprite("/sprites/player.spr");
        if (playerSprite == INVALID_RESOURCE) {
            error("Failed to load player sprite");
            return false;
        }
        
        // Create entities (quota-limited)
        player = api->createEntity();
        if (player == INVALID_ENTITY) {
            error("Failed to create player - quota exceeded");
            return false;
        }
        
        return true;
    }
    
    void update() override {
        // Game logic (performance-limited)
        handleInput();
        updatePlayer();
        updateEnemies();
        checkCollisions(); // Quota-limited
    }
    
    void render() override {
        // Drawing (quota-limited)
        api->drawSprite(playerSprite, playerX, playerY);
        renderUI();
    }
    
    void cleanup() override {
        // Free resources (required for quota)
        api->destroyEntity(player);
        api->unloadSprite(playerSprite);
    }
};

WISP_REGISTER_APP(MyGame) // Export for engine
```

### 2. Resource Management
```cpp
// Loading sprites (quota-limited)
ResourceHandle sprite = api->loadSprite("/sprites/hero.spr");
if (sprite == INVALID_RESOURCE) {
    // Handle failure gracefully
    warning("Failed to load sprite - quota exceeded?");
    return false;
}

// Always unload when done
api->unloadSprite(sprite);
```

### 3. Entity Management
```cpp
// Creating entities (quota-limited)
EntityHandle enemy = api->createEntity();
if (enemy == INVALID_ENTITY) {
    // Quota exceeded - clean up or skip
    warning("Cannot create enemy - quota full");
    return;
}

// Set up entity
api->setEntityPosition(enemy, WispVec2(100, 200));
api->setEntitySprite(enemy, enemySprite);

// Always destroy when done
api->destroyEntity(enemy);
```

### 4. Performance-Conscious Rendering
```cpp
void render() override {
    // Check quota before expensive operations
    const WispResourceQuota& quota = api->getQuota();
    
    if (quota.getDrawCallUsage() > 0.8f) {
        // Near draw call limit - reduce quality
        renderSimpleBackground();
    } else {
        // Safe to do detailed rendering
        renderDetailedBackground();
    }
    
    // Quota-safe drawing
    if (!api->drawSprite(sprite, x, y)) {
        // Draw quota exceeded - stop drawing
        return;
    }
}
```

## API Reference

### Core API Access
```cpp
class WispAppBase {
protected:
    WispCuratedAPI* api;    // Your ONLY access to engine features
    
    // Helper shortcuts
    const WispInputState& input() const;
    uint32_t time() const;
    uint32_t deltaTime() const;
    void print(const String& msg);
    void error(const String& msg);
};
```

### Graphics API
```cpp
// Resource loading (quota-limited)
ResourceHandle loadSprite(const String& filePath);
void unloadSprite(ResourceHandle handle);

// Drawing (quota-limited)
bool drawSprite(ResourceHandle sprite, float x, float y, uint8_t depth = 5);
bool drawSpriteFrame(ResourceHandle sprite, float x, float y, uint8_t frame, uint8_t depth = 5);
bool drawRect(float x, float y, float width, float height, WispColor color, uint8_t depth = 5);
bool drawText(const String& text, float x, float y, WispColor color, uint8_t depth = 5);

// Camera control
void setCameraPosition(float x, float y);
WispVec2 getCameraPosition() const;
```

### Entity System
```cpp
// Entity management (quota-limited)
EntityHandle createEntity();
void destroyEntity(EntityHandle entity);
bool isEntityValid(EntityHandle entity);

// Entity properties
void setEntityPosition(EntityHandle entity, WispVec2 position);
WispVec2 getEntityPosition(EntityHandle entity);
void setEntitySprite(EntityHandle entity, ResourceHandle sprite);

// Collision detection (quota-limited)
WispCollision checkCollision(EntityHandle entity1, EntityHandle entity2);
std::vector<EntityHandle> getEntitiesInRadius(WispVec2 center, float radius);
```

### Audio System
```cpp
// Audio resources (quota-limited)
ResourceHandle loadAudio(const String& filePath);
bool playAudio(ResourceHandle audio, const WispAudioParams& params = WispAudioParams());
void stopAudio(ResourceHandle audio);
void setMasterVolume(float volume);
```

### Input System
```cpp
// Input state (read-only)
const WispInputState& getInput() const;
bool isKeyPressed(uint8_t key);
bool isKeyJustPressed(uint8_t key);
bool isKeyJustReleased(uint8_t key);

struct WispInputState {
    bool left, right, up, down;
    bool buttonA, buttonB, buttonC;
    bool select, start;
    int16_t analogX, analogY;  // -100 to +100
    bool touched;
    int16_t touchX, touchY;
};
```

## Quota Monitoring

### Check Current Usage
```cpp
const WispResourceQuota& quota = api->getQuota();

print("Entities: " + String(quota.currentEntities) + "/" + String(quota.maxEntities));
print("Memory: " + String(quota.currentMemoryUsage) + "/" + String(quota.maxMemoryUsage));

// Check if approaching limits
if (quota.isEntityUsageHigh()) {
    warning("Entity quota at 80% - consider cleanup");
}
```

### Graceful Degradation
```cpp
void update() override {
    // Try to spawn enemy
    if (enemies.size() < 8) { // Self-imposed limit
        EntityHandle enemy = api->createEntity();
        if (enemy != INVALID_ENTITY) {
            enemies.push_back(enemy);
        } else {
            // Quota exceeded - clean up old enemies
            if (!enemies.empty()) {
                api->destroyEntity(enemies.front());
                enemies.erase(enemies.begin());
            }
        }
    }
}
```

## Error Handling

### Required Error Handling
```cpp
bool init() override {
    // Always check for failures
    ResourceHandle sprite = api->loadSprite("/sprites/player.spr");
    if (sprite == INVALID_RESOURCE) {
        error("Critical resource failed to load");
        return false; // Init failure
    }
    
    EntityHandle player = api->createEntity();
    if (player == INVALID_ENTITY) {
        error("Failed to create player entity");
        return false;
    }
    
    return true;
}

void onError(const String& error) override {
    print("Game error: " + error);
    
    // Attempt recovery
    if (error.indexOf("quota") >= 0) {
        // Quota exceeded - reduce quality
        reduceGameQuality();
    }
}

void onLowMemory() override {
    // Emergency cleanup
    cleanupNonEssentialEntities();
    reduceAudioQuality();
}
```

## Best Practices

### 1. Design for Constraints
- **Plan entity count**: Design levels knowing you can only have 64 entities
- **Optimize sprites**: Keep sprites under 8KB each
- **Limit audio**: Use compressed formats, limit concurrent streams
- **Pool resources**: Reuse entities instead of creating/destroying constantly

### 2. Graceful Degradation
```cpp
// Good: Adaptive quality
if (quota.isMemoryUsageHigh()) {
    useSimpleSprites();
    reduceParticleCount();
} else {
    useDetailedSprites();
    enableFullParticles();
}

// Bad: Ignoring limits
for (int i = 0; i < 1000; i++) {
    api->createEntity(); // Will fail after 64!
}
```

### 3. Performance Awareness
```cpp
void update() override {
    uint32_t startTime = api->getTime();
    
    // Do game logic
    updateGameLogic();
    
    uint32_t elapsed = api->getTime() - startTime;
    if (elapsed > 8) { // 8ms budget
        warning("Update took too long: " + String(elapsed) + "ms");
    }
}
```

### 4. Resource Lifecycle
```cpp
class MyGame : public WispAppBase {
private:
    ResourceHandle sprites[10];
    EntityHandle entities[20];
    
public:
    bool init() override {
        // Load all resources at start
        for (int i = 0; i < 10; i++) {
            sprites[i] = api->loadSprite("/sprites/sprite" + String(i) + ".spr");
        }
        
        // Create entity pool
        for (int i = 0; i < 20; i++) {
            entities[i] = api->createEntity();
        }
        
        return true;
    }
    
    void cleanup() override {
        // Clean up everything
        for (int i = 0; i < 10; i++) {
            api->unloadSprite(sprites[i]);
        }
        
        for (int i = 0; i < 20; i++) {
            api->destroyEntity(entities[i]);
        }
    }
};
```

## What You CAN'T Do

### Blocked Operations
- **Direct hardware access**: No GPIO, SPI, I2C manipulation
- **File system writes**: Can't corrupt the system
- **System calls**: No `system()`, `exec()`, etc.
- **Unlimited allocation**: No `malloc()` without quota checks
- **Direct memory access**: No pointers to engine internals
- **Network access**: Must request permission first

### Quota Violations
```cpp
// This will fail after 64 entities
for (int i = 0; i < 1000; i++) {
    api->createEntity();
}

// This will fail after 256 draw calls per frame
for (int i = 0; i < 1000; i++) {
    api->drawSprite(sprite, i, 0);
}

// This will fail if total memory exceeds 64KB
api->loadSprite("/huge_10mb_sprite.spr");
```

## Emergency Mode

If your app violates quotas too often, the engine enters **Emergency Mode**:
- App update/render calls are blocked
- Only cleanup is allowed
- System displays error message
- App must be restarted

### Avoiding Emergency Mode
```cpp
// Always check return values
if (!api->drawSprite(sprite, x, y)) {
    // Draw quota exceeded - stop drawing this frame
    return;
}

// Monitor error rate
void onError(const String& error) override {
    errorCount++;
    if (errorCount > 5) {
        // Too many errors - go into safe mode
        enableSafeMode();
    }
}
```

## Conclusion

The curated API ensures **your games run reliably** on ESP32 hardware by:
- **Preventing crashes** through quota enforcement
- **Maintaining performance** through frame time limits  
- **Ensuring fairness** by limiting resource usage
- **Enabling recovery** through graceful degradation

**Design within the limits and your games will be rock-solid!** 🎮✨
