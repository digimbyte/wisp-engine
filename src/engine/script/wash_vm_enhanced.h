#pragma once

#include "secure_wash_vm.h"
#include "../security/script_instance_authority.h"
#include "../security/secure_api_bridge.h"
#include "../security/uuid_authority.h"

/**
 * @brief Enhanced WASH Virtual Machine with Script Instance Authority Integration
 * 
 * This is an enhanced version of the original WASH VM that integrates directly
 * with our new security architecture. It provides backward compatibility while
 * adding the new security and authority systems.
 * 
 * Key Enhancements:
 * - Integrates with ScriptInstanceAuthority for script lifecycle management
 * - Uses SecureWASHAPIBridge for all API calls with validation
 * - Enforces UUID authority for all entity operations
 * - Provides enhanced security monitoring and violation tracking
 * - Supports the new permission-based script execution model
 */
namespace WispEngine {
namespace Script {

/**
 * @brief Enhanced WASH Virtual Machine with Authority Integration
 */
class EnhancedWASHVirtualMachine : public WASHVirtualMachine {
private:
    // Authority system integration
    ScriptInstanceAuthority* scriptAuthority;
    SecureWASHAPIBridge* secureApiBridge;
    EngineUUIDAuthority* uuidAuthority;
    
    // Enhanced security tracking
    std::string currentScriptName;
    ScriptInstanceAuthority::PermissionLevel currentPermissions;
    bool authorityValidationEnabled;
    
    // Performance monitoring
    uint32_t securityChecksPerformed;
    uint32_t violationsDetected;
    uint32_t apiCallsBlocked;
    
public:
    EnhancedWASHVirtualMachine();
    virtual ~EnhancedWASHVirtualMachine();
    
    /**
     * @brief Initialize enhanced VM with authority systems
     * @param api Traditional curated API (for backward compatibility)
     * @param tracker UUID tracker (for backward compatibility)  
     * @param scriptAuth Script instance authority system
     * @param secureBridge Secure API bridge for validated calls
     * @param uuidAuth UUID authority system
     * @return True if initialization successful
     */
    bool initializeWithAuthority(WispCuratedAPIExtended* api, UUIDTracker* tracker,
                                ScriptInstanceAuthority* scriptAuth,
                                SecureWASHAPIBridge* secureBridge,
                                EngineUUIDAuthority* uuidAuth);
    
    /**
     * @brief Execute script function with full authority validation
     * @param bytecode Script bytecode to execute
     * @param functionName Function within script to call
     * @param scriptName Name of the script (for authority checks)
     * @param entityUUID Entity context (0 for panel/global scripts)
     * @param panelId Panel context (0 for entity/global scripts)  
     * @return True if execution completed successfully
     */
    bool executeFunction(WASHBytecode* bytecode, const char* functionName,
                        const std::string& scriptName,
                        uint32_t entityUUID = 0, uint16_t panelId = 0);
    
    /**
     * @brief Execute script with enhanced security context
     * 
     * This method provides the same interface as the base class but adds
     * authority validation and security checks.
     */
    bool executeScript(WASHBytecode* bytecode, const String& functionName, 
                      uint32_t entityUUID = 0, uint16_t panelId = 0) override;
    
    /**
     * @brief Set current script execution context for authority validation
     * @param scriptName Name of the currently executing script
     * @param permissions Permission level for this script
     */
    void setSecurityContext(const std::string& scriptName, 
                           ScriptInstanceAuthority::PermissionLevel permissions);
    
    /**
     * @brief Enable or disable authority validation
     * 
     * When disabled, VM operates in legacy compatibility mode.
     * When enabled, all operations go through authority validation.
     * 
     * @param enabled True to enable authority validation
     */
    void setAuthorityValidation(bool enabled) { authorityValidationEnabled = enabled; }
    
    /**
     * @brief Get enhanced security statistics
     */
    struct SecurityStats {
        uint32_t securityChecksPerformed;
        uint32_t violationsDetected;  
        uint32_t apiCallsBlocked;
        uint32_t unauthorizedUUIDAccess;
        uint32_t permissionDenied;
    };
    SecurityStats getSecurityStats() const;
    
    /**
     * @brief Reset security statistics
     */
    void resetSecurityStats();

protected:
    /**
     * @brief Enhanced curated API call execution with authority validation
     * 
     * Overrides the base class method to add security validation through
     * the SecureWASHAPIBridge and authority systems.
     */
    bool executeCuratedAPICall(WASHOpCode apiCall) override;
    
    /**
     * @brief Validate API call against current script permissions
     * @param apiCall The API operation being attempted
     * @return True if script has permission to execute this call
     */
    bool validateAPIPermission(WASHOpCode apiCall);
    
    /**
     * @brief Validate UUID access for current script context
     * @param uuid The UUID being accessed
     * @param operation The operation being performed
     * @return True if script can access this UUID
     */
    bool validateUUIDAccess(uint32_t uuid, const std::string& operation);
    
    /**
     * @brief Record security violation and take appropriate action
     * @param violation Description of the violation
     * @param severity Severity level (0=minor, 3=critical)
     */
    void recordSecurityViolation(const std::string& violation, uint8_t severity = 1);

private:
    // Enhanced API implementations with authority validation
    bool secureApiMoveEntity();
    bool secureApiSetPosition();
    bool secureApiGetPosition();
    bool secureApiSetVelocity();
    bool secureApiGetVelocity();
    bool secureApiSetSprite();
    bool secureApiSetAnimation();
    bool secureApiSetLayer();
    bool secureApiSetVisible();
    bool secureApiDestroyEntity();
    bool secureApiSpawnEntity();
    
    bool secureApiSetCamera();
    bool secureApiGetCamera();
    bool secureApiAddTile();
    bool secureApiRemoveTile();
    bool secureApiSetBackground();
    bool secureApiFocusEntity();
    
    bool secureApiPlaySound();
    bool secureApiSaveData();
    bool secureApiLoadData();
    bool secureApiSetTimer();
    bool secureApiLogMessage();
    
    bool secureApiFindEntitiesByType();
    bool secureApiFindEntitiesInRadius();
    bool secureApiGetEntityType();
    bool secureApiGetCurrentPanel();
    
    // Permission mapping helpers
    std::string getOperationName(WASHOpCode apiCall);
    bool isEntityOperation(WASHOpCode apiCall);
    bool isPanelOperation(WASHOpCode apiCall);
    bool isSystemOperation(WASHOpCode apiCall);
};

/**
 * @brief Enhanced WASH Runtime with Authority Integration
 * 
 * Provides the same interface as WASHRuntime but integrates with the new
 * authority systems for secure script management.
 */
class EnhancedWASHRuntime : public WASHRuntime {
private:
    // Authority systems
    ScriptInstanceAuthority scriptAuthority;
    SecureWASHAPIBridge secureApiBridge;
    EngineUUIDAuthority* uuidAuthority;
    
    // Enhanced VM
    EnhancedWASHVirtualMachine enhancedVM;
    
    // Migration support
    bool legacyCompatibilityMode;
    
public:
    EnhancedWASHRuntime();
    virtual ~EnhancedWASHRuntime();
    
    /**
     * @brief Initialize enhanced runtime with authority systems
     * @param apiPtr Curated API pointer
     * @param sceneMgr Scene manager
     * @param uuidAuth UUID authority system
     * @param enableLegacyMode True to maintain backward compatibility
     * @return True if initialization successful
     */
    bool initializeEnhanced(WispCuratedAPIExtended* apiPtr, SceneManager* sceneMgr,
                           EngineUUIDAuthority* uuidAuth, bool enableLegacyMode = true);
    
    /**
     * @brief Create entity script with authority validation
     * @param scriptName Script bytecode name
     * @param entityUUID Entity UUID
     * @param permissions Permission level for this script
     * @return True if script created successfully
     */
    bool createEntityScriptSecure(const String& scriptName, uint32_t entityUUID,
                                 ScriptInstanceAuthority::PermissionLevel permissions = 
                                 ScriptInstanceAuthority::PermissionLevel::STANDARD);
    
    /**
     * @brief Create panel script with authority validation  
     * @param scriptName Script bytecode name
     * @param panelId Panel ID
     * @param permissions Permission level for this script
     * @return True if script created successfully
     */
    bool createPanelScriptSecure(const String& scriptName, uint16_t panelId,
                                ScriptInstanceAuthority::PermissionLevel permissions = 
                                ScriptInstanceAuthority::PermissionLevel::STANDARD);
    
    /**
     * @brief Create global script with system permissions
     * @param scriptName Script bytecode name
     * @return True if script created successfully
     */
    bool createGlobalScriptSecure(const String& scriptName);
    
    /**
     * @brief Enable or disable legacy compatibility mode
     * 
     * In legacy mode, the runtime provides backward compatibility with
     * existing code while gradually migrating to the authority system.
     * 
     * @param enabled True to enable legacy compatibility
     */
    void setLegacyCompatibility(bool enabled) { legacyCompatibilityMode = enabled; }
    
    /**
     * @brief Get script authority system for direct access
     * @return Pointer to script instance authority
     */
    ScriptInstanceAuthority* getScriptAuthority() { return &scriptAuthority; }
    
    /**
     * @brief Get secure API bridge for direct access
     * @return Pointer to secure API bridge
     */
    SecureWASHAPIBridge* getSecureAPIBridge() { return &secureApiBridge; }
    
    /**
     * @brief Execute all scripts using the enhanced security model
     */
    void updateAllScriptsSecure();
    
    /**
     * @brief Execute entity scripts with authority validation
     */
    void updateEntityScriptsSecure();
    
    /**
     * @brief Execute panel scripts with authority validation
     */
    void updatePanelScriptsSecure();
    
    /**
     * @brief Execute global scripts with system permissions
     */
    void updateGlobalScriptsSecure();
    
    /**
     * @brief Dispatch collision event with authority validation
     * @param entityA First entity in collision
     * @param entityB Second entity in collision
     */
    void dispatchCollisionEventSecure(uint32_t entityA, uint32_t entityB);
    
    /**
     * @brief Dispatch input event with context validation
     * @param input Input semantic triggered
     * @param pressed True if pressed, false if released
     */
    void dispatchInputEventSecure(WispInputSemantic input, bool pressed);
    
    /**
     * @brief Get comprehensive runtime statistics
     */
    struct RuntimeStats {
        // Base runtime stats
        uint16_t activeScriptCount;
        uint32_t frameExecutionTime;
        uint16_t errorCount;
        
        // Authority system stats
        uint16_t entityScriptsActive;
        uint16_t panelScriptsActive;
        uint16_t globalScriptsActive;
        uint16_t quarantinedScripts;
        
        // Security stats
        uint32_t securityViolations;
        uint32_t blockedAPICall;
        uint32_t unauthorizedUUIDAccess;
    };
    RuntimeStats getRuntimeStats() const;

protected:
    /**
     * @brief Migrate legacy script to authority system
     * @param scriptInstance Legacy script instance
     * @return True if migration successful
     */
    bool migrateLegacyScript(const WASHScriptInstance& scriptInstance);
    
private:
    // Internal migration helpers
    ScriptInstanceAuthority::PermissionLevel determineScriptPermissions(const String& scriptType);
    bool validateLegacyScriptSecurity(const WASHScriptInstance& script);
};

/**
 * @brief Factory class for creating enhanced WASH components
 */
class WASHFactory {
public:
    /**
     * @brief Create enhanced WASH runtime with full authority integration
     * @param api Curated API
     * @param sceneManager Scene manager
     * @param uuidAuthority UUID authority system
     * @return Configured enhanced runtime, or nullptr on failure
     */
    static std::unique_ptr<EnhancedWASHRuntime> createEnhancedRuntime(
        WispCuratedAPIExtended* api, 
        SceneManager* sceneManager,
        EngineUUIDAuthority* uuidAuthority);
    
    /**
     * @brief Create enhanced WASH VM for standalone use
     * @param scriptAuth Script authority
     * @param secureAPI Secure API bridge  
     * @param uuidAuth UUID authority
     * @return Configured enhanced VM, or nullptr on failure
     */
    static std::unique_ptr<EnhancedWASHVirtualMachine> createEnhancedVM(
        ScriptInstanceAuthority* scriptAuth,
        SecureWASHAPIBridge* secureAPI,
        EngineUUIDAuthority* uuidAuth);
};

} // namespace Script
} // namespace WispEngine
