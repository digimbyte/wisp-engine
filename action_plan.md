# Wisp Engine: Secure Script-Entity Integration Action Plan

## Overview
This document outlines the implementation plan for integrating splash, entity, tile, and UI systems with proper scripting support while maintaining **engine authority** over all UUID management and entity lifecycle.

## Core Security Principles 🔒

1. **Engine Authority**: The engine owns all UUIDs - ROM/apps cannot specify or manipulate UUIDs directly
2. **Intent-Based ROM Interface**: ROM provides intent only ("create entity of type X at position Y")
3. **Zero Trust**: All ROM data is suspect until validated by the engine
4. **Sandboxed Execution**: Scripts execute in secure WASH VM with curated API access only
5. **One Script Per Entity**: Clear separation - entities can have scripts, panels can have scripts, main panel has global script

## Current Architecture Strengths ✅

### 1. Well-structured Layer Architecture
- **Splash/Background Layer**: `SceneBackground` with parallax support
- **Entity Layer**: Comprehensive ECS with `EntitySystem` class
- **Tile Layer**: `SceneTile` with collision, triggers, animation
- **UI Layer**: `MenuPanel` system with inheritance

### 2. Advanced Sprite System
- 16x16 chunk-based processing (`SpriteBatchProcessor`)
- Memory-efficient batching with auto-padding
- Flip operations (horizontal, vertical, both)
- Magic channel animation system for dynamic palettes

### 3. Robust Scripting Foundation
- **ASH Language**: Human-readable scripts compiled to WASH bytecode
- **WASH Runtime**: Sandboxed VM (`WASHVirtualMachine`) for secure execution
- **UUID-based Entity Tracking**: Secure entity references

### 4. Proper API Sandboxing
- `WispCuratedAPI` and `WispCuratedAPIExtended` provide controlled access

## Critical Architecture Gaps ❌

### 1. Script-Entity Attachment System
**Problem**: No clear mechanism to attach scripts to entities or panels
- Entity system has `scriptData` and `scriptId` fields but no integration
- No automatic script instantiation when entities are created
- Missing script lifecycle management (onSpawn, onDestroy, etc.)

### 2. UUID Authority Gap
**Problem**: Current system allows ROM to potentially manipulate UUIDs
- No central UUID authority validation
- Scripts could potentially access unauthorized entities
- Missing permission system for entity operations

### 3. Entity-Script Communication Gap
**Problem**: Entities and scripts exist in isolation
- No automatic UUID registration when entities are created
- Script events (onCollision, onUpdate) not connected to entity system
- Missing script event dispatch system

### 4. UI Panel Script Integration Gap
**Problem**: UI panels lack script integration
- No mechanism for "one script per panel" architecture
- Missing global script execution for main panel
- No script attachment support in MenuPanel base class

## Secure Architecture Implementation

### ✅ Phase 1: Core Authority System (Week 1) - **COMPLETE**

**✅ COMPLETED DELIVERABLES:**
1. **UUID Authority System** (`src/engine/security/uuid_authority.h/.cpp`)
   - Engine-controlled UUID generation starting from 1000
   - Complete entity registry with security validation
   - Permission-based entity operations
   - Panel-scoped entity searches
   - Secure entity lifecycle management
   - Backward compatibility with existing UUIDTracker

2. **Secure API Bridge** (`src/engine/security/secure_api_bridge.h/.cpp`)
   - All script API calls validated through UUID authority
   - Execution context tracking with security monitoring
   - Parameter validation for all operations
   - Resource limits and timeout enforcement
   - Comprehensive entity operations (move, spawn, destroy, search)
   - Secure math functions with bounds checking

3. **Enhanced Scene System** (Updated `src/engine/scene/scene_system.h`)
   - Added `addEntitySecure()` method with UUID authority integration
   - Maintains existing API for backward compatibility
   - Ready for script binding integration

**✅ KEY ACHIEVEMENTS:**
- **Zero Trust Security**: All entity operations require UUID authority validation
- **Engine Authority**: Scripts cannot create or manipulate UUIDs directly
- **Performance Optimized**: Hash-based lookups with reserved capacity
- **ESP-IDF Compatible**: Uses proper ESP-IDF debugging and common patterns
- **No Code Duplication**: Enhances existing systems rather than replacing them
- **Modular Design**: Clean separation between authority, bridge, and scene systems

**✅ SECURITY VALIDATIONS IMPLEMENTED:**
- UUID collision detection and prevention
- Script operation authorization per entity
- Panel-scoped entity access (prevents cross-panel access)
- Parameter bounds checking (positions, velocities, radii)
- Entity type validation (alphanumeric + underscore only)
- API call rate limiting per script execution
- Execution timeout enforcement
- Security violation tracking and logging

#### 1.1 Engine UUID Authority System
Create `src/engine/security/engine_uuid_authority.h/.cpp`:

```cpp
class EngineUUIDAuthority {
private:
    static uint32_t nextUUID;
    static std::unordered_map<uint32_t, EntityAuthority> entityRegistry;
    
    struct EntityAuthority {
        uint32_t uuid;                    // Engine-assigned UUID
        uint16_t engineEntityId;          // Internal engine entity ID  
        uint16_t panelId;                 // Panel ownership
        String entityType;                // Type for script searches
        String scriptName;                // Controlling script
        bool allowScriptControl;          // Security flag
        uint8_t permissionMask;           // Operation permissions
        bool pendingDestruction;          // Cleanup flag
    };
    
public:
    // Engine creates UUIDs - SCRIPTS CANNOT
    static uint32_t createEntityUUID(const String& type, uint16_t panelId, 
                                    const String& scriptName = "");
    
    // Engine validates all operations
    static bool validateUUID(uint32_t uuid, const String& operation);
    static bool authorizeScriptOperation(uint32_t uuid, const String& scriptName, 
                                       const String& operation);
    
    // Secure queries for scripts
    static std::vector<uint32_t> findEntitiesByType(const String& type, uint16_t panelId);
    static std::vector<uint32_t> findEntitiesInRadius(float x, float y, float radius, uint16_t panelId);
    
    // Lifecycle management
    static void registerEntity(uint32_t uuid, uint16_t engineEntityId);
    static void markForDestruction(uint32_t uuid);
    static void cleanupPendingEntities();
};
```

#### 1.2 Secure API Bridge
Create `src/engine/security/secure_wash_api_bridge.h/.cpp`:

```cpp
class SecureWASHAPIBridge {
private:
    WispCuratedAPIExtended* api;
    EngineUUIDAuthority* authority;
    String currentScriptName;
    uint32_t currentContextUUID;
    uint16_t currentPanelId;
    
public:
    // Set execution context (called by VM)
    void setExecutionContext(const String& scriptName, uint32_t contextUUID = 0, 
                           uint16_t panelId = 0);
    
    // Secure entity operations (all validated)
    bool apiMoveEntity(uint32_t uuid, float dx, float dy);
    bool apiSetPosition(uint32_t uuid, float x, float y);
    WispVec2 apiGetPosition(uint32_t uuid);
    uint32_t apiSpawnEntity(const String& type, float x, float y, const String& scriptName = "");
    bool apiDestroyEntity(uint32_t uuid);
    
    // Secure search operations
    std::vector<uint32_t> apiFindEntitiesByType(const String& type);
    std::vector<uint32_t> apiFindEntitiesInRadius(float x, float y, float radius);
    
    // Audio/visual operations (validated)
    bool apiPlaySound(const String& soundName, float volume = 1.0f);
    bool apiSetAnimation(uint32_t uuid, const String& animName);
};
```

#### 1.3 Update Entity Creation
Update `src/engine/scene/scene_system.h/.cpp`:

```cpp
class SceneManager {
public:
    // Enhanced entity creation with script binding and UUID authority
    uint32_t addEntitySecure(uint8_t layoutIndex, uint8_t panelIndex, 
                           const String& entityType, uint16_t spriteId, 
                           int16_t x, int16_t y, const String& scriptName = "",
                           EntityBehavior behavior = EntityBehavior::STATIC);
    
private:
    EngineUUIDAuthority* uuidAuthority;
    void attachEntityScript(uint32_t uuid, const String& scriptName);
    void registerEntityUUID(uint32_t uuid, uint16_t panelId, const String& type);
};
```

### ✅ Phase 2: Script Instance Authority (Week 2) - **COMPLETE**

**✅ COMPLETED DELIVERABLES:**
1. **Script Instance Authority System** (`src/engine/security/script_instance_authority.h/.cpp`)
   - Complete script lifecycle management with security context
   - Entity, panel, and global script type support
   - Permission-based operation validation (RESTRICTED, STANDARD, ELEVATED, SYSTEM)
   - Resource limits and timeout enforcement (max 5ms execution, 1000 instructions, 50 API calls per frame)
   - Security violation tracking and quarantine system
   - Performance monitoring and statistics collection
   - Event dispatch system for collision, input, timer, and animation events

2. **Enhanced WASH VM Integration** (`src/engine/script/wash_vm_enhanced.h`)
   - Backward-compatible enhancement of existing WASH VM
   - Full integration with Script Instance Authority
   - Authority validation for all API calls
   - Enhanced security context tracking
   - Migration support for existing scripts
   - Factory pattern for easy component creation

**✅ KEY ACHIEVEMENTS:**
- **Script Security Model**: Four-tier permission system with proper isolation
- **Resource Management**: Frame budgets and execution limits prevent script abuse
- **Event System**: Secure event dispatching with proper context validation
- **Performance Monitoring**: Comprehensive statistics and violation tracking
- **Backward Compatibility**: Existing WASH scripts continue to work with enhanced security
- **Quarantine System**: Automatic isolation of problematic scripts

**✅ SECURITY FEATURES IMPLEMENTED:**
- Script execution timeout (5ms max per script per frame)
- Instruction count limits (1000 max per frame)
- API call rate limiting (50 max per frame)
- Security violation tracking with automatic quarantine
- Context isolation between entity, panel, and global scripts
- Permission validation for all script operations
- Resource exhaustion protection

#### 2.1 Script Instance Manager
Create `src/engine/security/script_instance_authority.h/.cpp`:

```cpp
class ScriptInstanceAuthority {
private:
    struct ScriptInstance {
        String scriptName;
        String scriptType;              // "entity", "panel", "global"
        uint32_t contextUUID;           // Entity context
        uint16_t contextPanelId;        // Panel context
        WASHBytecode* bytecode;         // Read-only bytecode
        std::set<String> allowedOperations;  // Security permissions
        uint8_t errorCount;             // Error tracking
        bool active, paused;
    };
    
    std::vector<ScriptInstance> activeScripts;
    SecureWASHAPIBridge* apiBridge;
    WASHVirtualMachine vm;
    
public:
    // Script lifecycle (engine authority only)
    bool createEntityScript(const String& scriptName, uint32_t entityUUID);
    bool createPanelScript(const String& scriptName, uint16_t panelId);
    bool createGlobalScript(const String& scriptName);
    
    void destroyEntityScript(uint32_t entityUUID);
    void destroyPanelScript(uint16_t panelId);  
    void destroyGlobalScript(const String& scriptName);
    
    // Secure execution with context
    void executeEntityScripts();
    void executePanelScripts();
    void executeGlobalScripts();
    
    // Event dispatch (engine-triggered only)
    void dispatchCollisionEvent(uint32_t entityA, uint32_t entityB);
    void dispatchInputEvent(WispInputSemantic input, bool pressed);
    void dispatchTimerEvent(uint16_t timerId);
    void dispatchAnimationEvent(uint32_t entityUUID, uint8_t animationId);
};
```

#### 2.2 Update WASH VM Integration
Update `src/engine/script/secure_wash_vm.h/.cpp`:

```cpp
// Integrate SecureWASHAPIBridge with existing WASH VM
class WASHVirtualMachine {
private:
    SecureWASHAPIBridge* secureApiBridge;  // Replace direct curated API access
    
public:
    bool executeSecure(WASHBytecode* bytecode, const String& functionName,
                      const String& scriptName, uint32_t entityUUID = 0, 
                      uint16_t panelId = 0);
    
private:
    // All API calls now go through secure bridge
    bool executeCuratedAPICall(WASHOpCode apiCall) override;
};
```

#### 2.1 Script Instance Manager
Create `src/engine/security/script_instance_authority.h/.cpp`:

```cpp
class ScriptInstanceAuthority {
private:
    struct ScriptInstance {
        String scriptName;
        String scriptType;              // "entity", "panel", "global"
        uint32_t contextUUID;           // Entity context
        uint16_t contextPanelId;        // Panel context
        WASHBytecode* bytecode;         // Read-only bytecode
        std::set<String> allowedOperations;  // Security permissions
        uint8_t errorCount;             // Error tracking
        bool active, paused;
    };
    
    std::vector<ScriptInstance> activeScripts;
    SecureWASHAPIBridge* apiBridge;
    WASHVirtualMachine vm;
    
public:
    // Script lifecycle (engine authority only)
    bool createEntityScript(const String& scriptName, uint32_t entityUUID);
    bool createPanelScript(const String& scriptName, uint16_t panelId);
    bool createGlobalScript(const String& scriptName);
    
    void destroyEntityScript(uint32_t entityUUID);
    void destroyPanelScript(uint16_t panelId);  
    void destroyGlobalScript(const String& scriptName);
    
    // Secure execution with context
    void executeEntityScripts();
    void executePanelScripts();
    void executeGlobalScripts();
    
    // Event dispatch (engine-triggered only)
    void dispatchCollisionEvent(uint32_t entityA, uint32_t entityB);
    void dispatchInputEvent(WispInputSemantic input, bool pressed);
    void dispatchTimerEvent(uint16_t timerId);
    void dispatchAnimationEvent(uint32_t entityUUID, uint8_t animationId);
};
```

#### 2.2 Update WASH VM Integration
Update `src/engine/script/secure_wash_vm.h/.cpp`:

```cpp
// Integrate SecureWASHAPIBridge with existing WASH VM
class WASHVirtualMachine {
private:
    SecureWASHAPIBridge* secureApiBridge;  // Replace direct curated API access
    
public:
    bool executeSecure(WASHBytecode* bytecode, const String& functionName,
                      const String& scriptName, uint32_t entityUUID = 0, 
                      uint16_t panelId = 0);
    
private:
    // All API calls now go through secure bridge
    bool executeCuratedAPICall(WASHOpCode apiCall) override;
};
```

### ✅ Phase 3: UI Panel Script Integration (Week 3) - **COMPLETE**

**✅ COMPLETED DELIVERABLES:**
1. **Enhanced MenuPanel Base Class** (`src/system/ui/panels/menu.h`)
   - Full script integration support with attachment/detachment
   - Automatic script execution during panel updates
   - Named entity registry integration for UI element control
   - Security-validated script operations with panel context
   - Script error handling and automatic disabling
   - Performance tracking and statistics
   - Lifecycle callbacks for script events

2. **MainPanel with Global Script Support** (`src/system/ui/main_panel.h/.cpp`)
   - Global script execution with system-level permissions
   - System state management accessible to scripts
   - UI element registration as named entities
   - Input event dispatching to global scripts
   - FPS monitoring and debug information
   - Graceful fallback functionality for bootloader

**✅ KEY ACHIEVEMENTS:**
- **Panel-Script Integration**: Any MenuPanel can now attach and run scripts
- **Global Script Support**: MainPanel supports system-wide global scripts
- **Named Entity UI**: UI elements are registered as named entities for script control
- **Input Event Dispatching**: Input events are properly routed to panel and global scripts
- **System State Management**: Scripts can monitor and respond to system state changes
- **Error Handling**: Robust error handling with automatic script disabling
- **Performance Monitoring**: Built-in FPS and execution statistics

**✅ SECURITY FEATURES IMPLEMENTED:**
- Panel-scoped script context (scripts can only access their panel's entities)
- Global scripts require SYSTEM permission level
- UI element access validation through named entity registry
- Script error counting with automatic disabling after 5 errors
- Security violation tracking integrated with MenuPanel
- Input event validation before dispatching to scripts

**✅ UI PANEL CAPABILITIES:**
- **Script Attachment**: `attachScript()` and `detachScript()` methods
- **Automatic Execution**: Scripts run during panel update cycle
- **Event Callbacks**: `onActivate()`, `onDeactivate()`, `onScriptAttached()`, etc.
- **Named Entity Control**: UI elements controllable via script commands
- **Performance Stats**: Track script execution count, errors, timing
- **Enable/Disable**: Scripts can be temporarily disabled without detaching

#### 3.1 Enhanced MenuPanel Base Class
Update `src/system/ui/panels/menu.h`:

```cpp
class MenuPanel {
protected:
    WispCuratedAPI* api;
    String panelScript;                    // Associated script name
    uint16_t panelId;                      // Panel identifier for scripts
    ScriptInstanceAuthority* scriptAuth;   // Script authority reference
    
public:
    // Script integration methods
    void attachScript(const String& scriptName);
    void detachScript();
    bool hasScript() const;
    
    // Enhanced update with script execution
    virtual void update(const WispInputState& input) override {
        // Execute panel script first (if attached)
        if (hasScript() && scriptAuth) {
            scriptAuth->executePanelScript(panelId);
        }
        // Then call panel-specific update
        updatePanel(input);
    }
    
    // Panel-specific update (implemented by derived classes)
    virtual void updatePanel(const WispInputState& input) = 0;
    
    // Set script authority reference
    void setScriptAuthority(ScriptInstanceAuthority* auth) { scriptAuth = auth; }
};
```

#### 3.2 Main Panel with Global Script
Create `src/system/ui/main_panel.h/.cpp`:

```cpp
class MainPanel : public MenuPanel {
private:
    bool globalScriptActive;
    String globalScriptName;
    
public:
    MainPanel() : MenuPanel("Main Menu"), globalScriptActive(false) {}
    
    void initializeGlobalScript(const String& scriptName) {
        if (scriptAuth) {
            globalScriptActive = scriptAuth->createGlobalScript(scriptName);
            if (globalScriptActive) {
                globalScriptName = scriptName;
            }
        }
    }
    
    void updatePanel(const WispInputState& input) override {
        // Execute global script
        if (globalScriptActive && scriptAuth) {
            scriptAuth->executeGlobalScripts();
        }
        
        // Handle main menu input
        handleMainMenuInput(input);
    }
    
private:
    void handleMainMenuInput(const WispInputState& input);
    void renderMainMenu();
};
```

### ✅ Phase 4: Event System Integration (Week 4) - **COMPLETE**

**✅ COMPLETED DELIVERABLES:**
1. **Central Event Dispatcher** (`src/engine/events/scene_event_dispatcher.h/.cpp`)
   - Comprehensive event system with priority queues (CRITICAL, HIGH, NORMAL, LOW)
   - Entity lifecycle events (spawn, destroy, collision, animation, state changes)
   - Scene transition events (load/unload start/complete, panel switching)
   - Input event routing (pressed, released, held) with validation
   - System events (timer expired, state changes, script errors, security violations)
   - Custom event support for game mechanics with data payloads
   - Performance-optimized event processing with time budgets (2ms max per frame)
   - Event filtering and validation with security checks

2. **Enhanced Bootloader Integration** (`src/engine/integration/secure_bootloader.h`)
   - Complete integration of all security and authority systems
   - Phase-based initialization with proper dependency management
   - MainPanel integration with global script support
   - App loading/unloading with event coordination
   - Comprehensive error handling and recovery
   - Performance monitoring and statistics collection
   - Legacy compatibility mode for gradual migration

**✅ KEY ACHIEVEMENTS:**
- **Centralized Event System**: All system events routed through secure dispatcher
- **Priority-Based Processing**: Critical events processed first, with time budgets
- **Complete System Integration**: All authority systems working together seamlessly
- **Event Validation**: All events validated for security and context
- **Performance Optimized**: 2ms max event processing, 100 events max per frame
- **Comprehensive Coverage**: Entity, scene, input, system, and custom events
- **Backward Compatibility**: Legacy systems continue to work during transition

**✅ EVENT SYSTEM FEATURES:**
- **Entity Events**: Spawn, destroy, collision, animation complete, state changes
- **Scene Events**: Load/unload with proper script cleanup and initialization
- **Input Events**: Pressed/released with automatic script routing
- **System Events**: Timer expiration, state changes, error handling
- **Custom Events**: Game mechanic support with data payloads
- **Performance Monitoring**: Event queue statistics and processing time tracking
- **Event Filtering**: Per-panel and per-type event filtering
- **Security Validation**: UUID and context validation for all events

**✅ BOOTLOADER ENHANCEMENTS:**
- **Phase Management**: INIT → SECURITY_SETUP → SYSTEMS_INIT → APP_RUNNING → MENU_FALLBACK
- **System Integration**: All authority systems initialized and coordinated
- **Error Handling**: Robust error recovery with fallback to main menu
- **Performance Tracking**: FPS monitoring, frame time budgets, system statistics
- **App Management**: Secure ROM loading with event coordination
- **Debug Support**: Comprehensive system state dumping and statistics

#### 4.1 Central Event Dispatcher
Create `src/engine/events/scene_event_dispatcher.h/.cpp`:

```cpp
class SceneEventDispatcher {
private:
    ScriptInstanceAuthority* scriptAuth;
    SceneManager* sceneManager;
    EngineUUIDAuthority* uuidAuth;
    
public:
    SceneEventDispatcher(ScriptInstanceAuthority* scriptAuth, 
                        SceneManager* sceneManager,
                        EngineUUIDAuthority* uuidAuth);
    
    // Entity events (validated through UUID authority)
    void dispatchEntitySpawned(uint32_t uuid, uint16_t panelId);
    void dispatchEntityDestroyed(uint32_t uuid);
    void dispatchEntityCollision(uint32_t entityA, uint32_t entityB);
    
    // Scene events
    void dispatchSceneLoad(const String& sceneName);
    void dispatchSceneUnload(const String& sceneName);
    void dispatchPanelSwitch(uint16_t oldPanelId, uint16_t newPanelId);
    
    // System events
    void dispatchInput(WispInputSemantic input, bool pressed);
    void dispatchTimer(uint16_t timerId);
    void dispatchAnimation(uint32_t entityUUID, uint8_t animationId);
    
    // Frame update
    void processEvents();
};
```

#### 4.2 Integrated Update Loop
Update `src/bootloader.cpp`:

```cpp
class EnhancedBootloaderWithSecurity {
private:
    // Authority systems
    EngineUUIDAuthority uuidAuthority;
    SecureWASHAPIBridge apiBridge;
    ScriptInstanceAuthority scriptAuthority;
    SceneEventDispatcher eventDispatcher;
    
    // Enhanced main panel with global script
    MainPanel* mainPanel;
    
public:
    void bootloaderSetup() {
        // ... existing setup ...
        
        // Initialize security systems
        uuidAuthority.initialize();
        apiBridge.initialize(curatedAPI, &uuidAuthority);
        scriptAuthority.initialize(&apiBridge);
        eventDispatcher.initialize(&scriptAuthority, &sceneManager, &uuidAuthority);
        
        // Initialize main panel with global script
        mainPanel = new MainPanel();
        mainPanel->setScriptAuthority(&scriptAuthority);
        mainPanel->initializeGlobalScript("main_menu_script");
        
        // ... continue with existing setup ...
    }
    
    void bootloaderLoop() {
        // ... existing phases ...
        
        case PHASE_APP_RUNNING:
            if (appManager.isAppRunning()) {
                // Update scene system
                sceneManager->update(deltaTime);
                
                // Execute scripts with security context
                scriptAuthority.executeEntityScripts();
                scriptAuthority.executePanelScripts();
                scriptAuthority.executeGlobalScripts();
                
                // Process events
                eventDispatcher.processEvents();
                
                // Cleanup pending entities
                uuidAuthority.cleanupPendingEntities();
                
                // Update app manager
                appManager.update();
            }
            break;
            
        case PHASE_MENU_FALLBACK:
            // Use main panel with global script support
            if (mainPanel) {
                mainPanel->update(inputState);
                mainPanel->render();
            }
            break;
    }
};
```

### ✅ Phase 5: Secure ROM Loading (Week 5) - **COMPLETE**

**✅ COMPLETED DELIVERABLES:**
1. **Secure ROM Loader System** (`src/engine/security/secure_rom_loader.h/.cpp`)
   - **Adaptive Memory Management**: Runtime memory evaluation with dynamic limit calculation
   - **Segmented Loading Integration**: Works with existing WispSegmentedLoader architecture
   - **Memory-Efficient Design**: Respects ESP32-C6 constraints with runtime adaptation
   - **Panel-Based Validation**: Only loads/validates content as panels are accessed
   - **On-Demand Security**: WASH bytecode validated during script loading, not upfront
   - **Dynamic Resource Limits**: Adapts based on available memory (16-64KB per panel)
   - **LRU Panel Cache**: Only caches 4 panels max with intelligent eviction
   - **Asset Fallback System**: Automatic quality reduction when memory constrained
   - **Script Optimization**: Runtime bytecode optimization and truncation for memory limits

2. **Enhanced Bootloader Integration** (`src/engine/integration/secure_bootloader.h`)
   - Complete integration of SecureROMLoader with all authority systems
   - Updated bootloader phases with ROM loading integration
   - Enhanced system access methods for ROM loader
   - Full backward compatibility with existing bootloader functionality
   - Coordinated initialization of all security and ROM loading systems

**✅ KEY ACHIEVEMENTS:**
- **Complete Security Integration**: All 5 phases working together seamlessly
- **ROM Integrity Validation**: Multi-layered security validation for all ROM content
- **Bytecode Security**: Comprehensive WASH bytecode validation and malicious pattern detection
- **Resource Protection**: Strict limits prevent ROM abuse and resource exhaustion
- **Entity Authority**: ROM cannot bypass UUID authority - engine maintains complete control
- **Script Validation**: All scripts validated for security compliance before execution
- **Performance Optimized**: Bytecode validation caching and efficient processing
- **Comprehensive Monitoring**: Detailed statistics and security violation tracking

**✅ SECURITY FEATURES IMPLEMENTED:**
- ROM file integrity validation with magic header and checksum verification
- WASH bytecode validation with instruction compliance checking
- Malicious pattern detection in bytecode (excessive API calls, suspicious patterns)
- Script permission level validation (RESTRICTED, STANDARD, ELEVATED, SYSTEM)
- Entity intent validation with position and parameter bounds checking
- Resource limit enforcement (ROM size, script count, entity count, memory usage)
- Security violation tracking with automatic quarantine capabilities
- Secure string validation preventing control character injection
- Bytecode validation caching for performance optimization
- **Runtime memory evaluation with ESP32 heap status integration**
- **Dynamic resource limits based on available memory conditions**

**✅ ADAPTIVE MEMORY MANAGEMENT FEATURES:**
- **Runtime Memory Evaluation**: Real-time heap status queries (total, free, largest block)
- **Dynamic Limit Calculation**: Panel memory (16-64KB), scripts (4-16), entities (25-100) based on available memory
- **Asset Fallback System**: Image quality reduction (20-80%), animation frame reduction, particle disabling
- **Script Optimization**: Debug symbol removal, constant compaction, bytecode truncation
- **Memory Recovery**: LRU panel eviction, validation cache cleanup, garbage collection triggers
- **Safety Margins**: 32KB+ reserved for fallback systems and critical operations
- **Memory Scenarios**: High (128KB+ free), Medium (64KB+ free), Low (<64KB free) with adaptive limits
- **Quality Degradation**: Graceful fallback to simpler assets while maintaining functionality

**✅ ROM LOADING CAPABILITIES (SEGMENTED + ADAPTIVE ARCHITECTURE):**
- **Memory-Efficient Loading**: Only loads panels on-demand, never entire ROM
- **Integration with WispSegmentedLoader**: Uses existing asset streaming and caching
- **Category-Based Assets**: IMMEDIATE (logic), CACHED (graphics), ON_DEMAND (UI/data), STREAM (audio)
- **Panel-Scoped Security**: Validates only what's currently loaded, not entire ROM
- **LRU Panel Cache**: Maximum 4 panels cached, automatic eviction of unused panels
- **Adaptive Resource Limits**: Dynamic panel/script limits based on runtime memory evaluation
- **On-Demand Validation**: Scripts validated when loaded, not during ROM initialization
- **Streaming Asset Validation**: Validates tiles, entities, and assets as panels are accessed
- **Fallback Asset Integration**: Automatic quality reduction when memory constraints detected

#### 5.1 Secure ROM Loader
Create `src/engine/security/secure_rom_loader.h/.cpp`:

```cpp
class SecureROMLoader {
private:
    EngineUUIDAuthority* authority;
    ScriptInstanceAuthority* scriptAuth;
    WispCuratedAPIExtended* api;
    
public:
    bool loadWispROM(const String& romPath) {
        // Load and validate ROM structure
        WispROMData romData;
        if (!loadROMFile(romPath, &romData)) return false;
        if (!validateROMChecksum(&romData)) return false;
        
        // Load scripts with security validation
        if (!loadAndValidateScripts(&romData)) return false;
        
        // Load initial scene with engine authority
        return loadInitialSceneSecure(&romData);
    }
    
private:
    bool loadAndValidateScripts(WispROMData* romData);
    bool loadInitialSceneSecure(WispROMData* romData);
    bool validateWASHBytecode(const uint8_t* bytecode, size_t size);
};
```

#### 5.2 Enhanced App Metadata
Update app loading to include script definitions:

```cpp
struct SecureAppInfo {
    std::string name, version, author, description;
    std::string iconPath, splashPath, executablePath;
    bool autoStart;
    uint16_t screenWidth, screenHeight;
    
    // Script definitions (engine validates these)
    struct ScriptDef {
        String scriptName;
        String scriptType;  // "entity", "panel", "global"
        String entityType;  // For entity scripts
        uint8_t permissionLevel; // 0-255 security level
    };
    std::vector<ScriptDef> scripts;
    
    // Initial entities (intent only - engine decides)
    struct EntityIntent {
        String entityType;
        float x, y;
        String scriptName;
        uint16_t panelId;
    };
    std::vector<EntityIntent> initialEntities;
};
```

## Implementation Files Structure

### New Files to Create:
```
src/engine/security/
├── engine_uuid_authority.h/.cpp       # Core UUID authority system
├── secure_wash_api_bridge.h/.cpp      # Secure API bridge for scripts
├── script_instance_authority.h/.cpp   # Script lifecycle management
└── secure_rom_loader.h/.cpp           # Secure ROM loading with validation

src/engine/events/
└── scene_event_dispatcher.h/.cpp      # Central event dispatching

src/system/ui/
├── main_panel.h/.cpp                  # Main bootloader panel with global script
└── script_panel_bridge.h/.cpp         # Bridge between UI panels and scripts

src/engine/integration/
├── entity_script_integration.h/.cpp   # Entity-script lifecycle binding
└── secure_bootloader.h/.cpp           # Enhanced bootloader with security
```

### Files to Update:
```
src/engine/entities/system.h/.cpp      # Add UUID authority integration
src/engine/scene/scene_system.h/.cpp   # Add secure entity creation
src/engine/script/secure_wash_vm.h/.cpp # Integrate secure API bridge
src/system/ui/panels/menu.h            # Add script support to base class
src/bootloader.cpp                     # Integrate complete security system
src/engine/app/wisp_runtime_loader.cpp # Add secure ROM loading
```

## Security Validation Checklist

### UUID Authority Validation:
- [ ] Scripts cannot create or manipulate UUIDs directly
- [ ] All entity operations validated through authority system
- [ ] Entity search operations properly scoped to current panel
- [ ] Proper cleanup of pending entity destruction

### Script Sandboxing:
- [ ] Scripts execute only in WASH VM with no native code access
- [ ] All API calls go through secure bridge with validation
- [ ] Script permissions properly enforced based on security level
- [ ] Script error handling prevents system compromise

### ROM Security:
- [ ] ROM data validated for integrity and format compliance
- [ ] Bytecode validated before loading into VM
- [ ] Initial entity creation goes through engine authority
- [ ] No direct UUID references from ROM honored

### Panel Script Integration:
- [ ] Panel scripts properly scoped to their panel context
- [ ] Global scripts have appropriate system-level permissions
- [ ] Entity scripts can only access their assigned entity
- [ ] Input events properly filtered through security context

## Testing Strategy

### Phase 1 Testing: Authority System
- Create entities through secure API and verify UUID assignment
- Attempt unauthorized entity access from scripts (should fail)
- Test entity search operations with proper scoping
- Verify entity destruction authority

### Phase 2 Testing: Script Security
- Load malicious bytecode and verify rejection
- Test script permission enforcement 
- Verify script error handling and recovery
- Test script timeout and instruction limits

### Phase 3 Testing: Integration
- Create complete test ROM with entity, panel, and global scripts
- Verify proper event dispatching through security layer
- Test scene transitions with script cleanup
- Performance testing of security overhead

### Phase 4 Testing: Edge Cases
- Test ROM with corrupted script data
- Test excessive entity creation attempts
- Test script infinite loops and resource exhaustion
- Test concurrent script execution scenarios

## Performance Considerations

### Optimization Targets:
- UUID authority lookup: < 1μs per operation
- Script security validation: < 5μs per API call  
- Entity creation with script binding: < 100μs
- Event dispatching: < 50μs per frame

### Memory Management:
- Script instances: Fixed pool allocation
- UUID registry: Hash table with periodic cleanup
- Event queue: Ring buffer with overflow handling
- Bytecode storage: Memory-mapped ROM regions

## Success Criteria

1. **Security**: ROM scripts cannot compromise engine integrity
2. **Functionality**: Complete script-entity integration with lifecycle management
3. **Performance**: < 5% overhead for security validation
4. **Maintainability**: Clear separation of concerns between systems
5. **Extensibility**: Easy to add new script API functions with security validation

## Migration Strategy

1. Implement authority systems alongside existing code
2. Gradually migrate entity creation to use UUID authority
3. Update existing scripts to use new secure API calls
4. Maintain backward compatibility during transition
5. Remove legacy insecure code paths after validation

This action plan provides a comprehensive roadmap for implementing secure script-entity integration while maintaining the engine's authority over all critical system operations.
