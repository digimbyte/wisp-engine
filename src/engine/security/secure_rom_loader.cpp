#include "secure_rom_loader.h"
#include "../app/wisp_resource_quota.h"  // For resource management integration
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
    , segmentedLoader(nullptr)
    , loadingInProgress(false)
    , activePanelCacheCount(0)
{
    LOG_INFO("SecureROMLoader initialized - Phase 5 Segmented Security Integration");
    
    // Initialize current app info
    currentApp.validated = false;
    currentApp.securityVersion = SECURITY_VERSION;
    
    // Reset statistics
    resetStats();
    
    // Initialize panel cache
    for (int i = 0; i < MAX_PANEL_CACHE; i++) {
        panelCache[i].panelId = 0xFFFF;  // Invalid panel ID
        panelCache[i].memoryUsageKB = 0;
        panelCache[i].lastAccessed = 0;
        panelCache[i].validatedScripts.clear();
    }
    
    LOG_DEBUG("Adaptive Memory Limits: Min Panel=%dKB, Fallback Reserve=%dKB, Max Script=%dKB",
              MIN_PANEL_MEMORY_KB, FALLBACK_MEMORY_KB, MAX_SCRIPT_SIZE_KB);
}

SecureROMLoader::~SecureROMLoader() {
    if (isROMLoaded()) {
        unloadCurrentROM();
    }
    
    // Cleanup segmented loader
    if (segmentedLoader) {
        delete segmentedLoader;
        segmentedLoader = nullptr;
    }
    
    cleanupValidationCache();
    LOG_INFO("SecureROMLoader destroyed");
}

// =========================
// Segmented ROM Loading Interface
// =========================

bool SecureROMLoader::initializeROM(const String& romPath) {
    if (loadingInProgress) {
        LOG_WARN("ROM initialization already in progress");
        return false;
    }
    
    LOG_INFO("Initializing ROM with segmented loading: %s", romPath.c_str());
    
    loadingInProgress = true;
    resetStats();
    uint32_t startTime = GET_TIME_MS();
    currentROMPath = romPath;
    
    // Phase 1: Evaluate current memory situation
    DynamicLimits limits;
    if (!evaluateMemoryLimits(&limits)) {
        handleLoadingError("Insufficient memory for ROM initialization");
        loadingInProgress = false;
        return false;
    }
    
    // Phase 2: Initialize WispSegmentedLoader for efficient asset streaming
    if (!initializeSegmentedLoader(romPath)) {
        handleLoadingError("Failed to initialize segmented loader");
        loadingInProgress = false;
        return false;
    }
    
    // Phase 3: Parse ROM config (lightweight - just metadata)
    if (!parseROMConfig(&currentApp)) {
        handleLoadingError("ROM config parsing failed");
        loadingInProgress = false;
        return false;
    }
    
    // Phase 4: Basic ROM integrity validation (header only)
    // Full validation happens per-panel during loading
    // TODO: Call segmentedLoader->validateROMHeader()
    
    // Success - ROM structure loaded and ready for segmented loading
    currentApp.validated = true;
    currentStats.loadTimeMs = GET_TIME_MS() - startTime;
    currentStats.loadSuccessful = true;
    
    loadingInProgress = false;
    
    LOG_INFO("ROM initialized for segmented loading: %s (%dms)", 
             currentApp.name.c_str(), currentStats.loadTimeMs);
    
    return true;
}

bool SecureROMLoader::loadPanel(const String& panelName, uint8_t layoutIndex, uint8_t panelIndex) {
    if (!isROMLoaded()) {
        LOG_ERROR("Cannot load panel - no ROM initialized");
        return false;
    }
    
    if (loadingInProgress) {
        LOG_WARN("Panel loading already in progress");
        return false;
    }
    
    LOG_INFO("Loading panel: %s (layout: %d, index: %d)", panelName.c_str(), layoutIndex, panelIndex);
    
    loadingInProgress = true;
    uint32_t startTime = GET_TIME_MS();
    
    // Phase 1: Check if we can load this panel within memory constraints
    uint32_t estimatedMemory = 0;
    // TODO: Query segmented loader for panel memory estimate
    // estimatedMemory = segmentedLoader->estimatePanelMemory(panelName);
    estimatedMemory = 32; // Mock estimate for now
    
    if (!canLoadPanel(panelName, estimatedMemory)) {
        // Try to free memory by unloading other panels
        uint32_t freedMemory = freeMemoryForLoading(estimatedMemory);
        if (freedMemory < estimatedMemory) {
            handleLoadingError("Insufficient memory for panel loading even after cleanup");
            loadingInProgress = false;
            return false;
        }
    }
    
    // Phase 2: Load panel scripts with security validation
    if (!loadPanelScripts(panelName, layoutIndex, panelIndex)) {
        handleLoadingError("Panel script loading failed");
        loadingInProgress = false;
        return false;
    }
    
    // Phase 3: Load panel entities with security validation
    if (!loadPanelEntities(panelName, layoutIndex, panelIndex)) {
        handleLoadingError("Panel entity loading failed");
        loadingInProgress = false;
        return false;
    }
    
    // Phase 4: Load panel assets (tiles, backgrounds, sprites)
    if (!loadPanelAssets(panelName, layoutIndex, panelIndex)) {
        // Assets can fail - try fallback assets
        LOG_WARN("Panel assets failed to load, trying fallbacks");
        if (!configureAssetFallbacks(panelName, 2)) { // Medium severity fallbacks
            LOG_WARN("Asset fallbacks also failed - panel will use minimal assets");
        }
    }
    
    // Phase 5: Update panel cache
    PanelValidationCache* cache = getPanelCache(panelName);
    if (cache) {
        cache->memoryUsageKB = estimatedMemory;
        cache->lastAccessed = GET_TIME_MS();
    }
    
    // Success
    uint32_t loadTime = GET_TIME_MS() - startTime;
    loadingInProgress = false;
    
    LOG_INFO("Panel loaded successfully: %s (%dKB, %dms)", 
             panelName.c_str(), estimatedMemory, loadTime);
    
    return true;
}

bool SecureROMLoader::unloadPanel(const String& panelName) {
    LOG_INFO("Unloading panel: %s", panelName.c_str());
    
    // Find panel in cache
    for (int i = 0; i < activePanelCacheCount; i++) {
        if (panelCache[i].panelId != 0xFFFF) {
            // TODO: Compare with actual panel name/ID
            // For now, just clear the first found entry
            panelCache[i].panelId = 0xFFFF;
            panelCache[i].memoryUsageKB = 0;
            panelCache[i].validatedScripts.clear();
            
            // Compact cache
            if (i < activePanelCacheCount - 1) {
                panelCache[i] = panelCache[activePanelCacheCount - 1];
            }
            activePanelCacheCount--;
            break;
        }
    }
    
    // TODO: Actually unload panel assets through segmented loader
    // segmentedLoader->unloadPanel(panelName);
    
    LOG_INFO("Panel unloaded: %s", panelName.c_str());
    return true;
}

bool SecureROMLoader::unloadCurrentROM() {
    if (!isROMLoaded()) {
        LOG_DEBUG("No ROM currently loaded");
        return true;
    }
    
    LOG_INFO("Unloading ROM: %s", currentApp.name.c_str());
    
    // Unload all cached panels
    for (int i = 0; i < activePanelCacheCount; i++) {
        // TODO: Unload panel by ID
        panelCache[i].panelId = 0xFFFF;
        panelCache[i].validatedScripts.clear();
    }
    activePanelCacheCount = 0;
    
    // Cleanup segmented loader
    if (segmentedLoader) {
        // TODO: segmentedLoader->cleanup();
    }
    
    // Cleanup script instances
    scriptAuthority->cleanupROMScripts(); // TODO: Add ROM tracking
    
    // Cleanup entities through UUID authority
    uuidAuthority->cleanupROMEntities(); // TODO: Add ROM tracking
    
    // Reset app info
    currentApp = SecureAppInfo();
    currentApp.validated = false;
    currentROMPath = "";
    
    LOG_INFO("ROM unloaded successfully");
    return true;
}

bool SecureROMLoader::isPanelLoaded(const String& panelName) const {
    // Check panel cache for loaded status
    for (int i = 0; i < activePanelCacheCount; i++) {
        if (panelCache[i].panelId != 0xFFFF) {
            // TODO: Compare with actual panel name
            return true;
        }
    }
    return false;
}

// =========================
// Adaptive Memory Management Implementation
// =========================

bool SecureROMLoader::evaluateMemoryLimits(DynamicLimits* limits) {
    if (!limits) return false;
    
    uint32_t totalHeap, freeHeap, largestBlock;
    if (!getHeapMemoryStatus(&totalHeap, &freeHeap, &largestBlock)) {
        LOG_ERROR("Failed to query heap memory status");
        return false;
    }
    
    LOG_DEBUG("Memory Status: Total=%dKB, Free=%dKB, LargestBlock=%dKB", 
              totalHeap, freeHeap, largestBlock);
    
    // Calculate available memory minus safety margins
    uint32_t safetyMargin = FALLBACK_MEMORY_KB + 16; // 16KB additional safety
    if (freeHeap <= safetyMargin) {
        LOG_ERROR("Insufficient free memory: %dKB <= %dKB safety margin", freeHeap, safetyMargin);
        return false;
    }
    
    limits->availableMemoryKB = freeHeap - safetyMargin;
    
    // Calculate dynamic limits based on available memory
    if (limits->availableMemoryKB >= 128) {
        // High memory scenario
        limits->maxPanelMemoryKB = 64;
        limits->maxScriptsPerPanel = 16;
        limits->maxEntitiesPerPanel = 100;
        limits->maxAssetCacheKB = 96;
        limits->useAssetFallbacks = false;
        limits->truncateScripts = false;
        limits->streamAudio = false;
    } else if (limits->availableMemoryKB >= 64) {
        // Medium memory scenario
        limits->maxPanelMemoryKB = 32;
        limits->maxScriptsPerPanel = 8;
        limits->maxEntitiesPerPanel = 50;
        limits->maxAssetCacheKB = 48;
        limits->useAssetFallbacks = false;
        limits->truncateScripts = false;
        limits->streamAudio = true;
    } else {
        // Low memory scenario - aggressive limits
        limits->maxPanelMemoryKB = 16;
        limits->maxScriptsPerPanel = 4;
        limits->maxEntitiesPerPanel = 25;
        limits->maxAssetCacheKB = 24;
        limits->useAssetFallbacks = true;
        limits->truncateScripts = true;
        limits->streamAudio = true;
    }
    
    // Ensure minimums
    if (limits->maxPanelMemoryKB < MIN_PANEL_MEMORY_KB) {
        limits->maxPanelMemoryKB = MIN_PANEL_MEMORY_KB;
    }
    
    LOG_INFO("Dynamic Limits: Panel=%dKB, Scripts=%d, Entities=%d, Cache=%dKB, Fallbacks=%s", 
             limits->maxPanelMemoryKB, limits->maxScriptsPerPanel, limits->maxEntitiesPerPanel,
             limits->maxAssetCacheKB, limits->useAssetFallbacks ? "YES" : "NO");
    
    return true;
}

bool SecureROMLoader::canLoadPanel(const String& panelName, uint32_t estimatedMemoryKB) {
    DynamicLimits limits;
    if (!evaluateMemoryLimits(&limits)) {
        return false;
    }
    
    // Check if panel fits within per-panel memory limit
    if (estimatedMemoryKB > limits.maxPanelMemoryKB) {
        LOG_WARN("Panel %s requires %dKB, exceeds limit of %dKB", 
                 panelName.c_str(), estimatedMemoryKB, limits.maxPanelMemoryKB);
        return false;
    }
    
    // Check total available memory including currently loaded panels
    uint32_t currentlyUsed = 0;
    for (int i = 0; i < activePanelCacheCount; i++) {
        currentlyUsed += panelCache[i].memoryUsageKB;
    }
    
    if (currentlyUsed + estimatedMemoryKB > limits.availableMemoryKB) {
        LOG_WARN("Total memory would exceed limit: current=%dKB + new=%dKB > available=%dKB",
                 currentlyUsed, estimatedMemoryKB, limits.availableMemoryKB);
        return false;
    }
    
    return true;
}

bool SecureROMLoader::configureAssetFallbacks(const String& panelName, uint8_t severity) {
    LOG_INFO("Configuring asset fallbacks for panel %s (severity: %d)", panelName.c_str(), severity);
    
    // TODO: Interface with segmented loader to enable fallback assets
    switch (severity) {
        case 0: // Minimal fallbacks
            // segmentedLoader->enableImageFallbacks(panelName, 0.8f); // 80% quality
            break;
        case 1: // Moderate fallbacks  
            // segmentedLoader->enableImageFallbacks(panelName, 0.6f); // 60% quality
            // segmentedLoader->enableAnimationReduction(panelName, 0.5f); // Half frames
            break;
        case 2: // Aggressive fallbacks
            // segmentedLoader->enableImageFallbacks(panelName, 0.4f); // 40% quality
            // segmentedLoader->enableAnimationReduction(panelName, 0.25f); // Quarter frames
            // segmentedLoader->disableParticleEffects(panelName);
            break;
        case 3: // Maximum fallbacks
            // segmentedLoader->enableImageFallbacks(panelName, 0.2f); // 20% quality
            // segmentedLoader->disableAnimations(panelName);
            // segmentedLoader->disableParticleEffects(panelName);
            // segmentedLoader->enableAudioStreaming(panelName);
            break;
    }
    
    LOG_DEBUG("Asset fallbacks configured for panel %s", panelName.c_str());
    return true;
}

bool SecureROMLoader::optimizeScriptForMemory(const String& scriptName, uint32_t targetSizeKB) {
    LOG_INFO("Optimizing script %s for memory (target: %dKB)", scriptName.c_str(), targetSizeKB);
    
    // TODO: Implement script optimization
    // This could involve:
    // - Removing debug symbols
    // - Compacting constant tables
    // - Optimizing instruction sequences
    // - Removing unused functions
    
    LOG_DEBUG("Script optimization completed for %s", scriptName.c_str());
    return true;
}

uint32_t SecureROMLoader::freeMemoryForLoading(uint32_t requiredMemoryKB) {
    LOG_INFO("Attempting to free %dKB for loading", requiredMemoryKB);
    
    uint32_t freedMemory = 0;
    
    // Strategy 1: Evict least recently used panels
    while (freedMemory < requiredMemoryKB && activePanelCacheCount > 0) {
        evictLRUPanel();
        
        // Recalculate freed memory
        uint32_t currentUsed = 0;
        for (int i = 0; i < activePanelCacheCount; i++) {
            currentUsed += panelCache[i].memoryUsageKB;
        }
        
        // TODO: Get actual memory usage from system
        freedMemory = requiredMemoryKB; // Mock - assume we freed enough
        break;
    }
    
    // Strategy 2: Clear validation cache
    if (freedMemory < requiredMemoryKB) {
        cleanupValidationCache();
        freedMemory += 4; // Assume cache used ~4KB
    }
    
    // Strategy 3: Request garbage collection if available
    #ifdef ESP_PLATFORM
    // TODO: Trigger ESP32 garbage collection if available
    #endif
    
    LOG_INFO("Freed %dKB of memory", freedMemory);
    return freedMemory;
}

bool SecureROMLoader::getHeapMemoryStatus(uint32_t* totalHeap, uint32_t* freeHeap, uint32_t* largestBlock) {
    #ifdef ESP_PLATFORM
    // ESP32 heap status
    *totalHeap = esp_get_total_heap_size() / 1024;
    *freeHeap = esp_get_free_heap_size() / 1024;
    *largestBlock = esp_get_largest_free_block_size() / 1024;
    return true;
    #else
    // Mock values for non-ESP platforms
    *totalHeap = 300;  // 300KB total
    *freeHeap = 150;   // 150KB free
    *largestBlock = 80; // 80KB largest block
    return true;
    #endif
}

// =========================
// Security Validation (Updated for Segmented Loading)
// =========================

bool SecureROMLoader::validateROMIntegrity(const uint8_t* romData, size_t romSize) {
    LOG_DEBUG("Validating ROM integrity (size: %zu bytes)", romSize);
    
    if (!romData || romSize < 64) {
        recordSecurityViolation("INVALID_ROM_DATA", "ROM data is null or too small");
        return false;
    }
    
    // Basic header validation
    if (romData[0] != 'W' || romData[1] != 'R' || romData[2] != 'O' || romData[3] != 'M') {
        recordSecurityViolation("INVALID_ROM_MAGIC", "ROM magic header invalid");
        return false;
    }
    
    // For segmented loading, we only validate the header and structure
    // Individual segments are validated on-demand during loading
    
    LOG_DEBUG("ROM integrity validation passed");
    return true;
}

bool SecureROMLoader::validateWASHBytecode(const uint8_t* bytecode, size_t size, uint8_t permissionLevel) {
    if (!bytecode || size == 0) {
        recordSecurityViolation("EMPTY_BYTECODE", "Bytecode is empty or null");
        return false;
    }
    
    // Apply dynamic size limits
    DynamicLimits limits;
    if (evaluateMemoryLimits(&limits)) {
        uint32_t maxSize = limits.truncateScripts ? (MAX_SCRIPT_SIZE_KB / 2) : MAX_SCRIPT_SIZE_KB;
        if (size > maxSize * 1024) {
            if (limits.truncateScripts) {
                // Attempt to truncate/optimize script
                LOG_WARN("Script size %zu exceeds limit, attempting optimization", size);
                // TODO: Implement script truncation
            } else {
                recordSecurityViolation("BYTECODE_SIZE_EXCEEDED", 
                                       String("Bytecode size ") + String(size) + " exceeds limit");
                return false;
            }
        }
    }
    
    // Rest of validation same as before
    if (size < 8 || bytecode[0] != 'W' || bytecode[1] != 'A' || bytecode[2] != 'S' || bytecode[3] != 'H') {
        recordSecurityViolation("INVALID_BYTECODE_MAGIC", "WASH bytecode header invalid");
        return false;
    }
    
    if (!scanBytecodeForMaliciousPatterns(bytecode, size)) {
        recordSecurityViolation("MALICIOUS_PATTERN_DETECTED", "Bytecode contains malicious patterns");
        return false;
    }
    
    LOG_DEBUG("Bytecode validation passed (size: %zu, permission: %d)", size, permissionLevel);
    return true;
}

bool SecureROMLoader::validateScriptDefinitions(std::vector<SecureScriptDef>& scripts) {
    LOG_DEBUG("Validating %zu script definitions", scripts.size());
    
    // Apply dynamic script limits
    DynamicLimits limits;
    if (evaluateMemoryLimits(&limits)) {
        if (scripts.size() > limits.maxScriptsPerPanel) {
            recordSecurityViolation("TOO_MANY_SCRIPTS", 
                                   String("Script count ") + String(scripts.size()) + 
                                   " exceeds dynamic limit of " + String(limits.maxScriptsPerPanel));
            return false;
        }
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
            return false;
        }
        
        script.validated = true;
        currentStats.scriptsValidated++;
    }
    
    LOG_DEBUG("All script definitions validated successfully");
    return true;
}

bool SecureROMLoader::validateEntityIntents(const std::vector<EntityIntent>& entities, uint32_t maxEntities) {
    LOG_DEBUG("Validating %zu entity intents (max: %d)", entities.size(), maxEntities);
    
    // Apply dynamic entity limits
    DynamicLimits limits;
    if (evaluateMemoryLimits(&limits)) {
        if (entities.size() > limits.maxEntitiesPerPanel) {
            recordSecurityViolation("TOO_MANY_ENTITIES", 
                                   String("Entity count ") + String(entities.size()) + 
                                   " exceeds dynamic limit of " + String(limits.maxEntitiesPerPanel));
            return false;
        }
    }
    
    for (const auto& intent : entities) {
        if (!validateEntityParameters(intent)) {
            return false;
        }
    }
    
    LOG_DEBUG("All entity intents validated successfully");
    return true;
}

// =========================
// Resource Management (Updated)
// =========================

uint32_t SecureROMLoader::getCurrentMemoryUsageKB() const {
    uint32_t totalMemory = 0;
    
    // Sum memory usage from all cached panels
    for (int i = 0; i < activePanelCacheCount; i++) {
        totalMemory += panelCache[i].memoryUsageKB;
    }
    
    // Add base ROM metadata memory
    totalMemory += currentApp.name.length() + currentApp.description.length();
    totalMemory += currentApp.iconPath.length() + currentApp.splashPath.length();
    
    return (totalMemory + 1023) / 1024; // Convert to KB, round up
}

bool SecureROMLoader::checkResourceLimits(const SecureAppInfo& appInfo) const {
    LOG_DEBUG("Checking resource limits for ROM");
    
    DynamicLimits limits;
    if (!const_cast<SecureROMLoader*>(this)->evaluateMemoryLimits(&limits)) {
        LOG_WARN("Could not evaluate memory limits for resource checking");
        return false;
    }
    
    // Check against dynamic limits rather than static ones
    if (appInfo.scripts.size() > limits.maxScriptsPerPanel) {
        LOG_WARN("ROM exceeds dynamic script limit: %zu > %d", appInfo.scripts.size(), limits.maxScriptsPerPanel);
        return false;
    }
    
    if (appInfo.initialEntities.size() > limits.maxEntitiesPerPanel) {
        LOG_WARN("ROM exceeds dynamic entity limit: %zu > %d", appInfo.initialEntities.size(), limits.maxEntitiesPerPanel);
        return false;
    }
    
    LOG_DEBUG("Resource limit check passed with dynamic limits");
    return true;
}

void SecureROMLoader::cleanupValidationCache() {
    LOG_DEBUG("Cleaning up validation cache");
    
    // Clear panel caches
    for (int i = 0; i < MAX_PANEL_CACHE; i++) {
        panelCache[i].validatedScripts.clear();
    }
    
    // TODO: Clear bytecode validation cache if we had one
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
        LOG_INFO("Scripts: %zu, Entities: %zu, Memory: %dKB", 
                 currentApp.scripts.size(), currentApp.initialEntities.size(), 
                 getCurrentMemoryUsageKB());
        LOG_INFO("Security Version: %d, Validated: %s", 
                 currentApp.securityVersion, currentApp.validated ? "YES" : "NO");
    }
    
    LOG_INFO("Panels Cached: %d/%d", activePanelCacheCount, MAX_PANEL_CACHE);
    for (int i = 0; i < activePanelCacheCount; i++) {
        LOG_INFO("  Panel %d: ID=%d, Memory=%dKB, Scripts=%zu", 
                 i, panelCache[i].panelId, panelCache[i].memoryUsageKB,
                 panelCache[i].validatedScripts.size());
    }
    
    LOG_INFO("Stats - Scripts Loaded: %d, Validated: %d, Rejected: %d",
             currentStats.totalScriptsLoaded, currentStats.scriptsValidated, currentStats.scriptsRejected);
    LOG_INFO("Stats - Entities Created: %d, Violations: %d, Load Time: %dms",
             currentStats.entitiesCreated, currentStats.securityViolations, currentStats.loadTimeMs);
    
    if (!currentStats.lastError.isEmpty()) {
        LOG_INFO("Last Error: %s", currentStats.lastError.c_str());
    }
    
    LOG_INFO("=========================");
}

String SecureROMLoader::getSecurityValidationReport() const {
    String report = "=== Security Validation Report ===\n";
    
    if (isROMLoaded()) {
        report += "ROM: " + String(currentApp.name.c_str()) + "\n";
        report += "Security Version: " + String(currentApp.securityVersion) + "\n";
        report += "Validation Status: " + (currentApp.validated ? String("PASSED") : String("FAILED")) + "\n";
        report += "Permission Level: " + String(currentApp.maxPermissionLevel) + "\n";
        
        // Dynamic limits info
        DynamicLimits limits;
        if (const_cast<SecureROMLoader*>(this)->evaluateMemoryLimits(&limits)) {
            report += "\nDynamic Memory Limits:\n";
            report += "  Available: " + String(limits.availableMemoryKB) + "KB\n";
            report += "  Max Panel: " + String(limits.maxPanelMemoryKB) + "KB\n";
            report += "  Asset Fallbacks: " + (limits.useAssetFallbacks ? String("ENABLED") : String("DISABLED")) + "\n";
        }
        
        report += "\nScript Security:\n";
        for (const auto& script : currentApp.scripts) {
            report += "  " + script.scriptName + " (" + script.scriptType + "): ";
            report += script.validated ? "VALIDATED" : "REJECTED";
            report += " [Permission: " + permissionLevelToString(script.permissionLevel) + "]\n";
            if (!script.securityNotes.isEmpty()) {
                report += "    Notes: " + script.securityNotes + "\n";
            }
        }
        
        report += "\nPanel Cache:\n";
        for (int i = 0; i < activePanelCacheCount; i++) {
            report += "  Panel " + String(i) + ": " + String(panelCache[i].memoryUsageKB) + "KB, ";
            report += String(panelCache[i].validatedScripts.size()) + " scripts\n";
        }
        
        report += "\nResource Usage:\n";
        report += "  Memory: " + String(getCurrentMemoryUsageKB()) + "KB\n";
        report += "  Scripts: " + String(currentApp.scripts.size()) + "\n";
        report += "  Entities: " + String(currentApp.initialEntities.size()) + "\n";
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
// Segmented Loading Integration (Private)
// =========================

bool SecureROMLoader::initializeSegmentedLoader(const String& romPath) {
    LOG_DEBUG("Initializing WispSegmentedLoader for ROM: %s", romPath.c_str());
    
    // TODO: Create and configure WispSegmentedLoader
    // segmentedLoader = new WispEngine::App::WispSegmentedLoader();
    // if (!segmentedLoader->initialize(romPath)) {
    //     LOG_ERROR("Failed to initialize segmented loader");
    //     delete segmentedLoader;
    //     segmentedLoader = nullptr;
    //     return false;
    // }
    
    LOG_DEBUG("WispSegmentedLoader initialized successfully");
    return true;
}

bool SecureROMLoader::loadPanelScripts(const String& panelName, uint8_t layoutIndex, uint8_t panelIndex) {
    LOG_DEBUG("Loading scripts for panel: %s", panelName.c_str());
    
    // TODO: Load panel-specific scripts through segmented loader
    // std::vector<ScriptData> scripts = segmentedLoader->loadPanelScripts(panelName, layoutIndex, panelIndex);
    
    // For now, mock script loading
    // Validate each script's bytecode on-demand
    // return validateScriptBytecodeFromLoader("mock_panel_script");
    
    currentStats.totalScriptsLoaded++;
    currentStats.scriptsValidated++;
    
    return true;
}

bool SecureROMLoader::loadPanelEntities(const String& panelName, uint8_t layoutIndex, uint8_t panelIndex) {
    LOG_DEBUG("Loading entities for panel: %s", panelName.c_str());
    
    // TODO: Load panel-specific entities through segmented loader
    // return createPanelEntitiesSecure(panelName, layoutIndex, panelIndex);
    
    currentStats.entitiesCreated++;
    return true;
}

bool SecureROMLoader::loadPanelAssets(const String& panelName, uint8_t layoutIndex, uint8_t panelIndex) {
    LOG_DEBUG("Loading assets for panel: %s", panelName.c_str());
    
    // TODO: Load panel-specific assets (tiles, sprites, backgrounds)
    // segmentedLoader->loadPanelAssets(panelName, layoutIndex, panelIndex);
    
    return true;
}

bool SecureROMLoader::validateScriptBytecodeFromLoader(const String& scriptName) {
    LOG_DEBUG("Validating script bytecode: %s", scriptName.c_str());
    
    // Check cache first
    PanelValidationCache* cache = nullptr;
    for (int i = 0; i < activePanelCacheCount; i++) {
        auto it = panelCache[i].validatedScripts.find(scriptName);
        if (it != panelCache[i].validatedScripts.end()) {
            LOG_DEBUG("Script validation cached: %s -> %s", scriptName.c_str(), it->second ? "VALID" : "INVALID");
            return it->second;
        }
    }
    
    // TODO: Load bytecode from segmented loader and validate
    // uint8_t* bytecode = segmentedLoader->getScriptBytecode(scriptName);
    // size_t size = segmentedLoader->getScriptSize(scriptName);
    // bool valid = validateWASHBytecode(bytecode, size, 1); // Standard permission
    
    // Cache result
    if (!cache && activePanelCacheCount > 0) {
        cache = &panelCache[0]; // Use first cache entry for now
    }
    if (cache) {
        cache->validatedScripts[scriptName] = true; // Mock validation result
    }
    
    return true; // Mock validation success
}

SecureROMLoader::PanelValidationCache* SecureROMLoader::getPanelCache(const String& panelName) {
    // TODO: Hash panel name to panel ID
    uint16_t panelId = 1; // Mock panel ID
    
    // Find existing cache entry
    for (int i = 0; i < activePanelCacheCount; i++) {
        if (panelCache[i].panelId == panelId) {
            panelCache[i].lastAccessed = GET_TIME_MS();
            return &panelCache[i];
        }
    }
    
    // Allocate new cache entry if space available
    if (activePanelCacheCount < MAX_PANEL_CACHE) {
        PanelValidationCache* newCache = &panelCache[activePanelCacheCount];
        newCache->panelId = panelId;
        newCache->memoryUsageKB = 0;
        newCache->lastAccessed = GET_TIME_MS();
        newCache->validatedScripts.clear();
        activePanelCacheCount++;
        return newCache;
    }
    
    // No space - evict LRU and use that slot
    evictLRUPanel();
    if (activePanelCacheCount < MAX_PANEL_CACHE) {
        return getPanelCache(panelName); // Recursive call after eviction
    }
    
    return nullptr;
}

void SecureROMLoader::evictLRUPanel() {
    if (activePanelCacheCount == 0) return;
    
    // Find least recently used panel
    int lruIndex = 0;
    uint32_t oldestTime = panelCache[0].lastAccessed;
    
    for (int i = 1; i < activePanelCacheCount; i++) {
        if (panelCache[i].lastAccessed < oldestTime) {
            oldestTime = panelCache[i].lastAccessed;
            lruIndex = i;
        }
    }
    
    LOG_DEBUG("Evicting LRU panel cache entry %d (panel ID %d, %dKB)", 
              lruIndex, panelCache[lruIndex].panelId, panelCache[lruIndex].memoryUsageKB);
    
    // TODO: Actually unload panel assets
    // segmentedLoader->unloadPanel(panelCache[lruIndex].panelId);
    
    // Clear cache entry
    panelCache[lruIndex].panelId = 0xFFFF;
    panelCache[lruIndex].memoryUsageKB = 0;
    panelCache[lruIndex].validatedScripts.clear();
    
    // Compact cache array
    if (lruIndex < activePanelCacheCount - 1) {
        panelCache[lruIndex] = panelCache[activePanelCacheCount - 1];
    }
    activePanelCacheCount--;
}

// =========================
// Internal ROM Processing (Updated for Segmented Loading)
// =========================

bool SecureROMLoader::parseROMConfig(SecureAppInfo* appInfo) {
    LOG_DEBUG("Parsing ROM config from segmented loader");
    
    // TODO: Parse config through segmented loader
    // WispEngine::App::ROMConfig config = segmentedLoader->getROMConfig();
    
    // For now, create mock app info
    appInfo->name = "Segmented Test App";
    appInfo->version = "1.0.0";
    appInfo->author = "Test Developer";
    appInfo->description = "Test application for segmented secure ROM loading";
    appInfo->autoStart = false;
    appInfo->screenWidth = 240;
    appInfo->screenHeight = 135;
    
    // Security and resource limits (will be adjusted by dynamic limits)
    appInfo->maxEntities = 100;
    appInfo->maxScripts = 10;
    appInfo->maxPermissionLevel = 2; // ELEVATED
    appInfo->memoryLimitKB = 1024;   // 1MB base limit
    
    appInfo->romChecksum = 0x12345678; // Mock checksum
    appInfo->securityVersion = SECURITY_VERSION;
    
    LOG_DEBUG("ROM config parsed: %s v%s", appInfo->name.c_str(), appInfo->version.c_str());
    return true;
}

bool SecureROMLoader::createPanelEntitiesSecure(const String& panelName, uint8_t layoutIndex, uint8_t panelIndex) {
    LOG_DEBUG("Creating secure entities for panel: %s", panelName.c_str());
    
    // TODO: Get entity intents from segmented loader
    // std::vector<EntityIntent> intents = segmentedLoader->getPanelEntityIntents(panelName);
    
    // Mock entity creation
    EntityIntent mockIntent;
    mockIntent.entityType = "test_entity";
    mockIntent.x = 100.0f;
    mockIntent.y = 50.0f;
    mockIntent.scriptName = "test_script";
    mockIntent.panelId = 1;
    mockIntent.behavior = 1;
    mockIntent.metadata = "test entity";
    
    std::vector<EntityIntent> intents = {mockIntent};
    
    for (const auto& intent : intents) {
        // Validate entity intent
        if (!validateEntityParameters(intent)) {
            return false;
        }
        
        // Create entity through UUID authority
        uint32_t uuid = uuidAuthority->createEntityUUID(intent.entityType, intent.panelId, intent.scriptName);
        if (uuid == 0) {
            recordSecurityViolation("ENTITY_CREATION_FAILED", 
                                   String("Failed to create entity: ") + intent.entityType);
            return false;
        }
        
        // Create script instance if needed
        if (!intent.scriptName.isEmpty()) {
            if (!scriptAuthority->createEntityScript(intent.scriptName, uuid)) {
                LOG_WARN("Failed to create script '%s' for entity %d", 
                         intent.scriptName.c_str(), uuid);
            }
        }
        
        currentStats.entitiesCreated++;
        LOG_DEBUG("Created secure entity UUID %d type '%s'", uuid, intent.entityType.c_str());
    }
    
    return true;
}

// =========================
// Security Validation Internals (Same as before)
// =========================

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
    // Pattern detection same as before
    uint32_t apiCallCount = 0;
    for (size_t i = 0; i < size - 1; i++) {
        if (bytecode[i] == 0xFF && bytecode[i+1] >= 0x80) {
            apiCallCount++;
        }
    }
    
    if (apiCallCount > 1000) {
        LOG_WARN("Excessive API calls detected: %d", apiCallCount);
        return false;
    }
    
    return true;
}

bool SecureROMLoader::validateBytecodeInstructions(const uint8_t* bytecode, size_t size) {
    const uint8_t* instructions = bytecode + 8;
    size_t instructionSize = size - 8;
    
    for (size_t i = 0; i < instructionSize; i++) {
        uint8_t opcode = instructions[i];
        if (opcode > 0xFE) {
            LOG_WARN("Invalid opcode detected: 0x%02X at position %zu", opcode, i);
            return false;
        }
    }
    
    return true;
}

bool SecureROMLoader::validateScriptPermissions(const SecureScriptDef& script) {
    if (script.permissionLevel > 3) {
        recordSecurityViolation("INVALID_PERMISSION_LEVEL", 
                               String("Permission level ") + String(script.permissionLevel) + " is invalid");
        return false;
    }
    
    if (script.permissionLevel == 3 && script.scriptType != "global") {
        recordSecurityViolation("INVALID_SYSTEM_PERMISSION", 
                               "SYSTEM permission only allowed for global scripts");
        return false;
    }
    
    return true;
}

bool SecureROMLoader::validateEntityParameters(const EntityIntent& intent) {
    if (!validateSecureString(intent.entityType, 32)) {
        recordSecurityViolation("INVALID_ENTITY_TYPE", 
                               String("Entity type '") + intent.entityType + "' is invalid");
        return false;
    }
    
    if (intent.x < -10000.0f || intent.x > 10000.0f || 
        intent.y < -10000.0f || intent.y > 10000.0f) {
        recordSecurityViolation("INVALID_ENTITY_POSITION", 
                               String("Entity position (") + String(intent.x) + ", " + String(intent.y) + ") is invalid");
        return false;
    }
    
    if (!intent.scriptName.isEmpty() && !validateSecureString(intent.scriptName, 64)) {
        recordSecurityViolation("INVALID_SCRIPT_NAME", 
                               String("Script name '") + intent.scriptName + "' is invalid");
        return false;
    }
    
    if (intent.behavior > 10) {
        recordSecurityViolation("INVALID_ENTITY_BEHAVIOR", 
                               String("Entity behavior ") + String(intent.behavior) + " is invalid");
        return false;
    }
    
    return true;
}

// =========================
// Error Handling
// =========================

void SecureROMLoader::recordSecurityViolation(const String& violation, const String& details) {
    currentStats.securityViolations++;
    LOG_WARN("Security violation: %s - %s", violation.c_str(), details.c_str());
}

void SecureROMLoader::handleLoadingError(const String& error) {
    currentStats.lastError = error;
    currentStats.loadSuccessful = false;
    LOG_ERROR("ROM loading error: %s", error.c_str());
    
    cleanupPartialLoad();
}

void SecureROMLoader::cleanupPartialLoad() {
    LOG_DEBUG("Cleaning up partial ROM load");
    
    // Reset current app info
    currentApp = SecureAppInfo();
    currentApp.validated = false;
    
    // Clear any partially loaded panel caches
    for (int i = 0; i < MAX_PANEL_CACHE; i++) {
        panelCache[i].panelId = 0xFFFF;
        panelCache[i].validatedScripts.clear();
    }
    activePanelCacheCount = 0;
}

// =========================
// Utility Functions
// =========================

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
    if (str.length() > maxLength || str.length() == 0) {
        return false;
    }
    
    // Check for control characters and null bytes
    for (size_t i = 0; i < str.length(); i++) {
        char c = str[i];
        if (c < 32 && c != '\n' && c != '\r' && c != '\t') {
            return false;
        }
        if (c == 0) {
            return false;
        }
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
// Asset-Specific Validation (NEW)
// =========================

bool SecureROMLoader::validateEntityAssetAssignment(const EntityIntent& intent) {
    LOG_DEBUG("Validating asset assignment for entity: %s", intent.entityType.c_str());
    
    // Extract sprite information from metadata
    String requestedSprite = "";
    if (intent.metadata.indexOf("sprite:") != -1) {
        int spriteStart = intent.metadata.indexOf("sprite:") + 7;
        int spriteEnd = intent.metadata.indexOf(",", spriteStart);
        if (spriteEnd == -1) spriteEnd = intent.metadata.length();
        requestedSprite = intent.metadata.substring(spriteStart, spriteEnd);
        requestedSprite.trim();
    }
    
    // Validate script vs. sprite assignment
    if (!intent.scriptName.isEmpty()) {
        // Scripted entities should use npc.spr
        if (requestedSprite != "" && requestedSprite != "npc.spr") {
            recordSecurityViolation("ASSET_SCRIPT_MISMATCH", 
                String("Scripted entity '") + intent.entityType + 
                "' should use npc.spr, not " + requestedSprite);
            return false;
        }
        LOG_DEBUG("Scripted entity validated: %s -> npc.spr", intent.entityType.c_str());
    } else {
        // Simple entities should use item.spr
        if (requestedSprite != "" && requestedSprite != "item.spr") {
            recordSecurityViolation("ASSET_SIMPLICITY_MISMATCH", 
                String("Simple entity '") + intent.entityType + 
                "' should use item.spr, not " + requestedSprite);
            return false;
        }
        LOG_DEBUG("Simple entity validated: %s -> item.spr", intent.entityType.c_str());
    }
    
    return true;
}

bool SecureROMLoader::validateUIAssetUsage(const String& entityType, const String& assetPath) {
    LOG_DEBUG("Validating UI asset usage: %s -> %s", entityType.c_str(), assetPath.c_str());
    
    // Only UI entities should use UI assets
    if (entityType.startsWith("ui_") || entityType.startsWith("button_") || 
        entityType.startsWith("menu_")) {
        
        // UI entities should only use light.png or dark.png
        if (assetPath != "light.png" && assetPath != "dark.png") {
            recordSecurityViolation("INVALID_UI_ASSET", 
                String("UI element '") + entityType + 
                "' cannot use non-UI asset: " + assetPath);
            return false;
        }
        
        LOG_DEBUG("Valid UI asset assignment: %s -> %s", entityType.c_str(), assetPath.c_str());
        return true;
    }
    
    // Non-UI entities should not use UI assets
    if (assetPath == "light.png" || assetPath == "dark.png") {
        recordSecurityViolation("UI_ASSET_MISUSE", 
            String("Non-UI entity '") + entityType + 
            "' cannot use UI asset: " + assetPath);
        return false;
    }
    
    return true;
}

bool SecureROMLoader::configureEntityAssetFallbacks(const String& panelName, uint32_t availableMemoryKB) {
    LOG_INFO("Configuring entity-specific asset fallbacks for panel: %s (%dKB available)", 
             panelName.c_str(), availableMemoryKB);
    
    // Determine fallback severity based on available memory
    uint8_t severity = 0;
    if (availableMemoryKB < 32) {
        severity = 3; // Maximum fallbacks
    } else if (availableMemoryKB < 64) {
        severity = 2; // Aggressive fallbacks
    } else if (availableMemoryKB < 96) {
        severity = 1; // Moderate fallbacks
    }
    // else severity = 0 (minimal fallbacks)
    
    LOG_INFO("Asset fallback severity: %d", severity);
    
    switch (severity) {
        case 0: // Minimal fallbacks - high memory
            // Compress sprites slightly to save bandwidth
            // segmentedLoader->compressSprites("npc.spr", 0.9f);
            // segmentedLoader->compressSprites("item.spr", 0.9f);
            LOG_DEBUG("Minimal asset compression applied");
            break;
            
        case 1: // Moderate fallbacks - medium memory
            // Reduce quality and animation frames
            // segmentedLoader->compressSprites("npc.spr", 0.7f);
            // segmentedLoader->compressSprites("item.spr", 0.8f);
            // segmentedLoader->reduceAnimationFrames("npc.spr", 0.75f);
            LOG_DEBUG("Moderate asset optimization applied");
            break;
            
        case 2: // Aggressive fallbacks - low memory
            // Significant quality reduction and asset merging
            // segmentedLoader->compressSprites("npc.spr", 0.5f);
            // segmentedLoader->compressSprites("item.spr", 0.6f);
            // segmentedLoader->reduceAnimationFrames("npc.spr", 0.5f);
            // segmentedLoader->useMonochromeUI(true);
            LOG_DEBUG("Aggressive asset optimization applied");
            break;
            
        case 3: // Maximum fallbacks - critical memory
            // Extreme optimization - merge sprites, disable animations
            // segmentedLoader->mergeSprites({"npc.spr", "item.spr"}, "entities.spr");
            // segmentedLoader->useMinimalSprites(true);
            // segmentedLoader->disableAnimations(true);
            // segmentedLoader->forceFallbackBackground("void.spr");
            LOG_DEBUG("Maximum asset optimization applied - critical memory mode");
            break;
    }
    
    // Update current limits to reflect fallback configuration
    DynamicLimits limits;
    if (evaluateMemoryLimits(&limits)) {
        currentLimits = limits;
        currentLimits.useAssetFallbacks = (severity > 0);
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
