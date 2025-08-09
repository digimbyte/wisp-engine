# Wisp Engine Application Examples

This document shows how the core component systems support diverse application types using the same fundamental building blocks.

## Example 1: RPG/Pokémon-Style Game

### Core Requirements
- Turn-based or real-time movement
- Character stats and inventory
- Menu systems and dialogues
- Save/load game state
- Translation support

### Implementation Using Components

```cpp
// === RPG Player Entity ===
class RPGPlayer {
private:
    uint16_t entityId;
    SpriteComponent* sprite;
    PhysicsComponent* physics;
    DataComponent* data;
    TimerComponent* moveTimer;
    
public:
    RPGPlayer(uint16_t id) : entityId(id) {
        // Create components
        sprite = CREATE_SPRITE(entityId);
        physics = CREATE_PHYSICS(entityId);
        data = CREATE_DATA(entityId);
        moveTimer = CREATE_TIMER(entityId, 1);
        
        // Setup sprite for 4-direction movement
        sprite->setSprite(SPRITE_PLAYER);
        sprite->setLayer(5);  // Character layer
        
        // Setup grid-based movement physics
        physics->setBodyType(BODY_KINEMATIC);
        physics->setCollisionShape(SHAPE_RECTANGLE, 16, 16);
        physics->enableGravity(false);  // Top-down movement
        
        // Initialize RPG stats
        data->setInt32("level", 1, true);        // Persistent
        data->setInt32("hp", 100, true);
        data->setInt32("maxhp", 100, true);
        data->setInt32("exp", 0, true);
        data->setString("name", "Hero", true);
        
        // Setup movement timer for turn-based feel
        moveTimer->setCompleteCallback([](uint16_t entity, uint16_t timer) {
            // Allow next movement
            RPGPlayer* player = getPlayerById(entity);
            player->canMove = true;
        });
    }
    
    void handleInput(const WispInputState& input) {
        if (!canMove) return;
        
        int32_t moveX = 0, moveY = 0;
        
        // Grid-based 4-direction movement
        if (input.up) {
            moveY = -TILE_SIZE;
            sprite->playAnimation(ANIM_MOVE);
            sprite->setFlip(false, false);
        }
        else if (input.down) {
            moveY = TILE_SIZE;
            sprite->playAnimation(ANIM_MOVE);
            sprite->setFlip(false, false);
        }
        else if (input.left) {
            moveX = -TILE_SIZE;
            sprite->playAnimation(ANIM_MOVE);
            sprite->setFlip(true, false);  // Face left
        }
        else if (input.right) {
            moveX = TILE_SIZE;
            sprite->playAnimation(ANIM_MOVE);
            sprite->setFlip(false, false); // Face right
        }
        
        if (moveX != 0 || moveY != 0) {
            // Start movement
            physics->move(moveX, moveY);
            canMove = false;
            moveTimer->start(TIMER_ONESHOT, 200); // 200ms move time
        } else {
            sprite->playAnimation(ANIM_IDLE);
        }
    }
    
    // RPG-specific functions
    void gainExp(int32_t exp) {
        int32_t currentExp = data->getInt32("exp");
        int32_t newExp = currentExp + exp;
        data->setInt32("exp", newExp, true);
        
        // Check for level up
        if (shouldLevelUp(newExp)) {
            levelUp();
        }
    }
    
    void takeDamage(int32_t damage) {
        int32_t hp = data->getInt32("hp");
        hp = max(0, hp - damage);
        data->setInt32("hp", hp, true);
        
        if (hp <= 0) {
            // Player died
            onPlayerDeath();
        }
    }
    
private:
    bool canMove = true;
};

// === RPG Menu System ===
class RPGMenu {
private:
    DataComponent* menuData;
    TimerComponent* cursorTimer;
    
public:
    RPGMenu() {
        menuData = CREATE_DATA(0);  // Use entity 0 for UI
        cursorTimer = CREATE_TIMER(0, 10);
        
        // Menu state
        menuData->setInt32("selected", 0);
        menuData->setBool("visible", false);
        menuData->setString("language", "en");
        
        // Blinking cursor animation
        cursorTimer->start(TIMER_REPEATING, 500);
        cursorTimer->setRepeatCallback([](uint16_t entity, uint16_t timer, uint32_t repeat) {
            // Toggle cursor visibility
            bool visible = (repeat % 2) == 0;
            // Update cursor sprite visibility
        });
    }
    
    void show() {
        menuData->setBool("visible", true);
    }
    
    void handleInput(const WispInputState& input) {
        if (!menuData->getBool("visible")) return;
        
        int32_t selected = menuData->getInt32("selected");
        int32_t maxItems = 4; // Items, Pokémon, Save, Exit
        
        if (input.up && selected > 0) {
            selected--;
            menuData->setInt32("selected", selected);
        }
        else if (input.down && selected < maxItems - 1) {
            selected++;
            menuData->setInt32("selected", selected);
        }
        else if (input.buttonA) {
            executeMenuItem(selected);
        }
        else if (input.buttonB) {
            menuData->setBool("visible", false);
        }
    }
    
    String getMenuText(const String& key) {
        return menuData->translate(key);  // Support multiple languages
    }
};
```

## Example 2: Platformer Game

### Core Requirements
- Physics-based movement with gravity
- Jumping mechanics
- Collision detection with platforms
- Simple enemy AI
- Level progression

### Implementation Using Components

```cpp
// === Platformer Player ===
class PlatformerPlayer {
private:
    uint16_t entityId;
    SpriteComponent* sprite;
    PhysicsComponent* physics;
    TimerComponent* jumpTimer;
    
    // Platformer-specific state
    bool canJump = true;
    int32_t jumpForce = 800;
    int32_t moveSpeed = 200;
    
public:
    PlatformerPlayer(uint16_t id) : entityId(id) {
        sprite = CREATE_SPRITE(entityId);
        physics = CREATE_PHYSICS(entityId);
        jumpTimer = CREATE_TIMER(entityId, 1);
        
        // Setup sprite
        sprite->setSprite(SPRITE_PLAYER);
        sprite->setLayer(5);
        
        // Setup platformer physics
        physics->setBodyType(BODY_DYNAMIC);
        physics->setCollisionShape(SHAPE_RECTANGLE, 16, 24);
        physics->enableGravity(true, 1000);  // Full gravity
        physics->setFriction(800);           // High ground friction
        physics->setBounce(0);               // No bounce
        physics->setMaxVelocity(400);        // Terminal velocity
        
        // Jump cooldown timer
        jumpTimer->setCompleteCallback([](uint16_t entity, uint16_t timer) {
            PlatformerPlayer* player = getPlayerById(entity);
            player->canJump = true;
        });
        
        // Collision callbacks
        physics->setCollisionEnterCallback([](uint16_t entity, uint16_t other, CollisionResponse response) {
            PlatformerPlayer* player = getPlayerById(entity);
            player->onCollision(other, response);
        });
    }
    
    void handleInput(const WispInputState& input) {
        int32_t velocityX = 0;
        
        // Horizontal movement
        if (input.left) {
            velocityX = -moveSpeed;
            sprite->setFlip(true, false);
            if (physics->isOnGround()) {
                sprite->playAnimation(ANIM_MOVE);
            }
        }
        else if (input.right) {
            velocityX = moveSpeed;
            sprite->setFlip(false, false);
            if (physics->isOnGround()) {
                sprite->playAnimation(ANIM_MOVE);
            }
        }
        else {
            if (physics->isOnGround()) {
                sprite->playAnimation(ANIM_IDLE);
            }
        }
        
        // Jumping
        if (input.buttonA && physics->isOnGround() && canJump) {
            physics->jump(jumpForce);
            sprite->playAnimation(ANIM_CUSTOM_1); // Jump animation
            canJump = false;
            jumpTimer->start(TIMER_ONESHOT, 100); // Jump cooldown
        }
        
        // Apply horizontal movement
        physics->setVelocity(velocityX, physics->getVelocityY());
        
        // Air animation
        if (!physics->isOnGround()) {
            if (physics->getVelocityY() < 0) {
                sprite->playAnimation(ANIM_CUSTOM_1); // Rising
            } else {
                sprite->playAnimation(ANIM_CUSTOM_2); // Falling
            }
        }
    }
    
    void onCollision(uint16_t otherId, CollisionResponse response) {
        // Handle collisions with enemies, power-ups, etc.
        EntityType otherType = getEntityType(otherId);
        
        switch (otherType) {
            case ENTITY_ENEMY:
                takeDamage();
                break;
            case ENTITY_POWERUP:
                collectPowerup(otherId);
                break;
            case ENTITY_CHECKPOINT:
                activateCheckpoint(otherId);
                break;
        }
    }
};

// === Simple Enemy AI ===
class SimpleEnemy {
private:
    uint16_t entityId;
    SpriteComponent* sprite;
    PhysicsComponent* physics;
    TimerComponent* aiTimer;
    
    int32_t patrolDistance = 64;
    int32_t startX;
    bool movingRight = true;
    
public:
    SimpleEnemy(uint16_t id, int32_t x, int32_t y) : entityId(id), startX(x) {
        sprite = CREATE_SPRITE(entityId);
        physics = CREATE_PHYSICS(entityId);
        aiTimer = CREATE_TIMER(entityId, 1);
        
        // Setup enemy
        sprite->setSprite(SPRITE_ENEMY);
        sprite->setPosition(x, y);
        sprite->playAnimation(ANIM_MOVE);
        
        physics->setBodyType(BODY_DYNAMIC);
        physics->setCollisionShape(SHAPE_RECTANGLE, 16, 16);
        physics->enableGravity(true);
        physics->setFriction(500);
        
        // AI update timer
        aiTimer->start(TIMER_REPEATING, 100); // Update AI every 100ms
        aiTimer->setRepeatCallback([](uint16_t entity, uint16_t timer, uint32_t repeat) {
            SimpleEnemy* enemy = getEnemyById(entity);
            enemy->updateAI();
        });
    }
    
    void updateAI() {
        int32_t currentX = physics->getX();
        int32_t moveSpeed = 50;
        
        // Simple patrol behavior
        if (movingRight) {
            if (currentX >= startX + patrolDistance) {
                movingRight = false;
                sprite->setFlip(true, false);
            } else {
                physics->setVelocity(moveSpeed, physics->getVelocityY());
            }
        } else {
            if (currentX <= startX - patrolDistance) {
                movingRight = true;
                sprite->setFlip(false, false);
            } else {
                physics->setVelocity(-moveSpeed, physics->getVelocityY());
            }
        }
    }
};
```

## Example 3: Memo/Notes Application

### Core Requirements
- Text input and storage
- Multiple notes/categories
- Search functionality
- Export/import
- Simple UI navigation

### Implementation Using Components

```cpp
// === Memo Application ===
class MemoApp {
private:
    DataComponent* appData;
    DataComponent* noteData;
    TimerComponent* autoSaveTimer;
    
    // UI state
    int32_t currentNoteIndex = 0;
    int32_t maxNotes = 50;
    bool editMode = false;
    
public:
    MemoApp() {
        appData = CREATE_DATA(1);    // App settings
        noteData = CREATE_DATA(2);   // Note storage
        autoSaveTimer = CREATE_TIMER(1, 1);
        
        // Initialize app settings
        appData->setString("language", "en", true);
        appData->setInt32("fontSize", 12, true);
        appData->setBool("darkMode", false, true);
        appData->setInt32("noteCount", 0, true);
        
        // Auto-save timer
        autoSaveTimer->start(TIMER_REPEATING, 30000); // Every 30 seconds
        autoSaveTimer->setRepeatCallback([](uint16_t entity, uint16_t timer, uint32_t repeat) {
            MemoApp* app = getMemoApp();
            app->autoSave();
        });
        
        // Load existing notes
        loadNotes();
    }
    
    void handleInput(const WispInputState& input) {
        if (!editMode) {
            // Navigation mode
            if (input.up && currentNoteIndex > 0) {
                currentNoteIndex--;
                displayCurrentNote();
            }
            else if (input.down && currentNoteIndex < getNoteCount() - 1) {
                currentNoteIndex++;
                displayCurrentNote();
            }
            else if (input.buttonA) {
                // Edit current note
                enterEditMode();
            }
            else if (input.buttonB) {
                // Show menu
                showMainMenu();
            }
        } else {
            // Edit mode - handle text input
            handleTextInput(input);
            
            if (input.buttonB) {
                // Exit edit mode
                exitEditMode();
            }
        }
    }
    
    // Note management
    void createNewNote() {
        int32_t noteCount = appData->getInt32("noteCount");
        if (noteCount >= maxNotes) return;
        
        String noteKey = "note_" + String(noteCount);
        String titleKey = "title_" + String(noteCount);
        String timestampKey = "time_" + String(noteCount);
        
        noteData->setString(titleKey, "New Note", true);
        noteData->setString(noteKey, "", true);
        noteData->setUInt32(timestampKey, getCurrentTime(), true);
        
        appData->setInt32("noteCount", noteCount + 1, true);
        currentNoteIndex = noteCount;
    }
    
    void deleteCurrentNote() {
        if (getNoteCount() == 0) return;
        
        // Shift all notes down
        int32_t noteCount = getNoteCount();
        for (int i = currentNoteIndex; i < noteCount - 1; i++) {
            copyNote(i + 1, i);
        }
        
        // Clear last note
        clearNote(noteCount - 1);
        appData->setInt32("noteCount", noteCount - 1, true);
        
        if (currentNoteIndex >= noteCount - 1) {
            currentNoteIndex = max(0, noteCount - 2);
        }
    }
    
    String getCurrentNoteTitle() {
        String titleKey = "title_" + String(currentNoteIndex);
        return noteData->getString(titleKey, "Untitled");
    }
    
    String getCurrentNoteContent() {
        String noteKey = "note_" + String(currentNoteIndex);
        return noteData->getString(noteKey, "");
    }
    
    void setCurrentNoteContent(const String& content) {
        String noteKey = "note_" + String(currentNoteIndex);
        String timestampKey = "time_" + String(currentNoteIndex);
        
        noteData->setString(noteKey, content, true);
        noteData->setUInt32(timestampKey, getCurrentTime(), true);
    }
    
    // Search functionality
    Vector<int32_t> searchNotes(const String& query) {
        Vector<int32_t> results;
        int32_t noteCount = getNoteCount();
        
        for (int i = 0; i < noteCount; i++) {
            String titleKey = "title_" + String(i);
            String noteKey = "note_" + String(i);
            
            String title = noteData->getString(titleKey);
            String content = noteData->getString(noteKey);
            
            if (title.indexOf(query) >= 0 || content.indexOf(query) >= 0) {
                results.push_back(i);
            }
        }
        
        return results;
    }
    
    // Translation support
    String getText(const String& key) {
        return appData->translate(key);
    }
    
    void setLanguage(const String& language) {
        appData->setString("language", language, true);
    }
    
private:
    int32_t getNoteCount() {
        return appData->getInt32("noteCount");
    }
    
    void autoSave() {
        appData->save();
        noteData->save();
    }
    
    void loadNotes() {
        appData->load();
        noteData->load();
    }
    
    void copyNote(int32_t from, int32_t to) {
        String fromTitle = "title_" + String(from);
        String fromNote = "note_" + String(from);
        String fromTime = "time_" + String(from);
        
        String toTitle = "title_" + String(to);
        String toNote = "note_" + String(to);
        String toTime = "time_" + String(to);
        
        noteData->setString(toTitle, noteData->getString(fromTitle), true);
        noteData->setString(toNote, noteData->getString(fromNote), true);
        noteData->setUInt32(toTime, noteData->getUInt32(fromTime), true);
    }
    
    void clearNote(int32_t index) {
        String titleKey = "title_" + String(index);
        String noteKey = "note_" + String(index);
        String timeKey = "time_" + String(index);
        
        noteData->removeKey(titleKey);
        noteData->removeKey(noteKey);
        noteData->removeKey(timeKey);
    }
};
```

## Common Patterns Across All Apps

### 1. Component Composition
All apps use the same fundamental components:
- **SpriteComponent**: Visual representation and animation
- **PhysicsComponent**: Movement and collision (even for UI cursor movement)
- **TimerComponent**: Time-based behaviors and delays
- **DataComponent**: State management and persistence

### 2. Event-Driven Architecture
Components communicate through callbacks:
- Animation completion events
- Collision events
- Timer events
- Input events

### 3. Data Persistence
All apps support:
- Save/load functionality through DataComponent
- Language translation support
- Settings persistence
- Auto-save capabilities

### 4. Modular Design
Each app type can:
- Add custom logic while reusing core components
- Scale complexity as needed
- Share common patterns (menus, navigation, input handling)
- Support different input methods and screen layouts

This demonstrates how the Wisp Engine's component-based architecture provides powerful building blocks that can support diverse application types while maintaining consistency and reusability.
