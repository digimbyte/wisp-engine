// engine/curated_api.cpp
#include "curated_api.h"

// Save system integration methods
// curated_api.cpp - Implementation of WispCuratedAPI
#include "curated_api.h"
#include "../database/save_system.h"

// Constructor - initialize API with proper defaults
WispCuratedAPI::WispCuratedAPI(WispEngine* eng) : 
    engine(eng),
    quota(),
    frameStartTime(0),
    updateStartTime(0),
    renderStartTime(0),
    errorsThisSecond(0),
    lastErrorReset(0),
    emergencyMode(false),
    quotaViolated(false),
    startTime(0), // Replace millis() with 0 since millis is not available
    deltaTime(0)
{
{
    // Initialize app permissions - defaults to restricted
    appPermissions.canLaunchApps = false;      // Most apps cannot launch others
    appPermissions.canAccessNetwork = false;   // Most apps don't need network
    appPermissions.canAccessStorage = false;   // Most apps use curated API only
    appPermissions.canModifySystem = false;    // Most apps cannot change system settings
    
    // Note: These permissions would be set based on app manifest/config
    // For now, they remain restrictive by default for security
}

bool WispCuratedAPI::setAppIdentity(const String& uuid, const String& version, uint32_t saveFormatVersion) {
    if (uuid.empty()) {
        recordError("App UUID cannot be empty");
        return false;
    }
    
    // Validate UUID format (basic reverse domain notation check)
    if (uuid.indexOf('.') == -1 || uuid.length() < 5) {
        recordError("App UUID should use reverse domain notation (e.g. com.developer.gamename)");
        return false;
    }
    
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return false;
    }
    
    WispAppIdentity identity(uuid, version, saveFormatVersion);
    g_SaveSystem->setAppIdentity(identity);
    
    print("App identity set: " + uuid + " v" + version);
    return true;
}

bool WispCuratedAPI::registerSaveField(const String& key, bool* value) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    if (!g_SaveSystem->registerField(key, value)) {
        recordError("Failed to register save field: " + key);
        return false;
    }
    
    return true;
}

bool WispCuratedAPI::registerSaveField(const String& key, int8_t* value) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    return g_SaveSystem->registerField(key, value);
}

bool WispCuratedAPI::registerSaveField(const String& key, uint8_t* value) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    return g_SaveSystem->registerField(key, value);
}

bool WispCuratedAPI::registerSaveField(const String& key, int16_t* value) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    return g_SaveSystem->registerField(key, value);
}

bool WispCuratedAPI::registerSaveField(const String& key, uint16_t* value) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    return g_SaveSystem->registerField(key, value);
}

bool WispCuratedAPI::registerSaveField(const String& key, int32_t* value) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    return g_SaveSystem->registerField(key, value);
}

bool WispCuratedAPI::registerSaveField(const String& key, uint32_t* value) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    return g_SaveSystem->registerField(key, value);
}

bool WispCuratedAPI::registerSaveField(const String& key, float* value) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    return g_SaveSystem->registerField(key, value);
}

bool WispCuratedAPI::registerSaveField(const String& key, String* value, size_t maxLength) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    return g_SaveSystem->registerStringField(key, value, maxLength);
}

bool WispCuratedAPI::registerSaveBlob(const String& key, void* data, size_t size) {
    if (!g_SaveSystem || !data || size == 0) {
        recordError("Invalid save system, null pointer, or zero size");
        return false;
    }
    
    return g_SaveSystem->registerBlobField(key, data, size);
}

template<typename T>
T* WispCuratedAPI::getSaveField(const String& key) {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return nullptr;
    }
    
    return g_SaveSystem->getField<T>(key);
}

String* WispCuratedAPI::getSaveString(const String& key) {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return nullptr;
    }
    
    return g_SaveSystem->getStringField(key);
}

void* WispCuratedAPI::getSaveBlob(const String& key, size_t* outSize) {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return nullptr;
    }
    
    return g_SaveSystem->getBlobField(key, outSize);
}

template<typename T>
bool WispCuratedAPI::setSaveField(const String& key, const T& value) {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return false;
    }
    
    return g_SaveSystem->setField(key, value);
}

bool WispCuratedAPI::setSaveString(const String& key, const String& value) {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return false;
    }
    
    return g_SaveSystem->setStringField(key, value);
}

bool WispCuratedAPI::setSaveBlob(const String& key, const void* data, size_t size) {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return false;
    }
    
    return g_SaveSystem->setBlobField(key, data, size);
}

bool WispCuratedAPI::save() {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return false;
    }
    
    WispSaveResult result = g_SaveSystem->save();
    
    if (result != SAVE_SUCCESS) {
        recordError("Save failed: " + String(getSaveResultString(result)));
        return false;
    }
    
    print("Game saved successfully");
    return true;
}

bool WispCuratedAPI::load() {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return false;
    }
    
    WispSaveResult result = g_SaveSystem->load();
    
    if (result != SAVE_SUCCESS) {
        // For load operations, some errors are not critical
        if (result == SAVE_ERROR_NOT_FOUND) {
            print("No save file found - starting fresh");
            return true; // This is OK for first run
        } else {
            printWarning("Load failed: " + String(getSaveResultString(result)));
            return false;
        }
    }
    
    print("Game loaded successfully");
    return true;
}

bool WispCuratedAPI::resetSaveData() {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return false;
    }
    
    WispSaveResult result = g_SaveSystem->reset();
    
    if (result != SAVE_SUCCESS) {
        recordError("Reset failed: " + String(getSaveResultString(result)));
        return false;
    }
    
    print("Save data reset to defaults");
    return true;
}

bool WispCuratedAPI::hasSaveFile() const {
    if (!g_SaveSystem) {
        return false;
    }
    
    return g_SaveSystem->hasSaveFile();
}

bool WispCuratedAPI::deleteSaveFile() {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return false;
    }
    
    if (g_SaveSystem->deleteSaveFile()) {
        print("Save file deleted");
        return true;
    } else {
        recordError("Failed to delete save file");
        return false;
    }
}

void WispCuratedAPI::enableAutoSave(bool enabled, uint32_t intervalMs) {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return;
    }
    
    g_SaveSystem->setAutoSave(enabled, intervalMs);
    
    if (enabled) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "Auto-save enabled (interval: %dms)", intervalMs);
        print(buffer);
    } else {
        print("Auto-save disabled");
    }
}

bool WispCuratedAPI::isSaveSystemReady() const {
    return g_SaveSystem && g_SaveSystem->isInitialized();
}

uint64_t WispCuratedAPI::getSaveTimestamp() const {
    if (!g_SaveSystem) {
        return 0;
    }
    
    return g_SaveSystem->getSaveTimestamp();
}

size_t WispCuratedAPI::getSaveFileSize() const {
    if (!g_SaveSystem) {
        return 0;
    }
    
    return g_SaveSystem->getSaveFileSize();
}

// === APP MANAGEMENT SYSTEM IMPLEMENTATION ===
// Forward declarations to external components (defined in bootloader)
extern bool launchApp(const String& appPath);

// We need access to the global app manager from bootloader
// This is declared as extern here and defined in bootloader.cpp
extern AppManager appManager;

int WispCuratedAPI::getAvailableApps(char appNames[][WISP_MAX_STRING_LENGTH], int maxApps) {
    // Get available apps from the app manager
    const auto& availableApps = appManager.getAvailableApps();
    int count = 0;
    
    for (const auto& app : availableApps) {
        if (count >= maxApps) break;
        strncpy(appNames[count], app.name.c_str(), WISP_MAX_STRING_LENGTH - 1);
        appNames[count][WISP_MAX_STRING_LENGTH - 1] = '\0';
        count++;
    }
    
    return count;
}

String WispCuratedAPI::getAppDescription(const String& appName) {
    const auto& availableApps = appManager.getAvailableApps();
    for (const auto& app : availableApps) {
        if (app.name == appName) {
            return app.description;
        }
    }
    return "Unknown";
}

String WispCuratedAPI::getAppAuthor(const String& appName) {
    const auto& availableApps = appManager.getAvailableApps();
    for (const auto& app : availableApps) {
        if (app.name == appName) {
            return app.author;
        }
    }
    return "Unknown";
}

String WispCuratedAPI::getAppVersion(const String& appName) {
    const auto& availableApps = appManager.getAvailableApps();
    for (const auto& app : availableApps) {
        if (app.name == appName) {
            return app.version;
        }
    }
    return "Unknown";
}

bool WispCuratedAPI::isAppCompatible(const String& appName) {
    // For now, assume all discovered apps are compatible
    // In a full implementation, this would check system requirements
    const auto& availableApps = appManager.getAvailableApps();
    for (const auto& app : availableApps) {
        if (app.name == appName) {
            return true;
        }
    }
    return false;
}

bool WispCuratedAPI::requestAppLaunch(const String& appName) {
    // Security check: only certain apps can launch other apps
    if (!canLaunchApps()) {
        recordError("App does not have permission to launch other apps");
        return false;
    }
    
    // Find the app path
    const auto& availableApps = appManager.getAvailableApps();
    for (const auto& app : availableApps) {
        if (app.name == appName) {
            // Use the bootloader's launchApp function (bypassing direct app manager access)
            bool result = launchApp(app.executablePath);
            if (result) {
                print("Launched app: " + appName);
            } else {
                recordError("Failed to launch app: " + appName);
            }
            return result;
        }
    }
    
    recordError("App not found: " + appName);
    return false;
}

bool WispCuratedAPI::canLaunchApps() const {
    // Only allow system apps and launchers to launch other apps
    // This prevents random apps from launching arbitrary code
    return appPermissions.canLaunchApps;
}

void WispCuratedAPI::setAppPermissions(bool canLaunch, bool canNetwork, bool canStorage, bool canSystem) {
    // This function should only be called by the system/bootloader
    // In a secure implementation, this would have additional authentication
    appPermissions.canLaunchApps = canLaunch;
    appPermissions.canAccessNetwork = canNetwork;
    appPermissions.canAccessStorage = canStorage;
    appPermissions.canModifySystem = canSystem;
    
    print("App permissions updated - Launch:" + String(canLaunch ? "Y" : "N") +
          " Network:" + String(canNetwork ? "Y" : "N") +
          " Storage:" + String(canStorage ? "Y" : "N") +
          " System:" + String(canSystem ? "Y" : "N"));
}

// === UTILITY FUNCTIONS IMPLEMENTATION ===
uint32_t WispCuratedAPI::getTime() const {
    return millis() - startTime;
}

uint32_t WispCuratedAPI::getDeltaTime() const {
    return deltaTime;
}

float WispCuratedAPI::random(float min, float max) {
    return min + (max - min) * (rand() / (float)RAND_MAX);
}

int WispCuratedAPI::randomInt(int min, int max) {
    return min + (rand() % (max - min + 1));
}

float WispCuratedAPI::lerp(float a, float b, float t) {
    return a + t * (b - a);
}

float WispCuratedAPI::distance(WispVec2 a, WispVec2 b) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    return sqrt(dx * dx + dy * dy);
}

float WispCuratedAPI::angle(WispVec2 from, WispVec2 to) {
    return atan2(to.y - from.y, to.x - from.x);
}

WispVec2 WispCuratedAPI::normalize(WispVec2 vec) {
    float len = sqrt(vec.x * vec.x + vec.y * vec.y);
    if (len > 0.0f) {
        return WispVec2(vec.x / len, vec.y / len);
    }
    return WispVec2(0, 0);
}

// Explicit template instantiations for common types
template bool* WispCuratedAPI::getSaveField<bool>(const String& key);
template int8_t* WispCuratedAPI::getSaveField<int8_t>(const String& key);
template uint8_t* WispCuratedAPI::getSaveField<uint8_t>(const String& key);
template int16_t* WispCuratedAPI::getSaveField<int16_t>(const String& key);
template uint16_t* WispCuratedAPI::getSaveField<uint16_t>(const String& key);
template int32_t* WispCuratedAPI::getSaveField<int32_t>(const String& key);
template uint32_t* WispCuratedAPI::getSaveField<uint32_t>(const String& key);
template float* WispCuratedAPI::getSaveField<float>(const String& key);

template bool WispCuratedAPI::setSaveField<bool>(const String& key, const bool& value);
template bool WispCuratedAPI::setSaveField<int8_t>(const String& key, const int8_t& value);
template bool WispCuratedAPI::setSaveField<uint8_t>(const String& key, const uint8_t& value);
template bool WispCuratedAPI::setSaveField<int16_t>(const String& key, const int16_t& value);
template bool WispCuratedAPI::setSaveField<uint16_t>(const String& key, const uint16_t& value);
template bool WispCuratedAPI::setSaveField<int32_t>(const String& key, const int32_t& value);
template bool WispCuratedAPI::setSaveField<uint32_t>(const String& key, const uint32_t& value);
template bool WispCuratedAPI::setSaveField<float>(const String& key, const float& value);
