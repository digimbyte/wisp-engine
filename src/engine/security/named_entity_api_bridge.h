#pragma once

#include "secure_api_bridge.h"
#include "named_entity_registry.h"
#include "uuid_context_resolver.h"
#include <string>
#include <vector>

/**
 * @brief Named Entity API Bridge
 * 
 * Extends SecureWASHAPIBridge with efficient named entity operations.
 * Provides game-mechanic friendly APIs for entity control without slow searches.
 * 
 * Key Features:
 * - O(1) named entity lookups instead of O(n) type searches
 * - Group operations for batch entity control
 * - State management for game mechanics
 * - Tag-based entity organization
 * - Metadata support for complex game logic
 */
class NamedEntityAPIBridge : public SecureWASHAPIBridge {
private:
    NamedEntityRegistry* namedRegistry;
    UUIDContextResolver* contextResolver;

public:
    NamedEntityAPIBridge(WispCuratedAPIExtended* api, EngineUUIDAuthority* uuidAuth,
                        ScriptInstanceAuthority* scriptAuth, NamedEntityRegistry* namedReg);
    
    virtual ~NamedEntityAPIBridge();
    
    // === NAMED ENTITY OPERATIONS ===
    
    /**
     * @brief Enable a named entity
     * @param entityName Name of entity to enable (e.g., "mythic_tree")
     * @return True if successful
     */
    bool apiEnableEntity(const std::string& entityName);
    
    /**
     * @brief Disable a named entity  
     * @param entityName Name of entity to disable
     * @return True if successful
     */
    bool apiDisableEntity(const std::string& entityName);
    
    /**
     * @brief Hide a named entity (invisible but still active)
     * @param entityName Name of entity to hide
     * @return True if successful
     */
    bool apiHideEntity(const std::string& entityName);
    
    /**
     * @brief Show a named entity
     * @param entityName Name of entity to show
     * @return True if successful
     */
    bool apiShowEntity(const std::string& entityName);
    
    /**
     * @brief Check if named entity exists and is accessible
     * @param entityName Name of entity to check
     * @return True if entity exists
     */
    bool apiEntityExists(const std::string& entityName);
    
    /**
     * @brief Get state of named entity
     * @param entityName Name of entity
     * @return Entity state as integer (0=ACTIVE, 1=INACTIVE, 2=HIDDEN, 3=DISABLED, 4=DESTROYED)
     */
    int32_t apiGetEntityState(const std::string& entityName);
    
    /**
     * @brief Move named entity by delta
     * @param entityName Name of entity to move
     * @param dx X movement delta
     * @param dy Y movement delta
     * @return True if successful
     */
    bool apiMoveNamedEntity(const std::string& entityName, float dx, float dy);
    
    /**
     * @brief Set position of named entity
     * @param entityName Name of entity
     * @param x New X position
     * @param y New Y position
     * @return True if successful
     */
    bool apiSetNamedEntityPosition(const std::string& entityName, float x, float y);
    
    /**
     * @brief Get position of named entity
     * @param entityName Name of entity
     * @return Entity position as WispVec2
     */
    WispVec2 apiGetNamedEntityPosition(const std::string& entityName);
    
    /**
     * @brief Set animation of named entity
     * @param entityName Name of entity
     * @param animName Animation name to set
     * @return True if successful
     */
    bool apiSetNamedEntityAnimation(const std::string& entityName, const std::string& animName);
    
    // === GROUP OPERATIONS ===
    
    /**
     * @brief Enable entire group of entities
     * @param groupName Name of group (e.g., "all_enemies", "ui_elements")
     * @return Number of entities enabled
     */
    uint32_t apiEnableGroup(const std::string& groupName);
    
    /**
     * @brief Disable entire group of entities
     * @param groupName Name of group
     * @return Number of entities disabled
     */
    uint32_t apiDisableGroup(const std::string& groupName);
    
    /**
     * @brief Hide entire group of entities
     * @param groupName Name of group
     * @return Number of entities hidden
     */
    uint32_t apiHideGroup(const std::string& groupName);
    
    /**
     * @brief Show entire group of entities
     * @param groupName Name of group
     * @return Number of entities shown
     */
    uint32_t apiShowGroup(const std::string& groupName);
    
    /**
     * @brief Add entity to a group
     * @param entityName Name of entity
     * @param groupName Name of group to add to
     * @return True if successful
     */
    bool apiAddEntityToGroup(const std::string& entityName, const std::string& groupName);
    
    /**
     * @brief Remove entity from a group
     * @param entityName Name of entity
     * @param groupName Name of group to remove from
     * @return True if successful
     */
    bool apiRemoveEntityFromGroup(const std::string& entityName, const std::string& groupName);
    
    /**
     * @brief Get all entity names in a group
     * @param groupName Name of group
     * @return Vector of entity names in the group
     */
    std::vector<std::string> apiGetGroupMembers(const std::string& groupName);
    
    // === TAG OPERATIONS ===
    
    /**
     * @brief Add tag to entity
     * @param entityName Name of entity
     * @param tag Tag to add (e.g., "interactive", "collectible")
     * @return True if successful
     */
    bool apiAddEntityTag(const std::string& entityName, const std::string& tag);
    
    /**
     * @brief Remove tag from entity
     * @param entityName Name of entity
     * @param tag Tag to remove
     * @return True if successful
     */
    bool apiRemoveEntityTag(const std::string& entityName, const std::string& tag);
    
    /**
     * @brief Check if entity has tag
     * @param entityName Name of entity
     * @param tag Tag to check
     * @return True if entity has the tag
     */
    bool apiEntityHasTag(const std::string& entityName, const std::string& tag);
    
    /**
     * @brief Get all entities with a specific tag
     * @param tag Tag to search for
     * @return Vector of entity names with the tag
     */
    std::vector<std::string> apiGetEntitiesWithTag(const std::string& tag);
    
    // === METADATA OPERATIONS ===
    
    /**
     * @brief Set entity metadata
     * @param entityName Name of entity
     * @param metadata Metadata string (JSON-like)
     * @return True if successful
     */
    bool apiSetEntityMetadata(const std::string& entityName, const std::string& metadata);
    
    /**
     * @brief Get entity metadata
     * @param entityName Name of entity
     * @return Metadata string
     */
    std::string apiGetEntityMetadata(const std::string& entityName);
    
    /**
     * @brief Set entity priority
     * @param entityName Name of entity
     * @param priority Priority value (higher = more important)
     * @return True if successful
     */
    bool apiSetEntityPriority(const std::string& entityName, float priority);
    
    /**
     * @brief Get entity priority
     * @param entityName Name of entity
     * @return Priority value
     */
    float apiGetEntityPriority(const std::string& entityName);
    
    // === BATCH OPERATIONS ===
    
    /**
     * @brief Execute batch state changes
     * @param entityNames Vector of entity names
     * @param newState State to set for all entities (0-4)
     * @return Number of successful operations
     */
    uint32_t apiSetBatchState(const std::vector<std::string>& entityNames, int32_t newState);
    
    /**
     * @brief Enable multiple entities by name
     * @param entityNames Vector of entity names to enable
     * @return Number of entities successfully enabled
     */
    uint32_t apiEnableBatch(const std::vector<std::string>& entityNames);
    
    /**
     * @brief Disable multiple entities by name
     * @param entityNames Vector of entity names to disable
     * @return Number of entities successfully disabled
     */
    uint32_t apiDisableBatch(const std::vector<std::string>& entityNames);

protected:
    /**
     * @brief Get current panel ID for security scoping
     * @return Panel ID of the currently executing script
     */
    uint16_t getCurrentPanelId();
    
    /**
     * @brief Validate entity access for current script context
     * @param entityName Name of entity being accessed
     * @param operation Operation being performed
     * @return True if access is allowed
     */
    bool validateEntityAccess(const std::string& entityName, const std::string& operation);
    
    /**
     * @brief Convert named entity registry state to integer
     * @param state Entity state enum
     * @return Integer representation of state
     */
    int32_t stateToInt(NamedEntityRegistry::EntityState state);
    
    /**
     * @brief Convert integer to named entity registry state
     * @param state Integer state value
     * @return Entity state enum
     */
    NamedEntityRegistry::EntityState intToState(int32_t state);
};

/**
 * @brief WASH Script API Extensions for Named Entities
 * 
 * These functions would be compiled into WASH bytecode from ASH scripts.
 */
namespace NamedEntityWASHAPI {
    
    // Direct entity control
    inline bool enable(const char* entityName) {
        // Compiles to: OP_API_ENABLE_ENTITY
    }
    
    inline bool disable(const char* entityName) {
        // Compiles to: OP_API_DISABLE_ENTITY
    }
    
    inline bool hide(const char* entityName) {
        // Compiles to: OP_API_HIDE_ENTITY
    }
    
    inline bool show(const char* entityName) {
        // Compiles to: OP_API_SHOW_ENTITY
    }
    
    inline bool exists(const char* entityName) {
        // Compiles to: OP_API_ENTITY_EXISTS
    }
    
    inline int getState(const char* entityName) {
        // Compiles to: OP_API_GET_ENTITY_STATE
    }
    
    // Group operations
    inline uint32_t enableGroup(const char* groupName) {
        // Compiles to: OP_API_ENABLE_GROUP
    }
    
    inline uint32_t disableGroup(const char* groupName) {
        // Compiles to: OP_API_DISABLE_GROUP
    }
    
    // Tag operations
    inline bool addTag(const char* entityName, const char* tag) {
        // Compiles to: OP_API_ADD_ENTITY_TAG
    }
    
    inline bool hasTag(const char* entityName, const char* tag) {
        // Compiles to: OP_API_ENTITY_HAS_TAG
    }
}

/**
 * @brief Example Game-Fi Mechanics Using Named Entities
 */
/*

// Panel script for game mechanics
panel_script "game_controller" {
    
    function onPlayerLevelUp() {
        // Enable new content based on level
        enable("mythic_tree");
        enable("boss_gate");
        
        // Show achievement effects
        enableGroup("level_up_effects");
        
        // Add collectible tag to new items
        addTag("rare_sword", "collectible");
        addTag("magic_potion", "collectible");
        
        playSound("level_up");
    }
    
    function onSeasonChange() {
        // Swap seasonal assets
        disable("summer_trees");
        enable("autumn_trees");
        
        // Update environmental groups
        disableGroup("summer_effects");
        enableGroup("autumn_effects");
        
        // Set priority for seasonal rendering
        setPriority("autumn_leaves", 10.0);
        setPriority("falling_snow", 5.0);
    }
    
    function onQuestComplete(questId) {
        if (questId == "dragon_slayer") {
            // Unlock dragon areas
            enableGroup("dragon_zones");
            
            // Hide quest markers
            disableGroup("dragon_quest_markers");
            
            // Enable reward items
            enable("dragon_sword");
            enable("dragon_armor");
            
            // Add legendary tags
            addTag("dragon_sword", "legendary");
            addTag("dragon_armor", "legendary");
        }
    }
    
    function onInventoryFull() {
        // Hide all collectibles to prevent confusion
        var collectibles = getEntitiesWithTag("collectible");
        for (var item : collectibles) {
            if (getState(item) == STATE_ACTIVE) {
                hide(item);
                addTag(item, "hidden_full_inventory");
            }
        }
    }
    
    function onInventorySpaceAvailable() {
        // Show previously hidden collectibles
        var hiddenItems = getEntitiesWithTag("hidden_full_inventory");
        for (var item : hiddenItems) {
            show(item);
            removeTag(item, "hidden_full_inventory");
        }
    }
    
    function onBossEncounter() {
        // Disable all non-essential entities for performance
        disableGroup("background_decorations");
        disableGroup("particle_effects");
        
        // Enable boss-specific elements
        enableGroup("boss_ui_elements");
        enableGroup("boss_music_triggers");
        
        // Set metadata for boss fight tracking
        setMetadata("boss_dragon", "{\"health\": 1000, \"phase\": 1}");
    }
    
    function onEnvironmentalHazard() {
        // Example: Fire spreads to nearby flammable objects
        var flammableObjects = getEntitiesWithTag("flammable");
        var firePositions = getGroupMembers("active_fires");
        
        for (var fire : firePositions) {
            var firePos = getPosition(fire);
            
            for (var obj : flammableObjects) {
                var objPos = getPosition(obj);
                var distance = length(firePos - objPos);
                
                if (distance < 20.0 && getState(obj) == STATE_ACTIVE) {
                    // Object catches fire
                    setAnimation(obj, "burning");
                    addToGroup(obj, "active_fires");
                    addTag(obj, "burning");
                    
                    playSound("fire_ignite");
                }
            }
        }
    }
}

// Entity script for interactive objects
entity_script "mythic_tree" {
    function onCollision(otherUUID) {
        if (getEntityType(otherUUID) == "player") {
            // Tree was touched by player
            setAnimation(this, "glow");
            
            // Grant mystical buff
            var players = getEntitiesWithTag("player");
            for (var player : players) {
                addTag(player, "mystical_blessing");
                setMetadata(player, "{\"buff_duration\": 300}"); // 5 minutes
            }
            
            // Tree becomes inactive after use
            setState(this, STATE_INACTIVE);
            addTag(this, "used");
            
            playSound("mystical_chime");
        }
    }
    
    function onUpdate() {
        // Check if tree should regenerate
        if (getState(this) == STATE_INACTIVE && hasTag(this, "used")) {
            var metadata = getMetadata(this);
            // Parse JSON to check regeneration timer
            // ... timer logic ...
            
            if (/* timer expired */) {
                setState(this, STATE_ACTIVE);
                removeTag(this, "used");
                setAnimation(this, "idle");
            }
        }
    }
}

*/
