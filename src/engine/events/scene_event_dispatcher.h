#pragma once

#include "../security/script_instance_authority.h"
#include "../security/uuid_authority.h"
#include "../security/named_entity_registry.h"
#include "../scene/scene_system.h"
#include "../../system/definitions.h"
#include <queue>
#include <unordered_map>
#include <vector>
#include <string>
#include "esp_log.h"
#include "esp_timer.h"

static const char* EVENT_DISPATCHER_TAG = "EventDispatcher";

/**
 * @brief Central Event Dispatcher for Script and System Communication
 * 
 * Coordinates events between all engine systems with proper security validation.
 * Ensures scripts receive events in the correct context with appropriate permissions.
 * 
 * Key Features:
 * - Entity lifecycle events (spawn, destroy, collision)
 * - Scene transition events with proper cleanup
 * - Input event routing to appropriate scripts
 * - Timer and animation event dispatch
 * - Custom event support for game mechanics
 * - Event queuing and batch processing
 * - Security validation for all event operations
 */
class SceneEventDispatcher {
public:
    /**
     * @brief Types of events that can be dispatched
     */
    enum class EventType : uint8_t {
        // Entity events
        ENTITY_SPAWNED = 0,
        ENTITY_DESTROYED = 1,
        ENTITY_COLLISION = 2,
        ENTITY_ANIMATION_COMPLETE = 3,
        ENTITY_STATE_CHANGED = 4,
        
        // Scene events  
        SCENE_LOAD_START = 10,
        SCENE_LOAD_COMPLETE = 11,
        SCENE_UNLOAD_START = 12,
        SCENE_UNLOAD_COMPLETE = 13,
        PANEL_SWITCHED = 14,
        
        // Input events
        INPUT_PRESSED = 20,
        INPUT_RELEASED = 21,
        INPUT_HELD = 22,
        
        // System events
        TIMER_EXPIRED = 30,
        SYSTEM_STATE_CHANGED = 31,
        SCRIPT_ERROR = 32,
        SECURITY_VIOLATION = 33,
        
        // Custom events
        CUSTOM_EVENT = 40
    };
    
    /**
     * @brief Event priority levels
     */
    enum class EventPriority : uint8_t {
        LOW = 0,
        NORMAL = 1,
        HIGH = 2,
        CRITICAL = 3
    };

private:
    /**
     * @brief Event data structure
     */
    struct GameEvent {
        EventType type;
        EventPriority priority;
        uint32_t timestamp;
        
        // Event context
        uint32_t sourceUUID;        // Entity that generated the event
        uint32_t targetUUID;        // Entity that should receive the event (0 = all)
        uint16_t panelId;           // Panel context for the event
        
        // Event-specific data
        union {
            struct { // Entity events
                uint32_t entityA, entityB;     // For collisions
                uint8_t animationId;            // For animation events
                uint8_t newState;               // For state changes
            } entity;
            
            struct { // Scene events
                uint16_t oldPanelId;            // For panel switches
                char sceneName[32];             // Scene name (limited length)
            } scene;
            
            struct { // Input events
                WispInputSemantic input;
                bool pressed;
            } input;
            
            struct { // System events  
                uint16_t timerId;               // For timer events
                uint8_t systemState;            // For system state changes
                char errorMessage[64];          // For errors (limited length)
            } system;
            
            struct { // Custom events
                char eventName[32];             // Custom event name
                char data[64];                  // Custom data payload
            } custom;
        };
        
        GameEvent() : type(EventType::CUSTOM_EVENT), priority(EventPriority::NORMAL),
                     timestamp(0), sourceUUID(0), targetUUID(0), panelId(0) {
            memset(&custom, 0, sizeof(custom));
        }
    };
    
    // Core systems
    ScriptInstanceAuthority* scriptAuthority;
    EngineUUIDAuthority* uuidAuthority;
    NamedEntityRegistry* namedRegistry;
    SceneManager* sceneManager;
    
    // Event queue
    std::queue<GameEvent> eventQueue;
    std::queue<GameEvent> highPriorityQueue;
    std::queue<GameEvent> criticalQueue;
    
    // Performance tracking
    uint32_t eventsProcessedThisFrame;
    uint32_t totalEventsProcessed;
    uint32_t droppedEvents;
    uint32_t lastFrameTime;
    
    // Resource limits
    static constexpr uint16_t MAX_EVENTS_PER_FRAME = 100;
    static constexpr uint16_t MAX_QUEUE_SIZE = 500;
    static constexpr uint32_t MAX_PROCESSING_TIME_MICROS = 2000; // 2ms max per frame
    
    // Event filtering
    std::unordered_map<EventType, bool> eventTypeEnabled;
    std::unordered_map<uint16_t, bool> panelEventsEnabled; // Per-panel event filtering

public:
    SceneEventDispatcher(ScriptInstanceAuthority* scriptAuth, 
                        EngineUUIDAuthority* uuidAuth,
                        NamedEntityRegistry* namedReg,
                        SceneManager* sceneMgr);
    
    ~SceneEventDispatcher();
    
    // === INITIALIZATION ===
    
    /**
     * @brief Initialize the event dispatcher
     * @return True if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Shutdown and cleanup event dispatcher
     */
    void shutdown();
    
    // === ENTITY EVENTS ===
    
    /**
     * @brief Dispatch entity spawned event
     * @param uuid UUID of the spawned entity
     * @param panelId Panel where entity was spawned
     * @param entityType Type of entity that was spawned
     */
    void dispatchEntitySpawned(uint32_t uuid, uint16_t panelId, const std::string& entityType = "");
    
    /**
     * @brief Dispatch entity destroyed event
     * @param uuid UUID of the destroyed entity
     * @param destroyerUUID UUID of entity that caused destruction (0 if system)
     */
    void dispatchEntityDestroyed(uint32_t uuid, uint32_t destroyerUUID = 0);
    
    /**
     * @brief Dispatch entity collision event
     * @param entityA UUID of first entity in collision
     * @param entityB UUID of second entity in collision
     */
    void dispatchEntityCollision(uint32_t entityA, uint32_t entityB);
    
    /**
     * @brief Dispatch animation complete event
     * @param entityUUID UUID of entity whose animation completed
     * @param animationId ID of the animation that completed
     */
    void dispatchAnimationComplete(uint32_t entityUUID, uint8_t animationId);
    
    /**
     * @brief Dispatch entity state change event
     * @param entityUUID UUID of entity whose state changed
     * @param newState New state of the entity
     */
    void dispatchEntityStateChanged(uint32_t entityUUID, uint8_t newState);
    
    // === SCENE EVENTS ===
    
    /**
     * @brief Dispatch scene load start event
     * @param sceneName Name of scene being loaded
     */
    void dispatchSceneLoadStart(const std::string& sceneName);
    
    /**
     * @brief Dispatch scene load complete event
     * @param sceneName Name of scene that was loaded
     */
    void dispatchSceneLoadComplete(const std::string& sceneName);
    
    /**
     * @brief Dispatch scene unload start event
     * @param sceneName Name of scene being unloaded
     */
    void dispatchSceneUnloadStart(const std::string& sceneName);
    
    /**
     * @brief Dispatch scene unload complete event
     * @param sceneName Name of scene that was unloaded
     */
    void dispatchSceneUnloadComplete(const std::string& sceneName);
    
    /**
     * @brief Dispatch panel switch event
     * @param oldPanelId Panel ID being switched from
     * @param newPanelId Panel ID being switched to
     */
    void dispatchPanelSwitch(uint16_t oldPanelId, uint16_t newPanelId);
    
    // === INPUT EVENTS ===
    
    /**
     * @brief Dispatch input pressed event
     * @param input Input semantic that was pressed
     */
    void dispatchInputPressed(WispInputSemantic input);
    
    /**
     * @brief Dispatch input released event
     * @param input Input semantic that was released
     */
    void dispatchInputReleased(WispInputSemantic input);
    
    /**
     * @brief Dispatch input held event
     * @param input Input semantic that is being held
     */
    void dispatchInputHeld(WispInputSemantic input);
    
    // === SYSTEM EVENTS ===
    
    /**
     * @brief Dispatch timer expired event
     * @param timerId ID of the timer that expired
     */
    void dispatchTimerExpired(uint16_t timerId);
    
    /**
     * @brief Dispatch system state change event
     * @param newState New system state
     * @param message Optional status message
     */
    void dispatchSystemStateChanged(uint8_t newState, const std::string& message = "");
    
    /**
     * @brief Dispatch script error event
     * @param scriptName Name of script that encountered error
     * @param error Error message
     */
    void dispatchScriptError(const std::string& scriptName, const std::string& error);
    
    /**
     * @brief Dispatch security violation event
     * @param violationType Type of security violation
     * @param details Violation details
     */
    void dispatchSecurityViolation(const std::string& violationType, const std::string& details);
    
    // === CUSTOM EVENTS ===
    
    /**
     * @brief Dispatch custom event
     * @param eventName Name of the custom event
     * @param data Optional data payload
     * @param targetUUID Target entity (0 for broadcast)
     * @param panelId Panel context (0 for all panels)
     * @param priority Event priority
     */
    void dispatchCustomEvent(const std::string& eventName, 
                           const std::string& data = "",
                           uint32_t targetUUID = 0,
                           uint16_t panelId = 0,
                           EventPriority priority = EventPriority::NORMAL);
    
    // === EVENT PROCESSING ===
    
    /**
     * @brief Process all queued events for this frame
     * Called once per frame by the main update loop
     */
    void processEvents();
    
    /**
     * @brief Process events with time limit
     * @param maxProcessingTimeMicros Maximum time to spend processing events
     * @return Number of events processed
     */
    uint32_t processEventsWithTimeLimit(uint32_t maxProcessingTimeMicros = MAX_PROCESSING_TIME_MICROS);
    
    // === EVENT FILTERING ===
    
    /**
     * @brief Enable or disable specific event types
     * @param eventType Type of event to enable/disable
     * @param enabled True to enable, false to disable
     */
    void setEventTypeEnabled(EventType eventType, bool enabled);
    
    /**
     * @brief Enable or disable events for specific panel
     * @param panelId Panel to enable/disable events for
     * @param enabled True to enable, false to disable
     */
    void setPanelEventsEnabled(uint16_t panelId, bool enabled);
    
    /**
     * @brief Check if event type is enabled
     * @param eventType Event type to check
     * @return True if enabled
     */
    bool isEventTypeEnabled(EventType eventType) const;
    
    // === PERFORMANCE MONITORING ===
    
    /**
     * @brief Get event dispatcher statistics
     */
    struct DispatcherStats {
        uint32_t eventsProcessedThisFrame;
        uint32_t totalEventsProcessed;
        uint32_t droppedEvents;
        uint32_t queueSize;
        uint32_t highPriorityQueueSize;
        uint32_t criticalQueueSize;
        uint32_t lastProcessingTimeMicros;
    };
    
    DispatcherStats getStats() const;
    
    /**
     * @brief Clear all event queues
     */
    void clearAllQueues();
    
    /**
     * @brief Dump current event queue state for debugging
     */
    void dumpEventQueues() const;

private:
    /**
     * @brief Add event to appropriate queue based on priority
     * @param event Event to enqueue
     * @return True if event was enqueued successfully
     */
    bool enqueueEvent(const GameEvent& event);
    
    /**
     * @brief Process a single event
     * @param event Event to process
     * @return True if event was processed successfully
     */
    bool processEvent(const GameEvent& event);
    
    /**
     * @brief Dispatch event to appropriate scripts
     * @param event Event to dispatch
     */
    void dispatchEventToScripts(const GameEvent& event);
    
    /**
     * @brief Validate event permissions and context
     * @param event Event to validate
     * @return True if event is valid and should be processed
     */
    bool validateEvent(const GameEvent& event);
    
    /**
     * @brief Get scripts that should receive this event
     * @param event Event to get recipients for
     * @return List of script contexts that should receive the event
     */
    std::vector<std::pair<std::string, uint32_t>> getEventRecipients(const GameEvent& event);
    
    /**
     * @brief Convert event to string for logging
     * @param event Event to convert
     * @return String representation of event
     */
    std::string eventToString(const GameEvent& event) const;
    
    /**
     * @brief Get current time in microseconds
     * @return Current time
     */
    uint32_t getCurrentTimeMicros() const;
    
    /**
     * @brief Initialize default event type settings
     */
    void initializeDefaultEventSettings();
};
