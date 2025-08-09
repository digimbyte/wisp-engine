# API Comparison: Current Test Apps vs New Component Design

## Overview

This document compares the existing test applications with our new component-based API design to identify gaps, improvements, and alignment issues.

## Current API Analysis

### ✅ **What's Already Available (WispCuratedAPI)**

**Graphics System:**
- `loadSprite()`, `unloadSprite()`, `drawSprite()`
- `drawSpriteFrame()`, `drawSpriteScaled()`, `drawSpriteRotated()`
- `drawRect()`, `drawCircle()`, `drawLine()`, `drawText()`
- Camera management, screen utilities

**Entity System:**
- `createEntity()`, `destroyEntity()`, `isEntityValid()`
- `setEntityPosition()`, `getEntityPosition()`
- `setEntityVelocity()`, `getEntityVelocity()`
- `setEntitySprite()`, `setEntityAnimation()`

**Audio System:**
- `loadAudio()`, `unloadAudio()`, `playAudio()`
- `stopAudio()`, `setMasterVolume()`

**Save System:**
- Comprehensive field registration system
- Multiple data type support
- Auto-save functionality

**Utilities:**
- Timer system, collision detection
- Math utilities, random functions

## Comparison Analysis

### 1. Sprite Test App

#### **Current Implementation Issues:**
```cpp
// Current - Direct API calls, no component system
api->drawSprite(testSprites[sprite.spriteIndex], sprite.x, sprite.y, sprite.depth);

// Missing rotation support (claimed but not implemented)
rotation += 2.0f;  // Variable exists but not used
```

#### **New Component Design:**
```cpp
// Component-based approach
SpriteComponent* sprite = CREATE_SPRITE(entityId);
sprite->setSprite(SPRITE_TEST);
sprite->setPosition(x, y);
sprite->playAnimation(ANIM_MOVE);
sprite->setFlip(flipX, flipY);  // Better than rotation for retro feel
```

#### **Key Differences:**
- ❌ **Current**: Manual position tracking in app code
- ✅ **New**: Components handle state automatically
- ❌ **Current**: No animation system
- ✅ **New**: Built-in animation with callbacks
- ❌ **Current**: No physics integration
- ✅ **New**: Physics component available

### 2. Save Test App  

#### **Current Implementation Strengths:**
```cpp
// Comprehensive save field registration - GOOD!
api->registerSaveField("playerName", saveData.playerName);
api->registerSaveField("playTime", saveData.playTime);
api->registerSaveField("gymBadges", saveData.gymBadges);
```

#### **New Component Design Alignment:**
```cpp
// DataComponent provides same functionality but cleaner
DataComponent* data = CREATE_DATA(entityId);
data->setString("playerName", "Hero", true);  // persistent=true
data->setInt32("playTime", 3600, true);
data->setBool("gymBadge_1", true, true);
```

#### **Key Differences:**
- ✅ **Current**: Already excellent save system
- ✅ **New**: Integrates save with component system
- ✅ **Current**: Multiple data types supported
- ✅ **New**: Same support plus translation
- ➕ **New**: Auto-save timers built into DataComponent

### 3. Database Test App

#### **Current Implementation Issues:**
```cpp
// Complex database API that apps probably shouldn't access directly
pokemonDB = api->createDatabase("test_pokemon.wdb");
api->registerField(pokemonDB, "id", FIELD_UINT32, true);
```

#### **New Component Design:**
```cpp
// Simpler, data-focused approach
DataComponent* pokemonData = CREATE_DATA(pokemonEntityId);
pokemonData->setString("name", "Pikachu", true);
pokemonData->setInt32("level", 25, true);
pokemonData->setString("type", "Electric", true);
```

#### **Key Differences:**
- ❌ **Current**: Complex database operations exposed to apps
- ✅ **New**: Simple key-value storage focused on app needs
- ❌ **Current**: Apps need to manage database schemas
- ✅ **New**: Components handle data structure automatically

## Major Gaps in Current API

### 1. **Missing Physics System**
**Current State:** Apps manually handle movement and collision
```cpp
// Apps do this manually
sprites[i].x += sprites[i].dx;
sprites[i].y += sprites[i].dy;
if (sprites[i].x < 0 || sprites[i].x > 300) sprites[i].dx = -sprites[i].dx;
```

**New Design:** Physics component handles automatically
```cpp
PhysicsComponent* physics = CREATE_PHYSICS(entityId);
physics->setVelocity(velocityX, velocityY);
physics->setBounce(800);  // Automatic edge bouncing
```

### 2. **Missing Timer System**
**Current State:** Apps manually track timing
```cpp
// Manual timing in apps
static uint32_t lastTimeUpdate = currentTime;
if (currentTime - lastTimeUpdate >= 1000) {
    saveData.playTime++;
    lastTimeUpdate = currentTime;
}
```

**New Design:** Timer component handles timing
```cpp
TimerComponent* timer = CREATE_TIMER(entityId, 1);
timer->start(TIMER_REPEATING, 1000);
timer->setCompleteCallback(updatePlayTime);
```

### 3. **Missing Animation System**  
**Current State:** Manual frame management
```cpp
// Apps handle animation manually
rotation += 2.0f;
if (rotation >= 360.0f) rotation -= 360.0f;
```

**New Design:** Built-in animation support
```cpp
sprite->playAnimation(ANIM_MOVE, true);  // looping
sprite->setAnimationSpeed(200);  // 200ms per frame
```

## API Alignment Issues

### 1. **Entity System Mismatch**

**Current API Design:**
```cpp
// Current - Entity system exists but underutilized
EntityHandle entity = api->createEntity();
api->setEntityPosition(entity, position);
api->setEntitySprite(entity, sprite);
```

**Test Apps Reality:**
```cpp  
// Apps ignore entity system, manage data manually
struct SpriteTest {
    float x, y, dx, dy;
    uint8_t spriteIndex;
    uint8_t depth;
};
SpriteTest sprites[16];  // Manual management
```

**Recommendation:** Bridge entity system with components

### 2. **Resource Management Inconsistency**

**Current API:** Resource handles for sprites/audio
**New Design:** Component system manages resources internally
**Issue:** Need unified approach to resource lifetime

### 3. **Input Handling Pattern**

**Current Pattern (Good):**
```cpp
const WispInputState& input = api->getInput();
static WispInputState lastInput;
if (input.buttonA && !lastInput.buttonA) {
    // Button press handling
}
```

**New Design:** Same pattern works well with components

## Recommendations for API Alignment

### 1. **Extend Current API (Don't Replace)**
```cpp
// Add component creation to existing API
class WispCuratedAPI {
    // Keep all existing methods...
    
    // Add component system access
    SpriteComponent* createSpriteComponent(EntityHandle entity);
    PhysicsComponent* createPhysicsComponent(EntityHandle entity);
    TimerComponent* createTimerComponent(EntityHandle entity, uint16_t timerId);
    DataComponent* createDataComponent(EntityHandle entity);
};
```

### 2. **Bridge Entity System with Components**
```cpp
// Existing entities become component containers
EntityHandle player = api->createEntity();
SpriteComponent* playerSprite = api->createSpriteComponent(player);
PhysicsComponent* playerPhysics = api->createPhysicsComponent(player);
DataComponent* playerData = api->createDataComponent(player);
```

### 3. **Maintain Backward Compatibility**
```cpp
// Old way still works
api->drawSprite(sprite, x, y, depth);

// New way provides more features  
SpriteComponent* spriteComp = api->createSpriteComponent(entity);
spriteComp->setSprite(sprite);
spriteComp->setPosition(x, y);
spriteComp->playAnimation(ANIM_IDLE);
```

## Updated Example: Sprite Test with Components

```cpp
class ImprovedSpriteTestApp : public WispAppBase {
private:
    EntityHandle testEntities[16];
    SpriteComponent* sprites[16];
    PhysicsComponent* physics[16];
    TimerComponent* animTimer;
    uint8_t activeEntities = 0;
    
public:
    bool init() override {
        // Create animation timer
        EntityHandle timerEntity = api->createEntity();
        animTimer = api->createTimerComponent(timerEntity, 1);
        animTimer->start(TIMER_REPEATING, 100);
        animTimer->setCompleteCallback([](uint16_t entity, uint16_t timer) {
            // Update all sprite animations
            updateAllAnimations();
        });
        
        // Create initial sprites using components
        for (int i = 0; i < 8; i++) {
            createMovingSprite(
                api->random(50, 270), api->random(50, 190),
                api->random(-2.0f, 2.0f), api->random(-2.0f, 2.0f)
            );
        }
        
        return true;
    }
    
    void createMovingSprite(float x, float y, float vx, float vy) {
        if (activeEntities >= 16) return;
        
        // Create entity
        testEntities[activeEntities] = api->createEntity();
        
        // Add sprite component
        sprites[activeEntities] = api->createSpriteComponent(testEntities[activeEntities]);
        sprites[activeEntities]->setSprite(SPRITE_TEST);
        sprites[activeEntities]->setPosition(x * 65536, y * 65536);  // Fixed-point
        sprites[activeEntities]->playAnimation(ANIM_MOVE, true);
        
        // Add physics component
        physics[activeEntities] = api->createPhysicsComponent(testEntities[activeEntities]);
        physics[activeEntities]->setBodyType(BODY_KINEMATIC);
        physics[activeEntities]->setVelocity(vx * 65536, vy * 65536);  // Fixed-point
        physics[activeEntities]->setBounce(1000);  // Full bounce
        
        // Set up screen boundary collision
        physics[activeEntities]->setCollisionEnterCallback([](uint16_t entity, uint16_t other, CollisionResponse response) {
            // Bounce off screen edges automatically handled by physics component
        });
        
        activeEntities++;
    }
    
    void update() override {
        // Input handling - same as before
        const WispInputState& input = api->getInput();
        static WispInputState lastInput;
        
        if (input.buttonA && !lastInput.buttonA) {
            createMovingSprite(160, 120, 
                              api->random(-3.0f, 3.0f), 
                              api->random(-3.0f, 3.0f));
        }
        
        // Components handle movement automatically - no manual position updates!
        // Physics component updates positions
        // Sprite component handles animation timing
        // Timer component triggers animation updates
        
        lastInput = input;
    }
    
    void render() override {
        // Background
        api->drawRect(0, 0, 320, 240, WispColor(20, 20, 40), 0);
        
        // Components render automatically when their entities are processed
        // Or we can still manually draw if needed for special effects
        
        api->drawText("IMPROVED SPRITE TEST", 160, 10, WispColor(255, 255, 255), 10);
        api->drawText("A: Add Sprite  B: Remove Sprite", 10, 220, WispColor(200, 200, 200), 8);
    }
};
```

## Conclusion

**Strengths of Current API:**
- ✅ Comprehensive save system
- ✅ Good resource management
- ✅ Solid input handling
- ✅ Reasonable graphics API

**Major Missing Pieces:**
- ❌ No physics system
- ❌ No timer system  
- ❌ No animation system
- ❌ Component architecture not utilized

**Recommendation:**
Extend the current `WispCuratedAPI` with component system access while maintaining full backward compatibility. This gives developers choice between simple direct calls and more powerful component-based development.
