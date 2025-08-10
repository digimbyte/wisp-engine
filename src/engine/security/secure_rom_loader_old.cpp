#include "secure_rom_loader.h"
#include "../app/wisp_resource_quota.h" // For resource management integration
#include "system/esp_task_utils.h"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <set>

#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"  // For heap memory queries
static const char* TAG = "SecureROMLoader";
#define LOG_DEBUG(fmt, ...) ESP_LOGD(TAG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)
#define GET_TIME_MS() (esp_timer_get_time() / 1000)
#else
#include <chrono>
#define LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) printf("[WARN] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#define GET_TIME_MS() std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()
#endif

// =========================
// Constructor / Destructor
// =========================

SecureROMLoader::SecureROMLoader(EngineUUIDAuthority* uuidAuth,
                                ScriptInstanceAuthority* scriptAuth,
                                SecureWASHAPIBridge* bridge,
                                SceneManager* sceneMgr,
                                WispCuratedAPIExtended* api)
    : uuidAuthority(uuidAuth)
    , scriptAuthority(scriptAuth)
    , apiBridge(bridge)
    , sceneManager(sceneMgr)
    , curatedAPI(api)
    , loadingInProgress(false)
{
    LOG_INFO("SecureROMLoader initialized - Phase 5 Security Integration");
    
    // Initialize current app info
    currentApp.validated = false;
    currentApp.securityVersion = SECURITY_VERSION;
    
    // Reset statistics
    resetStats();
    
    LOG_DEBUG("ROM Loader Security Limits: ROM=%dMB, Script=%dKB, MaxScripts=%d, MaxEntities=%d",
              MAX_ROM_SIZE_MB, MAX_SCRIPT_SIZE_KB, MAX_SCRIPTS_PER_ROM, MAX_ENTITIES_PER_ROM);
}

SecureROMLoader::~SecureROMLoader() {
    if (isROMLoaded()) {
        unloadCurrentROM();
    }
    
    cleanupValidationCache();
    LOG_INFO("SecureROMLoader destroyed");
}

// =========================
// ROM Loading Interface
// =========================

bool SecureROMLoader::loadWispROM(const String& romPath, uint16_t targetPanelId) {
    if (loadingInProgress) {
        LOG_WARN("ROM loading already in progress");
        return false;
    }
    
    LOG_INFO("Loading Wisp ROM: %s (target panel: %d)", romPath.c_str(), targetPanelId);
    
    loadingInProgress = true;
    resetStats();
    uint32_t startTime = GET_TIME_MS();
    
    // Phase 1: Load ROM file and basic validation
    WispROMData romData;
    if (!loadROMFile(romPath, &romData)) {
        handleLoadingError("Failed to load ROM file");
        loadingInProgress = false;
        return false;
    }
    
    // Phase 2: Validate ROM integrity
    if (!validateROMIntegrity(romData.rawData, romData.dataSize)) {
        handleLoadingError("ROM integrity validation failed");
        loadingInProgress = false;
        return false;
    }
    
    // Phase 3: Parse ROM metadata and app information
    SecureAppInfo appInfo;
    if (!parseROMMetadata(&romData, &appInfo)) {
        handleLoadingError("ROM metadata parsing failed");
        loadingInProgress = false;
        return false;
    }
    
    // Phase 4: Resource limit validation
    if (!checkResourceLimits(appInfo)) {
        handleLoadingError("ROM exceeds resource limits");
        loadingInProgress = false;
        return false;
    }
    
    // Phase 5: Load and validate scripts
    if (!loadAndValidateScripts(&romData, &appInfo)) {
        handleLoadingError("Script validation failed");
        loadingInProgress = false;
        return false;
    }
    
    // Phase 6: Validate entity intents
    if (!validateEntityIntents(appInfo.initialEntities, appInfo.maxEntities)) {
        handleLoadingError("Entity intent validation failed");
        loadingInProgress = false;
        return false;
    }
    
    // Phase 7: Load initial scene
    if (!loadInitialSceneSecure(&romData, targetPanelId)) {
        handleLoadingError("Initial scene loading failed");
        loadingInProgress = false;
        return false;
    }
    
    // Phase 8: Create initial entities through UUID authority
    if (!createInitialEntitiesSecure(&appInfo, targetPanelId)) {
        handleLoadingError("Initial entity creation failed");
        loadingInProgress = false;
        return false;
    }
    
    // Success - store app info and update statistics
    currentApp = appInfo;
    currentApp.validated = true;
    currentStats.loadTimeMs = GET_TIME_MS() - startTime;
    currentStats.loadSuccessful = true;
    
    loadingInProgress = false;
    
    LOG_INFO("ROM loaded successfully: %s (%d scripts, %d entities, %dms)",
             appInfo.name.c_str(), appInfo.scripts.size(), appInfo.initialEntities.size(),
             currentStats.loadTimeMs);
    
    return true;
}

bool SecureROMLoader::unloadCurrentROM() {
    if (!isROMLoaded()) {
        LOG_DEBUG("No ROM currently loaded");
        return true;
    }
    
    LOG_INFO("Unloading ROM: %s", currentApp.name.c_str());
    
    // Cleanup all scripts created by this ROM
    for (const auto& script : currentApp.scripts) {
        if (script.scriptType == "entity") {
            // Entity scripts will be cleaned up when entities are destroyed
        } else if (script.scriptType == "panel") {
            scriptAuthority->destroyPanelScript(0); // TODO: Track panel ID
        } else if (script.scriptType == "global") {
            scriptAuthority->destroyGlobalScript(script.scriptName);
        }
    }
    
    // Cleanup entities created by this ROM through UUID authority
    uuidAuthority->cleanupROMEntities(); // TODO: Add ROM tracking to UUID authority
    
    // Reset app info
    currentApp = SecureAppInfo();
    currentApp.validated = false;
    
    LOG_INFO("ROM unloaded successfully");
    return true;
}

// =========================
// Security Validation
// =========================

bool SecureROMLoader::validateROMIntegrity(const uint8_t* romData, size_t romSize) {
    LOG_DEBUG("Validating ROM integrity (size: %d bytes)", romSize);
    
    // Check ROM size limits
    if (romSize > MAX_ROM_SIZE_MB * 1024 * 1024) {
        recordSecurityViolation("ROM_SIZE_EXCEEDED", 
                               String("ROM size ") + String(romSize) + " exceeds limit");
        return false;
    }
    
    if (romSize < 64) { // Minimum reasonable ROM size
        recordSecurityViolation("ROM_TOO_SMALL", "ROM size too small");
        return false;
    }
    
    // Validate ROM header magic
    if (romData[0] != 'W' || romData[1] != 'R' || romData[2] != 'O' || romData[3] != 'M') {
        recordSecurityViolation("INVALID_ROM_MAGIC", "ROM magic header invalid");
        return false;
    }
    
    // Calculate and verify checksum
    uint32_t calculatedChecksum = calculateROMChecksum(romData, romSize);
    uint32_t storedChecksum = *((uint32_t*)(romData + 4)); // Checksum stored after magic
    
    if (calculatedChecksum != storedChecksum) {
        recordSecurityViolation("CHECKSUM_MISMATCH", 
                               String("Calculated: ") + String(calculatedChecksum) + 
                               ", Stored: " + String(storedChecksum));
        return false;
    }
    
    LOG_DEBUG("ROM integrity validation passed (checksum: 0x%08X)", calculatedChecksum);
    return true;
}

bool SecureROMLoader::validateWASHBytecode(const uint8_t* bytecode, size_t size, uint8_t permissionLevel) {
    if (!bytecode || size == 0) {
        recordSecurityViolation("EMPTY_BYTECODE", "Bytecode is empty or null");
        return false;
    }
    
    // Check size limits
    if (size > MAX_SCRIPT_SIZE_KB * 1024) {
        recordSecurityViolation("BYTECODE_SIZE_EXCEEDED", 
                               String("Bytecode size ") + String(size) + " exceeds limit");
        return false;
    }
    
    // Check bytecode cache first
    uint32_t bytecodeHash = calculateROMChecksum(bytecode, size);
    auto cacheIt = validatedBytecode.find(bytecodeHash);
    if (cacheIt != validatedBytecode.end()) {
        LOG_DEBUG("Bytecode validation cached (hash: 0x%08X)", bytecodeHash);
        return cacheIt->second;
    }
    
    // Validate bytecode header
    if (size < 8 || bytecode[0] != 'W' || bytecode[1] != 'A' || bytecode[2] != 'S' || bytecode[3] != 'H') {
        recordSecurityViolation("INVALID_BYTECODE_MAGIC", "WASH bytecode header invalid");
        validatedBytecode[bytecodeHash] = false;
        return false;
    }
    
    // Scan for malicious patterns
    if (!scanBytecodeForMaliciousPatterns(bytecode, size)) {
        recordSecurityViolation("MALICIOUS_PATTERN_DETECTED", "Bytecode contains malicious patterns");
        validatedBytecode[bytecodeHash] = false;
        return false;
    }
    
    // Validate instruction compliance
    if (!validateBytecodeInstructions(bytecode, size)) {
        recordSecurityViolation("INVALID_INSTRUCTIONS", "Bytecode contains invalid instructions");
        validatedBytecode[bytecodeHash] = false;
        return false;
    }
    
    LOG_DEBUG("Bytecode validation passed (size: %d, permission: %d, hash: 0x%08X)", 
              size, permissionLevel, bytecodeHash);
    
    validatedBytecode[bytecodeHash] = true;
    return true;
}

bool SecureROMLoader::validateScriptDefinitions(std::vector<SecureScriptDef>& scripts) {
    LOG_DEBUG("Validating %d script definitions", scripts.size());
    
    if (scripts.size() > MAX_SCRIPTS_PER_ROM) {
        recordSecurityViolation("TOO_MANY_SCRIPTS", 
                               String("Script count ") + String(scripts.size()) + " exceeds limit");
        return false;
    }
    
    std::set<String> scriptNames;
    
    for (auto& script : scripts) {
        // Check for duplicate names
        if (scriptNames.find(script.scriptName) != scriptNames.end()) {
            recordSecurityViolation("DUPLICATE_SCRIPT_NAME", 
                                   String("Script name '") + script.scriptName + "' is duplicate");
            return false;
        }
        scriptNames.insert(script.scriptName);
        
        // Validate script name
        if (!validateSecureString(script.scriptName, 64)) {
            recordSecurityViolation("INVALID_SCRIPT_NAME", 
                                   String("Script name '") + script.scriptName + "' is invalid");
            return false;
        }
        
        // Validate script type
        if (script.scriptType != "entity" && script.scriptType != "panel" && script.scriptType != "global") {
            recordSecurityViolation("INVALID_SCRIPT_TYPE", 
                                   String("Script type '") + script.scriptType + "' is invalid");
            return false;
        }
        
        // Validate permissions
        if (!validateScriptPermissions(script)) {
            return false; // Error already recorded in validateScriptPermissions
        }
        
        // Validate entity type for entity scripts
        if (script.scriptType == "entity" && !validateSecureString(script.entityType, 32)) {
            recordSecurityViolation("INVALID_ENTITY_TYPE", 
                                   String("Entity type '") + script.entityType + "' is invalid");
            return false;
        }
        
        script.validated = true;
        currentStats.scriptsValidated++;
    }
    
    LOG_DEBUG("All script definitions validated successfully");
    return true;
}

bool SecureROMLoader::validateEntityIntents(const std::vector<EntityIntent>& entities, uint32_t maxEntities) {
    LOG_DEBUG("Validating %d entity intents (max: %d)", entities.size(), maxEntities);
    
    if (entities.size() > maxEntities) {
        recordSecurityViolation("TOO_MANY_ENTITIES", 
                               String("Entity count ") + String(entities.size()) + " exceeds limit");
        return false;
    }
    
    if (entities.size() > MAX_ENTITIES_PER_ROM) {
        recordSecurityViolation("ENTITY_COUNT_EXCEEDED", 
                               String("Entity count ") + String(entities.size()) + " exceeds system limit");
        return false;
    }
    
    for (const auto& intent : entities) {
        if (!validateEntityParameters(intent)) {
            return false; // Error already recorded in validateEntityParameters
        }
    }
    
    LOG_DEBUG("All entity intents validated successfully");
    return true;
}

// =========================
// Resource Management
// =========================

uint32_t SecureROMLoader::getCurrentMemoryUsageKB() const {
    uint32_t totalMemory = 0;
    
    // Calculate script bytecode memory
    for (const auto& script : currentApp.scripts) {
        totalMemory += script.bytecodeSize;
    }
    
    // Calculate entity memory (rough estimate)
    totalMemory += currentApp.initialEntities.size() * 256; // ~256 bytes per entity
    
    // Calculate metadata memory
    totalMemory += currentApp.name.length() + currentApp.description.length();
    totalMemory += currentApp.iconPath.length() + currentApp.splashPath.length();
    
    return (totalMemory + 1023) / 1024; // Convert to KB, round up
}

bool SecureROMLoader::checkResourceLimits(const SecureAppInfo& appInfo) const {
    LOG_DEBUG("Checking resource limits for ROM");
    
    // Check memory limit
    if (appInfo.memoryLimitKB > 0) {
        // Calculate estimated memory usage
        uint32_t estimatedMemory = 0;
        for (const auto& script : appInfo.scripts) {
            estimatedMemory += script.bytecodeSize;
        }
        estimatedMemory += appInfo.initialEntities.size() * 256; // Entity memory estimate
        estimatedMemory = (estimatedMemory + 1023) / 1024; // Convert to KB
        
        if (estimatedMemory > appInfo.memoryLimitKB) {
            LOG_WARN("ROM exceeds memory limit: %dKB > %dKB", estimatedMemory, appInfo.memoryLimitKB);
            return false;
        }
    }
    
    // Check script count
    if (appInfo.scripts.size() > appInfo.maxScripts) {
        LOG_WARN("ROM exceeds script limit: %d > %d", appInfo.scripts.size(), appInfo.maxScripts);
        return false;
    }
    
    // Check entity count
    if (appInfo.initialEntities.size() > appInfo.maxEntities) {
        LOG_WARN("ROM exceeds entity limit: %d > %d", appInfo.initialEntities.size(), appInfo.maxEntities);
        return false;
    }
    
    // Check permission level
    for (const auto& script : appInfo.scripts) {
        if (script.permissionLevel > appInfo.maxPermissionLevel) {
            LOG_WARN("Script '%s' exceeds permission limit: %d > %d", 
                     script.scriptName.c_str(), script.permissionLevel, appInfo.maxPermissionLevel);
            return false;
        }
    }
    
    LOG_DEBUG("Resource limit check passed");
    return true;
}

void SecureROMLoader::cleanupValidationCache() {
    LOG_DEBUG("Cleaning up bytecode validation cache (%d entries)", validatedBytecode.size());
    
    // For now, just clear all cache entries
    // In a more sophisticated implementation, we could implement LRU eviction
    validatedBytecode.clear();
}

// =========================
// Debug and Statistics
// =========================

void SecureROMLoader::dumpLoadingState() const {
    LOG_INFO("=== ROM Loading State ===");
    LOG_INFO("ROM Loaded: %s", isROMLoaded() ? "YES" : "NO");
    LOG_INFO("Loading in Progress: %s", loadingInProgress ? "YES" : "NO");
    
    if (isROMLoaded()) {
        LOG_INFO("App: %s v%s by %s", currentApp.name.c_str(), 
                 currentApp.version.c_str(), currentApp.author.c_str());
        LOG_INFO("Scripts: %d, Entities: %d, Memory: %dKB", 
                 currentApp.scripts.size(), currentApp.initialEntities.size(), 
                 getCurrentMemoryUsageKB());
        LOG_INFO("Security Version: %d, Validated: %s", 
                 currentApp.securityVersion, currentApp.validated ? "YES" : "NO");
    }
    
    LOG_INFO("Stats - Scripts Loaded: %d, Validated: %d, Rejected: %d",
             currentStats.totalScriptsLoaded, currentStats.scriptsValidated, currentStats.scriptsRejected);
    LOG_INFO("Stats - Entities Created: %d, Violations: %d, Load Time: %dms",
             currentStats.entitiesCreated, currentStats.securityViolations, currentStats.loadTimeMs);
    
    if (!currentStats.lastError.isEmpty()) {
        LOG_INFO("Last Error: %s", currentStats.lastError.c_str());
    }
    
    LOG_INFO("Validation Cache: %d entries", validatedBytecode.size());
    LOG_INFO("=========================");
}

String SecureROMLoader::getSecurityValidationReport() const {
    String report = "=== Security Validation Report ===\n";
    
    if (isROMLoaded()) {
        report += "ROM: " + String(currentApp.name.c_str()) + "\n";
        report += "Security Version: " + String(currentApp.securityVersion) + "\n";
        report += "Validation Status: " + (currentApp.validated ? String("PASSED") : String("FAILED")) + "\n";
        report += "Permission Level: " + String(currentApp.maxPermissionLevel) + "\n";
        
        report += "\nScript Security:\n";
        for (const auto& script : currentApp.scripts) {
            report += "  " + script.scriptName + " (" + script.scriptType + "): ";
            report += script.validated ? "VALIDATED" : "REJECTED";
            report += " [Permission: " + permissionLevelToString(script.permissionLevel) + "]\n";
            if (!script.securityNotes.isEmpty()) {
                report += "    Notes: " + script.securityNotes + "\n";
            }
        }
        
        report += "\nResource Usage:\n";
        report += "  Memory: " + String(getCurrentMemoryUsageKB()) + "KB\n";
        report += "  Scripts: " + String(currentApp.scripts.size()) + "/" + String(currentApp.maxScripts) + "\n";
        report += "  Entities: " + String(currentApp.initialEntities.size()) + "/" + String(currentApp.maxEntities) + "\n";
    } else {
        report += "No ROM currently loaded\n";
    }
    
    report += "\nSecurity Statistics:\n";
    report += "  Scripts Validated: " + String(currentStats.scriptsValidated) + "\n";
    report += "  Scripts Rejected: " + String(currentStats.scriptsRejected) + "\n";
    report += "  Security Violations: " + String(currentStats.securityViolations) + "\n";
    report += "  Load Time: " + String(currentStats.loadTimeMs) + "ms\n";
    
    if (!currentStats.lastError.isEmpty()) {
        report += "  Last Error: " + currentStats.lastError + "\n";
    }
    
    report += "===================================";
    return report;
}

void SecureROMLoader::resetStats() {
    currentStats.totalScriptsLoaded = 0;
    currentStats.scriptsValidated = 0;
    currentStats.scriptsRejected = 0;
    currentStats.entitiesCreated = 0;
    currentStats.securityViolations = 0;
    currentStats.loadTimeMs = 0;
    currentStats.lastError = "";
    currentStats.loadSuccessful = false;
}

// =========================
// Private Implementation
// =========================

bool SecureROMLoader::loadROMFile(const String& romPath, WispROMData* romData) {
    LOG_DEBUG("Loading ROM file: %s", romPath.c_str());
    
    // This would normally interface with the file system
    // For now, we'll simulate basic file loading
    // TODO: Implement actual file loading based on Wisp engine's storage system
    
    LOG_WARN("ROM file loading not yet implemented - returning mock data");
    
    // Mock ROM data for testing
    static uint8_t mockRomData[1024] = {'W', 'R', 'O', 'M', 0, 0, 0, 0}; // Magic + checksum placeholder
    romData->rawData = mockRomData;
    romData->dataSize = sizeof(mockRomData);
    
    return true;
}

bool SecureROMLoader::parseROMMetadata(const WispROMData* romData, SecureAppInfo* appInfo) {
    LOG_DEBUG("Parsing ROM metadata");
    
    // TODO: Implement actual ROM metadata parsing
    // For now, create mock app info
    
    appInfo->name = "Test App";
    appInfo->version = "1.0.0";
    appInfo->author = "Test Developer";
    appInfo->description = "Test application for secure ROM loading";
    appInfo->autoStart = false;
    appInfo->screenWidth = 240;
    appInfo->screenHeight = 135;
    
    // Security and resource limits
    appInfo->maxEntities = 100;
    appInfo->maxScripts = 10;
    appInfo->maxPermissionLevel = 2; // ELEVATED
    appInfo->memoryLimitKB = 1024;   // 1MB limit
    
    appInfo->romChecksum = calculateROMChecksum(romData->rawData, romData->dataSize);
    appInfo->securityVersion = SECURITY_VERSION;
    
    LOG_DEBUG("ROM metadata parsed: %s v%s", appInfo->name.c_str(), appInfo->version.c_str());
    return true;
}

bool SecureROMLoader::loadAndValidateScripts(const WispROMData* romData, SecureAppInfo* appInfo) {
    LOG_DEBUG("Loading and validating scripts from ROM");
    
    // TODO: Implement actual script loading from ROM
    // For now, create mock script definitions
    
    SecureScriptDef mockScript;
    mockScript.scriptName = "test_entity_script";
    mockScript.scriptType = "entity";
    mockScript.entityType = "player";
    mockScript.permissionLevel = 1; // STANDARD
    mockScript.bytecodeSize = 512;
    mockScript.bytecodeChecksum = 0x12345678;
    mockScript.validated = false;
    mockScript.securityNotes = "";
    
    appInfo->scripts.push_back(mockScript);
    currentStats.totalScriptsLoaded = appInfo->scripts.size();
    
    // Validate all scripts
    if (!validateScriptDefinitions(appInfo->scripts)) {
        return false;
    }
    
    LOG_DEBUG("Scripts loaded and validated: %d", appInfo->scripts.size());
    return true;
}

bool SecureROMLoader::createInitialEntitiesSecure(const SecureAppInfo* appInfo, uint16_t targetPanelId) {
    LOG_DEBUG("Creating %d initial entities through UUID authority", appInfo->initialEntities.size());
    
    for (const auto& intent : appInfo->initialEntities) {
        // Create entity through UUID authority (engine has authority)
        uint32_t uuid = uuidAuthority->createEntityUUID(intent.entityType, targetPanelId, intent.scriptName);
        
        if (uuid == 0) {
            recordSecurityViolation("ENTITY_CREATION_FAILED", 
                                   String("Failed to create entity: ") + intent.entityType);
            return false;
        }
        
        // TODO: Actually create the entity in the scene system
        // sceneManager->addEntitySecure(...);
        
        // If entity has a script, create script instance
        if (!intent.scriptName.isEmpty() && scriptAuthority) {
            if (!scriptAuthority->createEntityScript(intent.scriptName, uuid)) {
                LOG_WARN("Failed to create script '%s' for entity %d", 
                         intent.scriptName.c_str(), uuid);
                // Don't fail entity creation, just log warning
            }
        }
        
        currentStats.entitiesCreated++;
        LOG_DEBUG("Created entity UUID %d type '%s' with script '%s'", 
                  uuid, intent.entityType.c_str(), intent.scriptName.c_str());
    }
    
    LOG_DEBUG("Initial entities created successfully: %d", currentStats.entitiesCreated);
    return true;
}

bool SecureROMLoader::loadInitialSceneSecure(const WispROMData* romData, uint16_t targetPanelId) {
    LOG_DEBUG("Loading initial scene for panel %d", targetPanelId);
    
    // TODO: Implement initial scene loading
    // This would typically involve:
    // 1. Loading scene layout from ROM
    // 2. Creating background sprites
    // 3. Setting up tile maps
    // 4. Configuring scene parameters
    
    LOG_DEBUG("Initial scene loaded successfully");
    return true;
}

uint32_t SecureROMLoader::calculateROMChecksum(const uint8_t* data, size_t size) {
    if (!data || size == 0) return 0;
    
    uint32_t checksum = 0x12345678; // Initial seed
    
    for (size_t i = 0; i < size; i++) {
        checksum ^= data[i];
        checksum = (checksum << 1) | (checksum >> 31); // Rotate left
        checksum ^= (i & 0xFF); // Mix in position
    }
    
    return checksum;
}

bool SecureROMLoader::scanBytecodeForMaliciousPatterns(const uint8_t* bytecode, size_t size) {
    // Scan for known malicious patterns in bytecode
    
    // Pattern 1: Excessive API calls (potential DoS)
    uint32_t apiCallCount = 0;
    for (size_t i = 0; i < size - 1; i++) {
        if (bytecode[i] == 0xFF && bytecode[i+1] >= 0x80) { // Hypothetical API call pattern
            apiCallCount++;
        }
    }
    
    if (apiCallCount > 1000) { // Arbitrary limit
        LOG_WARN("Excessive API calls detected: %d", apiCallCount);
        return false;
    }
    
    // Pattern 2: Suspicious memory access patterns
    // TODO: Implement based on WASH VM architecture
    
    // Pattern 3: Infinite loop detection
    // TODO: Static analysis for loop structures
    
    return true; // No malicious patterns detected
}

bool SecureROMLoader::validateBytecodeInstructions(const uint8_t* bytecode, size_t size) {
    // Validate that all instructions are valid WASH opcodes
    
    // Skip header (first 8 bytes: "WASH" + version info)
    const uint8_t* instructions = bytecode + 8;
    size_t instructionSize = size - 8;
    
    for (size_t i = 0; i < instructionSize; i++) {
        uint8_t opcode = instructions[i];
        
        // Validate opcode range (based on WASH VM specification)
        // TODO: Use actual WASH opcode definitions
        if (opcode > 0xFE) { // 0xFF is reserved for API calls
            LOG_WARN("Invalid opcode detected: 0x%02X at position %d", opcode, i);
            return false;
        }
    }
    
    return true;
}

bool SecureROMLoader::validateScriptPermissions(const SecureScriptDef& script) {
    // Validate permission level
    if (script.permissionLevel > 3) { // 0=RESTRICTED, 1=STANDARD, 2=ELEVATED, 3=SYSTEM
        recordSecurityViolation("INVALID_PERMISSION_LEVEL", 
                               String("Permission level ") + String(script.permissionLevel) + " is invalid");
        return false;
    }
    
    // System-level permissions require special validation
    if (script.permissionLevel == 3) { // SYSTEM
        if (script.scriptType != "global") {
            recordSecurityViolation("INVALID_SYSTEM_PERMISSION", 
                                   "SYSTEM permission only allowed for global scripts");
            return false;
        }
    }
    
    return true;
}

bool SecureROMLoader::validateEntityParameters(const EntityIntent& intent) {
    // Validate entity type
    if (!validateSecureString(intent.entityType, 32)) {
        recordSecurityViolation("INVALID_ENTITY_TYPE", 
                               String("Entity type '") + intent.entityType + "' is invalid");
        return false;
    }
    
    // Validate position (reasonable bounds)
    if (intent.x < -10000.0f || intent.x > 10000.0f || 
        intent.y < -10000.0f || intent.y > 10000.0f) {
        recordSecurityViolation("INVALID_ENTITY_POSITION", 
                               String("Entity position (") + String(intent.x) + ", " + String(intent.y) + ") is invalid");
        return false;
    }
    
    // Validate script name if provided
    if (!intent.scriptName.isEmpty() && !validateSecureString(intent.scriptName, 64)) {
        recordSecurityViolation("INVALID_SCRIPT_NAME", 
                               String("Script name '") + intent.scriptName + "' is invalid");
        return false;
    }
    
    // Validate behavior value
    if (intent.behavior > 10) { // Assuming max 10 behavior types
        recordSecurityViolation("INVALID_ENTITY_BEHAVIOR", 
                               String("Entity behavior ") + String(intent.behavior) + " is invalid");
        return false;
    }
    
    return true;
}

void SecureROMLoader::recordSecurityViolation(const String& violation, const String& details) {
    currentStats.securityViolations++;
    LOG_WARN("Security violation: %s - %s", violation.c_str(), details.c_str());
    
    // TODO: Report to security monitoring system
}

void SecureROMLoader::handleLoadingError(const String& error) {
    currentStats.lastError = error;
    currentStats.loadSuccessful = false;
    LOG_ERROR("ROM loading error: %s", error.c_str());
    
    cleanupPartialLoad();
}

void SecureROMLoader::cleanupPartialLoad() {
    // Cleanup any partially loaded resources
    LOG_DEBUG("Cleaning up partial ROM load");
    
    // Reset current app info
    currentApp = SecureAppInfo();
    currentApp.validated = false;
}

String SecureROMLoader::permissionLevelToString(uint8_t level) const {
    switch (level) {
        case 0: return "RESTRICTED";
        case 1: return "STANDARD";
        case 2: return "ELEVATED";
        case 3: return "SYSTEM";
        default: return "UNKNOWN";
    }
}

String SecureROMLoader::formatSizeString(uint32_t sizeBytes) const {
    if (sizeBytes < 1024) {
        return String(sizeBytes) + "B";
    } else if (sizeBytes < 1024 * 1024) {
        return String((sizeBytes + 512) / 1024) + "KB";
    } else {
        return String((sizeBytes + 512 * 1024) / (1024 * 1024)) + "MB";
    }
}

bool SecureROMLoader::validateSecureString(const String& str, uint32_t maxLength) const {
    if (str.length() > maxLength) {
        return false;
    }
    
    // Check for control characters and null bytes
    for (size_t i = 0; i < str.length(); i++) {
        char c = str[i];
        if (c < 32 && c != '\n' && c != '\r' && c != '\t') {
            return false; // Invalid control character
        }
        if (c == 0) {
            return false; // Null byte
        }
    }
    
    // Must not be empty
    if (str.length() == 0) {
        return false;
    }
    
    // Must start with alphanumeric or underscore
    char first = str[0];
    if (!(first >= 'A' && first <= 'Z') && 
        !(first >= 'a' && first <= 'z') && 
        !(first >= '0' && first <= '9') && 
        first != '_') {
        return false;
    }
    
    return true;
}

// =========================
// Factory Function
// =========================

SecureROMLoader* createSecureROMLoader(EngineUUIDAuthority* uuidAuth,
                                      ScriptInstanceAuthority* scriptAuth,
                                      SecureWASHAPIBridge* bridge,
                                      SceneManager* sceneMgr,
                                      WispCuratedAPIExtended* api) {
    if (!uuidAuth || !scriptAuth || !bridge || !sceneMgr || !api) {
        LOG_ERROR("Cannot create SecureROMLoader: missing required dependencies");
        return nullptr;
    }
    
    return new SecureROMLoader(uuidAuth, scriptAuth, bridge, sceneMgr, api);
}
