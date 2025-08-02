// app_loop.h
// app_loop.h - ESP32-C6/S3 Application Loop using ESP-IDF
// Structured application loop with timing and state management for ESP32
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include <vector>
#include "../graphics/engine.h"
#include "../physics/engine.h"
#include "../audio/engine.h"
#include "../../system/input_controller.h"

// App loop stages - executed in order each frame
enum AppStage {
    STAGE_INPUT_COLLECTION,     // Gather input events, update controller state
    STAGE_HEARTBEAT,           // Backend systems update, timers, state management
    STAGE_LOGIC_UPDATE,        // App/application logic execution (C++ applications)
    STAGE_PHYSICS_PREDICTION,  // Calculate intended movements and physics
    STAGE_COLLISION_DETECTION, // Check for overlaps and collisions
    STAGE_PHYSICS_RESOLUTION,  // Resolve conflicts, apply final positions
    STAGE_TRIGGER_PROCESSING,  // Process trigger events and interactions
    STAGE_AUDIO_UPDATE,        // Update audio systems, apply effects
    STAGE_RENDER_PREPARE,      // Prepare rendering data, depth sorting
    STAGE_RENDER_EXECUTE,      // Draw to frame buffer
    STAGE_RENDER_PRESENT,      // Present frame to display
    STAGE_COUNT
};

// Physics region types
enum RegionType {
    REGION_COLLISION,          // Solid collision - blocks movement
    REGION_TRIGGER,            // Trigger - detects overlap, allows movement
};

// Collision/trigger mask types for filtering
enum MaskType {
    MASK_PLAYER    = 0x01,     // Player entities
    MASK_ENEMY     = 0x02,     // Enemy entities  
    MASK_PROJECTILE = 0x04,    // Bullets, projectiles
    MASK_ITEM      = 0x08,     // Collectible items
    MASK_ALL       = 0x0F      // All mask types
};

// Physics region definition
struct PhysicsRegion {
    BoundingBox bounds;
    RegionType type;
    uint8_t collisionMask;     // What this region collides with
    uint8_t triggerMask;       // What this region triggers with
    uint16_t regionId;         // Unique identifier
    bool active;
    
    // Trigger logic type
    enum TriggerLogic {
        TRIGGER_ON_ENTER,      // Fire when entity enters
        TRIGGER_ON_EXIT,       // Fire when entity exits
        TRIGGER_WHILE_INSIDE,  // Fire every frame while inside
        TRIGGER_ON_OVERLAP     // Fire when any overlap detected
    } triggerLogic;
};

// Entity for physics and collision
struct AppEntity {
    uint16_t entityId;
    BoundingBox bounds;
    int16_t velocityX, velocityY;
    uint8_t collisionMask;     // What this entity collides with
    uint8_t triggerMask;       // What triggers this entity responds to
    bool active;
    bool kinematic;            // If true, not affected by collision resolution
    
    // Rendering info (optional)
    uint16_t spriteId;
    uint8_t spriteFrame;
    uint8_t depth;
    
    // App-specific data pointer
    void* appData;
};

// Collision/trigger event
struct PhysicsEvent {
    enum EventType {
        EVENT_COLLISION,       // Solid collision occurred
        EVENT_TRIGGER_ENTER,   // Entity entered trigger
        EVENT_TRIGGER_EXIT,    // Entity exited trigger
        EVENT_TRIGGER_STAY     // Entity staying in trigger
    } type;
    
    uint16_t entityId;         // Entity involved
    uint16_t regionId;         // Region involved (0xFFFF for entity-entity)
    uint16_t otherEntityId;    // Other entity (0xFFFF for entity-region)
    BoundingBox overlap;       // Overlap area
};

class AppLoop {
public:
    // Core systems
    GraphicsEngine* graphics;
    PhysicsEngine* physics;
    AudioEngine* audio;
    InputController* input;
    
    // App state
    std::vector<AppEntity> entities;
    std::vector<PhysicsRegion> regions;
    std::vector<PhysicsEvent> frameEvents;
    
    // Timing and performance
    uint32_t frameStartTime;
    uint32_t lastFrameTime;
    uint32_t deltaTime;
    uint16_t targetFrameTime;  // Microseconds per frame
    uint32_t frameCount;
    
    // Stage tracking
    AppStage currentStage;
    uint32_t stageTimings[STAGE_COUNT];
    
    // Entity management
    uint16_t nextEntityId;
    uint16_t nextRegionId;

    void init(GraphicsEngine* gfx, PhysicsEngine* phys, AudioEngine* aud, InputController* inp) {
        graphics = gfx;
        physics = phys;
        audio = aud;
        input = inp;
        
        frameCount = 0;
        nextEntityId = 1;
        nextRegionId = 1;
        currentStage = STAGE_INPUT_COLLECTION;
        targetFrameTime = 16666; // ~60 FPS in microseconds
        
        entities.clear();
        regions.clear();
        frameEvents.clear();
        
        memset(stageTimings, 0, sizeof(stageTimings));
        
        Serial.println("App Loop initialized");
    }
    
    // Main app loop - call this every frame
    void update() {
        frameStartTime = micros();
        deltaTime = frameStartTime - lastFrameTime;
        frameCount++;
        
        // Execute all app stages in order
        for (int stage = 0; stage < STAGE_COUNT; stage++) {
            currentStage = (AppStage)stage;
            uint32_t stageStart = micros();
            
            executeStage(currentStage);
            
            stageTimings[stage] = micros() - stageStart;
        }
        
        lastFrameTime = frameStartTime;
        
        // Frame rate limiting (if needed)
        uint32_t frameTime = micros() - frameStartTime;
        if (frameTime < targetFrameTime) {
            delayMicroseconds(targetFrameTime - frameTime);
        }
    }
    
    void executeStage(AppStage stage) {
        switch (stage) {
            case STAGE_INPUT_COLLECTION:
                stageInputCollection();
                break;
                
            case STAGE_HEARTBEAT:
                stageHeartbeat();
                break;
                
            case STAGE_LOGIC_UPDATE:
                stageLogicUpdate();
                break;
                
            case STAGE_PHYSICS_PREDICTION:
                stagePhysicsPrediction();
                break;
                
            case STAGE_COLLISION_DETECTION:
                stageCollisionDetection();
                break;
                
            case STAGE_PHYSICS_RESOLUTION:
                stagePhysicsResolution();
                break;
                
            case STAGE_TRIGGER_PROCESSING:
                stageTriggerProcessing();
                break;
                
            case STAGE_AUDIO_UPDATE:
                stageAudioUpdate();
                break;
                
            case STAGE_RENDER_PREPARE:
                stageRenderPrepare();
                break;
                
            case STAGE_RENDER_EXECUTE:
                stageRenderExecute();
                break;
                
            case STAGE_RENDER_PRESENT:
                stageRenderPresent();
                break;
        }
    }
    
    // Stage implementations
    void stageInputCollection() {
        input->update();
        // Input events are collected and will be processed in logic stage
    }
    
    void stageHeartbeat() {
        // Backend system updates
        audio->update();
        
        // Update any internal timers, state machines
        // This is where engine-level systems tick
        
        // Clean up frame events from previous frame
        frameEvents.clear();
    }
    
    void stageLogicUpdate() {
        // This is where C++ applications execute their logic
        // Apps update their app logic here
        // Entity behaviors, AI, app rules, etc.
        
        // C++ applications handle updates through direct function calls
        // Native app logic executes without scripting overhead
    }
    
    void stagePhysicsPrediction() {
        // Calculate intended movements for all entities
        physics->resetQueue();
        
        for (auto& entity : entities) {
            if (!entity.active || entity.kinematic) continue;
            
            // Apply velocity to position (prediction)
            EntityPhysics ephys;
            ephys.id = entity.entityId;
            ephys.x = entity.bounds.left;
            ephys.y = entity.bounds.top;
            ephys.dx = entity.velocityX;
            ephys.dy = entity.velocityY;
            ephys.shape.physical[0] = entity.bounds;
            ephys.active = true;
            
            physics->enqueuePrediction(ephys);
        }
    }
    
    void stageCollisionDetection() {
        // Check entity vs entity collisions
        checkEntityCollisions();
        
        // Check entity vs region collisions/triggers
        checkRegionInteractions();
        
        // Resolve conflicts in physics engine
        physics->resolveConflicts();
    }
    
    void stagePhysicsResolution() {
        // Apply resolved physics back to entities
        for (auto& entity : entities) {
            if (!entity.active || entity.kinematic) continue;
            
            // Find this entity in physics queue
            for (int i = 0; i < physics->queuedCount; i++) {
                if (physics->queue[i].entityId == entity.entityId) {
                    const auto& intent = physics->queue[i];
                    
                    if (intent.isValid) {
                        // Movement was valid, apply it
                        entity.bounds.left = intent.projectedX;
                        entity.bounds.top = intent.projectedY;
                        entity.bounds.right = intent.projectedX + (entity.bounds.right - entity.bounds.left);
                        entity.bounds.bottom = intent.projectedY + (entity.bounds.bottom - entity.bounds.top);
                    } else {
                        // Movement was blocked, stop velocity
                        entity.velocityX = 0;
                        entity.velocityY = 0;
                    }
                    break;
                }
            }
        }
    }
    
    void stageTriggerProcessing() {
        // Process all trigger events that occurred this frame
        for (const auto& event : frameEvents) {
            if (event.type >= PhysicsEvent::EVENT_TRIGGER_ENTER) {
                processTriggerEvent(event);
            }
        }
    }
    
    void stageAudioUpdate() {
        // Audio system already updated in heartbeat
        // This stage could handle 3D audio positioning, etc.
    }
    
    void stageRenderPrepare() {
        // Sort entities by depth for rendering
        std::sort(entities.begin(), entities.end(), 
                 [](const AppEntity& a, const AppEntity& b) {
                     return a.depth > b.depth; // Back to front
                 });
    }
    
    void stageRenderExecute() {
        // Clear frame
        graphics->clearBuffers(0x0000);
        
        // Render all entities
        for (const auto& entity : entities) {
            if (!entity.active) continue;
            
            if (entity.spriteId != 0xFFFF) {
                graphics->drawSprite(entity.spriteId, 
                                   entity.bounds.left, 
                                   entity.bounds.top);
            }
        }
    }
    
    void stageRenderPresent() {
        graphics->present();
    }
    
    // Entity management
    uint16_t createEntity(int16_t x, int16_t y, uint16_t w, uint16_t h, 
                         uint8_t collisionMask = MASK_ALL, uint8_t triggerMask = MASK_ALL) {
        AppEntity entity;
        entity.entityId = nextEntityId++;
        entity.bounds = {x, y, x + w, y + h};
        entity.velocityX = 0;
        entity.velocityY = 0;
        entity.collisionMask = collisionMask;
        entity.triggerMask = triggerMask;
        entity.active = true;
        entity.kinematic = false;
        entity.spriteId = 0xFFFF;
        entity.spriteFrame = 0;
        entity.depth = 6; // Mid depth
        entity.appData = nullptr;
        
        entities.push_back(entity);
        return entity.entityId;
    }
    
    uint16_t createRegion(int16_t x, int16_t y, uint16_t w, uint16_t h, 
                         RegionType type, uint8_t mask = MASK_ALL,
                         PhysicsRegion::TriggerLogic logic = PhysicsRegion::TRIGGER_ON_ENTER) {
        PhysicsRegion region;
        region.regionId = nextRegionId++;
        region.bounds = {x, y, x + w, y + h};
        region.type = type;
        region.collisionMask = (type == REGION_COLLISION) ? mask : 0;
        region.triggerMask = (type == REGION_TRIGGER) ? mask : 0;
        region.triggerLogic = logic;
        region.active = true;
        
        regions.push_back(region);
        return region.regionId;
    }
    
    AppEntity* getEntity(uint16_t entityId) {
        for (auto& entity : entities) {
            if (entity.entityId == entityId) {
                return &entity;
            }
        }
        return nullptr;
    }
    
    PhysicsRegion* getRegion(uint16_t regionId) {
        for (auto& region : regions) {
            if (region.regionId == regionId) {
                return &region;
            }
        }
        return nullptr;
    }
    
private:
    void checkEntityCollisions() {
        for (size_t i = 0; i < entities.size(); i++) {
            for (size_t j = i + 1; j < entities.size(); j++) {
                AppEntity& entityA = entities[i];
                AppEntity& entityB = entities[j];
                
                if (!entityA.active || !entityB.active) continue;
                
                // Check mask filtering
                bool shouldCollide = (entityA.collisionMask & entityB.collisionMask) != 0;
                if (!shouldCollide) continue;
                
                // Check for overlap
                if (intersects(entityA.bounds, entityB.bounds)) {
                    PhysicsEvent event;
                    event.type = PhysicsEvent::EVENT_COLLISION;
                    event.entityId = entityA.entityId;
                    event.otherEntityId = entityB.entityId;
                    event.regionId = 0xFFFF;
                    
                    // Calculate overlap area
                    event.overlap.left = max(entityA.bounds.left, entityB.bounds.left);
                    event.overlap.top = max(entityA.bounds.top, entityB.bounds.top);
                    event.overlap.right = min(entityA.bounds.right, entityB.bounds.right);
                    event.overlap.bottom = min(entityA.bounds.bottom, entityB.bounds.bottom);
                    
                    frameEvents.push_back(event);
                }
            }
        }
    }
    
    void checkRegionInteractions() {
        for (auto& entity : entities) {
            if (!entity.active) continue;
            
            for (auto& region : regions) {
                if (!region.active) continue;
                
                // Check mask filtering
                bool shouldInteract = false;
                if (region.type == REGION_COLLISION) {
                    shouldInteract = (entity.collisionMask & region.collisionMask) != 0;
                } else {
                    shouldInteract = (entity.triggerMask & region.triggerMask) != 0;
                }
                
                if (!shouldInteract) continue;
                
                // Check for overlap
                if (intersects(entity.bounds, region.bounds)) {
                    PhysicsEvent event;
                    event.type = (region.type == REGION_COLLISION) ? 
                                PhysicsEvent::EVENT_COLLISION : 
                                PhysicsEvent::EVENT_TRIGGER_ENTER;
                    event.entityId = entity.entityId;
                    event.regionId = region.regionId;
                    event.otherEntityId = 0xFFFF;
                    
                    frameEvents.push_back(event);
                }
            }
        }
    }
    
    void processTriggerEvent(const PhysicsEvent& event) {
        // This would call into C++ to handle trigger events
        // For now, just log them
        Serial.print("Trigger: Entity ");
        Serial.print(event.entityId);
        Serial.print(" -> Region ");
        Serial.println(event.regionId);
    }
    
    int16_t max(int16_t a, int16_t b) { return (a > b) ? a : b; }
    int16_t min(int16_t a, int16_t b) { return (a < b) ? a : b; }
};

// Global app loop instance
extern AppLoop appLoop;
