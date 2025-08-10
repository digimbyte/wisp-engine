#pragma once

#include "uuid_authority.h"
#include "script_instance_authority.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include "esp_log.h"

static const char* NAMED_ENTITY_TAG = "NamedEntity";

/**
 * @brief Named Entity Reference System
 * 
 * Provides efficient O(1) lookup for named entities instead of slow type-based searches.
 * Supports game mechanics like enable/disable, group operations, and state management.
 * 
 * Key Features:
 * - Named entity registration: "mythic_tree", "player_spawn", "boss_gate"
 * - Group operations: "all_enemies", "ui_elements", "interactive_objects"  
 * - State management: enable/disable, visible/hidden, active/inactive
 * - Efficient hash-based lookups instead of linear searches
 * - Panel-scoped access control for security
 * - Batch operations for performance
 */
class NamedEntityRegistry {
public:
    /**
     * @brief Entity states that can be controlled via scripts
     */
    enum class EntityState : uint8_t {
        ACTIVE = 0,      // Entity is active and processing
        INACTIVE = 1,    // Entity exists but is paused
        HIDDEN = 2,      // Entity is invisible but still active
        DISABLED = 3,    // Entity is completely disabled
        DESTROYED = 4    // Entity is marked for destruction
    };
    
    /**
     * @brief Named entity reference with metadata
     */
    struct NamedEntity {
        uint32_t uuid;              // Engine UUID
        std::string name;           // Human-readable name
        std::string entityType;     // Type for categorization  
        uint16_t panelId;           // Panel ownership
        EntityState state;          // Current state
        
        // Game mechanics support
        std::unordered_set<std::string> tags;    // Tags like "interactive", "collectible"
        std::unordered_set<std::string> groups;  // Groups like "enemies", "ui_elements"
        
        // Metadata for game mechanics
        float priority;             // For sorting/priority operations
        uint32_t lastModified;      // Timestamp of last state change
        std::string metadata;       // JSON-like metadata string
        
        NamedEntity() : uuid(0), panelId(0), state(EntityState::ACTIVE), 
                       priority(0.0f), lastModified(0) {}
        
        NamedEntity(uint32_t uuid, const std::string& name, const std::string& type, uint16_t panel)
            : uuid(uuid), name(name), entityType(type), panelId(panel), 
              state(EntityState::ACTIVE), priority(0.0f), lastModified(0) {}
    };

private:
    // Core storage
    std::unordered_map<std::string, NamedEntity> namedEntities;      // name -> entity
    std::unordered_map<uint32_t, std::string> uuidToName;           // uuid -> name
    
    // Group and tag indices for fast lookups
    std::unordered_map<std::string, std::unordered_set<std::string>> groupMembers;  // group -> entity names
    std::unordered_map<std::string, std::unordered_set<std::string>> tagMembers;    // tag -> entity names
    
    // Panel-scoped access
    std::unordered_map<uint16_t, std::unordered_set<std::string>> panelEntities;   // panel -> entity names
    
    // Authority systems
    EngineUUIDAuthority* uuidAuthority;
    ScriptInstanceAuthority* scriptAuthority;
    
    // Performance tracking
    uint32_t totalLookups;
    uint32_t cacheHits;
    uint32_t cacheMisses;

public:
    NamedEntityRegistry(EngineUUIDAuthority* uuidAuth, ScriptInstanceAuthority* scriptAuth);
    ~NamedEntityRegistry();
    
    // === ENTITY REGISTRATION ===
    
    /**
     * @brief Register a named entity in the system
     * @param uuid Engine-assigned UUID
     * @param name Human-readable name (unique per panel)
     * @param entityType Entity type for categorization
     * @param panelId Panel this entity belongs to
     * @return True if registration successful
     */
    bool registerEntity(uint32_t uuid, const std::string& name, 
                       const std::string& entityType, uint16_t panelId);
    
    /**
     * @brief Unregister entity by name
     * @param name Entity name to unregister
     * @return True if entity was found and removed
     */
    bool unregisterEntity(const std::string& name);
    
    /**
     * @brief Unregister entity by UUID
     * @param uuid Entity UUID to unregister
     * @return True if entity was found and removed
     */
    bool unregisterEntityByUUID(uint32_t uuid);
    
    /**
     * @brief Clear all entities for a specific panel
     * @param panelId Panel to clear
     */
    void clearPanel(uint16_t panelId);
    
    // === ENTITY LOOKUP ===
    
    /**
     * @brief Get UUID by entity name (O(1) lookup)
     * @param name Entity name
     * @param requesterPanelId Panel of the requesting script (for security)
     * @return UUID or 0 if not found/not accessible
     */
    uint32_t getUUID(const std::string& name, uint16_t requesterPanelId);
    
    /**
     * @brief Get entity name by UUID
     * @param uuid Entity UUID
     * @return Entity name or empty string if not found
     */
    std::string getName(uint32_t uuid);
    
    /**
     * @brief Check if named entity exists and is accessible
     * @param name Entity name
     * @param requesterPanelId Panel of the requesting script
     * @return True if entity exists and is accessible
     */
    bool exists(const std::string& name, uint16_t requesterPanelId);
    
    /**
     * @brief Get entity state
     * @param name Entity name
     * @param requesterPanelId Panel of the requesting script
     * @return Current entity state
     */
    EntityState getState(const std::string& name, uint16_t requesterPanelId);
    
    // === STATE MANAGEMENT ===
    
    /**
     * @brief Set entity state (enable/disable/hide/etc.)
     * @param name Entity name
     * @param state New state to set
     * @param requesterPanelId Panel of the requesting script
     * @return True if state was changed successfully
     */
    bool setState(const std::string& name, EntityState state, uint16_t requesterPanelId);
    
    /**
     * @brief Enable entity (set to ACTIVE state)
     * @param name Entity name
     * @param requesterPanelId Panel of the requesting script
     * @return True if successful
     */
    bool enableEntity(const std::string& name, uint16_t requesterPanelId);
    
    /**
     * @brief Disable entity (set to DISABLED state)
     * @param name Entity name
     * @param requesterPanelId Panel of the requesting script
     * @return True if successful
     */
    bool disableEntity(const std::string& name, uint16_t requesterPanelId);
    
    /**
     * @brief Hide entity (set to HIDDEN state)
     * @param name Entity name
     * @param requesterPanelId Panel of the requesting script
     * @return True if successful
     */
    bool hideEntity(const std::string& name, uint16_t requesterPanelId);
    
    /**
     * @brief Show entity (set to ACTIVE state)
     * @param name Entity name
     * @param requesterPanelId Panel of the requesting script
     * @return True if successful
     */
    bool showEntity(const std::string& name, uint16_t requesterPanelId);
    
    // === GROUP OPERATIONS ===
    
    /**
     * @brief Add entity to a group
     * @param name Entity name
     * @param group Group name (e.g., "enemies", "ui_elements")
     * @param requesterPanelId Panel of the requesting script
     * @return True if successful
     */
    bool addToGroup(const std::string& name, const std::string& group, uint16_t requesterPanelId);
    
    /**
     * @brief Remove entity from a group
     * @param name Entity name
     * @param group Group name
     * @param requesterPanelId Panel of the requesting script
     * @return True if successful
     */
    bool removeFromGroup(const std::string& name, const std::string& group, uint16_t requesterPanelId);
    
    /**
     * @brief Get all entity names in a group (scoped to requester's panel)
     * @param group Group name
     * @param requesterPanelId Panel of the requesting script
     * @return Vector of entity names in the group
     */
    std::vector<std::string> getGroupMembers(const std::string& group, uint16_t requesterPanelId);
    
    /**
     * @brief Set state for entire group
     * @param group Group name
     * @param state State to set for all group members
     * @param requesterPanelId Panel of the requesting script
     * @return Number of entities affected
     */
    uint32_t setGroupState(const std::string& group, EntityState state, uint16_t requesterPanelId);
    
    /**
     * @brief Enable entire group
     * @param group Group name
     * @param requesterPanelId Panel of the requesting script
     * @return Number of entities enabled
     */
    uint32_t enableGroup(const std::string& group, uint16_t requesterPanelId);
    
    /**
     * @brief Disable entire group
     * @param group Group name
     * @param requesterPanelId Panel of the requesting script
     * @return Number of entities disabled
     */
    uint32_t disableGroup(const std::string& group, uint16_t requesterPanelId);
    
    // === TAG OPERATIONS ===
    
    /**
     * @brief Add tag to entity
     * @param name Entity name
     * @param tag Tag to add (e.g., "interactive", "collectible", "boss")
     * @param requesterPanelId Panel of the requesting script
     * @return True if successful
     */
    bool addTag(const std::string& name, const std::string& tag, uint16_t requesterPanelId);
    
    /**
     * @brief Remove tag from entity
     * @param name Entity name
     * @param tag Tag to remove
     * @param requesterPanelId Panel of the requesting script
     * @return True if successful
     */
    bool removeTag(const std::string& name, const std::string& tag, uint16_t requesterPanelId);
    
    /**
     * @brief Check if entity has tag
     * @param name Entity name
     * @param tag Tag to check
     * @param requesterPanelId Panel of the requesting script
     * @return True if entity has the tag
     */
    bool hasTag(const std::string& name, const std::string& tag, uint16_t requesterPanelId);
    
    /**
     * @brief Get all entities with a specific tag (scoped to requester's panel)
     * @param tag Tag to search for
     * @param requesterPanelId Panel of the requesting script
     * @return Vector of entity names with the tag
     */
    std::vector<std::string> getEntitiesWithTag(const std::string& tag, uint16_t requesterPanelId);
    
    // === METADATA OPERATIONS ===
    
    /**
     * @brief Set entity metadata (JSON-like string)
     * @param name Entity name
     * @param metadata Metadata string
     * @param requesterPanelId Panel of the requesting script
     * @return True if successful
     */
    bool setMetadata(const std::string& name, const std::string& metadata, uint16_t requesterPanelId);
    
    /**
     * @brief Get entity metadata
     * @param name Entity name
     * @param requesterPanelId Panel of the requesting script
     * @return Metadata string or empty if not found
     */
    std::string getMetadata(const std::string& name, uint16_t requesterPanelId);
    
    /**
     * @brief Set entity priority (for sorting/ordering operations)
     * @param name Entity name
     * @param priority Priority value (higher = more important)
     * @param requesterPanelId Panel of the requesting script
     * @return True if successful
     */
    bool setPriority(const std::string& name, float priority, uint16_t requesterPanelId);
    
    /**
     * @brief Get entity priority
     * @param name Entity name
     * @param requesterPanelId Panel of the requesting script
     * @return Priority value or 0.0 if not found
     */
    float getPriority(const std::string& name, uint16_t requesterPanelId);
    
    // === BATCH OPERATIONS ===
    
    /**
     * @brief Execute batch state changes atomically
     * @param operations Vector of {entityName, newState} pairs
     * @param requesterPanelId Panel of the requesting script
     * @return Number of successful operations
     */
    struct BatchOperation {
        std::string entityName;
        EntityState newState;
    };
    uint32_t executeBatch(const std::vector<BatchOperation>& operations, uint16_t requesterPanelId);
    
    // === STATISTICS AND DEBUGGING ===
    
    /**
     * @brief Get performance statistics
     */
    struct Stats {
        uint32_t totalEntities;
        uint32_t totalGroups;
        uint32_t totalTags;
        uint32_t totalLookups;
        uint32_t cacheHitRate;  // Percentage
    };
    Stats getStats() const;
    
    /**
     * @brief Dump all named entities for debugging
     * @param panelId Panel to dump (0 for all panels)
     */
    void dumpEntities(uint16_t panelId = 0);

private:
    /**
     * @brief Validate access to named entity
     * @param name Entity name
     * @param requesterPanelId Panel of the requesting script
     * @return True if access is allowed
     */
    bool validateAccess(const std::string& name, uint16_t requesterPanelId);
    
    /**
     * @brief Get current time in milliseconds
     * @return Current timestamp
     */
    uint32_t getCurrentTimeMs();
    
    /**
     * @brief Clean up destroyed entities
     */
    void cleanupDestroyedEntities();
};
