#pragma once
// Script Control Pipeline System
// Manages input events, script execution triggers, and control flow for both native and scripted logic

#include "engine_common.h"
#include "../app/curated_api_extended.h"
#include "script_system.h"

namespace WispEngine {
namespace Script {

// Control flow event types
enum ControlEventType : uint8_t {
    EVENT_INPUT,
    EVENT_TIMER,
    EVENT_COLLISION,
    EVENT_ANIMATION,
    EVENT_CUSTOM
};

// Control event structure
struct ControlEvent {
    ControlEventType type;
    uint32_t timestamp;
    uint16_t entityId;
    
    union {
        struct {
            WispInputSemantic input;
            bool pressed;
            int16_t value;
        } inputEvent;
        
        struct {
            uint16_t timerId;
            uint32_t elapsed;
        } timerEvent;
        
        struct {
            uint16_t otherId;
            CollisionResponse response;
        } collisionEvent;
        
        struct {
            uint8_t animationId;
            uint8_t frame;
            bool completed;
        } animationEvent;
        
        struct {
            char name[32];
            ScriptValue data;
        } customEvent;
    };
    
    ControlEvent() : type(EVENT_INPUT), timestamp(0), entityId(0) {
        memset(&inputEvent, 0, sizeof(inputEvent));
    }
};

// Script binding types
enum ScriptBindingType : uint8_t {
    BINDING_INPUT,      // Input event triggers script
    BINDING_TIMER,      // Timer completion triggers script
    BINDING_COLLISION,  // Collision triggers script
    BINDING_ANIMATION,  // Animation event triggers script
    BINDING_LIFECYCLE,  // Entity lifecycle events (spawn, destroy)
    BINDING_CUSTOM      // Custom event triggers script
};

// Script binding
struct ScriptBinding {
    ScriptBindingType type;
    uint16_t entityId;
    String scriptName;
    String functionName;
    bool enabled;
    uint32_t priority;  // Lower = higher priority
    
    union {
        WispInputSemantic inputTrigger;
        uint16_t timerTrigger;
        uint8_t animationTrigger;
        char customTrigger[32];
    };
    
    ScriptBinding() : type(BINDING_INPUT), entityId(0), enabled(true), priority(100) {
        inputTrigger = INPUT_UP;
    }
};

// Control pipeline state machine
enum PipelineState : uint8_t {
    PIPELINE_IDLE,
    PIPELINE_PROCESSING_INPUT,
    PIPELINE_EXECUTING_SCRIPTS,
    PIPELINE_UPDATING_COMPONENTS,
    PIPELINE_ERROR
};

// Main control pipeline class
class ControlPipeline {
private:
    ScriptSystem* scriptSystem;
    WispCuratedAPIExtended* api;
    
    // Pipeline state
    PipelineState currentState;
    uint32_t stateStartTime;
    uint32_t frameStartTime;
    
    // Event processing
    static const uint16_t MAX_EVENTS = 128;
    ControlEvent eventQueue[MAX_EVENTS];
    uint16_t eventQueueHead;
    uint16_t eventQueueTail;
    uint16_t eventCount;
    
    // Script bindings
    static const uint16_t MAX_BINDINGS = 256;
    ScriptBinding bindings[MAX_BINDINGS];
    uint16_t bindingCount;
    
    // Input state tracking
    WispInputState currentInput;
    WispInputState lastInput;
    uint32_t inputChangeTime[INPUT_SEMANTIC_COUNT];
    bool inputHeld[INPUT_SEMANTIC_COUNT];
    uint32_t inputHoldTime[INPUT_SEMANTIC_COUNT];
    
    // Input sequence detection
    struct InputSequence {
        WispInputSemantic sequence[8];
        uint8_t length;
        String name;
        uint32_t timeout;
        uint32_t lastTrigger;
        bool active;
    };
    static const uint8_t MAX_SEQUENCES = 16;
    InputSequence sequences[MAX_SEQUENCES];
    uint8_t sequenceCount;
    
    // Performance tracking
    uint32_t eventsProcessedThisFrame;
    uint32_t scriptsExecutedThisFrame;
    uint32_t totalProcessingTime;
    
    // Error handling
    uint16_t errorCount;
    String lastError;
    bool emergencyMode;
    
public:
    ControlPipeline();
    ~ControlPipeline();
    
    // Initialization
    bool initialize(ScriptSystem* scripts, WispCuratedAPIExtended* apiPtr);
    void shutdown();
    
    // === FRAME PROCESSING ===
    
    void beginFrame();
    void processInput(const WispInputState& input);
    void processTimerEvents();
    void processCollisionEvents();
    void processAnimationEvents();
    void executeScripts();
    void updateComponents();
    void endFrame();
    
    // === INPUT SYSTEM ===
    
    // Input event management
    void queueInputEvent(WispInputSemantic input, bool pressed, int16_t value = 0);
    void processInputEvents();
    
    // Input mapping and queries
    bool isInputPressed(WispInputSemantic input);
    bool isInputJustPressed(WispInputSemantic input);
    bool isInputJustReleased(WispInputSemantic input);
    int16_t getAnalogInput(WispInputSemantic input);
    uint32_t getInputHoldTime(WispInputSemantic input);
    
    // Input sequences (combos)
    bool registerInputSequence(const WispInputSemantic* sequence, uint8_t length, 
                              const String& name, uint32_t timeoutMs = 1000);
    bool wasSequenceTriggered(const String& name);
    void clearSequences();
    
    // === SCRIPT BINDING SYSTEM ===
    
    // Bind scripts to events
    bool bindInputScript(uint16_t entityId, WispInputSemantic input, 
                        const String& scriptName, const String& functionName = "onInput");
    bool bindTimerScript(uint16_t entityId, uint16_t timerId, 
                        const String& scriptName, const String& functionName = "onTimer");
    bool bindCollisionScript(uint16_t entityId, 
                           const String& scriptName, const String& functionName = "onCollision");
    bool bindAnimationScript(uint16_t entityId, uint8_t animationId,
                           const String& scriptName, const String& functionName = "onAnimation");
    bool bindCustomScript(uint16_t entityId, const String& eventName,
                         const String& scriptName, const String& functionName);
    
    // Binding management
    void unbindScript(uint16_t entityId, ScriptBindingType type);
    void unbindAllScripts(uint16_t entityId);
    void setBindingEnabled(uint16_t entityId, ScriptBindingType type, bool enabled);
    void setBindingPriority(uint16_t entityId, ScriptBindingType type, uint32_t priority);
    
    // Query bindings
    bool hasBinding(uint16_t entityId, ScriptBindingType type);
    uint16_t getBindingCount(uint16_t entityId);
    uint16_t getAllBindingCount() const { return bindingCount; }
    
    // === EVENT SYSTEM ===
    
    // Queue events for processing
    void queueTimerEvent(uint16_t entityId, uint16_t timerId, uint32_t elapsed);
    void queueCollisionEvent(uint16_t entityId, uint16_t otherId, CollisionResponse response);
    void queueAnimationEvent(uint16_t entityId, uint8_t animationId, uint8_t frame, bool completed);
    void queueCustomEvent(uint16_t entityId, const String& eventName, const ScriptValue& data);
    
    // Event processing
    bool processEvent(const ControlEvent& event);
    void clearEventQueue();
    uint16_t getQueuedEventCount() const { return eventCount; }
    
    // === SCRIPT EXECUTION CONTROL ===
    
    // Execute scripts with context
    bool executeScriptWithContext(const String& scriptName, const String& functionName, 
                                 uint16_t entityId, const ControlEvent& event);
    bool executeInputScript(const ScriptBinding& binding, const ControlEvent& event);
    bool executeTimerScript(const ScriptBinding& binding, const ControlEvent& event);
    bool executeCollisionScript(const ScriptBinding& binding, const ControlEvent& event);
    
    // Script context management
    void setScriptContext(uint16_t entityId);
    void clearScriptContext();
    uint16_t getCurrentScriptEntity() const;
    
    // === COMPONENT INTEGRATION ===
    
    // Component event integration
    void onComponentCreated(uint16_t entityId, const String& componentType);
    void onComponentDestroyed(uint16_t entityId, const String& componentType);
    void onComponentChanged(uint16_t entityId, const String& componentType, const String& property);
    
    // Component script bindings
    void bindComponentScript(uint16_t entityId, const String& componentType, 
                            const String& scriptName);
    void updateComponentScripts();
    
    // === PERFORMANCE MONITORING ===
    
    // Performance metrics
    uint32_t getEventsProcessedThisFrame() const { return eventsProcessedThisFrame; }
    uint32_t getScriptsExecutedThisFrame() const { return scriptsExecutedThisFrame; }
    uint32_t getTotalProcessingTime() const { return totalProcessingTime; }
    float getAverageProcessingTime() const;
    
    // Performance limits
    void setMaxEventsPerFrame(uint16_t maxEvents);
    void setMaxScriptsPerFrame(uint16_t maxScripts);
    void setMaxProcessingTimeMs(uint32_t maxTimeMs);
    
    // Performance optimization
    void optimizeBindings();
    void optimizeEventQueue();
    void enablePerformanceMode(bool enable);
    
    // === ERROR HANDLING ===
    
    // Error state
    bool isInErrorState() const { return currentState == PIPELINE_ERROR; }
    bool isInEmergencyMode() const { return emergencyMode; }
    uint16_t getErrorCount() const { return errorCount; }
    const String& getLastError() const { return lastError; }
    
    // Error recovery
    void clearErrors();
    void resetPipeline();
    void enterEmergencyMode();
    void exitEmergencyMode();
    
    // === DEBUG AND DIAGNOSTICS ===
    
    // State information
    PipelineState getCurrentState() const { return currentState; }
    uint32_t getStateTime() const;
    void printPipelineState();
    void printBindings();
    void printEventQueue();
    
    // Performance reporting
    void printPerformanceReport();
    void printScriptExecutionStats();
    void resetPerformanceCounters();
    
    // Validation
    bool validateBindings();
    bool validateEventQueue();
    bool validateScriptReferences();
    
private:
    // Internal state management
    void setState(PipelineState newState);
    bool canProcessEvents() const;
    bool canExecuteScripts() const;
    
    // Event queue management
    bool queueEvent(const ControlEvent& event);
    ControlEvent* dequeueEvent();
    bool isEventQueueFull() const;
    void compactEventQueue();
    
    // Binding management
    ScriptBinding* findBinding(uint16_t entityId, ScriptBindingType type, uint32_t trigger = 0);
    bool addBinding(const ScriptBinding& binding);
    void removeBinding(uint16_t index);
    void sortBindingsByPriority();
    
    // Input processing
    void updateInputState(const WispInputState& input);
    void detectInputChanges();
    void processInputSequences();
    bool matchesSequence(const InputSequence& seq);
    
    // Script execution
    bool executeBinding(const ScriptBinding& binding, const ControlEvent& event);
    void setupScriptParameters(const ControlEvent& event);
    void handleScriptError(const String& scriptName, const String& error);
    
    // Performance management
    void updatePerformanceMetrics();
    bool checkPerformanceLimits();
    void throttleProcessing();
    
    // Error handling
    void logError(const String& error);
    void handleCriticalError(const String& error);
    
    // Memory management
    void cleanupBindings();
    void cleanupEvents();
    void optimizeMemoryUsage();
};

// Control pipeline integration functions
bool initializeControlPipeline(ScriptSystem* scripts, WispCuratedAPIExtended* api);
void shutdownControlPipeline();
ControlPipeline* getControlPipeline();

// Convenience macros for script bindings
#define BIND_INPUT_SCRIPT(entity, input, script) \
    getControlPipeline()->bindInputScript(entity, input, script)

#define BIND_TIMER_SCRIPT(entity, timer, script) \
    getControlPipeline()->bindTimerScript(entity, timer, script)

#define BIND_COLLISION_SCRIPT(entity, script) \
    getControlPipeline()->bindCollisionScript(entity, script)

#define QUEUE_CUSTOM_EVENT(entity, event, data) \
    getControlPipeline()->queueCustomEvent(entity, event, data)

// Pipeline state helpers
#define IS_INPUT_PRESSED(input) getControlPipeline()->isInputPressed(input)
#define IS_INPUT_JUST_PRESSED(input) getControlPipeline()->isInputJustPressed(input)
#define WAS_SEQUENCE_TRIGGERED(name) getControlPipeline()->wasSequenceTriggered(name)

} // namespace Script
} // namespace WispEngine
