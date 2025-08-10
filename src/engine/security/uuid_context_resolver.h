#pragma once

#include "secure_api_bridge.h"
#include "uuid_authority.h"
#include <unordered_map>
#include <string>

/**
 * @brief UUID Context Resolver for Script References
 * 
 * Handles the mapping between script-friendly references and actual UUIDs.
 * Provides context-aware resolution of 'this', named entities, and search results.
 */
class UUIDContextResolver {
public:
    /**
     * @brief Special UUID constants for script context
     */
    static constexpr uint32_t UUID_THIS = 0xFFFFFFFF;      // 'this' reference
    static constexpr uint32_t UUID_INVALID = 0;           // Invalid/null reference
    
    /**
     * @brief Context-aware entity references
     */
    enum class EntityReference : uint32_t {
        THIS_ENTITY = UUID_THIS,       // The entity this script is attached to
        INVALID = UUID_INVALID,        // Invalid/null reference
        DYNAMIC_START = 1000          // Starting point for engine-assigned UUIDs
    };

private:
    // Current execution context
    std::string currentScriptName;
    uint32_t currentContextUUID;        // Entity UUID for entity scripts
    uint16_t currentPanelId;            // Panel ID for panel scripts
    ScriptInstanceAuthority::ScriptType currentScriptType;
    
    // Authority systems
    EngineUUIDAuthority* uuidAuthority;
    ScriptInstanceAuthority* scriptAuthority;
    
    // Named entity cache (for performance)
    std::unordered_map<std::string, std::vector<uint32_t>> namedEntityCache;
    uint32_t lastCacheUpdate;
    static constexpr uint32_t CACHE_LIFETIME_MS = 100; // 100ms cache lifetime

public:
    UUIDContextResolver(EngineUUIDAuthority* uuidAuth, ScriptInstanceAuthority* scriptAuth);
    
    /**
     * @brief Set the current script execution context
     * @param scriptName Name of the executing script
     * @param scriptType Type of script (entity, panel, global)
     * @param contextUUID Entity UUID for entity scripts (0 for others)
     * @param panelId Panel ID for panel/entity scripts (0 for global)
     */
    void setExecutionContext(const std::string& scriptName,
                           ScriptInstanceAuthority::ScriptType scriptType,
                           uint32_t contextUUID, uint16_t panelId);
    
    /**
     * @brief Resolve a UUID reference in the current script context
     * @param scriptUUID The UUID reference from the script (may be UUID_THIS)
     * @param operation The operation being performed (for security validation)
     * @return Actual engine UUID, or UUID_INVALID if not accessible
     */
    uint32_t resolveUUID(uint32_t scriptUUID, const std::string& operation);
    
    /**
     * @brief Find entities by type with context validation
     * @param entityType Type of entities to find
     * @return Vector of UUIDs accessible to the current script
     */
    std::vector<uint32_t> findEntitiesByType(const std::string& entityType);
    
    /**
     * @brief Find entities in radius with context validation  
     * @param x Center X coordinate
     * @param y Center Y coordinate
     * @param radius Search radius
     * @return Vector of UUIDs accessible to the current script
     */
    std::vector<uint32_t> findEntitiesInRadius(float x, float y, float radius);
    
    /**
     * @brief Check if current script can access a specific UUID
     * @param uuid The UUID to check
     * @param operation The operation being attempted
     * @return True if script has access to this UUID
     */
    bool canAccessUUID(uint32_t uuid, const std::string& operation);
    
    /**
     * @brief Get the current script's context UUID ("this" entity)
     * @return Context UUID, or UUID_INVALID if not an entity script
     */
    uint32_t getThisUUID() const { return currentContextUUID; }
    
    /**
     * @brief Get the current script's panel ID
     * @return Panel ID, or 0 if global script
     */
    uint16_t getCurrentPanelId() const { return currentPanelId; }
    
    /**
     * @brief Clear entity cache (called when entities are created/destroyed)
     */
    void invalidateCache();

private:
    /**
     * @brief Update named entity cache if needed
     */
    void updateCacheIfNeeded();
    
    /**
     * @brief Validate UUID access based on script type and permissions
     * @param uuid UUID being accessed
     * @param operation Operation being performed
     * @return True if access is allowed
     */
    bool validateUUIDAccess(uint32_t uuid, const std::string& operation);
    
    /**
     * @brief Get current time in milliseconds
     * @return Current time for cache management
     */
    uint32_t getCurrentTimeMs();
};

/**
 * @brief Enhanced SecureWASHAPIBridge with Context Resolution
 * 
 * Extends the existing SecureWASHAPIBridge to handle context-aware UUID resolution.
 */
class ContextAwareSecureWASHAPIBridge : public SecureWASHAPIBridge {
private:
    UUIDContextResolver uuidResolver;
    
public:
    ContextAwareSecureWASHAPIBridge(WispCuratedAPIExtended* api, EngineUUIDAuthority* uuidAuth,
                                   ScriptInstanceAuthority* scriptAuth);
    
    /**
     * @brief Set execution context with enhanced UUID resolution
     */
    void setExecutionContext(const std::string& scriptName, uint32_t contextUUID = 0, 
                           uint16_t panelId = 0) override;
    
    /**
     * @brief Move entity with context-aware UUID resolution
     * @param scriptUUID UUID reference from script (may be UUID_THIS)
     * @param dx X movement delta
     * @param dy Y movement delta
     * @return True if movement successful
     */
    bool apiMoveEntity(uint32_t scriptUUID, float dx, float dy) override;
    
    /**
     * @brief Set entity position with context resolution
     * @param scriptUUID UUID reference from script (may be UUID_THIS)
     * @param x New X position
     * @param y New Y position
     * @return True if position set successfully
     */
    bool apiSetPosition(uint32_t scriptUUID, float x, float y) override;
    
    /**
     * @brief Get entity position with context resolution
     * @param scriptUUID UUID reference from script (may be UUID_THIS)
     * @return Entity position, or (0,0) if inaccessible
     */
    WispVec2 apiGetPosition(uint32_t scriptUUID) override;
    
    /**
     * @brief Set entity animation with context resolution
     * @param scriptUUID UUID reference from script (may be UUID_THIS)
     * @param animName Animation name to set
     * @return True if animation set successfully
     */
    bool apiSetAnimation(uint32_t scriptUUID, const std::string& animName) override;
    
    /**
     * @brief Destroy entity with context resolution and validation
     * @param scriptUUID UUID reference from script (may be UUID_THIS)
     * @return True if entity destroyed successfully
     */
    bool apiDestroyEntity(uint32_t scriptUUID) override;
    
    /**
     * @brief Find entities by type with automatic context scoping
     * @param entityType Type of entities to find
     * @return Vector of UUIDs accessible to current script
     */
    std::vector<uint32_t> apiFindEntitiesByType(const std::string& entityType) override;
    
    /**
     * @brief Find entities in radius with automatic context scoping
     * @param x Center X coordinate
     * @param y Center Y coordinate  
     * @param radius Search radius
     * @return Vector of UUIDs accessible to current script
     */
    std::vector<uint32_t> apiFindEntitiesInRadius(float x, float y, float radius) override;
    
    /**
     * @brief Get UUID context resolver for direct access
     * @return Reference to the UUID context resolver
     */
    UUIDContextResolver& getContextResolver() { return uuidResolver; }
};

/**
 * @brief WASH Script API Extensions for Context References
 * 
 * These would be compiled into the WASH bytecode from ASH scripts.
 */
namespace WASHScriptAPI {
    
    /**
     * @brief Script-side API functions that get compiled to WASH opcodes
     */
    
    // Entity operations on 'this'
    inline void moveThis(float dx, float dy) {
        // Compiles to: OP_API_MOVE_ENTITY with UUID_THIS
    }
    
    inline void setThisPosition(float x, float y) {
        // Compiles to: OP_API_SET_POSITION with UUID_THIS
    }
    
    inline void setThisAnimation(const char* animName) {
        // Compiles to: OP_API_SET_ANIMATION with UUID_THIS
    }
    
    inline void destroyThis() {
        // Compiles to: OP_API_DESTROY_ENTITY with UUID_THIS
    }
    
    // Entity operations on other entities
    inline void moveEntity(uint32_t uuid, float dx, float dy) {
        // Compiles to: OP_API_MOVE_ENTITY with provided UUID
    }
    
    inline void setEntityAnimation(uint32_t uuid, const char* animName) {
        // Compiles to: OP_API_SET_ANIMATION with provided UUID
    }
    
    // Entity discovery
    inline std::vector<uint32_t> findByType(const char* type) {
        // Compiles to: OP_API_FIND_ENTITIES_BY_TYPE
    }
    
    inline std::vector<uint32_t> findNearby(float x, float y, float radius) {
        // Compiles to: OP_API_FIND_ENTITIES_IN_RADIUS
    }
}

/**
 * @brief Example ASH script showing UUID reference usage
 */
/*

// Entity script example - attached to a player entity
entity_script "player_controller" {
    function onUpdate() {
        // 'this' automatically resolves to the player entity's UUID
        var pos = getPosition(this);
        
        // Find nearby enemies (automatically scoped to current panel)
        var enemies = findByType("enemy");
        
        // Check each enemy
        for (var enemy : enemies) {
            var enemyPos = getPosition(enemy);
            var distance = length(pos - enemyPos);
            
            if (distance < 30.0) {
                // Enemy is close, play defensive animation
                setAnimation(this, "defend");
                break;
            }
        }
    }
    
    function onInputPressed(input) {
        if (input == INPUT_LEFT) {
            moveThis(-2.0, 0.0);
            setAnimation(this, "walk_left");
        }
        else if (input == INPUT_ATTACK) {
            setAnimation(this, "attack");
            
            // Find enemies in attack range
            var pos = getPosition(this);
            var targets = findNearby(pos.x, pos.y, 25.0);
            
            for (var target : targets) {
                if (getEntityType(target) == "enemy") {
                    // Attack this enemy
                    destroyEntity(target);
                    playSound("hit");
                }
            }
        }
    }
    
    function onCollision(otherUUID) {
        var otherType = getEntityType(otherUUID);
        
        if (otherType == "powerup") {
            destroyEntity(otherUUID);  // Destroy the powerup
            playSound("pickup");
        }
        else if (otherType == "enemy") {
            // Take damage
            setAnimation(this, "hurt");
            playSound("damage");
        }
    }
}

// Panel script example - attached to a UI panel
panel_script "game_ui" {
    function onInputPressed(input) {
        if (input == INPUT_PAUSE) {
            // Find all entities in this panel and pause their animations
            var allEntities = findByType("*");  // Special case for all types
            
            for (var entity : allEntities) {
                setAnimation(entity, "paused");
            }
        }
    }
}

*/
