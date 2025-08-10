#pragma once

#include <vector>
#include <unordered_map>
#include <set>
#include <string>
#include "esp_log.h"
#include "secure_api_bridge.h"
#include "../script/wash_vm.h"
#include "../script/wash_bytecode.h"

static const char* SCRIPT_AUTH_TAG = "ScriptAuth";

/**
 * @brief Script Instance Authority System
 * 
 * Manages the lifecycle of all script instances with proper security context.
 * Ensures scripts execute with appropriate permissions and context isolation.
 * 
 * Key Features:
 * - Entity scripts: One script per entity with entity context
 * - Panel scripts: One script per panel with panel context  
 * - Global scripts: System-wide scripts with elevated permissions
 * - Security validation for all script operations
 * - Event dispatch system for script communication
 * - Resource limits and error handling
 */
class ScriptInstanceAuthority {
public:
    /**
     * @brief Types of scripts supported by the system
     */
    enum class ScriptType : uint8_t {
        ENTITY = 0,     // Script attached to a specific entity
        PANEL = 1,      // Script attached to a UI panel
        GLOBAL = 2      // Global system script
    };

    /**
     * @brief Permission levels for script operations
     */
    enum class PermissionLevel : uint8_t {
        RESTRICTED = 0,     // Basic entity operations only
        STANDARD = 1,       // Standard entity + audio operations
        ELEVATED = 2,       // Can spawn/destroy entities
        SYSTEM = 3          // Full system access (global scripts only)
    };

private:
    /**
     * @brief Internal script instance data
     */
    struct ScriptInstance {
        std::string scriptName;         // Script identifier
        ScriptType scriptType;          // Type of script
        uint32_t contextUUID;          // Entity context (0 for panel/global)
        uint16_t contextPanelId;       // Panel context (0 for entity/global)
        WASHBytecode* bytecode;        // Read-only bytecode reference
        PermissionLevel permissions;    // Security permission level
        std::set<std::string> allowedOperations;  // Allowed API operations
        
        // Runtime state
        bool active;                   // Script is running
        bool paused;                   // Script is paused
        uint8_t errorCount;            // Runtime error counter
        uint32_t lastExecutionTime;    // Last execution timestamp
        uint32_t totalExecutionTime;   // Total execution time (microseconds)
        uint16_t instructionCount;     // Instructions executed this frame
        
        // Security tracking
        uint16_t apiCallCount;         // API calls this frame
        uint8_t securityViolations;    // Security violation counter
        bool quarantined;              // Script is quarantined due to violations
        
        ScriptInstance() : scriptType(ScriptType::ENTITY), contextUUID(0), contextPanelId(0),
                          bytecode(nullptr), permissions(PermissionLevel::RESTRICTED),
                          active(false), paused(false), errorCount(0), lastExecutionTime(0),
                          totalExecutionTime(0), instructionCount(0), apiCallCount(0),
                          securityViolations(0), quarantined(false) {}
    };

    // Core systems
    SecureWASHAPIBridge* apiBridge;
    WASHVirtualMachine vm;
    EngineUUIDAuthority* uuidAuthority;
    
    // Script storage
    std::vector<ScriptInstance> activeScripts;
    std::unordered_map<uint32_t, size_t> entityScriptMap;    // UUID -> script index
    std::unordered_map<uint16_t, size_t> panelScriptMap;     // Panel ID -> script index
    std::unordered_map<std::string, size_t> globalScriptMap; // Script name -> script index
    
    // Resource limits
    static constexpr uint16_t MAX_ACTIVE_SCRIPTS = 64;
    static constexpr uint16_t MAX_INSTRUCTIONS_PER_FRAME = 1000;
    static constexpr uint16_t MAX_API_CALLS_PER_FRAME = 50;
    static constexpr uint32_t MAX_EXECUTION_TIME_MICROS = 5000; // 5ms per script per frame
    static constexpr uint8_t MAX_ERRORS_BEFORE_QUARANTINE = 5;
    static constexpr uint8_t MAX_SECURITY_VIOLATIONS = 3;
    
    // Performance tracking
    uint32_t frameStartTime;
    uint16_t totalScriptsExecuted;
    uint32_t totalExecutionTimeMicros;
    
public:
    /**
     * @brief Initialize the script instance authority system
     */
    bool initialize(SecureWASHAPIBridge* bridge, EngineUUIDAuthority* authority);
    
    /**
     * @brief Shutdown and cleanup all scripts
     */
    void shutdown();
    
    // Script Lifecycle Management
    
    /**
     * @brief Create a new entity script instance
     * @param scriptName Name/path of the script
     * @param entityUUID UUID of the entity this script controls
     * @param permissions Permission level for this script
     * @return True if script was created successfully
     */
    bool createEntityScript(const std::string& scriptName, uint32_t entityUUID, 
                           PermissionLevel permissions = PermissionLevel::STANDARD);
    
    /**
     * @brief Create a new panel script instance
     * @param scriptName Name/path of the script
     * @param panelId ID of the panel this script controls
     * @param permissions Permission level for this script
     * @return True if script was created successfully
     */
    bool createPanelScript(const std::string& scriptName, uint16_t panelId,
                          PermissionLevel permissions = PermissionLevel::STANDARD);
    
    /**
     * @brief Create a new global script instance
     * @param scriptName Name/path of the script
     * @param permissions Permission level for this script (usually SYSTEM)
     * @return True if script was created successfully
     */
    bool createGlobalScript(const std::string& scriptName,
                           PermissionLevel permissions = PermissionLevel::SYSTEM);
    
    /**
     * @brief Destroy entity script when entity is destroyed
     * @param entityUUID UUID of the entity being destroyed
     */
    void destroyEntityScript(uint32_t entityUUID);
    
    /**
     * @brief Destroy panel script when panel is closed
     * @param panelId ID of the panel being closed
     */
    void destroyPanelScript(uint16_t panelId);
    
    /**
     * @brief Destroy global script by name
     * @param scriptName Name of the global script to destroy
     */
    void destroyGlobalScript(const std::string& scriptName);
    
    // Script Execution
    
    /**
     * @brief Execute all active entity scripts
     * Called once per frame during entity update phase
     */
    void executeEntityScripts();
    
    /**
     * @brief Execute all active panel scripts
     * Called once per frame during UI update phase
     */
    void executePanelScripts();
    
    /**
     * @brief Execute all active global scripts
     * Called once per frame during global update phase
     */
    void executeGlobalScripts();
    
    /**
     * @brief Execute a specific script function with context
     * @param scriptName Name of the script
     * @param functionName Function to execute (e.g., "onUpdate", "onCollision")
     * @param contextUUID Entity context (0 for panel/global scripts)
     * @param panelId Panel context (0 for entity/global scripts)
     * @return True if execution completed successfully
     */
    bool executeScriptFunction(const std::string& scriptName, const std::string& functionName,
                              uint32_t contextUUID = 0, uint16_t panelId = 0);
    
    // Event Dispatch System
    
    /**
     * @brief Dispatch collision event to entity scripts
     * @param entityA First entity in collision
     * @param entityB Second entity in collision
     */
    void dispatchCollisionEvent(uint32_t entityA, uint32_t entityB);
    
    /**
     * @brief Dispatch input event to panel and global scripts
     * @param input Input semantic that was triggered
     * @param pressed True if pressed, false if released
     */
    void dispatchInputEvent(WispInputSemantic input, bool pressed);
    
    /**
     * @brief Dispatch timer event to scripts
     * @param timerId Timer that expired
     */
    void dispatchTimerEvent(uint16_t timerId);
    
    /**
     * @brief Dispatch animation event to entity script
     * @param entityUUID Entity whose animation completed
     * @param animationId Animation that completed
     */
    void dispatchAnimationEvent(uint32_t entityUUID, uint8_t animationId);
    
    /**
     * @brief Dispatch custom event to scripts by name
     * @param eventName Name of the custom event
     * @param entityUUID Target entity (0 for all entities)
     * @param panelId Target panel (0 for all panels)
     */
    void dispatchCustomEvent(const std::string& eventName, uint32_t entityUUID = 0, uint16_t panelId = 0);
    
    // Script Management
    
    /**
     * @brief Pause a specific script
     * @param scriptName Name of script to pause
     */
    void pauseScript(const std::string& scriptName);
    
    /**
     * @brief Resume a paused script
     * @param scriptName Name of script to resume
     */
    void resumeScript(const std::string& scriptName);
    
    /**
     * @brief Check if a script is currently active
     * @param scriptName Name of script to check
     * @return True if script exists and is active
     */
    bool isScriptActive(const std::string& scriptName) const;
    
    /**
     * @brief Get script performance statistics
     * @param scriptName Name of script to get stats for
     * @return Performance stats or nullptr if script not found
     */
    struct ScriptStats {
        uint32_t totalExecutionTime;
        uint16_t averageInstructionsPerFrame;
        uint8_t errorCount;
        uint8_t securityViolations;
        bool quarantined;
    };
    const ScriptStats* getScriptStats(const std::string& scriptName) const;
    
    /**
     * @brief Clean up quarantined and errored scripts
     * Called periodically to maintain system health
     */
    void cleanupScripts();
    
    /**
     * @brief Get system-wide script execution statistics
     */
    struct SystemStats {
        uint16_t activeEntityScripts;
        uint16_t activePanelScripts;
        uint16_t activeGlobalScripts;
        uint16_t quarantinedScripts;
        uint32_t totalExecutionTimeThisFrame;
        uint16_t totalAPICallsThisFrame;
    };
    SystemStats getSystemStats() const;

private:
    // Internal methods
    bool loadScriptBytecode(const std::string& scriptName, WASHBytecode** bytecode);
    bool validateScriptPermissions(const ScriptInstance& script, const std::string& operation);
    void recordSecurityViolation(size_t scriptIndex, const std::string& violation);
    void quarantineScript(size_t scriptIndex, const std::string& reason);
    size_t findScriptIndex(const std::string& scriptName) const;
    bool executeScriptWithContext(size_t scriptIndex, const std::string& functionName);
    void updateScriptStats(size_t scriptIndex, uint32_t executionTime, uint16_t instructions, uint16_t apiCalls);
    void resetFrameCounters();
    
    // Permission helpers
    std::set<std::string> getPermittedOperations(PermissionLevel level, ScriptType type);
    bool canScriptAccessEntity(const ScriptInstance& script, uint32_t entityUUID);
    bool canScriptAccessPanel(const ScriptInstance& script, uint16_t panelId);
};
