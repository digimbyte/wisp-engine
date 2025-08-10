// uuid_authority.cpp - Engine UUID Authority System Implementation
// ESP-IDF compatible implementation with zero-trust security model

#include "uuid_authority.h"
#include <algorithm>

namespace WispEngine {
namespace Security {

// Static member initialization
uint32_t EngineUUIDAuthority::nextUUID = 1000; // Start from 1000 to avoid low number conflicts
bool EngineUUIDAuthority::initialized = false;

// Global instance
static EngineUUIDAuthority* g_uuidAuthority = nullptr;

// === CONSTRUCTOR / DESTRUCTOR ===

EngineUUIDAuthority::EngineUUIDAuthority() 
    : totalEntitiesCreated(0), totalValidationCalls(0), totalSecurityViolations(0),
      sceneManager(nullptr) {
    // Reserve initial capacity for performance
    entityRegistry.reserve(128);
    panelEntities.reserve(16);
    typeEntities.reserve(32);
    pendingDestruction.reserve(16);
}

EngineUUIDAuthority::~EngineUUIDAuthority() {
    shutdown();
}

// === INITIALIZATION ===

bool EngineUUIDAuthority::initialize(SceneManager* sceneMgr) {
    if (initialized) {
        WISP_DEBUG_WARNING("UUID_AUTH", "Already initialized");
        return true;
    }
    
    if (!sceneMgr) {
        WISP_DEBUG_ERROR("UUID_AUTH", "Scene manager is null");
        return false;
    }
    
    sceneManager = sceneMgr;
    initialized = true;
    
    WISP_DEBUG_INFO("UUID_AUTH", "Engine UUID Authority initialized");
    return true;
}

void EngineUUIDAuthority::shutdown() {
    if (!initialized) return;
    
    // Clean up all entities
    clearAll();
    
    sceneManager = nullptr;
    initialized = false;
    
    WISP_DEBUG_INFO("UUID_AUTH", "Engine UUID Authority shutdown");
}

EngineUUIDAuthority& EngineUUIDAuthority::getInstance() {
    if (!g_uuidAuthority) {
        g_uuidAuthority = new EngineUUIDAuthority();
    }
    return *g_uuidAuthority;
}

// === UUID CREATION (ENGINE AUTHORITY ONLY) ===

uint32_t EngineUUIDAuthority::createEntityUUID(const String& entityType, uint16_t panelId, 
                                              const String& scriptName) {
    if (!initialized) {
        WISP_DEBUG_ERROR("UUID_AUTH", "Not initialized");
        return 0;
    }
    
    // Generate new UUID
    uint32_t uuid = generateNextUUID();
    if (uuid == 0) {
        WISP_DEBUG_ERROR("UUID_AUTH", "Failed to generate UUID");
        return 0;
    }
    
    // Create entity authority record
    EntityAuthority authority(uuid, 0, panelId, entityType, scriptName);
    
    // Add to registry
    entityRegistry[uuid] = authority;
    
    // Add to panel index
    panelEntities[panelId].insert(uuid);
    
    // Add to type index
    typeEntities[entityType].insert(uuid);
    
    totalEntitiesCreated++;
    
    WISP_DEBUG_INFO("UUID_AUTH", String("Created UUID " + String(uuid) + 
                    " for type '" + entityType + "' in panel " + String(panelId)).c_str());
    
    return uuid;
}

uint32_t EngineUUIDAuthority::generateNextUUID() {
    const uint32_t MAX_ATTEMPTS = 1000;
    
    for (uint32_t attempts = 0; attempts < MAX_ATTEMPTS; attempts++) {
        uint32_t candidate = nextUUID++;
        
        // Skip reserved values
        if (candidate == 0 || candidate == UINT32_MAX) {
            continue;
        }
        
        // Check for collision
        if (!isUUIDCollision(candidate)) {
            return candidate;
        }
    }
    
    WISP_DEBUG_ERROR("UUID_AUTH", "UUID generation failed - too many collisions");
    return 0;
}

bool EngineUUIDAuthority::isUUIDCollision(uint32_t uuid) const {
    return entityRegistry.find(uuid) != entityRegistry.end();
}

// === VALIDATION ===

bool EngineUUIDAuthority::validateUUID(uint32_t uuid) const {
    totalValidationCalls++; // Mutable despite const (for statistics)
    
    if (uuid == 0) return false;
    
    return entityRegistry.find(uuid) != entityRegistry.end();
}

bool EngineUUIDAuthority::isValidForOperation(uint32_t uuid, EntityPermission operation) const {
    auto it = entityRegistry.find(uuid);
    if (it == entityRegistry.end()) {
        return false;
    }
    
    const EntityAuthority& auth = it->second;
    
    // Check if entity is pending destruction
    if (auth.pendingDestruction) {
        return false;
    }
    
    // Check permission mask
    return (auth.permissionMask & operation) != 0;
}

// === SCRIPT AUTHORIZATION ===

bool EngineUUIDAuthority::authorizeScriptOperation(uint32_t uuid, const String& scriptName, 
                                                  const String& operation) const {
    auto it = entityRegistry.find(uuid);
    if (it == entityRegistry.end()) {
        recordSecurityViolation(uuid, operation, scriptName);
        return false;
    }
    
    const EntityAuthority& auth = it->second;
    
    // Check if script control is allowed
    if (!auth.allowScriptControl) {
        recordSecurityViolation(uuid, operation, scriptName);
        return false;
    }
    
    // Check if entity is pending destruction
    if (auth.pendingDestruction) {
        recordSecurityViolation(uuid, operation, scriptName);
        return false;
    }
    
    // If entity has a controlling script, only that script can control it
    if (!auth.scriptName.empty() && auth.scriptName != scriptName) {
        recordSecurityViolation(uuid, operation, scriptName);
        return false;
    }
    
    // Update access tracking
    updateAccessTracking(uuid);
    
    return true;
}

void EngineUUIDAuthority::recordSecurityViolation(uint32_t uuid, const String& operation, 
                                                 const String& scriptName) const {
    totalSecurityViolations++;
    
    WISP_DEBUG_WARNING("SEC_VIOLATION", 
        String("Script '" + scriptName + "' denied " + operation + 
               " on UUID " + String(uuid)).c_str());
}

void EngineUUIDAuthority::updateAccessTracking(uint32_t uuid) const {
    auto it = entityRegistry.find(uuid);
    if (it != entityRegistry.end()) {
        // Const cast for access tracking (statistics only)
        EntityAuthority& auth = const_cast<EntityAuthority&>(it->second);
        auth.lastAccessTime = get_millis();
        auth.accessCount++;
    }
}

// === ENTITY LIFECYCLE MANAGEMENT ===

bool EngineUUIDAuthority::registerEntity(uint32_t uuid, uint16_t sceneEntityId) {
    auto it = entityRegistry.find(uuid);
    if (it == entityRegistry.end()) {
        WISP_DEBUG_ERROR("UUID_AUTH", String("Cannot register unknown UUID " + String(uuid)).c_str());
        return false;
    }
    
    // Update the authority record with scene entity ID
    it->second.engineEntityId = sceneEntityId;
    
    WISP_DEBUG_INFO("UUID_AUTH", String("Registered UUID " + String(uuid) + 
                    " with scene entity " + String(sceneEntityId)).c_str());
    
    return true;
}

void EngineUUIDAuthority::markForDestruction(uint32_t uuid, const String& requestingScript) {
    auto it = entityRegistry.find(uuid);
    if (it == entityRegistry.end()) {
        WISP_DEBUG_WARNING("UUID_AUTH", String("Cannot mark unknown UUID " + String(uuid) + 
                          " for destruction").c_str());
        return;
    }
    
    // Authorize the destruction request
    if (!requestingScript.empty() && 
        !authorizeScriptOperation(uuid, requestingScript, "destroy")) {
        return;
    }
    
    it->second.pendingDestruction = true;
    pendingDestruction.insert(uuid);
    
    WISP_DEBUG_INFO("UUID_AUTH", String("Marked UUID " + String(uuid) + 
                    " for destruction by '" + requestingScript + "'").c_str());
}

void EngineUUIDAuthority::cleanupPendingEntities() {
    if (pendingDestruction.empty()) return;
    
    std::vector<uint32_t> toRemove;
    toRemove.reserve(pendingDestruction.size());
    
    for (uint32_t uuid : pendingDestruction) {
        auto it = entityRegistry.find(uuid);
        if (it != entityRegistry.end()) {
            const EntityAuthority& auth = it->second;
            
            // Remove from scene system if registered
            if (sceneManager && auth.engineEntityId != 0) {
                sceneManager->removeEntity(auth.engineEntityId);
            }
            
            // Remove from indices
            removeFromPanelIndex(uuid, auth.panelId);
            removeFromTypeIndex(uuid, auth.entityType);
            
            // Remove from registry
            entityRegistry.erase(it);
            
            toRemove.push_back(uuid);
        }
    }
    
    // Clear pending destruction set
    for (uint32_t uuid : toRemove) {
        pendingDestruction.erase(uuid);
    }
    
    if (!toRemove.empty()) {
        WISP_DEBUG_INFO("UUID_AUTH", String("Cleaned up " + String(toRemove.size()) + 
                        " pending entities").c_str());
    }
}

void EngineUUIDAuthority::unregisterEntity(uint32_t uuid) {
    auto it = entityRegistry.find(uuid);
    if (it == entityRegistry.end()) {
        return; // Already removed
    }
    
    const EntityAuthority& auth = it->second;
    
    // Remove from indices
    removeFromPanelIndex(uuid, auth.panelId);
    removeFromTypeIndex(uuid, auth.entityType);
    
    // Remove from pending destruction
    pendingDestruction.erase(uuid);
    
    // Remove from registry
    entityRegistry.erase(it);
    
    WISP_DEBUG_INFO("UUID_AUTH", String("Unregistered UUID " + String(uuid)).c_str());
}

// === SECURE ENTITY QUERIES ===

std::vector<uint32_t> EngineUUIDAuthority::findEntitiesByType(const String& type, 
                                                             uint16_t panelId) const {
    std::vector<uint32_t> result;
    
    // Find entities of the specified type
    auto typeIt = typeEntities.find(type);
    if (typeIt == typeEntities.end()) {
        return result; // No entities of this type
    }
    
    // Filter by panel
    auto panelIt = panelEntities.find(panelId);
    if (panelIt == panelEntities.end()) {
        return result; // No entities in this panel
    }
    
    const auto& typeSet = typeIt->second;
    const auto& panelSet = panelIt->second;
    
    // Find intersection of type and panel sets
    for (uint32_t uuid : typeSet) {
        if (panelSet.find(uuid) != panelSet.end()) {
            auto entityIt = entityRegistry.find(uuid);
            if (entityIt != entityRegistry.end() && !entityIt->second.pendingDestruction) {
                result.push_back(uuid);
            }
        }
    }
    
    return result;
}

std::vector<uint32_t> EngineUUIDAuthority::findEntitiesInRadius(float centerX, float centerY,
                                                               float radius, uint16_t panelId) const {
    std::vector<uint32_t> result;
    
    // Clamp radius for security
    const float MAX_SEARCH_RADIUS = 1024.0f;
    if (radius > MAX_SEARCH_RADIUS) {
        radius = MAX_SEARCH_RADIUS;
    }
    
    auto panelIt = panelEntities.find(panelId);
    if (panelIt == panelEntities.end()) {
        return result;
    }
    
    const float radiusSquared = radius * radius;
    
    for (uint32_t uuid : panelIt->second) {
        auto entityIt = entityRegistry.find(uuid);
        if (entityIt == entityRegistry.end() || entityIt->second.pendingDestruction) {
            continue;
        }
        
        // Get entity position from scene system
        if (sceneManager) {
            SceneEntity* sceneEntity = sceneManager->findEntity(entityIt->second.engineEntityId);
            if (sceneEntity) {
                float dx = sceneEntity->worldX - centerX;
                float dy = sceneEntity->worldY - centerY;
                float distanceSquared = dx * dx + dy * dy;
                
                if (distanceSquared <= radiusSquared) {
                    result.push_back(uuid);
                }
            }
        }
    }
    
    return result;
}

// === GETTERS ===

String EngineUUIDAuthority::getEntityType(uint32_t uuid) const {
    auto it = entityRegistry.find(uuid);
    return (it != entityRegistry.end()) ? it->second.entityType : String("");
}

uint16_t EngineUUIDAuthority::getEntityPanelId(uint32_t uuid) const {
    auto it = entityRegistry.find(uuid);
    return (it != entityRegistry.end()) ? it->second.panelId : 0;
}

uint16_t EngineUUIDAuthority::getEngineEntityId(uint32_t uuid) const {
    auto it = entityRegistry.find(uuid);
    return (it != entityRegistry.end()) ? it->second.engineEntityId : 0;
}

const EntityAuthority* EngineUUIDAuthority::getEntityAuthority(uint32_t uuid) const {
    auto it = entityRegistry.find(uuid);
    return (it != entityRegistry.end()) ? &it->second : nullptr;
}

// === PANEL MANAGEMENT ===

void EngineUUIDAuthority::clearPanel(uint16_t panelId) {
    auto panelIt = panelEntities.find(panelId);
    if (panelIt == panelEntities.end()) {
        return;
    }
    
    std::vector<uint32_t> toRemove(panelIt->second.begin(), panelIt->second.end());
    
    for (uint32_t uuid : toRemove) {
        unregisterEntity(uuid);
    }
    
    panelEntities.erase(panelIt);
    
    WISP_DEBUG_INFO("UUID_AUTH", String("Cleared panel " + String(panelId) + 
                    " - removed " + String(toRemove.size()) + " entities").c_str());
}

// === INTERNAL HELPERS ===

void EngineUUIDAuthority::removeFromPanelIndex(uint32_t uuid, uint16_t panelId) {
    auto panelIt = panelEntities.find(panelId);
    if (panelIt != panelEntities.end()) {
        panelIt->second.erase(uuid);
        if (panelIt->second.empty()) {
            panelEntities.erase(panelIt);
        }
    }
}

void EngineUUIDAuthority::removeFromTypeIndex(uint32_t uuid, const String& type) {
    auto typeIt = typeEntities.find(type);
    if (typeIt != typeEntities.end()) {
        typeIt->second.erase(uuid);
        if (typeIt->second.empty()) {
            typeEntities.erase(typeIt);
        }
    }
}

// === COMPATIBILITY INTERFACE ===

bool EngineUUIDAuthority::registerEntity(uint32_t uuid, uint16_t entityIndex, 
                                        uint16_t panelId, const String& type) {
    // This is called by existing code that expects UUIDTracker interface
    // We need to create the authority record and register it
    
    if (entityRegistry.find(uuid) != entityRegistry.end()) {
        return false; // UUID already exists
    }
    
    EntityAuthority authority(uuid, entityIndex, panelId, type);
    entityRegistry[uuid] = authority;
    panelEntities[panelId].insert(uuid);
    typeEntities[type].insert(uuid);
    
    return true;
}

void EngineUUIDAuthority::clearAll() {
    entityRegistry.clear();
    panelEntities.clear();
    typeEntities.clear();
    pendingDestruction.clear();
    
    totalEntitiesCreated = 0;
    totalValidationCalls = 0;
    totalSecurityViolations = 0;
    
    WISP_DEBUG_INFO("UUID_AUTH", "Cleared all entities");
}

// === DEBUGGING ===

void EngineUUIDAuthority::dumpEntityRegistry() const {
    WISP_DEBUG_INFO("UUID_AUTH", "=== Entity Registry Dump ===");
    WISP_DEBUG_INFO("UUID_AUTH", String("Total entities: " + String(entityRegistry.size())).c_str());
    
    for (const auto& pair : entityRegistry) {
        const EntityAuthority& auth = pair.second;
        WISP_DEBUG_INFO("UUID_AUTH", 
            String("UUID " + String(auth.uuid) + 
                   ": type='" + auth.entityType + 
                   "', panel=" + String(auth.panelId) +
                   ", script='" + auth.scriptName + 
                   "', pending=" + (auth.pendingDestruction ? "YES" : "NO")).c_str());
    }
}

// === GLOBAL ACCESS ===

EngineUUIDAuthority& GetUUIDAuthority() {
    return EngineUUIDAuthority::getInstance();
}

} // namespace Security  
} // namespace WispEngine
