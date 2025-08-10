// uuid_authority.h - Engine UUID Authority System
// Enhances existing UUIDTracker with full engine authority over entity lifecycle
// ESP-IDF compatible, zero-trust security model

#pragma once
#include "../engine_common.h"
#include "../script/secure_wash_vm.h"
#include "../scene/scene_system.h"
#include <unordered_map>
#include <unordered_set>

namespace WispEngine {
namespace Security {

// Forward declarations
class SecureWASHAPIBridge;

// === ENHANCED ENTITY AUTHORITY STRUCTURE ===

struct EntityAuthority {
    uint32_t uuid;                       // Engine-assigned UUID (immutable)
    uint16_t engineEntityId;             // Internal scene entity ID
    uint16_t panelId;                    // Panel ownership
    String entityType;                   // Type for script searches
    String scriptName;                   // Controlling script (if any)
    
    // Security and permission flags
    bool allowScriptControl;             // Can scripts control this entity
    uint8_t permissionMask;              // Bitfield of allowed operations
    bool pendingDestruction;             // Marked for cleanup
    
    // Lifecycle tracking
    uint32_t creationTime;               // When entity was created
    uint32_t lastAccessTime;             // Last script access (for debugging)
    uint16_t accessCount;                // Total script accesses
    
    EntityAuthority() : uuid(0), engineEntityId(0), panelId(0), 
                       allowScriptControl(true), permissionMask(0xFF),
                       pendingDestruction(false), creationTime(0),
                       lastAccessTime(0), accessCount(0) {}
                       
    EntityAuthority(uint32_t _uuid, uint16_t _entityId, uint16_t _panelId, 
                   const String& _type, const String& _scriptName = "") 
        : uuid(_uuid), engineEntityId(_entityId), panelId(_panelId),
          entityType(_type), scriptName(_scriptName), allowScriptControl(true),
          permissionMask(0xFF), pendingDestruction(false), 
          creationTime(get_millis()), lastAccessTime(0), accessCount(0) {}
};

// Permission bit masks for entity operations
enum EntityPermission : uint8_t {
    PERM_READ_POSITION   = 0x01,    // Can read entity position
    PERM_WRITE_POSITION  = 0x02,    // Can modify entity position
    PERM_READ_VELOCITY   = 0x04,    // Can read entity velocity
    PERM_WRITE_VELOCITY  = 0x08,    // Can modify entity velocity
    PERM_CONTROL_SPRITE  = 0x10,    // Can change sprite/animation
    PERM_CONTROL_AUDIO   = 0x20,    // Can play entity sounds
    PERM_DESTROY_ENTITY  = 0x40,    // Can destroy entity
    PERM_SPAWN_ENTITY    = 0x80,    // Can spawn new entities
    
    PERM_READ_ONLY       = PERM_READ_POSITION | PERM_READ_VELOCITY,
    PERM_BASIC_CONTROL   = PERM_READ_ONLY | PERM_WRITE_POSITION | PERM_WRITE_VELOCITY,
    PERM_FULL_CONTROL    = 0xFF     // All permissions
};

// === ENGINE UUID AUTHORITY CLASS ===
// Extends and enhances the existing UUIDTracker with full security model

class EngineUUIDAuthority {
private:
    // Core UUID management
    static uint32_t nextUUID;
    static bool initialized;
    
    // Enhanced entity registry (replaces simple array in UUIDTracker)
    std::unordered_map<uint32_t, EntityAuthority> entityRegistry;
    std::unordered_map<uint16_t, std::unordered_set<uint32_t>> panelEntities;
    std::unordered_map<String, std::unordered_set<uint32_t>> typeEntities;
    
    // Security validation
    std::unordered_set<uint32_t> pendingDestruction;
    
    // Performance tracking
    uint32_t totalEntitiesCreated;
    uint32_t totalValidationCalls;
    uint32_t totalSecurityViolations;
    
    // Integration with existing scene system
    SceneManager* sceneManager;
    
public:
    EngineUUIDAuthority();
    ~EngineUUIDAuthority();
    
    // === INITIALIZATION ===
    bool initialize(SceneManager* sceneMgr);
    void shutdown();
    static EngineUUIDAuthority& getInstance();
    
    // === UUID CREATION (ENGINE AUTHORITY ONLY) ===
    
    // Primary entity creation method - ENGINE OWNS UUID GENERATION
    uint32_t createEntityUUID(const String& entityType, uint16_t panelId, 
                             const String& scriptName = "");
    
    // Validate UUID without exposing internal structure
    bool validateUUID(uint32_t uuid) const;
    bool isValidForOperation(uint32_t uuid, EntityPermission operation) const;
    
    // === SCRIPT AUTHORIZATION ===
    
    // Authorize script operations on entities
    bool authorizeScriptOperation(uint32_t uuid, const String& scriptName, 
                                 const String& operation) const;
    
    // Set entity permissions for specific script
    bool setEntityPermissions(uint32_t uuid, const String& scriptName, 
                             uint8_t permissionMask);
    
    // === ENTITY LIFECYCLE MANAGEMENT ===
    
    // Register entity with scene system (called after SceneManager creates entity)
    bool registerEntity(uint32_t uuid, uint16_t sceneEntityId);
    
    // Mark entity for destruction (cleanup handled by engine)
    void markForDestruction(uint32_t uuid, const String& requestingScript = "");
    
    // Clean up pending entities (called by main update loop)
    void cleanupPendingEntities();
    
    // Unregister entity (called by scene system during cleanup)
    void unregisterEntity(uint32_t uuid);
    
    // === SECURE ENTITY QUERIES (FOR SCRIPTS) ===
    
    // Find entities by type within specific panel (prevents cross-panel access)
    std::vector<uint32_t> findEntitiesByType(const String& type, uint16_t panelId) const;
    
    // Find entities within radius (with bounds checking)
    std::vector<uint32_t> findEntitiesInRadius(float centerX, float centerY, 
                                              float radius, uint16_t panelId) const;
    
    // Get entity information (validated)
    String getEntityType(uint32_t uuid) const;
    uint16_t getEntityPanelId(uint32_t uuid) const;
    String getEntityScriptName(uint32_t uuid) const;
    
    // === ENGINE-ONLY ACCESS METHODS ===
    
    // Get internal scene entity ID (for engine use only)
    uint16_t getEngineEntityId(uint32_t uuid) const;
    
    // Get entity authority data (for debugging and validation)
    const EntityAuthority* getEntityAuthority(uint32_t uuid) const;
    
    // === PANEL MANAGEMENT ===
    
    // Clear all entities in panel (called during scene transitions)
    void clearPanel(uint16_t panelId);
    
    // Get all entities in panel (for engine use)
    std::vector<uint32_t> getPanelEntities(uint16_t panelId) const;
    
    // === SECURITY VALIDATION ===
    
    // Validate entity exists and is accessible
    bool validateEntityAccess(uint32_t uuid, const String& requestingScript, 
                             const String& operation) const;
    
    // Check if operation is permitted
    bool isOperationPermitted(uint32_t uuid, EntityPermission permission) const;
    
    // === DEBUGGING AND MONITORING ===
    
    // Get statistics
    uint32_t getTotalEntities() const { return entityRegistry.size(); }
    uint32_t getTotalCreated() const { return totalEntitiesCreated; }
    uint32_t getTotalValidations() const { return totalValidationCalls; }
    uint32_t getSecurityViolations() const { return totalSecurityViolations; }
    
    // Debug output
    void dumpEntityRegistry() const;
    void dumpSecurityStats() const;
    bool validateSystemIntegrity() const;
    
    // === COMPATIBILITY WITH EXISTING UUIDTracker ===
    
    // Provides same interface as existing UUIDTracker for backward compatibility
    bool registerEntity(uint32_t uuid, uint16_t entityIndex, uint16_t panelId, const String& type);
    uint16_t getEntityIndex(uint32_t uuid) const { return getEngineEntityId(uuid); }
    bool isValid(uint32_t uuid) const { return validateUUID(uuid); }
    void clearAll();
    uint16_t getTrackedCount() const { return static_cast<uint16_t>(entityRegistry.size()); }
    
private:
    // === INTERNAL VALIDATION ===
    
    void recordSecurityViolation(uint32_t uuid, const String& operation, 
                                const String& scriptName) const;
    
    bool isUUIDInRegistry(uint32_t uuid) const;
    void updateAccessTracking(uint32_t uuid) const;
    
    // === INTERNAL CLEANUP ===
    
    void removeFromPanelIndex(uint32_t uuid, uint16_t panelId);
    void removeFromTypeIndex(uint32_t uuid, const String& type);
    
    // === UUID GENERATION ===
    
    uint32_t generateNextUUID();
    bool isUUIDCollision(uint32_t uuid) const;
};

// === GLOBAL INSTANCE ACCESS ===

// Get the global UUID authority instance
EngineUUIDAuthority& GetUUIDAuthority();

} // namespace Security
} // namespace WispEngine
