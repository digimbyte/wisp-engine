#pragma once
// Extended Wisp Curated API - Component System Integration
// Extends the existing API with component-based development while maintaining backward compatibility

#include "curated_api.h"
#include "../core/component_systems.h"
#include "../script/script_system.h"

using namespace WispEngine::Core;
using namespace WispEngine::Script;

// Extended input system with semantic mapping
enum WispInputSemantic : uint8_t {
    INPUT_UP = 0,
    INPUT_DOWN,
    INPUT_LEFT, 
    INPUT_RIGHT,
    INPUT_ACCEPT,     // Primary action (A button, Enter, Tap)
    INPUT_BACK,       // Cancel/Back (B button, Escape, Back gesture)
    INPUT_MENU,       // Menu/Options (Start, Menu button)
    INPUT_ALT,        // Alternative action (C button, Shift, Alt tap)
    INPUT_PAUSE,      // Pause/Select
    INPUT_ANALOG_X,   // Analog stick X (-100 to +100)
    INPUT_ANALOG_Y,   // Analog stick Y (-100 to +100)
    INPUT_TOUCH,      // Touch/Click
    INPUT_SEMANTIC_COUNT
};

// Input event structure for scripts
struct WispInputEvent {
    WispInputSemantic input;
    bool pressed;        // True for press, false for release
    bool justChanged;    // True if state changed this frame
    int16_t value;       // For analog inputs (-100 to +100)
    uint32_t timestamp;  // When the event occurred
    
    WispInputEvent() : input(INPUT_UP), pressed(false), justChanged(false), value(0), timestamp(0) {}
};

// Script control pipeline interface
class ScriptControlPipeline {
public:
    // Input event callbacks
    typedef void (*InputEventCallback)(WispInputSemantic input, bool pressed, int16_t value);
    typedef void (*InputSequenceCallback)(const WispInputSemantic* sequence, uint8_t length);
    
    // Timer-based callbacks
    typedef void (*TimerCallback)(uint16_t entityId, uint16_t timerId);
    typedef void (*AnimationCallback)(uint16_t entityId, uint8_t animationId);
    typedef void (*CollisionCallback)(uint16_t entityId, uint16_t otherId);
    
private:
    InputEventCallback inputCallbacks[INPUT_SEMANTIC_COUNT];
    InputSequenceCallback sequenceCallback;
    
public:
    ScriptControlPipeline() : sequenceCallback(nullptr) {
        memset(inputCallbacks, 0, sizeof(inputCallbacks));
    }
    
    void registerInputCallback(WispInputSemantic input, InputEventCallback callback);
    void registerSequenceCallback(InputSequenceCallback callback);
    void processInputEvents(const WispInputState& currentInput, const WispInputState& lastInput);
    void detectInputSequences();
};

// Extended Curated API class
class WispCuratedAPIExtended : public WispCuratedAPI {
private:
    ComponentManager* componentManager;
    ScriptSystem* scriptSystem;
    ScriptControlPipeline controlPipeline;
    
    // Input mapping and processing
    WispInputState lastInputState;
    WispInputEvent inputEvents[32];  // Event buffer
    uint8_t eventCount;
    uint32_t lastInputTime;
    
    // Input configuration
    struct InputMapping {
        WispInputSemantic semantic;
        bool physicalPressed;
        bool logicalPressed;
        bool justPressed;
        bool justReleased;
        int16_t analogValue;
    } inputMap[INPUT_SEMANTIC_COUNT];
    
    void updateInputMapping();
    void processInputEvents();
    
public:
    WispCuratedAPIExtended(WispEngine::Engine* eng);
    ~WispCuratedAPIExtended();
    
    // Initialize extended systems
    bool initializeExtendedSystems();
    void shutdownExtendedSystems();
    
    // === COMPONENT SYSTEM API ===
    
    // Component creation (tied to existing entity system)
    SpriteComponent* createSpriteComponent(EntityHandle entity);
    PhysicsComponent* createPhysicsComponent(EntityHandle entity);
    TimerComponent* createTimerComponent(EntityHandle entity, uint16_t timerId);
    DataComponent* createDataComponent(EntityHandle entity);
    
    // Component retrieval
    SpriteComponent* getSpriteComponent(EntityHandle entity);
    PhysicsComponent* getPhysicsComponent(EntityHandle entity);
    TimerComponent* getTimerComponent(EntityHandle entity, uint16_t timerId);
    DataComponent* getDataComponent(EntityHandle entity);
    
    // Component destruction
    void destroySpriteComponent(EntityHandle entity);
    void destroyPhysicsComponent(EntityHandle entity);
    void destroyTimerComponent(EntityHandle entity, uint16_t timerId);
    void destroyDataComponent(EntityHandle entity);
    
    // Batch component operations
    void updateAllComponents();
    void clearAllComponents();
    
    // === EXTENDED INPUT SYSTEM ===
    
    // Semantic input queries (preferred for new apps)
    bool isInputPressed(WispInputSemantic input);
    bool isInputJustPressed(WispInputSemantic input);
    bool isInputJustReleased(WispInputSemantic input);
    int16_t getAnalogInput(WispInputSemantic input);
    
    // Input event system for advanced apps
    uint8_t getInputEvents(WispInputEvent* events, uint8_t maxEvents);
    void clearInputEvents();
    
    // Input sequence detection (for combos, shortcuts)
    bool checkInputSequence(const WispInputSemantic* sequence, uint8_t length);
    void registerInputSequence(const WispInputSemantic* sequence, uint8_t length, const String& name);
    bool wasSequenceTriggered(const String& name);
    
    // Input customization
    void setInputMapping(WispInputSemantic semantic, bool up, bool down, bool left, bool right, 
                        bool buttonA, bool buttonB, bool buttonC, bool start, bool select);
    void resetInputMappings(); // Reset to defaults
    
    // === SCRIPT SYSTEM API ===
    
    // Script loading and management
    bool loadScript(const String& scriptName, const uint8_t* scriptData, size_t dataSize);
    bool unloadScript(const String& scriptName);
    bool isScriptLoaded(const String& scriptName);
    
    // Script execution
    bool executeScript(const String& scriptName, const String& functionName = "main");
    bool executeScriptWithParams(const String& scriptName, const String& functionName, 
                                const ScriptValue* params, uint8_t paramCount);
    void pauseScript(const String& scriptName);
    void resumeScript(const String& scriptName);
    void stopScript(const String& scriptName);
    
    // Entity script binding
    bool bindEntityScript(EntityHandle entity, const String& scriptName);
    void unbindEntityScript(EntityHandle entity);
    bool hasEntityScript(EntityHandle entity);
    
    // Script event system
    void triggerScriptEvent(const String& eventName, const ScriptValue& data = ScriptValue());
    void registerScriptEventHandler(const String& eventName, const String& scriptName, const String& functionName);
    
    // Script global variables
    bool setScriptGlobal(const String& name, const ScriptValue& value, ScriptValueType type);
    ScriptValue getScriptGlobal(const String& name);
    bool hasScriptGlobal(const String& name);
    
    // Script control pipeline integration
    void registerInputScriptCallback(WispInputSemantic input, const String& scriptName, const String& functionName);
    void registerTimerScriptCallback(uint16_t timerId, const String& scriptName, const String& functionName);
    void registerCollisionScriptCallback(EntityHandle entity, const String& scriptName, const String& functionName);
    
    // === ENHANCED ENTITY SYSTEM ===
    
    // Entity templates (for rapid prototyping)
    EntityHandle createPlayerEntity(const String& spriteName, float x, float y);
    EntityHandle createEnemyEntity(const String& spriteName, float x, float y, const String& aiScript = "");
    EntityHandle createItemEntity(const String& spriteName, float x, float y, const String& itemId = "");
    EntityHandle createUIEntity(const String& spriteName, float x, float y);
    
    // Entity groups and tags
    void setEntityTag(EntityHandle entity, const String& tag);
    String getEntityTag(EntityHandle entity);
    uint8_t getEntitiesByTag(const String& tag, EntityHandle* entities, uint8_t maxEntities);
    
    // Entity state management
    void setEntityActive(EntityHandle entity, bool active);
    bool isEntityActive(EntityHandle entity);
    void setEntityVisible(EntityHandle entity, bool visible);
    bool isEntityVisible(EntityHandle entity);
    
    // === ENHANCED GRAPHICS API ===
    
    // Layer management
    void setEntityLayer(EntityHandle entity, uint8_t layer);
    uint8_t getEntityLayer(EntityHandle entity);
    void setLayerVisible(uint8_t layer, bool visible);
    bool isLayerVisible(uint8_t layer);
    
    // Advanced drawing with components
    void renderEntity(EntityHandle entity);
    void renderAllEntities();
    void renderEntitiesInLayer(uint8_t layer);
    
    // Screen effects
    void setScreenShake(float intensity, uint32_t durationMs);
    void setScreenFade(float fadeAmount, uint32_t durationMs);
    void setScreenFlash(WispColor color, uint32_t durationMs);
    
    // === ENHANCED AUDIO API ===
    
    // 3D positional audio (simple)
    bool playAudioAt(ResourceHandle audio, float x, float y, const WispAudioParams& params = WispAudioParams());
    void setAudioListener(float x, float y);
    void setAudio3DParams(float maxDistance, float rolloffFactor);
    
    // Audio scripting integration
    void registerAudioEvent(const String& eventName, ResourceHandle audio, const WispAudioParams& params);
    void triggerAudioEvent(const String& eventName);
    
    // === UTILITY EXTENSIONS ===
    
    // Math helpers for components
    float getDistance(EntityHandle entity1, EntityHandle entity2);
    float getAngle(EntityHandle from, EntityHandle to);
    bool isEntityInRange(EntityHandle entity, EntityHandle target, float range);
    
    // Scene management
    void pauseScene();
    void resumeScene();
    void resetScene();
    
    // Performance monitoring
    uint32_t getComponentCount();
    uint32_t getActiveEntityCount();
    uint32_t getActiveScriptCount();
    float getFrameProcessingTime();
    
    // Memory management
    uint32_t getComponentMemoryUsage();
    uint32_t getScriptMemoryUsage();
    void optimizeMemory(); // Force garbage collection/optimization
    
    // === DEBUG EXTENSIONS ===
    
    // Component debugging
    void printComponentInfo(EntityHandle entity);
    void printAllComponentInfo();
    void validateComponentIntegrity();
    
    // Script debugging
    void printScriptState(const String& scriptName);
    void printAllScriptStates();
    bool isScriptInError(const String& scriptName);
    String getScriptError(const String& scriptName);
    
    // Performance debugging
    void enablePerformanceLogging(bool enable);
    void printPerformanceReport();
    void resetPerformanceCounters();
    
private:
    // Internal component system integration
    void syncEntityWithComponents(EntityHandle entity);
    void updateComponentSystems();
    
    // Internal input processing
    void mapPhysicalToSemantic(const WispInputState& physical);
    void detectInputCombos();
    void processInputBuffer();
    
    // Internal script integration
    void updateScriptSystems();
    void processScriptEvents();
    void handleScriptInputEvents();
    
    // Internal performance monitoring
    void updatePerformanceMetrics();
    
public:
    // Frame processing (called by engine)
    void updateExtendedSystems(uint32_t deltaTimeMs) override;
    void renderExtendedSystems() override;
    
    // Convenience macros for component access
    #define GET_SPRITE_COMP(entity) getSpriteComponent(entity)
    #define GET_PHYSICS_COMP(entity) getPhysicsComponent(entity)
    #define GET_TIMER_COMP(entity, id) getTimerComponent(entity, id)
    #define GET_DATA_COMP(entity) getDataComponent(entity)
    
    #define INPUT_PRESSED(semantic) isInputPressed(semantic)
    #define INPUT_JUST_PRESSED(semantic) isInputJustPressed(semantic)
    #define INPUT_JUST_RELEASED(semantic) isInputJustReleased(semantic)
    
    #define EXECUTE_SCRIPT(name) executeScript(name)
    #define TRIGGER_EVENT(name, data) triggerScriptEvent(name, data)
};

// Global extended API instance (set by engine)
extern WispCuratedAPIExtended* g_ExtendedAPI;

// Convenience function for apps
inline WispCuratedAPIExtended* getExtendedAPI() { return g_ExtendedAPI; }

// Enhanced app base class that uses extended API
class WispAppBaseExtended : public WispAppBase {
protected:
    WispCuratedAPIExtended* extendedAPI;
    
    // Convenience input methods
    bool up() const { return extendedAPI->isInputPressed(INPUT_UP); }
    bool down() const { return extendedAPI->isInputPressed(INPUT_DOWN); }
    bool left() const { return extendedAPI->isInputPressed(INPUT_LEFT); }
    bool right() const { return extendedAPI->isInputPressed(INPUT_RIGHT); }
    bool accept() const { return extendedAPI->isInputPressed(INPUT_ACCEPT); }
    bool back() const { return extendedAPI->isInputPressed(INPUT_BACK); }
    bool menu() const { return extendedAPI->isInputPressed(INPUT_MENU); }
    
    bool upPressed() const { return extendedAPI->isInputJustPressed(INPUT_UP); }
    bool downPressed() const { return extendedAPI->isInputJustPressed(INPUT_DOWN); }
    bool leftPressed() const { return extendedAPI->isInputJustPressed(INPUT_LEFT); }
    bool rightPressed() const { return extendedAPI->isInputJustPressed(INPUT_RIGHT); }
    bool acceptPressed() const { return extendedAPI->isInputJustPressed(INPUT_ACCEPT); }
    bool backPressed() const { return extendedAPI->isInputJustPressed(INPUT_BACK); }
    bool menuPressed() const { return extendedAPI->isInputJustPressed(INPUT_MENU); }
    
public:
    WispAppBaseExtended() : extendedAPI(nullptr) {}
    
    virtual bool initExtended(WispCuratedAPIExtended* api) {
        extendedAPI = api;
        WispEngine::AppInitData data;
        data.api = api;
        return init(data);
    }
    
    // Component convenience methods
    SpriteComponent* createSprite(EntityHandle entity) { return extendedAPI->createSpriteComponent(entity); }
    PhysicsComponent* createPhysics(EntityHandle entity) { return extendedAPI->createPhysicsComponent(entity); }
    TimerComponent* createTimer(EntityHandle entity, uint16_t id) { return extendedAPI->createTimerComponent(entity, id); }
    DataComponent* createData(EntityHandle entity) { return extendedAPI->createDataComponent(entity); }
    
    // Entity template convenience
    EntityHandle createPlayer(const String& sprite, float x, float y) { return extendedAPI->createPlayerEntity(sprite, x, y); }
    EntityHandle createEnemy(const String& sprite, float x, float y, const String& ai = "") { return extendedAPI->createEnemyEntity(sprite, x, y, ai); }
    EntityHandle createItem(const String& sprite, float x, float y, const String& id = "") { return extendedAPI->createItemEntity(sprite, x, y, id); }
    
    // Script convenience
    void runScript(const String& name) { extendedAPI->executeScript(name); }
    void bindScript(EntityHandle entity, const String& script) { extendedAPI->bindEntityScript(entity, script); }
    void fireEvent(const String& event, const ScriptValue& data = ScriptValue()) { extendedAPI->triggerScriptEvent(event, data); }
};

// Enhanced app registration macro
#define WISP_REGISTER_EXTENDED_APP(AppClass) \
    extern "C" { \
        WispAppBase* createWispApp() { \
            return new AppClass(); \
        } \
        void destroyWispApp(WispAppBase* app) { \
            delete app; \
        } \
        bool initWispAppExtended(WispAppBase* app, WispCuratedAPIExtended* api) { \
            AppClass* extApp = dynamic_cast<AppClass*>(app); \
            return extApp ? extApp->initExtended(api) : false; \
        } \
        const char* getWispAppName() { \
            static AppClass tempApp; \
            return tempApp.getName().c_str(); \
        } \
    }
