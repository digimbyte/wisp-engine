// secure_api_bridge.h - Secure WASH API Bridge
// All script API calls go through this secure bridge with UUID Authority validation
// ESP-IDF compatible, zero-trust security model

#pragma once
#include "../engine_common.h"
#include "../app/curated_api_extended.h"
#include "../script/secure_wash_vm.h"
#include "uuid_authority.h"

namespace WispEngine {
namespace Security {

// Forward declarations
class EngineUUIDAuthority;

// === SCRIPT EXECUTION CONTEXT ===
// Tracks the current executing script for security validation

struct ScriptExecutionContext {
    String scriptName;                   // Name of currently executing script
    String scriptType;                   // "entity", "panel", "global"
    uint32_t contextUUID;               // Entity UUID (for entity scripts)
    uint16_t contextPanelId;            // Panel ID (for panel/global scripts)
    
    // Security tracking
    uint32_t executionStartTime;        // When execution started
    uint32_t apiCallCount;              // Number of API calls made
    uint32_t securityViolations;        // Number of violations in this execution
    
    ScriptExecutionContext() : contextUUID(0), contextPanelId(0), 
                              executionStartTime(0), apiCallCount(0), 
                              securityViolations(0) {}
                              
    void reset() {
        scriptName = "";
        scriptType = "";
        contextUUID = 0;
        contextPanelId = 0;
        executionStartTime = 0;
        apiCallCount = 0;
        securityViolations = 0;
    }
    
    bool isValid() const {
        return !scriptName.empty();
    }
};

// === SECURE WASH API BRIDGE CLASS ===
// Implements all WASH VM API calls with security validation

class SecureWASHAPIBridge {
private:
    // Core system references
    WispCuratedAPIExtended* curatedAPI;
    EngineUUIDAuthority* uuidAuthority;
    
    // Current execution context
    ScriptExecutionContext currentContext;
    
    // Security configuration
    static const uint32_t MAX_API_CALLS_PER_FRAME = 100;
    static const uint32_t MAX_EXECUTION_TIME_MS = 50;
    static const float MAX_SEARCH_RADIUS = 512.0f;
    
    // Performance tracking
    uint32_t totalAPICalls;
    uint32_t totalSecurityViolations;
    uint32_t totalExecutionTime;
    
public:
    SecureWASHAPIBridge();
    ~SecureWASHAPIBridge();
    
    // === INITIALIZATION ===
    bool initialize(WispCuratedAPIExtended* api, EngineUUIDAuthority* authority);
    void shutdown();
    
    // === EXECUTION CONTEXT MANAGEMENT ===
    // Called by WASH VM before/after script execution
    
    bool setExecutionContext(const String& scriptName, const String& scriptType,
                           uint32_t contextUUID = 0, uint16_t contextPanelId = 0);
    void clearExecutionContext();
    const ScriptExecutionContext& getExecutionContext() const { return currentContext; }
    
    // === SECURE API IMPLEMENTATIONS ===
    // These implement the actual WASH VM opcodes with security validation
    
    // --- Entity Position/Movement Operations ---
    bool apiMoveEntity(uint32_t uuid, float deltaX, float deltaY);
    bool apiSetPosition(uint32_t uuid, float x, float y);
    WispVec2 apiGetPosition(uint32_t uuid);
    bool apiSetVelocity(uint32_t uuid, float vx, float vy);
    WispVec2 apiGetVelocity(uint32_t uuid);
    
    // --- Entity Visual Operations ---
    bool apiSetSprite(uint32_t uuid, uint16_t spriteId);
    bool apiSetAnimation(uint32_t uuid, const String& animationName);
    bool apiSetLayer(uint32_t uuid, uint8_t layer);
    bool apiSetVisible(uint32_t uuid, bool visible);
    
    // --- Entity Lifecycle Operations ---
    uint32_t apiSpawnEntity(const String& entityType, float x, float y, 
                           const String& scriptName = "");
    bool apiDestroyEntity(uint32_t uuid);
    
    // --- Entity Query Operations ---
    std::vector<uint32_t> apiFindEntitiesByType(const String& type);
    std::vector<uint32_t> apiFindEntitiesInRadius(float x, float y, float radius);
    String apiGetEntityType(uint32_t uuid);
    uint16_t apiGetCurrentPanel();
    
    // --- Camera/Panel Operations ---
    bool apiSetCamera(float x, float y);
    WispVec2 apiGetCamera();
    bool apiSetBackground(uint16_t spriteId);
    bool apiFocusEntity(uint32_t uuid, float speed = 1.0f);
    
    // --- Tile Operations ---
    bool apiAddTile(uint16_t spriteId, float x, float y, uint8_t layer = 4);
    bool apiRemoveTile(float x, float y);
    
    // --- Audio Operations ---
    bool apiPlaySound(const String& soundName, float volume = 1.0f);
    
    // --- Data Operations ---
    bool apiSaveData(const String& key, const String& value);
    String apiLoadData(const String& key);
    
    // --- Timer Operations ---
    bool apiSetTimer(uint16_t timerId, uint32_t delayMs, bool repeat = false);
    
    // --- Utility Operations ---
    void apiLogMessage(const String& message);
    
    // === MATH FUNCTIONS (SECURE IMPLEMENTATIONS) ===
    float mathSqrt(float x);
    float mathSin(float x);
    float mathCos(float x);
    float mathClamp(float value, float min, float max);
    float mathLerp(float a, float b, float t);
    float mathRandom();
    float mathLength(float x, float y);
    
    // === VALIDATION AND SECURITY ===
    
    // Validate script can perform operation
    bool validateOperation(const String& operation, const String& details = "");
    
    // Validate UUID access
    bool validateUUIDAccess(uint32_t uuid, const String& operation);
    
    // Check resource limits
    bool checkAPICallLimit();
    bool checkExecutionTime();
    bool checkParameterSafety(float value, float min = -1000000.0f, float max = 1000000.0f);
    
    // === DEBUGGING AND MONITORING ===
    
    uint32_t getTotalAPICalls() const { return totalAPICalls; }
    uint32_t getTotalViolations() const { return totalSecurityViolations; }
    uint32_t getTotalExecutionTime() const { return totalExecutionTime; }
    
    void dumpSecurityStats() const;
    void dumpExecutionContext() const;
    
private:
    // === INTERNAL VALIDATION ===
    
    bool validateContextExists() const;
    bool validateEntityUUID(uint32_t uuid, const String& operation) const;
    bool validatePanelAccess(uint16_t panelId) const;
    void recordAPICall(const String& operation);
    void recordSecurityViolation(const String& violation);
    
    // === PARAMETER VALIDATION ===
    
    bool validatePosition(float x, float y) const;
    bool validateVelocity(float vx, float vy) const;
    bool validateRadius(float radius) const;
    bool validateVolume(float volume) const;
    bool validateSpriteId(uint16_t spriteId) const;
    bool validateLayer(uint8_t layer) const;
    
    // === ENTITY TYPE VALIDATION ===
    
    bool validateEntityType(const String& type) const;
    bool validateScriptName(const String& scriptName) const;
    bool validateSoundName(const String& soundName) const;
    
    // === UTILITY HELPERS ===
    
    uint16_t getCurrentPanelId() const;
    SceneEntity* getSceneEntity(uint32_t uuid) const;
    bool isInScriptControlledEntity(uint32_t uuid) const;
    
    // === RESOURCE TRACKING ===
    
    void updatePerformanceMetrics(uint32_t executionTime);
    void resetFrameCounters();
};

// === INTEGRATION WITH WASH VM ===

// Helper class to manage execution context automatically
class ScopedExecutionContext {
private:
    SecureWASHAPIBridge* bridge;
    bool contextSet;
    
public:
    ScopedExecutionContext(SecureWASHAPIBridge* bridge, const String& scriptName, 
                          const String& scriptType, uint32_t contextUUID = 0, 
                          uint16_t contextPanelId = 0)
        : bridge(bridge), contextSet(false) {
        if (bridge) {
            contextSet = bridge->setExecutionContext(scriptName, scriptType, 
                                                   contextUUID, contextPanelId);
        }
    }
    
    ~ScopedExecutionContext() {
        if (bridge && contextSet) {
            bridge->clearExecutionContext();
        }
    }
    
    bool isValid() const { return contextSet; }
};

// Macro for easy context management in WASH VM
#define SECURE_API_CONTEXT(bridge, script, type, uuid, panel) \
    ScopedExecutionContext ctx(bridge, script, type, uuid, panel); \
    if (!ctx.isValid()) return false;

} // namespace Security
} // namespace WispEngine
