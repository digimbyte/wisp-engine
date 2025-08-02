// engine/entity_system.h - ESP32-C6/S3 Entity Component System using ESP-IDF
// Memory-efficient ECS optimized for ESP32 microcontroller constraints
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include <vector>

// Clean entity component system
constexpr uint16_t MAX_ENTITIES = 512;
constexpr uint16_t INVALID_ENTITY = 0xFFFF;

// Entity component flags
enum EntityFlags : uint16_t {
    ENTITY_ACTIVE = 1 << 0,
    ENTITY_VISIBLE = 1 << 1,
    ENTITY_KINEMATIC = 1 << 2,
    ENTITY_COLLISION_ENABLED = 1 << 3,
    ENTITY_TRIGGER_ENABLED = 1 << 4,
    ENTITY_DESTROY_PENDING = 1 << 5
};

// Core entity data - struct of arrays for performance
struct EntityComponents {
    // Transform components
    int16_t positionX[MAX_ENTITIES];
    int16_t positionY[MAX_ENTITIES];
    int16_t velocityX[MAX_ENTITIES];
    int16_t velocityY[MAX_ENTITIES];
    
    // Size components
    uint16_t width[MAX_ENTITIES];
    uint16_t height[MAX_ENTITIES];
    
    // Render components
    uint16_t spriteId[MAX_ENTITIES];
    uint8_t spriteFrame[MAX_ENTITIES];
    uint8_t depth[MAX_ENTITIES];
    
    // Physics components
    uint8_t collisionMask[MAX_ENTITIES];
    uint8_t triggerMask[MAX_ENTITIES];
    
    // State
    uint16_t flags[MAX_ENTITIES];
    uint32_t userData[MAX_ENTITIES]; // App-specific data
    
    EntityComponents() {
        clear();
    }
    
    void clear() {
        memset(positionX, 0, sizeof(positionX));
        memset(positionY, 0, sizeof(positionY));
        memset(velocityX, 0, sizeof(velocityX));
        memset(velocityY, 0, sizeof(velocityY));
        memset(width, 0, sizeof(width));
        memset(height, 0, sizeof(height));
        memset(spriteId, 0, sizeof(spriteId));
        memset(spriteFrame, 0, sizeof(spriteFrame));
        memset(depth, 0, sizeof(depth));
        memset(collisionMask, 0, sizeof(collisionMask));
        memset(triggerMask, 0, sizeof(triggerMask));
        memset(flags, 0, sizeof(flags));
        memset(userData, 0, sizeof(userData));
    }
};

// Entity system manager
class EntitySystem {
private:
    EntityComponents components;
    std::vector<uint16_t> freeEntityIds;
    uint16_t nextEntityId;
    uint16_t activeEntityCount;
    
public:
    EntitySystem() : nextEntityId(0), activeEntityCount(0) {
        // Pre-populate free ID list
        freeEntityIds.reserve(MAX_ENTITIES);
        for (uint16_t i = MAX_ENTITIES - 1; i > 0; i--) {
            freeEntityIds.push_back(i);
        }
    }
    
    // Entity lifecycle
    uint16_t createEntity();
    uint16_t createEntity(int16_t x, int16_t y, uint16_t w, uint16_t h);
    bool destroyEntity(uint16_t entityId);
    void destroyAllEntities();
    
    // Entity validation
    bool isValidEntity(uint16_t entityId) const;
    bool isEntityActive(uint16_t entityId) const;
    
    // Component access - position
    void setPosition(uint16_t entityId, int16_t x, int16_t y);
    void getPosition(uint16_t entityId, int16_t& x, int16_t& y) const;
    void translate(uint16_t entityId, int16_t dx, int16_t dy);
    
    // Component access - velocity
    void setVelocity(uint16_t entityId, int16_t vx, int16_t vy);
    void getVelocity(uint16_t entityId, int16_t& vx, int16_t& vy) const;
    
    // Component access - size
    void setSize(uint16_t entityId, uint16_t w, uint16_t h);
    void getSize(uint16_t entityId, uint16_t& w, uint16_t& h) const;
    
    // Component access - sprite
    void setSpriteId(uint16_t entityId, uint16_t spriteId);
    void setSpriteFrame(uint16_t entityId, uint8_t frame);
    uint16_t getSpriteId(uint16_t entityId) const;
    uint8_t getSpriteFrame(uint16_t entityId) const;
    
    // Component access - depth
    void setDepth(uint16_t entityId, uint8_t depth);
    uint8_t getDepth(uint16_t entityId) const;
    
    // Component access - physics
    void setCollisionMask(uint16_t entityId, uint8_t mask);
    void setTriggerMask(uint16_t entityId, uint8_t mask);
    uint8_t getCollisionMask(uint16_t entityId) const;
    uint8_t getTriggerMask(uint16_t entityId) const;
    
    // Component access - flags
    void setFlag(uint16_t entityId, EntityFlags flag, bool value);
    bool getFlag(uint16_t entityId, EntityFlags flag) const;
    
    // Component access - user data
    void setUserData(uint16_t entityId, uint32_t data);
    uint32_t getUserData(uint16_t entityId) const;
    
    // Bulk operations
    void updatePositions(float deltaTime);
    void processPendingDestruction();
    
    // System queries
    uint16_t getActiveEntityCount() const { return activeEntityCount; }
    uint16_t getMaxEntities() const { return MAX_ENTITIES; }
    
    // Iteration support
    class Iterator {
    private:
        const EntitySystem* system;
        uint16_t currentIndex;
        
        void findNextActive() {
            while (currentIndex < MAX_ENTITIES && 
                   !system->isEntityActive(currentIndex)) {
                currentIndex++;
            }
        }
        
    public:
        Iterator(const EntitySystem* sys, uint16_t start) : 
            system(sys), currentIndex(start) {
            findNextActive();
        }
        
        uint16_t operator*() const { return currentIndex; }
        Iterator& operator++() {
            currentIndex++;
            findNextActive();
            return *this;
        }
        
        bool operator!=(const Iterator& other) const {
            return currentIndex != other.currentIndex;
        }
    };
    
    Iterator begin() const { return Iterator(this, 0); }
    Iterator end() const { return Iterator(this, MAX_ENTITIES); }
    
    // Direct component access (for performance-critical code)
    const EntityComponents& getComponents() const { return components; }
    
    // Debug and diagnostics
    void printEntityInfo(uint16_t entityId) const;
    void printSystemStats() const;
    bool validateSystem() const;
    
private:
    uint16_t allocateEntityId();
    void freeEntityId(uint16_t entityId);
};
