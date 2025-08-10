#include "scene_event_dispatcher.h"
#include <algorithm>
#include <cstring>

SceneEventDispatcher::SceneEventDispatcher(ScriptInstanceAuthority* scriptAuth, 
                                         EngineUUIDAuthority* uuidAuth,
                                         NamedEntityRegistry* namedReg,
                                         SceneManager* sceneMgr)
    : scriptAuthority(scriptAuth), uuidAuthority(uuidAuth), namedRegistry(namedReg),
      sceneManager(sceneMgr), eventsProcessedThisFrame(0), totalEventsProcessed(0),
      droppedEvents(0), lastFrameTime(0) {
    
    ESP_LOGI(EVENT_DISPATCHER_TAG, "SceneEventDispatcher created");
}

SceneEventDispatcher::~SceneEventDispatcher() {
    ESP_LOGI(EVENT_DISPATCHER_TAG, "SceneEventDispatcher destructor called");
    shutdown();
}

bool SceneEventDispatcher::initialize() {
    if (!scriptAuthority || !uuidAuthority) {
        ESP_LOGE(EVENT_DISPATCHER_TAG, "Cannot initialize: missing required systems");
        return false;
    }
    
    ESP_LOGI(EVENT_DISPATCHER_TAG, "Initializing SceneEventDispatcher");
    
    // Initialize default event settings
    initializeDefaultEventSettings();
    
    // Clear all queues
    clearAllQueues();
    
    // Reset performance counters
    eventsProcessedThisFrame = 0;
    totalEventsProcessed = 0;
    droppedEvents = 0;
    lastFrameTime = getCurrentTimeMicros();
    
    ESP_LOGI(EVENT_DISPATCHER_TAG, "SceneEventDispatcher initialized successfully");
    ESP_LOGI(EVENT_DISPATCHER_TAG, "Max events per frame: %d, Max queue size: %d, Max processing time: %d μs", 
             MAX_EVENTS_PER_FRAME, MAX_QUEUE_SIZE, MAX_PROCESSING_TIME_MICROS);
    
    return true;
}

void SceneEventDispatcher::shutdown() {
    ESP_LOGI(EVENT_DISPATCHER_TAG, "Shutting down SceneEventDispatcher");
    ESP_LOGI(EVENT_DISPATCHER_TAG, "Final stats - Total events processed: %lu, Events dropped: %lu", 
             totalEventsProcessed, droppedEvents);
    
    clearAllQueues();
}

// === ENTITY EVENTS ===

void SceneEventDispatcher::dispatchEntitySpawned(uint32_t uuid, uint16_t panelId, const std::string& entityType) {
    GameEvent event;
    event.type = EventType::ENTITY_SPAWNED;
    event.priority = EventPriority::NORMAL;
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = uuid;
    event.targetUUID = 0; // Broadcast to all scripts
    event.panelId = panelId;
    
    if (!enqueueEvent(event)) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Failed to enqueue ENTITY_SPAWNED event for UUID %lu", uuid);
    } else {
        ESP_LOGD(EVENT_DISPATCHER_TAG, "Entity spawned event dispatched: UUID %lu, Panel %d", uuid, panelId);
    }
}

void SceneEventDispatcher::dispatchEntityDestroyed(uint32_t uuid, uint32_t destroyerUUID) {
    GameEvent event;
    event.type = EventType::ENTITY_DESTROYED;
    event.priority = EventPriority::HIGH; // Destruction events are high priority
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = destroyerUUID;
    event.targetUUID = uuid;
    
    // Get panel ID from UUID authority
    if (uuidAuthority) {
        // TODO: Add method to get panel ID from UUID
        // event.panelId = uuidAuthority->getPanelId(uuid);
        event.panelId = 0;
    }
    
    if (!enqueueEvent(event)) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Failed to enqueue ENTITY_DESTROYED event for UUID %lu", uuid);
    } else {
        ESP_LOGD(EVENT_DISPATCHER_TAG, "Entity destroyed event dispatched: UUID %lu, Destroyer %lu", 
                 uuid, destroyerUUID);
    }
}

void SceneEventDispatcher::dispatchEntityCollision(uint32_t entityA, uint32_t entityB) {
    GameEvent event;
    event.type = EventType::ENTITY_COLLISION;
    event.priority = EventPriority::HIGH; // Collisions are time-sensitive
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = entityA;
    event.targetUUID = entityB;
    event.entity.entityA = entityA;
    event.entity.entityB = entityB;
    
    if (!enqueueEvent(event)) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Failed to enqueue ENTITY_COLLISION event: %lu <-> %lu", 
                 entityA, entityB);
    } else {
        ESP_LOGD(EVENT_DISPATCHER_TAG, "Entity collision event dispatched: %lu <-> %lu", entityA, entityB);
    }
}

void SceneEventDispatcher::dispatchAnimationComplete(uint32_t entityUUID, uint8_t animationId) {
    GameEvent event;
    event.type = EventType::ENTITY_ANIMATION_COMPLETE;
    event.priority = EventPriority::NORMAL;
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = entityUUID;
    event.targetUUID = entityUUID; // Target the entity whose animation completed
    event.entity.animationId = animationId;
    
    if (!enqueueEvent(event)) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Failed to enqueue ENTITY_ANIMATION_COMPLETE event: UUID %lu, Anim %d", 
                 entityUUID, animationId);
    } else {
        ESP_LOGD(EVENT_DISPATCHER_TAG, "Animation complete event dispatched: UUID %lu, Animation %d", 
                 entityUUID, animationId);
    }
}

void SceneEventDispatcher::dispatchEntityStateChanged(uint32_t entityUUID, uint8_t newState) {
    GameEvent event;
    event.type = EventType::ENTITY_STATE_CHANGED;
    event.priority = EventPriority::NORMAL;
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = entityUUID;
    event.targetUUID = entityUUID;
    event.entity.newState = newState;
    
    if (!enqueueEvent(event)) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Failed to enqueue ENTITY_STATE_CHANGED event: UUID %lu, State %d", 
                 entityUUID, newState);
    } else {
        ESP_LOGD(EVENT_DISPATCHER_TAG, "Entity state changed event dispatched: UUID %lu, New State %d", 
                 entityUUID, newState);
    }
}

// === SCENE EVENTS ===

void SceneEventDispatcher::dispatchSceneLoadStart(const std::string& sceneName) {
    GameEvent event;
    event.type = EventType::SCENE_LOAD_START;
    event.priority = EventPriority::HIGH; // Scene transitions are important
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = 0;
    event.targetUUID = 0; // Broadcast
    event.panelId = 0; // All panels
    
    strncpy(event.scene.sceneName, sceneName.c_str(), sizeof(event.scene.sceneName) - 1);
    event.scene.sceneName[sizeof(event.scene.sceneName) - 1] = '\0';
    
    if (!enqueueEvent(event)) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Failed to enqueue SCENE_LOAD_START event: %s", sceneName.c_str());
    } else {
        ESP_LOGI(EVENT_DISPATCHER_TAG, "Scene load start event dispatched: %s", sceneName.c_str());
    }
}

void SceneEventDispatcher::dispatchSceneLoadComplete(const std::string& sceneName) {
    GameEvent event;
    event.type = EventType::SCENE_LOAD_COMPLETE;
    event.priority = EventPriority::HIGH;
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = 0;
    event.targetUUID = 0; // Broadcast
    event.panelId = 0; // All panels
    
    strncpy(event.scene.sceneName, sceneName.c_str(), sizeof(event.scene.sceneName) - 1);
    event.scene.sceneName[sizeof(event.scene.sceneName) - 1] = '\0';
    
    if (!enqueueEvent(event)) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Failed to enqueue SCENE_LOAD_COMPLETE event: %s", sceneName.c_str());
    } else {
        ESP_LOGI(EVENT_DISPATCHER_TAG, "Scene load complete event dispatched: %s", sceneName.c_str());
    }
}

void SceneEventDispatcher::dispatchSceneUnloadStart(const std::string& sceneName) {
    GameEvent event;
    event.type = EventType::SCENE_UNLOAD_START;
    event.priority = EventPriority::HIGH;
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = 0;
    event.targetUUID = 0;
    event.panelId = 0;
    
    strncpy(event.scene.sceneName, sceneName.c_str(), sizeof(event.scene.sceneName) - 1);
    event.scene.sceneName[sizeof(event.scene.sceneName) - 1] = '\0';
    
    if (!enqueueEvent(event)) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Failed to enqueue SCENE_UNLOAD_START event: %s", sceneName.c_str());
    } else {
        ESP_LOGI(EVENT_DISPATCHER_TAG, "Scene unload start event dispatched: %s", sceneName.c_str());
    }
}

void SceneEventDispatcher::dispatchSceneUnloadComplete(const std::string& sceneName) {
    GameEvent event;
    event.type = EventType::SCENE_UNLOAD_COMPLETE;
    event.priority = EventPriority::HIGH;
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = 0;
    event.targetUUID = 0;
    event.panelId = 0;
    
    strncpy(event.scene.sceneName, sceneName.c_str(), sizeof(event.scene.sceneName) - 1);
    event.scene.sceneName[sizeof(event.scene.sceneName) - 1] = '\0';
    
    if (!enqueueEvent(event)) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Failed to enqueue SCENE_UNLOAD_COMPLETE event: %s", sceneName.c_str());
    } else {
        ESP_LOGI(EVENT_DISPATCHER_TAG, "Scene unload complete event dispatched: %s", sceneName.c_str());
    }
}

void SceneEventDispatcher::dispatchPanelSwitch(uint16_t oldPanelId, uint16_t newPanelId) {
    GameEvent event;
    event.type = EventType::PANEL_SWITCHED;
    event.priority = EventPriority::HIGH;
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = 0;
    event.targetUUID = 0; // Broadcast
    event.panelId = newPanelId;
    event.scene.oldPanelId = oldPanelId;
    
    if (!enqueueEvent(event)) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Failed to enqueue PANEL_SWITCHED event: %d -> %d", 
                 oldPanelId, newPanelId);
    } else {
        ESP_LOGI(EVENT_DISPATCHER_TAG, "Panel switch event dispatched: %d -> %d", oldPanelId, newPanelId);
    }
}

// === INPUT EVENTS ===

void SceneEventDispatcher::dispatchInputPressed(WispInputSemantic input) {
    GameEvent event;
    event.type = EventType::INPUT_PRESSED;
    event.priority = EventPriority::HIGH; // Input events are time-sensitive
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = 0;
    event.targetUUID = 0; // Broadcast to all scripts
    event.panelId = 0; // All panels can receive input
    event.input.input = input;
    event.input.pressed = true;
    
    if (!enqueueEvent(event)) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Failed to enqueue INPUT_PRESSED event: %d", (int)input);
    } else {
        ESP_LOGV(EVENT_DISPATCHER_TAG, "Input pressed event dispatched: %d", (int)input);
    }
}

void SceneEventDispatcher::dispatchInputReleased(WispInputSemantic input) {
    GameEvent event;
    event.type = EventType::INPUT_RELEASED;
    event.priority = EventPriority::HIGH;
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = 0;
    event.targetUUID = 0;
    event.panelId = 0;
    event.input.input = input;
    event.input.pressed = false;
    
    if (!enqueueEvent(event)) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Failed to enqueue INPUT_RELEASED event: %d", (int)input);
    } else {
        ESP_LOGV(EVENT_DISPATCHER_TAG, "Input released event dispatched: %d", (int)input);
    }
}

void SceneEventDispatcher::dispatchInputHeld(WispInputSemantic input) {
    GameEvent event;
    event.type = EventType::INPUT_HELD;
    event.priority = EventPriority::NORMAL; // Held events are lower priority
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = 0;
    event.targetUUID = 0;
    event.panelId = 0;
    event.input.input = input;
    event.input.pressed = true; // Held means it's currently pressed
    
    if (!enqueueEvent(event)) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Failed to enqueue INPUT_HELD event: %d", (int)input);
    } else {
        ESP_LOGV(EVENT_DISPATCHER_TAG, "Input held event dispatched: %d", (int)input);
    }
}

// === SYSTEM EVENTS ===

void SceneEventDispatcher::dispatchTimerExpired(uint16_t timerId) {
    GameEvent event;
    event.type = EventType::TIMER_EXPIRED;
    event.priority = EventPriority::NORMAL;
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = 0;
    event.targetUUID = 0; // Broadcast
    event.panelId = 0;
    event.system.timerId = timerId;
    
    if (!enqueueEvent(event)) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Failed to enqueue TIMER_EXPIRED event: %d", timerId);
    } else {
        ESP_LOGD(EVENT_DISPATCHER_TAG, "Timer expired event dispatched: %d", timerId);
    }
}

void SceneEventDispatcher::dispatchSystemStateChanged(uint8_t newState, const std::string& message) {
    GameEvent event;
    event.type = EventType::SYSTEM_STATE_CHANGED;
    event.priority = EventPriority::HIGH; // System state changes are important
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = 0;
    event.targetUUID = 0; // Broadcast
    event.panelId = 0;
    event.system.systemState = newState;
    
    // Copy message to error message field (reusing for system message)
    strncpy(event.system.errorMessage, message.c_str(), sizeof(event.system.errorMessage) - 1);
    event.system.errorMessage[sizeof(event.system.errorMessage) - 1] = '\0';
    
    if (!enqueueEvent(event)) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Failed to enqueue SYSTEM_STATE_CHANGED event: %d", newState);
    } else {
        ESP_LOGI(EVENT_DISPATCHER_TAG, "System state changed event dispatched: %d (%s)", 
                 newState, message.c_str());
    }
}

void SceneEventDispatcher::dispatchScriptError(const std::string& scriptName, const std::string& error) {
    GameEvent event;
    event.type = EventType::SCRIPT_ERROR;
    event.priority = EventPriority::CRITICAL; // Script errors are critical
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = 0;
    event.targetUUID = 0; // Broadcast
    event.panelId = 0;
    
    // Store script name in custom event name field and error in custom data
    strncpy(event.custom.eventName, scriptName.c_str(), sizeof(event.custom.eventName) - 1);
    event.custom.eventName[sizeof(event.custom.eventName) - 1] = '\0';
    
    strncpy(event.custom.data, error.c_str(), sizeof(event.custom.data) - 1);
    event.custom.data[sizeof(event.custom.data) - 1] = '\0';
    
    if (!enqueueEvent(event)) {
        ESP_LOGE(EVENT_DISPATCHER_TAG, "Failed to enqueue SCRIPT_ERROR event: %s", scriptName.c_str());
    } else {
        ESP_LOGE(EVENT_DISPATCHER_TAG, "Script error event dispatched: %s - %s", scriptName.c_str(), error.c_str());
    }
}

void SceneEventDispatcher::dispatchSecurityViolation(const std::string& violationType, const std::string& details) {
    GameEvent event;
    event.type = EventType::SECURITY_VIOLATION;
    event.priority = EventPriority::CRITICAL; // Security violations are critical
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = 0;
    event.targetUUID = 0; // Broadcast
    event.panelId = 0;
    
    // Store violation type and details
    strncpy(event.custom.eventName, violationType.c_str(), sizeof(event.custom.eventName) - 1);
    event.custom.eventName[sizeof(event.custom.eventName) - 1] = '\0';
    
    strncpy(event.custom.data, details.c_str(), sizeof(event.custom.data) - 1);
    event.custom.data[sizeof(event.custom.data) - 1] = '\0';
    
    if (!enqueueEvent(event)) {
        ESP_LOGE(EVENT_DISPATCHER_TAG, "Failed to enqueue SECURITY_VIOLATION event: %s", violationType.c_str());
    } else {
        ESP_LOGE(EVENT_DISPATCHER_TAG, "Security violation event dispatched: %s - %s", 
                 violationType.c_str(), details.c_str());
    }
}

// === CUSTOM EVENTS ===

void SceneEventDispatcher::dispatchCustomEvent(const std::string& eventName, 
                                              const std::string& data,
                                              uint32_t targetUUID,
                                              uint16_t panelId,
                                              EventPriority priority) {
    GameEvent event;
    event.type = EventType::CUSTOM_EVENT;
    event.priority = priority;
    event.timestamp = getCurrentTimeMicros();
    event.sourceUUID = 0;
    event.targetUUID = targetUUID;
    event.panelId = panelId;
    
    strncpy(event.custom.eventName, eventName.c_str(), sizeof(event.custom.eventName) - 1);
    event.custom.eventName[sizeof(event.custom.eventName) - 1] = '\0';
    
    strncpy(event.custom.data, data.c_str(), sizeof(event.custom.data) - 1);
    event.custom.data[sizeof(event.custom.data) - 1] = '\0';
    
    if (!enqueueEvent(event)) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Failed to enqueue CUSTOM_EVENT: %s", eventName.c_str());
    } else {
        ESP_LOGD(EVENT_DISPATCHER_TAG, "Custom event dispatched: %s (target: %lu, panel: %d)", 
                 eventName.c_str(), targetUUID, panelId);
    }
}

// === EVENT PROCESSING ===

void SceneEventDispatcher::processEvents() {
    processEventsWithTimeLimit(MAX_PROCESSING_TIME_MICROS);
}

uint32_t SceneEventDispatcher::processEventsWithTimeLimit(uint32_t maxProcessingTimeMicros) {
    uint32_t startTime = getCurrentTimeMicros();
    uint32_t eventsProcessed = 0;
    eventsProcessedThisFrame = 0;
    
    // Process critical events first (no time limit)
    while (!criticalQueue.empty()) {
        GameEvent event = criticalQueue.front();
        criticalQueue.pop();
        
        if (processEvent(event)) {
            eventsProcessed++;
            eventsProcessedThisFrame++;
            totalEventsProcessed++;
        }
        
        if (eventsProcessedThisFrame >= MAX_EVENTS_PER_FRAME) {
            ESP_LOGW(EVENT_DISPATCHER_TAG, "Hit max events per frame limit processing critical events");
            break;
        }
    }
    
    // Process high priority events
    while (!highPriorityQueue.empty() && eventsProcessedThisFrame < MAX_EVENTS_PER_FRAME) {
        uint32_t currentTime = getCurrentTimeMicros();
        if (currentTime - startTime > maxProcessingTimeMicros) {
            ESP_LOGD(EVENT_DISPATCHER_TAG, "Hit processing time limit during high priority events");
            break;
        }
        
        GameEvent event = highPriorityQueue.front();
        highPriorityQueue.pop();
        
        if (processEvent(event)) {
            eventsProcessed++;
            eventsProcessedThisFrame++;
            totalEventsProcessed++;
        }
    }
    
    // Process normal events
    while (!eventQueue.empty() && eventsProcessedThisFrame < MAX_EVENTS_PER_FRAME) {
        uint32_t currentTime = getCurrentTimeMicros();
        if (currentTime - startTime > maxProcessingTimeMicros) {
            ESP_LOGD(EVENT_DISPATCHER_TAG, "Hit processing time limit during normal events");
            break;
        }
        
        GameEvent event = eventQueue.front();
        eventQueue.pop();
        
        if (processEvent(event)) {
            eventsProcessed++;
            eventsProcessedThisFrame++;
            totalEventsProcessed++;
        }
    }
    
    uint32_t endTime = getCurrentTimeMicros();
    lastFrameTime = endTime - startTime;
    
    if (eventsProcessed > 0) {
        ESP_LOGV(EVENT_DISPATCHER_TAG, "Processed %lu events in %lu μs", eventsProcessed, lastFrameTime);
    }
    
    return eventsProcessed;
}

// === EVENT FILTERING ===

void SceneEventDispatcher::setEventTypeEnabled(EventType eventType, bool enabled) {
    eventTypeEnabled[eventType] = enabled;
    ESP_LOGD(EVENT_DISPATCHER_TAG, "Event type %d %s", (int)eventType, enabled ? "enabled" : "disabled");
}

void SceneEventDispatcher::setPanelEventsEnabled(uint16_t panelId, bool enabled) {
    panelEventsEnabled[panelId] = enabled;
    ESP_LOGD(EVENT_DISPATCHER_TAG, "Panel %d events %s", panelId, enabled ? "enabled" : "disabled");
}

bool SceneEventDispatcher::isEventTypeEnabled(EventType eventType) const {
    auto it = eventTypeEnabled.find(eventType);
    return it != eventTypeEnabled.end() ? it->second : true; // Default to enabled
}

// === PERFORMANCE MONITORING ===

SceneEventDispatcher::DispatcherStats SceneEventDispatcher::getStats() const {
    return {
        eventsProcessedThisFrame,
        totalEventsProcessed,
        droppedEvents,
        static_cast<uint32_t>(eventQueue.size()),
        static_cast<uint32_t>(highPriorityQueue.size()),
        static_cast<uint32_t>(criticalQueue.size()),
        lastFrameTime
    };
}

void SceneEventDispatcher::clearAllQueues() {
    // Clear all event queues
    std::queue<GameEvent> empty;
    eventQueue.swap(empty);
    highPriorityQueue.swap(empty);
    criticalQueue.swap(empty);
    
    ESP_LOGD(EVENT_DISPATCHER_TAG, "All event queues cleared");
}

void SceneEventDispatcher::dumpEventQueues() const {
    ESP_LOGI(EVENT_DISPATCHER_TAG, "=== Event Queue Status ===");
    ESP_LOGI(EVENT_DISPATCHER_TAG, "Normal events: %zu", eventQueue.size());
    ESP_LOGI(EVENT_DISPATCHER_TAG, "High priority events: %zu", highPriorityQueue.size());
    ESP_LOGI(EVENT_DISPATCHER_TAG, "Critical events: %zu", criticalQueue.size());
    ESP_LOGI(EVENT_DISPATCHER_TAG, "Total events this frame: %lu", eventsProcessedThisFrame);
    ESP_LOGI(EVENT_DISPATCHER_TAG, "Total events processed: %lu", totalEventsProcessed);
    ESP_LOGI(EVENT_DISPATCHER_TAG, "Dropped events: %lu", droppedEvents);
    ESP_LOGI(EVENT_DISPATCHER_TAG, "Last processing time: %lu μs", lastFrameTime);
    ESP_LOGI(EVENT_DISPATCHER_TAG, "=========================");
}

// === PRIVATE METHODS ===

bool SceneEventDispatcher::enqueueEvent(const GameEvent& event) {
    // Check if event type is enabled
    if (!isEventTypeEnabled(event.type)) {
        ESP_LOGV(EVENT_DISPATCHER_TAG, "Event type %d is disabled, dropping", (int)event.type);
        return false;
    }
    
    // Check if panel events are enabled
    if (event.panelId != 0) {
        auto it = panelEventsEnabled.find(event.panelId);
        if (it != panelEventsEnabled.end() && !it->second) {
            ESP_LOGV(EVENT_DISPATCHER_TAG, "Panel %d events disabled, dropping", event.panelId);
            return false;
        }
    }
    
    // Validate the event
    if (!validateEvent(event)) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Event validation failed, dropping: %s", eventToString(event).c_str());
        droppedEvents++;
        return false;
    }
    
    // Add to appropriate queue based on priority
    switch (event.priority) {
        case EventPriority::CRITICAL:
            if (criticalQueue.size() >= MAX_QUEUE_SIZE / 4) {
                ESP_LOGW(EVENT_DISPATCHER_TAG, "Critical queue full, dropping event");
                droppedEvents++;
                return false;
            }
            criticalQueue.push(event);
            break;
            
        case EventPriority::HIGH:
            if (highPriorityQueue.size() >= MAX_QUEUE_SIZE / 2) {
                ESP_LOGW(EVENT_DISPATCHER_TAG, "High priority queue full, dropping event");
                droppedEvents++;
                return false;
            }
            highPriorityQueue.push(event);
            break;
            
        case EventPriority::NORMAL:
        case EventPriority::LOW:
        default:
            if (eventQueue.size() >= MAX_QUEUE_SIZE) {
                ESP_LOGW(EVENT_DISPATCHER_TAG, "Normal event queue full, dropping event");
                droppedEvents++;
                return false;
            }
            eventQueue.push(event);
            break;
    }
    
    return true;
}

bool SceneEventDispatcher::processEvent(const GameEvent& event) {
    ESP_LOGV(EVENT_DISPATCHER_TAG, "Processing event: %s", eventToString(event).c_str());
    
    try {
        // Dispatch to appropriate scripts
        dispatchEventToScripts(event);
        return true;
    } catch (const std::exception& e) {
        ESP_LOGE(EVENT_DISPATCHER_TAG, "Exception processing event: %s", e.what());
        return false;
    }
}

void SceneEventDispatcher::dispatchEventToScripts(const GameEvent& event) {
    if (!scriptAuthority) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "No script authority available for event dispatch");
        return;
    }
    
    // Dispatch based on event type
    switch (event.type) {
        case EventType::ENTITY_COLLISION:
            scriptAuthority->dispatchCollisionEvent(event.entity.entityA, event.entity.entityB);
            break;
            
        case EventType::INPUT_PRESSED:
            scriptAuthority->dispatchInputEvent(event.input.input, true);
            break;
            
        case EventType::INPUT_RELEASED:
            scriptAuthority->dispatchInputEvent(event.input.input, false);
            break;
            
        case EventType::TIMER_EXPIRED:
            scriptAuthority->dispatchTimerEvent(event.system.timerId);
            break;
            
        case EventType::ENTITY_ANIMATION_COMPLETE:
            scriptAuthority->dispatchAnimationEvent(event.targetUUID, event.entity.animationId);
            break;
            
        case EventType::CUSTOM_EVENT:
            // TODO: Add custom event dispatch to script authority
            ESP_LOGD(EVENT_DISPATCHER_TAG, "Custom event: %s", event.custom.eventName);
            break;
            
        default:
            ESP_LOGV(EVENT_DISPATCHER_TAG, "Event type %d not handled by script dispatch", (int)event.type);
            break;
    }
}

bool SceneEventDispatcher::validateEvent(const GameEvent& event) {
    // Basic validation
    if (event.timestamp == 0) {
        ESP_LOGW(EVENT_DISPATCHER_TAG, "Event has no timestamp");
        return false;
    }
    
    // Validate UUIDs if they are set
    if (event.sourceUUID != 0 && uuidAuthority) {
        if (!uuidAuthority->validateUUID(event.sourceUUID, "event_dispatch")) {
            ESP_LOGW(EVENT_DISPATCHER_TAG, "Invalid source UUID: %lu", event.sourceUUID);
            return false;
        }
    }
    
    if (event.targetUUID != 0 && uuidAuthority) {
        if (!uuidAuthority->validateUUID(event.targetUUID, "event_dispatch")) {
            ESP_LOGW(EVENT_DISPATCHER_TAG, "Invalid target UUID: %lu", event.targetUUID);
            return false;
        }
    }
    
    return true;
}

std::string SceneEventDispatcher::eventToString(const GameEvent& event) const {
    std::string result = "Event{";
    
    // Add event type
    result += "type:" + std::to_string((int)event.type);
    result += ", priority:" + std::to_string((int)event.priority);
    result += ", timestamp:" + std::to_string(event.timestamp);
    
    if (event.sourceUUID != 0) {
        result += ", source:" + std::to_string(event.sourceUUID);
    }
    
    if (event.targetUUID != 0) {
        result += ", target:" + std::to_string(event.targetUUID);
    }
    
    if (event.panelId != 0) {
        result += ", panel:" + std::to_string(event.panelId);
    }
    
    // Add event-specific data based on type
    switch (event.type) {
        case EventType::ENTITY_COLLISION:
            result += ", entityA:" + std::to_string(event.entity.entityA);
            result += ", entityB:" + std::to_string(event.entity.entityB);
            break;
            
        case EventType::INPUT_PRESSED:
        case EventType::INPUT_RELEASED:
        case EventType::INPUT_HELD:
            result += ", input:" + std::to_string((int)event.input.input);
            break;
            
        case EventType::CUSTOM_EVENT:
            result += ", name:" + std::string(event.custom.eventName);
            if (strlen(event.custom.data) > 0) {
                result += ", data:" + std::string(event.custom.data);
            }
            break;
            
        default:
            break;
    }
    
    result += "}";
    return result;
}

uint32_t SceneEventDispatcher::getCurrentTimeMicros() const {
    return esp_timer_get_time();
}

void SceneEventDispatcher::initializeDefaultEventSettings() {
    // Enable all event types by default
    eventTypeEnabled[EventType::ENTITY_SPAWNED] = true;
    eventTypeEnabled[EventType::ENTITY_DESTROYED] = true;
    eventTypeEnabled[EventType::ENTITY_COLLISION] = true;
    eventTypeEnabled[EventType::ENTITY_ANIMATION_COMPLETE] = true;
    eventTypeEnabled[EventType::ENTITY_STATE_CHANGED] = true;
    
    eventTypeEnabled[EventType::SCENE_LOAD_START] = true;
    eventTypeEnabled[EventType::SCENE_LOAD_COMPLETE] = true;
    eventTypeEnabled[EventType::SCENE_UNLOAD_START] = true;
    eventTypeEnabled[EventType::SCENE_UNLOAD_COMPLETE] = true;
    eventTypeEnabled[EventType::PANEL_SWITCHED] = true;
    
    eventTypeEnabled[EventType::INPUT_PRESSED] = true;
    eventTypeEnabled[EventType::INPUT_RELEASED] = true;
    eventTypeEnabled[EventType::INPUT_HELD] = false; // Disabled by default to reduce spam
    
    eventTypeEnabled[EventType::TIMER_EXPIRED] = true;
    eventTypeEnabled[EventType::SYSTEM_STATE_CHANGED] = true;
    eventTypeEnabled[EventType::SCRIPT_ERROR] = true;
    eventTypeEnabled[EventType::SECURITY_VIOLATION] = true;
    
    eventTypeEnabled[EventType::CUSTOM_EVENT] = true;
    
    ESP_LOGD(EVENT_DISPATCHER_TAG, "Default event settings initialized");
}
