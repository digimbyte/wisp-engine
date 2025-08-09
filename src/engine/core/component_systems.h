#pragma once
// Core Component Systems for Wisp Engine
// Essential building blocks for diverse applications: RPGs, platformers, memo apps, etc.

#include "engine_common.h"
#include "../scene/scene_system.h"

namespace WispEngine {
namespace Core {

// Forward declarations
class SpriteComponent;
class PhysicsComponent;
class TimerComponent;
class DataComponent;

// === SPRITE COMPONENT SYSTEM ===

// Sprite animation state
enum SpriteAnimState : uint8_t {
    ANIM_IDLE = 0,
    ANIM_MOVE,
    ANIM_CUSTOM_1,
    ANIM_CUSTOM_2,
    ANIM_CUSTOM_3,
    ANIM_CUSTOM_4,
    ANIM_CUSTOM_5,
    ANIM_CUSTOM_6
};

// Sprite component for entities
class SpriteComponent {
private:
    uint16_t entityId;
    uint16_t spriteId;
    
    // Position and rendering
    int32_t x, y;                    // Fixed-point 16.16
    uint8_t layer;                   // Rendering layer (0-7)
    bool visible;
    bool flipX, flipY;
    uint8_t opacity;                 // 0-255
    
    // Animation
    SpriteAnimState currentAnim;
    uint8_t frameCount;
    uint8_t currentFrame;
    uint16_t frameDelayMs;
    uint32_t lastFrameTime;
    bool looping;
    bool animating;
    
    // Callbacks for animation events
    void (*onAnimationComplete)(uint16_t entityId, SpriteAnimState anim);
    void (*onFrameChanged)(uint16_t entityId, uint8_t newFrame);
    
public:
    SpriteComponent(uint16_t entId);
    ~SpriteComponent();
    
    // Basic sprite operations
    void setSprite(uint16_t spriteId);
    void setPosition(int32_t x, int32_t y);
    void move(int32_t deltaX, int32_t deltaY);
    void setVisible(bool visible);
    void setLayer(uint8_t layer);
    void setFlip(bool flipX, bool flipY);
    void setOpacity(uint8_t opacity);
    
    // Animation control
    void playAnimation(SpriteAnimState anim, bool loop = true);
    void stopAnimation();
    void setFrame(uint8_t frame);
    void setAnimationSpeed(uint16_t frameDelayMs);
    
    // Frame update
    void update(uint32_t currentTime);
    
    // Getters
    int32_t getX() const { return x; }
    int32_t getY() const { return y; }
    uint16_t getSpriteId() const { return spriteId; }
    bool isVisible() const { return visible; }
    uint8_t getLayer() const { return layer; }
    SpriteAnimState getCurrentAnimation() const { return currentAnim; }
    uint8_t getCurrentFrame() const { return currentFrame; }
    
    // Event handlers
    void setAnimationCompleteCallback(void (*callback)(uint16_t, SpriteAnimState));
    void setFrameChangedCallback(void (*callback)(uint16_t, uint8_t));
};

// === PHYSICS COMPONENT SYSTEM ===

// Collision shapes
enum CollisionShape : uint8_t {
    SHAPE_RECTANGLE,
    SHAPE_CIRCLE,
    SHAPE_POINT
};

// Collision response types
enum CollisionResponse : uint8_t {
    COLLISION_NONE,      // No collision
    COLLISION_STOP,      // Stop movement
    COLLISION_BOUNCE,    // Bounce off
    COLLISION_SLIDE,     // Slide along surface
    COLLISION_TRIGGER    // Trigger event only
};

// Physics body types
enum PhysicsBodyType : uint8_t {
    BODY_STATIC,         // Never moves (walls, platforms)
    BODY_KINEMATIC,      // Moves but not affected by forces
    BODY_DYNAMIC         // Affected by forces and collisions
};

// Basic physics component
class PhysicsComponent {
private:
    uint16_t entityId;
    
    // Body properties
    PhysicsBodyType bodyType;
    CollisionShape shape;
    
    // Position and size
    int32_t x, y;                    // Fixed-point 16.16
    uint16_t width, height;          // For rectangles
    uint16_t radius;                 // For circles
    
    // Velocity and forces
    int32_t velocityX, velocityY;    // Fixed-point 16.16
    int32_t accelerationX, accelerationY;
    int32_t maxVelocity;
    
    // Physics properties
    uint16_t friction;               // 0-1000 (0 = no friction, 1000 = full)
    uint16_t bounce;                 // 0-1000 (0 = no bounce, 1000 = full)
    bool enableGravity;
    int32_t gravityScale;            // 0-1000 (1000 = normal gravity)
    
    // Collision
    uint8_t collisionMask;           // What can collide with this
    uint8_t collisionLayer;          // What layer this is on
    CollisionResponse responseType;
    
    // Ground detection
    bool onGround;
    bool wasOnGround;
    uint32_t lastGroundTime;
    
    // Collision callbacks
    void (*onCollisionEnter)(uint16_t entityId, uint16_t otherId, CollisionResponse response);
    void (*onCollisionExit)(uint16_t entityId, uint16_t otherId);
    void (*onTriggerEnter)(uint16_t entityId, uint16_t triggerId);
    
public:
    PhysicsComponent(uint16_t entId);
    ~PhysicsComponent();
    
    // Setup
    void setBodyType(PhysicsBodyType type);
    void setCollisionShape(CollisionShape shape, uint16_t width, uint16_t height);
    void setCollisionCircle(uint16_t radius);
    void setCollisionMask(uint8_t mask, uint8_t layer);
    void setResponseType(CollisionResponse response);
    
    // Position and movement
    void setPosition(int32_t x, int32_t y);
    void setVelocity(int32_t vx, int32_t vy);
    void addForce(int32_t fx, int32_t fy);
    void setMaxVelocity(int32_t maxVel);
    
    // Physics properties
    void setFriction(uint16_t friction);
    void setBounce(uint16_t bounce);
    void enableGravity(bool enable, int32_t scale = 1000);
    
    // Movement helpers
    void move(int32_t deltaX, int32_t deltaY);
    void jump(int32_t jumpForce);
    void applyImpulse(int32_t impulseX, int32_t impulseY);
    
    // Collision detection
    bool checkCollision(const PhysicsComponent& other) const;
    bool isPointInside(int32_t px, int32_t py) const;
    
    // Frame update
    void update(uint32_t deltaTimeMs);
    
    // Getters
    int32_t getX() const { return x; }
    int32_t getY() const { return y; }
    int32_t getVelocityX() const { return velocityX; }
    int32_t getVelocityY() const { return velocityY; }
    bool isOnGround() const { return onGround; }
    PhysicsBodyType getBodyType() const { return bodyType; }
    
    // Event handlers
    void setCollisionEnterCallback(void (*callback)(uint16_t, uint16_t, CollisionResponse));
    void setCollisionExitCallback(void (*callback)(uint16_t, uint16_t));
    void setTriggerEnterCallback(void (*callback)(uint16_t, uint16_t));
};

// === TIMER COMPONENT SYSTEM ===

// Timer types
enum TimerType : uint8_t {
    TIMER_ONESHOT,       // Fire once and stop
    TIMER_REPEATING,     // Fire repeatedly
    TIMER_COUNTDOWN,     // Count down to zero
    TIMER_STOPWATCH      // Count up indefinitely
};

// Timer states
enum TimerState : uint8_t {
    TIMER_STOPPED,
    TIMER_RUNNING,
    TIMER_PAUSED,
    TIMER_FINISHED
};

// Timer component for time-based behaviors
class TimerComponent {
private:
    uint16_t entityId;
    uint16_t timerId;
    
    TimerType type;
    TimerState state;
    
    uint32_t duration;               // Duration in milliseconds
    uint32_t elapsed;                // Elapsed time in milliseconds
    uint32_t repeatCount;            // How many times to repeat (0 = infinite)
    uint32_t currentRepeats;         // Current repeat count
    
    // Callbacks
    void (*onTimerComplete)(uint16_t entityId, uint16_t timerId);
    void (*onTimerTick)(uint16_t entityId, uint16_t timerId, uint32_t elapsed);
    void (*onTimerRepeat)(uint16_t entityId, uint16_t timerId, uint32_t repeatNum);
    
public:
    TimerComponent(uint16_t entId, uint16_t id);
    ~TimerComponent();
    
    // Timer control
    void start(TimerType type, uint32_t durationMs);
    void stop();
    void pause();
    void resume();
    void reset();
    
    // Configuration
    void setRepeating(uint32_t repeatCount = 0); // 0 = infinite
    void setDuration(uint32_t durationMs);
    
    // Frame update
    void update(uint32_t deltaTimeMs);
    
    // Getters
    TimerState getState() const { return state; }
    uint32_t getElapsed() const { return elapsed; }
    uint32_t getDuration() const { return duration; }
    uint32_t getRemainingTime() const { return duration > elapsed ? duration - elapsed : 0; }
    float getProgress() const { return duration > 0 ? (float)elapsed / duration : 0.0f; }
    
    // Event handlers
    void setCompleteCallback(void (*callback)(uint16_t, uint16_t));
    void setTickCallback(void (*callback)(uint16_t, uint16_t, uint32_t));
    void setRepeatCallback(void (*callback)(uint16_t, uint16_t, uint32_t));
};

// === DATA COMPONENT SYSTEM ===

// Data value types
enum DataValueType : uint8_t {
    DATA_NONE,
    DATA_BOOL,
    DATA_INT8,
    DATA_INT16,
    DATA_INT32,
    DATA_UINT8,
    DATA_UINT16,
    DATA_UINT32,
    DATA_FLOAT,
    DATA_STRING
};

// Data value container
union DataValue {
    bool boolVal;
    int8_t i8Val;
    int16_t i16Val;
    int32_t i32Val;
    uint8_t u8Val;
    uint16_t u16Val;
    uint32_t u32Val;
    float floatVal;
    char stringVal[64];
    
    DataValue() { memset(this, 0, sizeof(DataValue)); }
};

// Data entry
struct DataEntry {
    char key[32];
    DataValueType type;
    DataValue value;
    bool persistent;                 // Should be saved
    
    DataEntry() : type(DATA_NONE), persistent(false) {
        memset(key, 0, sizeof(key));
    }
};

// Data component for storing app state
class DataComponent {
private:
    uint16_t entityId;
    
    static const uint8_t MAX_DATA_ENTRIES = 32;
    DataEntry data[MAX_DATA_ENTRIES];
    uint8_t entryCount;
    
    // Change tracking
    bool hasChanges;
    uint32_t lastSaveTime;
    uint32_t autoSaveInterval;       // Auto-save interval in ms (0 = disabled)
    
    // Translation support
    String currentLanguage;
    
    DataEntry* findEntry(const String& key);
    
public:
    DataComponent(uint16_t entId);
    ~DataComponent();
    
    // Data operations
    bool setBool(const String& key, bool value, bool persistent = false);
    bool setInt8(const String& key, int8_t value, bool persistent = false);
    bool setInt16(const String& key, int16_t value, bool persistent = false);
    bool setInt32(const String& key, int32_t value, bool persistent = false);
    bool setUInt8(const String& key, uint8_t value, bool persistent = false);
    bool setUInt16(const String& key, uint16_t value, bool persistent = false);
    bool setUInt32(const String& key, uint32_t value, bool persistent = false);
    bool setFloat(const String& key, float value, bool persistent = false);
    bool setString(const String& key, const String& value, bool persistent = false);
    
    // Data retrieval
    bool getBool(const String& key, bool defaultValue = false);
    int8_t getInt8(const String& key, int8_t defaultValue = 0);
    int16_t getInt16(const String& key, int16_t defaultValue = 0);
    int32_t getInt32(const String& key, int32_t defaultValue = 0);
    uint8_t getUInt8(const String& key, uint8_t defaultValue = 0);
    uint16_t getUInt16(const String& key, uint16_t defaultValue = 0);
    uint32_t getUInt32(const String& key, uint32_t defaultValue = 0);
    float getFloat(const String& key, float defaultValue = 0.0f);
    String getString(const String& key, const String& defaultValue = "");
    
    // Data management
    bool hasKey(const String& key);
    bool removeKey(const String& key);
    void clearAll();
    void clearNonPersistent();
    
    // Translation support
    void setLanguage(const String& language);
    String translate(const String& key);  // Returns translated text or key if not found
    
    // Persistence
    void setAutoSave(uint32_t intervalMs);
    void save();
    void load();
    bool hasUnsavedChanges() const { return hasChanges; }
    
    // Frame update
    void update(uint32_t currentTime);
    
    // Getters
    uint8_t getEntryCount() const { return entryCount; }
    const String& getCurrentLanguage() const { return currentLanguage; }
};

// === COMPONENT MANAGER ===

// Central manager for all components
class ComponentManager {
private:
    // Component arrays
    static const uint8_t MAX_ENTITIES = 64;
    
    SpriteComponent* sprites[MAX_ENTITIES];
    PhysicsComponent* physics[MAX_ENTITIES];
    TimerComponent* timers[MAX_ENTITIES * 4];    // Multiple timers per entity
    DataComponent* dataComponents[MAX_ENTITIES];
    
    uint8_t spriteCount;
    uint8_t physicsCount;
    uint8_t timerCount;
    uint8_t dataCount;
    
    // Global physics settings
    int32_t globalGravity;
    uint16_t globalFriction;
    
public:
    ComponentManager();
    ~ComponentManager();
    
    // Component creation
    SpriteComponent* createSpriteComponent(uint16_t entityId);
    PhysicsComponent* createPhysicsComponent(uint16_t entityId);
    TimerComponent* createTimerComponent(uint16_t entityId, uint16_t timerId);
    DataComponent* createDataComponent(uint16_t entityId);
    
    // Component retrieval
    SpriteComponent* getSpriteComponent(uint16_t entityId);
    PhysicsComponent* getPhysicsComponent(uint16_t entityId);
    TimerComponent* getTimerComponent(uint16_t entityId, uint16_t timerId);
    DataComponent* getDataComponent(uint16_t entityId);
    
    // Component destruction
    void destroySpriteComponent(uint16_t entityId);
    void destroyPhysicsComponent(uint16_t entityId);
    void destroyTimerComponent(uint16_t entityId, uint16_t timerId);
    void destroyDataComponent(uint16_t entityId);
    void destroyAllComponents(uint16_t entityId);
    
    // System updates
    void updateSprites(uint32_t currentTime);
    void updatePhysics(uint32_t deltaTimeMs);
    void updateTimers(uint32_t deltaTimeMs);
    void updateData(uint32_t currentTime);
    void updateAll(uint32_t currentTime, uint32_t deltaTimeMs);
    
    // Collision detection
    void processCollisions();
    
    // Global physics settings
    void setGlobalGravity(int32_t gravity);
    void setGlobalFriction(uint16_t friction);
    
    // Utility
    void clearAll();
    uint8_t getActiveEntityCount();
    
    // Statistics
    uint8_t getSpriteCount() const { return spriteCount; }
    uint8_t getPhysicsCount() const { return physicsCount; }
    uint8_t getTimerCount() const { return timerCount; }
    uint8_t getDataCount() const { return dataCount; }
};

// Global component manager instance
extern ComponentManager g_ComponentManager;

// Convenience macros for common operations
#define CREATE_SPRITE(entityId) g_ComponentManager.createSpriteComponent(entityId)
#define CREATE_PHYSICS(entityId) g_ComponentManager.createPhysicsComponent(entityId)
#define CREATE_TIMER(entityId, timerId) g_ComponentManager.createTimerComponent(entityId, timerId)
#define CREATE_DATA(entityId) g_ComponentManager.createDataComponent(entityId)

#define GET_SPRITE(entityId) g_ComponentManager.getSpriteComponent(entityId)
#define GET_PHYSICS(entityId) g_ComponentManager.getPhysicsComponent(entityId)
#define GET_TIMER(entityId, timerId) g_ComponentManager.getTimerComponent(entityId, timerId)
#define GET_DATA(entityId) g_ComponentManager.getDataComponent(entityId)

} // namespace Core
} // namespace WispEngine
