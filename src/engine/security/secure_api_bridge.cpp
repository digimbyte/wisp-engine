// secure_api_bridge.cpp - Secure WASH API Bridge Implementation
// Core implementation of secure API bridge with UUID authority validation

#include "secure_api_bridge.h"
#include <cmath>

namespace WispEngine {
namespace Security {

// === CONSTRUCTOR / DESTRUCTOR ===

SecureWASHAPIBridge::SecureWASHAPIBridge()
    : curatedAPI(nullptr), uuidAuthority(nullptr), totalAPICalls(0),
      totalSecurityViolations(0), totalExecutionTime(0) {
}

SecureWASHAPIBridge::~SecureWASHAPIBridge() {
    shutdown();
}

// === INITIALIZATION ===

bool SecureWASHAPIBridge::initialize(WispCuratedAPIExtended* api, EngineUUIDAuthority* authority) {
    if (!api) {
        WISP_DEBUG_ERROR("SEC_BRIDGE", "Curated API is null");
        return false;
    }
    
    if (!authority) {
        WISP_DEBUG_ERROR("SEC_BRIDGE", "UUID Authority is null");
        return false;
    }
    
    curatedAPI = api;
    uuidAuthority = authority;
    
    WISP_DEBUG_INFO("SEC_BRIDGE", "Secure WASH API Bridge initialized");
    return true;
}

void SecureWASHAPIBridge::shutdown() {
    clearExecutionContext();
    curatedAPI = nullptr;
    uuidAuthority = nullptr;
    
    WISP_DEBUG_INFO("SEC_BRIDGE", "Secure WASH API Bridge shutdown");
}

// === EXECUTION CONTEXT MANAGEMENT ===

bool SecureWASHAPIBridge::setExecutionContext(const String& scriptName, const String& scriptType,
                                             uint32_t contextUUID, uint16_t contextPanelId) {
    if (scriptName.empty()) {
        WISP_DEBUG_ERROR("SEC_BRIDGE", "Script name cannot be empty");
        return false;
    }
    
    // Clear any existing context
    clearExecutionContext();
    
    // Set new context
    currentContext.scriptName = scriptName;
    currentContext.scriptType = scriptType;
    currentContext.contextUUID = contextUUID;
    currentContext.contextPanelId = contextPanelId;
    currentContext.executionStartTime = get_millis();
    currentContext.apiCallCount = 0;
    currentContext.securityViolations = 0;
    
    WISP_DEBUG_INFO("SEC_BRIDGE", String("Set execution context: " + scriptName + 
                    " (" + scriptType + ")").c_str());
    
    return true;
}

void SecureWASHAPIBridge::clearExecutionContext() {
    if (currentContext.isValid()) {
        uint32_t executionTime = get_millis() - currentContext.executionStartTime;
        updatePerformanceMetrics(executionTime);
        
        WISP_DEBUG_INFO("SEC_BRIDGE", String("Cleared context: " + currentContext.scriptName +
                        " (" + String(executionTime) + "ms, " + 
                        String(currentContext.apiCallCount) + " calls)").c_str());
    }
    
    currentContext.reset();
}

// === SECURE API IMPLEMENTATIONS ===

// --- Entity Position/Movement Operations ---

bool SecureWASHAPIBridge::apiMoveEntity(uint32_t uuid, float deltaX, float deltaY) {
    if (!validateOperation("moveEntity")) return false;
    if (!validateUUIDAccess(uuid, "move")) return false;
    if (!validateVelocity(deltaX, deltaY)) return false;
    
    recordAPICall("moveEntity");
    
    // Get scene entity through UUID authority
    uint16_t entityId = uuidAuthority->getEngineEntityId(uuid);
    if (entityId == 0) return false;
    
    // Get current position and apply delta
    WispVec2 currentPos = curatedAPI->getEntityPosition(entityId);
    WispVec2 newPos(currentPos.x + deltaX, currentPos.y + deltaY);
    
    if (!validatePosition(newPos.x, newPos.y)) return false;
    
    return curatedAPI->setEntityPosition(entityId, newPos);
}

bool SecureWASHAPIBridge::apiSetPosition(uint32_t uuid, float x, float y) {
    if (!validateOperation("setPosition")) return false;
    if (!validateUUIDAccess(uuid, "setPosition")) return false;
    if (!validatePosition(x, y)) return false;
    
    recordAPICall("setPosition");
    
    uint16_t entityId = uuidAuthority->getEngineEntityId(uuid);
    if (entityId == 0) return false;
    
    return curatedAPI->setEntityPosition(entityId, WispVec2(x, y));
}

WispVec2 SecureWASHAPIBridge::apiGetPosition(uint32_t uuid) {
    if (!validateOperation("getPosition")) return WispVec2();
    if (!validateUUIDAccess(uuid, "getPosition")) return WispVec2();
    
    recordAPICall("getPosition");
    
    uint16_t entityId = uuidAuthority->getEngineEntityId(uuid);
    if (entityId == 0) return WispVec2();
    
    return curatedAPI->getEntityPosition(entityId);
}

bool SecureWASHAPIBridge::apiSetVelocity(uint32_t uuid, float vx, float vy) {
    if (!validateOperation("setVelocity")) return false;
    if (!validateUUIDAccess(uuid, "setVelocity")) return false;
    if (!validateVelocity(vx, vy)) return false;
    
    recordAPICall("setVelocity");
    
    uint16_t entityId = uuidAuthority->getEngineEntityId(uuid);
    if (entityId == 0) return false;
    
    return curatedAPI->setEntityVelocity(entityId, WispVec2(vx, vy));
}

// --- Entity Lifecycle Operations ---

uint32_t SecureWASHAPIBridge::apiSpawnEntity(const String& entityType, float x, float y, 
                                           const String& scriptName) {
    if (!validateOperation("spawnEntity")) return 0;
    if (!validateEntityType(entityType)) return 0;
    if (!validatePosition(x, y)) return 0;
    if (!scriptName.empty() && !validateScriptName(scriptName)) return 0;
    
    recordAPICall("spawnEntity");
    
    // Get current panel for security scoping
    uint16_t panelId = getCurrentPanelId();
    
    // ENGINE CREATES UUID - scripts cannot specify UUIDs
    uint32_t uuid = uuidAuthority->createEntityUUID(entityType, panelId, scriptName);
    if (uuid == 0) {
        recordSecurityViolation("Failed to create UUID for spawn");
        return 0;
    }
    
    // Create actual entity through curated API
    EntityHandle entityHandle = curatedAPI->createEntity();
    if (entityHandle == INVALID_ENTITY) {
        uuidAuthority->unregisterEntity(uuid);
        return 0;
    }
    
    // Register with UUID authority
    if (!uuidAuthority->registerEntity(uuid, entityHandle)) {
        curatedAPI->destroyEntity(entityHandle);
        uuidAuthority->unregisterEntity(uuid);
        return 0;
    }
    
    // Set initial position
    curatedAPI->setEntityPosition(entityHandle, WispVec2(x, y));
    
    // Attach script if specified
    if (!scriptName.empty()) {
        curatedAPI->bindEntityScript(entityHandle, scriptName);
    }
    
    WISP_DEBUG_INFO("SEC_BRIDGE", String("Spawned entity UUID " + String(uuid) + 
                    " of type '" + entityType + "'").c_str());
    
    return uuid;
}

bool SecureWASHAPIBridge::apiDestroyEntity(uint32_t uuid) {
    if (!validateOperation("destroyEntity")) return false;
    if (!validateUUIDAccess(uuid, "destroy")) return false;
    
    recordAPICall("destroyEntity");
    
    // Mark for destruction through UUID authority (secure cleanup)
    uuidAuthority->markForDestruction(uuid, currentContext.scriptName);
    
    return true;
}

// --- Entity Query Operations ---

std::vector<uint32_t> SecureWASHAPIBridge::apiFindEntitiesByType(const String& type) {
    if (!validateOperation("findEntitiesByType")) return {};
    if (!validateEntityType(type)) return {};
    
    recordAPICall("findEntitiesByType");
    
    // Only search within current panel for security
    uint16_t panelId = getCurrentPanelId();
    return uuidAuthority->findEntitiesByType(type, panelId);
}

std::vector<uint32_t> SecureWASHAPIBridge::apiFindEntitiesInRadius(float x, float y, float radius) {
    if (!validateOperation("findEntitiesInRadius")) return {};
    if (!validatePosition(x, y)) return {};
    if (!validateRadius(radius)) return {};
    
    recordAPICall("findEntitiesInRadius");
    
    // Only search within current panel for security
    uint16_t panelId = getCurrentPanelId();
    return uuidAuthority->findEntitiesInRadius(x, y, radius, panelId);
}

String SecureWASHAPIBridge::apiGetEntityType(uint32_t uuid) {
    if (!validateOperation("getEntityType")) return "";
    if (!validateUUIDAccess(uuid, "getEntityType")) return "";
    
    recordAPICall("getEntityType");
    
    return uuidAuthority->getEntityType(uuid);
}

uint16_t SecureWASHAPIBridge::apiGetCurrentPanel() {
    if (!validateOperation("getCurrentPanel")) return 0;
    
    recordAPICall("getCurrentPanel");
    
    return getCurrentPanelId();
}

// --- Audio Operations ---

bool SecureWASHAPIBridge::apiPlaySound(const String& soundName, float volume) {
    if (!validateOperation("playSound")) return false;
    if (!validateSoundName(soundName)) return false;
    if (!validateVolume(volume)) return false;
    
    recordAPICall("playSound");
    
    ResourceHandle audioHandle = curatedAPI->loadAudio(soundName);
    if (audioHandle == INVALID_RESOURCE) return false;
    
    WispAudioParams params;
    params.volume = volume;
    
    return curatedAPI->playAudio(audioHandle, params);
}

// === MATH FUNCTIONS ===

float SecureWASHAPIBridge::mathSqrt(float x) {
    if (!checkParameterSafety(x, 0.0f, 1000000.0f)) return 0.0f;
    recordAPICall("mathSqrt");
    return std::sqrt(x);
}

float SecureWASHAPIBridge::mathSin(float x) {
    if (!checkParameterSafety(x)) return 0.0f;
    recordAPICall("mathSin");
    return std::sin(x);
}

float SecureWASHAPIBridge::mathClamp(float value, float min, float max) {
    if (!checkParameterSafety(value)) return 0.0f;
    if (!checkParameterSafety(min)) return 0.0f;
    if (!checkParameterSafety(max)) return 0.0f;
    recordAPICall("mathClamp");
    
    if (min > max) {
        float temp = min;
        min = max;
        max = temp;
    }
    
    return (value < min) ? min : (value > max) ? max : value;
}

float SecureWASHAPIBridge::mathRandom() {
    recordAPICall("mathRandom");
    return static_cast<float>(rand()) / RAND_MAX;
}

// === VALIDATION FUNCTIONS ===

bool SecureWASHAPIBridge::validateOperation(const String& operation, const String& details) {
    if (!validateContextExists()) {
        recordSecurityViolation("No execution context for " + operation);
        return false;
    }
    
    if (!checkAPICallLimit()) {
        recordSecurityViolation("API call limit exceeded for " + operation);
        return false;
    }
    
    if (!checkExecutionTime()) {
        recordSecurityViolation("Execution time limit exceeded for " + operation);
        return false;
    }
    
    return true;
}

bool SecureWASHAPIBridge::validateUUIDAccess(uint32_t uuid, const String& operation) {
    if (!uuidAuthority->validateUUID(uuid)) {
        recordSecurityViolation("Invalid UUID " + String(uuid) + " for " + operation);
        return false;
    }
    
    if (!uuidAuthority->authorizeScriptOperation(uuid, currentContext.scriptName, operation)) {
        recordSecurityViolation("Unauthorized " + operation + " on UUID " + String(uuid));
        return false;
    }
    
    return true;
}

bool SecureWASHAPIBridge::checkAPICallLimit() {
    return currentContext.apiCallCount < MAX_API_CALLS_PER_FRAME;
}

bool SecureWASHAPIBridge::checkExecutionTime() {
    uint32_t elapsed = get_millis() - currentContext.executionStartTime;
    return elapsed < MAX_EXECUTION_TIME_MS;
}

bool SecureWASHAPIBridge::checkParameterSafety(float value, float min, float max) {
    if (std::isnan(value) || std::isinf(value)) return false;
    return (value >= min && value <= max);
}

// === VALIDATION HELPERS ===

bool SecureWASHAPIBridge::validateContextExists() const {
    return currentContext.isValid();
}

bool SecureWASHAPIBridge::validatePosition(float x, float y) const {
    const float MAX_POSITION = 10000.0f;
    return checkParameterSafety(x, -MAX_POSITION, MAX_POSITION) &&
           checkParameterSafety(y, -MAX_POSITION, MAX_POSITION);
}

bool SecureWASHAPIBridge::validateVelocity(float vx, float vy) const {
    const float MAX_VELOCITY = 1000.0f;
    return checkParameterSafety(vx, -MAX_VELOCITY, MAX_VELOCITY) &&
           checkParameterSafety(vy, -MAX_VELOCITY, MAX_VELOCITY);
}

bool SecureWASHAPIBridge::validateRadius(float radius) const {
    return checkParameterSafety(radius, 0.0f, MAX_SEARCH_RADIUS);
}

bool SecureWASHAPIBridge::validateVolume(float volume) const {
    return checkParameterSafety(volume, 0.0f, 1.0f);
}

bool SecureWASHAPIBridge::validateEntityType(const String& type) const {
    if (type.empty() || type.length() > 32) return false;
    
    // Check for valid characters (alphanumeric + underscore)
    for (size_t i = 0; i < type.length(); i++) {
        char c = type[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
              (c >= '0' && c <= '9') || c == '_')) {
            return false;
        }
    }
    
    return true;
}

bool SecureWASHAPIBridge::validateSoundName(const String& soundName) const {
    return !soundName.empty() && soundName.length() <= 64;
}

// === UTILITY HELPERS ===

uint16_t SecureWASHAPIBridge::getCurrentPanelId() const {
    // For entity scripts, get panel from UUID authority
    if (currentContext.scriptType == "entity" && currentContext.contextUUID != 0) {
        return uuidAuthority->getEntityPanelId(currentContext.contextUUID);
    }
    
    // For panel/global scripts, use context panel ID
    return currentContext.contextPanelId;
}

// === INTERNAL TRACKING ===

void SecureWASHAPIBridge::recordAPICall(const String& operation) {
    currentContext.apiCallCount++;
    totalAPICalls++;
    
    // Optional: Log high-frequency operations for debugging
    static uint32_t lastLogTime = 0;
    uint32_t currentTime = get_millis();
    if (currentTime - lastLogTime > 5000) { // Log every 5 seconds
        WISP_DEBUG_INFO("SEC_BRIDGE", String("API Stats: " + String(totalAPICalls) + 
                        " total calls, " + String(totalSecurityViolations) + 
                        " violations").c_str());
        lastLogTime = currentTime;
    }
}

void SecureWASHAPIBridge::recordSecurityViolation(const String& violation) {
    currentContext.securityViolations++;
    totalSecurityViolations++;
    
    WISP_DEBUG_WARNING("SEC_VIOLATION", String("Script '" + currentContext.scriptName + 
                       "': " + violation).c_str());
}

void SecureWASHAPIBridge::updatePerformanceMetrics(uint32_t executionTime) {
    totalExecutionTime += executionTime;
}

// === DEBUGGING ===

void SecureWASHAPIBridge::dumpSecurityStats() const {
    WISP_DEBUG_INFO("SEC_BRIDGE", "=== Security Statistics ===");
    WISP_DEBUG_INFO("SEC_BRIDGE", String("Total API calls: " + String(totalAPICalls)).c_str());
    WISP_DEBUG_INFO("SEC_BRIDGE", String("Security violations: " + String(totalSecurityViolations)).c_str());
    WISP_DEBUG_INFO("SEC_BRIDGE", String("Total execution time: " + String(totalExecutionTime) + "ms").c_str());
}

void SecureWASHAPIBridge::dumpExecutionContext() const {
    if (!currentContext.isValid()) {
        WISP_DEBUG_INFO("SEC_BRIDGE", "No active execution context");
        return;
    }
    
    WISP_DEBUG_INFO("SEC_BRIDGE", "=== Current Execution Context ===");
    WISP_DEBUG_INFO("SEC_BRIDGE", String("Script: " + currentContext.scriptName).c_str());
    WISP_DEBUG_INFO("SEC_BRIDGE", String("Type: " + currentContext.scriptType).c_str());
    WISP_DEBUG_INFO("SEC_BRIDGE", String("UUID: " + String(currentContext.contextUUID)).c_str());
    WISP_DEBUG_INFO("SEC_BRIDGE", String("Panel: " + String(currentContext.contextPanelId)).c_str());
    WISP_DEBUG_INFO("SEC_BRIDGE", String("API calls: " + String(currentContext.apiCallCount)).c_str());
    WISP_DEBUG_INFO("SEC_BRIDGE", String("Violations: " + String(currentContext.securityViolations)).c_str());
}

} // namespace Security
} // namespace WispEngine
