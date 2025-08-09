// engine/curated_api.cpp
#include "../../system/esp32_common.h"
#include "../database/save_system.h"

// Include the header AFTER other includes to avoid namespace conflicts
#include "curated_api.h"

// Use fully qualified names to avoid namespace nesting issues

// Constructor - initialize API with proper defaults
WispCuratedAPI::WispCuratedAPI(WispEngine::Engine* eng) : 
    engine(eng),
    quota(),
    frameStartTime(0),
    updateStartTime(0),
    renderStartTime(0),
    errorsThisSecond(0),
    lastErrorReset(0),
    emergencyMode(false),
    quotaViolated(false),
    startTime(get_millis()), // Initialize with current time from ESP-IDF timer
    deltaTime(0)
{
    // Initialize app permissions - defaults to restricted
    appPermissions.canLaunchApps = false;      // Most apps cannot launch others
    appPermissions.canAccessNetwork = false;   // Most apps don't need network
    appPermissions.canAccessStorage = false;   // Most apps use curated API only
    appPermissions.canModifySystem = false;    // Most apps cannot change system settings
    
    // Note: These permissions would be set based on app manifest/config
    // For now, they remain restrictive by default for security
}

bool WispCuratedAPI::setAppIdentity(const std::string& uuid, const std::string& version, uint32_t saveFormatVersion) {
    if (uuid.empty()) {
        recordError(String("App UUID cannot be empty"));
        return false;
    }
    
    // Validate UUID format (basic reverse domain notation check)
    if (uuid.find('.') == std::string::npos || uuid.length() < 5) {
        recordError(String("App UUID should use reverse domain notation (e.g. com.developer.gamename)"));
        return false;
    }
    
    if (!g_SaveSystem) {
        recordError(String("Save system not initialized"));
        return false;
    }
    
    WispAppIdentity identity(uuid.c_str(), version.c_str(), saveFormatVersion);
    g_SaveSystem->setAppIdentity(identity);
    
    print(String("App identity set: ") + String(uuid) + String(" v") + String(version));
    return true;
}

bool WispCuratedAPI::registerSaveField(const std::string& key, bool* value) {
    if (!g_SaveSystem || !value) {
        recordError(String("Invalid save system or null pointer"));
        return false;
    }
    
    if (!g_SaveSystem->registerField(key.c_str(), value)) {
        recordError(String("Failed to register save field: ") + String(key));
        return false;
    }
    
    return true;
}

bool WispCuratedAPI::registerSaveField(const std::string& key, int8_t* value) {
    if (!g_SaveSystem || !value) {
        recordError(String("Invalid save system or null pointer"));
        return false;
    }
    
    return g_SaveSystem->registerField(key.c_str(), value);
}

bool WispCuratedAPI::registerSaveField(const std::string& key, uint8_t* value) {
    if (!g_SaveSystem || !value) {
        recordError(String("Invalid save system or null pointer"));
        return false;
    }
    
    return g_SaveSystem->registerField(key.c_str(), value);
}

bool WispCuratedAPI::registerSaveField(const std::string& key, int16_t* value) {
    if (!g_SaveSystem || !value) {
        recordError(String("Invalid save system or null pointer"));
        return false;
    }
    
    return g_SaveSystem->registerField(key.c_str(), value);
}

bool WispCuratedAPI::registerSaveField(const std::string& key, uint16_t* value) {
    if (!g_SaveSystem || !value) {
        recordError(String("Invalid save system or null pointer"));
        return false;
    }
    
    return g_SaveSystem->registerField(key.c_str(), value);
}

bool WispCuratedAPI::registerSaveField(const std::string& key, int32_t* value) {
    if (!g_SaveSystem || !value) {
        recordError(String("Invalid save system or null pointer"));
        return false;
    }
    
    return g_SaveSystem->registerField(key.c_str(), value);
}

bool WispCuratedAPI::registerSaveField(const std::string& key, uint32_t* value) {
    if (!g_SaveSystem || !value) {
        recordError(String("Invalid save system or null pointer"));
        return false;
    }
    
    return g_SaveSystem->registerField(key.c_str(), value);
}

bool WispCuratedAPI::registerSaveField(const std::string& key, float* value) {
    if (!g_SaveSystem || !value) {
        recordError(String("Invalid save system or null pointer"));
        return false;
    }
    
    return g_SaveSystem->registerField(key.c_str(), value);
}

bool WispCuratedAPI::registerSaveField(const std::string& key, std::string* value, size_t maxLength) {
    if (!g_SaveSystem || !value) {
        recordError(String("Invalid save system or null pointer"));
        return false;
    }
    
    // TODO: Fix conversion from std::string* to char*
    // return g_SaveSystem->registerStringField(key.c_str(), value, maxLength);
    recordError(String("String field registration not yet implemented"));
    return false;
}

bool WispCuratedAPI::registerSaveBlob(const std::string& key, void* data, size_t size) {
    if (!g_SaveSystem || !data || size == 0) {
        recordError(String("Invalid save system, null pointer, or zero size"));
        return false;
    }
    
    return g_SaveSystem->registerBlobField(key.c_str(), data, size);
}

template<typename T>
T* WispCuratedAPI::getSaveField(const std::string& key) {
    if (!g_SaveSystem) {
        recordError(String("Save system not initialized"));
        return nullptr;
    }
    
    return g_SaveSystem->getField<T>(key.c_str());
}

std::string* WispCuratedAPI::getSaveString(const std::string& key) {
    if (!g_SaveSystem) {
        recordError(String("Save system not initialized"));
        return nullptr;
    }
    
    // TODO: Fix conversion from char* to std::string*
    // return g_SaveSystem->getStringField(key.c_str());
    recordError(String("String field access not yet implemented"));
    return nullptr;
}

void* WispCuratedAPI::getSaveBlob(const std::string& key, size_t* outSize) {
    if (!g_SaveSystem) {
        recordError(String("Save system not initialized"));
        return nullptr;
    }
    
    return g_SaveSystem->getBlobField(key.c_str(), outSize);
}

template<typename T>
bool WispCuratedAPI::setSaveField(const std::string& key, const T& value) {
    if (!g_SaveSystem) {
        recordError(String("Save system not initialized"));
        return false;
    }
    
    return g_SaveSystem->setField(key.c_str(), value);
}

bool WispCuratedAPI::setSaveString(const std::string& key, const std::string& value) {
    if (!g_SaveSystem) {
        recordError(String("Save system not initialized"));
        return false;
    }
    
    // TODO: Fix conversion from std::string to const char*
    // return g_SaveSystem->setStringField(key.c_str(), value);
    return g_SaveSystem->setStringField(key.c_str(), value.c_str());
}

bool WispCuratedAPI::setSaveBlob(const std::string& key, const void* data, size_t size) {
    if (!g_SaveSystem) {
        recordError(String("Save system not initialized"));
        return false;
    }
    
    return g_SaveSystem->setBlobField(key.c_str(), data, size);
}

bool WispCuratedAPI::save() {
    if (!g_SaveSystem) {
        recordError(String("Save system not initialized"));
        return false;
    }
    
    WispSaveResult result = g_SaveSystem->save();
    
    if (result != SAVE_SUCCESS) {
        recordError(String("Save failed: ") + String(getSaveResultString(result)));
        return false;
    }
    
    print("Game saved successfully");
    return true;
}

bool WispCuratedAPI::load() {
    if (!g_SaveSystem) {
        recordError(String("Save system not initialized"));
        return false;
    }
    
    WispSaveResult result = g_SaveSystem->load();
    
    if (result != SAVE_SUCCESS) {
        // For load operations, some errors are not critical
        if (result == SAVE_ERROR_NOT_FOUND) {
            print("No save file found - starting fresh");
            return true; // This is OK for first run
        } else {
            printWarning(String("Load failed: ") + String(getSaveResultString(result)));
            return false;
        }
    }
    
    print("Game loaded successfully");
    return true;
}

bool WispCuratedAPI::resetSaveData() {
    if (!g_SaveSystem) {
        recordError(String("Save system not initialized"));
        return false;
    }
    
    WispSaveResult result = g_SaveSystem->reset();
    
    if (result != SAVE_SUCCESS) {
        recordError(String("Reset failed: ") + String(getSaveResultString(result)));
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
        recordError(String("Save system not initialized"));
        return false;
    }
    
    if (g_SaveSystem->deleteSaveFile()) {
        print("Save file deleted");
        return true;
    } else {
        recordError(String("Failed to delete save file"));
        return false;
    }
}

void WispCuratedAPI::enableAutoSave(bool enabled, uint32_t intervalMs) {
    if (!g_SaveSystem) {
        recordError(String("Save system not initialized"));
        return;
    }
    
    g_SaveSystem->setAutoSave(enabled, intervalMs);
    
    if (enabled) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "Auto-save enabled (interval: %lums)", (unsigned long)intervalMs);
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
extern bool launchApp(const std::string& appPath);

int WispCuratedAPI::getAvailableApps(char appNames[][WISP_MAX_STRING_LENGTH], int maxApps) {
    // TODO: Implement proper app discovery
    // For now, return empty list
    return 0;
}

bool WispCuratedAPI::getAppDescription(const char* appName, char* description, int maxLen) {
    // TODO: Implement app description lookup
    if (!appName || !description || maxLen <= 0) return false;
    strncpy(description, "Demo App Description", maxLen - 1);
    description[maxLen - 1] = '\0';
    return true;
}

bool WispCuratedAPI::getAppAuthor(const char* appName, char* author, int maxLen) {
    // TODO: Implement app author lookup
    if (!appName || !author || maxLen <= 0) return false;
    strncpy(author, "Demo Author", maxLen - 1);
    author[maxLen - 1] = '\0';
    return true;
}

bool WispCuratedAPI::getAppVersion(const char* appName, char* version, int maxLen) {
    // TODO: Implement app version lookup
    if (!appName || !version || maxLen <= 0) return false;
    strncpy(version, "1.0.0", maxLen - 1);
    version[maxLen - 1] = '\0';
    return true;
}

bool WispCuratedAPI::isAppCompatible(const char* appName) {
    // For now, assume all discovered apps are compatible
    // In a full implementation, this would check system requirements
    if (!appName) return false;
    return true; // Simplified implementation
}

bool WispCuratedAPI::requestAppLaunch(const char* appName) {
    // Security check: only certain apps can launch other apps
    if (!canLaunchApps()) {
        recordError(String("App does not have permission to launch other apps"));
        return false;
    }
    
    if (!appName) {
        recordError(String("Invalid app name"));
        return false;
    }
    
    // TODO: Implement proper app discovery and launch
    // For now, return false as a placeholder
    recordError(String("App launch not implemented: ") + String(appName));
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
    
    print(String("App permissions updated - Launch:") + String(canLaunch ? "Y" : "N") +
          String(" Network:") + String(canNetwork ? "Y" : "N") +
          String(" Storage:") + String(canStorage ? "Y" : "N") +
          String(" System:") + String(canSystem ? "Y" : "N"));
}

// === UTILITY FUNCTIONS IMPLEMENTATION ===
uint32_t WispCuratedAPI::getTime() const {
    return get_millis() - startTime;
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

// Debug output functions
void WispCuratedAPI::print(const std::string& message) {
    Serial.print("WISP: ");
    Serial.println(message.c_str());
}

void WispCuratedAPI::printWarning(const std::string& message) {
    Serial.print("WISP WARNING: ");
    Serial.println(message.c_str());
}

void WispCuratedAPI::printError(const std::string& message) {
    Serial.print("WISP ERROR: ");
    Serial.println(message.c_str());
}

// Explicit template instantiations for common types
template bool* WispCuratedAPI::getSaveField<bool>(const std::string& key);
template int8_t* WispCuratedAPI::getSaveField<int8_t>(const std::string& key);
template uint8_t* WispCuratedAPI::getSaveField<uint8_t>(const std::string& key);
template int16_t* WispCuratedAPI::getSaveField<int16_t>(const std::string& key);
template uint16_t* WispCuratedAPI::getSaveField<uint16_t>(const std::string& key);
template int32_t* WispCuratedAPI::getSaveField<int32_t>(const std::string& key);
template uint32_t* WispCuratedAPI::getSaveField<uint32_t>(const std::string& key);
template float* WispCuratedAPI::getSaveField<float>(const std::string& key);

template bool WispCuratedAPI::setSaveField<bool>(const std::string& key, const bool& value);
template bool WispCuratedAPI::setSaveField<int8_t>(const std::string& key, const int8_t& value);
template bool WispCuratedAPI::setSaveField<uint8_t>(const std::string& key, const uint8_t& value);
template bool WispCuratedAPI::setSaveField<int16_t>(const std::string& key, const int16_t& value);
template bool WispCuratedAPI::setSaveField<uint16_t>(const std::string& key, const uint16_t& value);
template bool WispCuratedAPI::setSaveField<int32_t>(const std::string& key, const int32_t& value);
template bool WispCuratedAPI::setSaveField<uint32_t>(const std::string& key, const uint32_t& value);
template bool WispCuratedAPI::setSaveField<float>(const std::string& key, const float& value);

// === BLUETOOTH API IMPLEMENTATIONS ===

bool WispCuratedAPI::isBluetoothSupported() {
    // Include the board-specific Bluetooth configuration
    #include "../connectivity/bluetooth_config.h"
    
    return WISP_HAS_ANY_BLUETOOTH;
}

bool WispCuratedAPI::isBluetoothEnabled() {
    if (!isBluetoothSupported()) {
        return false;
    }
    
    // TODO: Check actual Bluetooth stack status
    // For now, assume it matches hardware capability
    return WISP_HAS_ANY_BLUETOOTH;
}

bool WispCuratedAPI::enableBluetooth(const std::string& deviceName) {
    // Check network access permission
    if (!appPermissions.canAccessNetwork) {
        recordError("App does not have network access permission for Bluetooth");
        return false;
    }
    
    if (!isBluetoothSupported()) {
        recordError(String("Bluetooth not supported on this board (") + String(WISP_BLUETOOTH_TYPE_STRING) + String(")"));
        return false;
    }
    
    // Forward to BluetoothManager with appropriate type detection
    #include "../connectivity/bluetooth_config.h"
    #include "../../system/services/bt.h"
    
    if (BluetoothManager::begin(String(deviceName.c_str()))) {
        print(String("Bluetooth enabled: ") + String(deviceName) + String(" (") + String(WISP_BLUETOOTH_DESCRIPTION) + String(")"));
        return true;
    } else {
        recordError("Failed to initialize Bluetooth");
        return false;
    }
}

void WispCuratedAPI::disableBluetooth() {
    if (!isBluetoothSupported()) {
        // Silently return - no error since there's nothing to disable
        return;
    }
    
    #include "../../system/services/bt.h"
    BluetoothManager::stop();
    print("Bluetooth disabled");
}

bool WispCuratedAPI::startBLEAdvertising(const std::string& deviceName, const std::string& serviceUUID) {
    #include "../connectivity/bluetooth_config.h"
    
    if (!WISP_HAS_BLE) {
        recordError("BLE not supported on this board (board supports: " + String(WISP_BLUETOOTH_TYPE_STRING) + ")");
        return false;
    }
    
    if (!appPermissions.canAccessNetwork) {
        recordError("App does not have network access permission for BLE");
        return false;
    }
    
    // TODO: Implement BLE advertising through BluetoothManager
    recordError("BLE advertising not fully implemented yet");
    return false;
}

void WispCuratedAPI::stopBLEAdvertising() {
    #include "../connectivity/bluetooth_config.h"
    
    if (!WISP_HAS_BLE) {
        // Silently return - no error since BLE not available
        return;
    }
    
    // TODO: Stop BLE advertising
    print("BLE advertising stopped");
}

bool WispCuratedAPI::sendBLEData(const std::string& data) {
    #include "../connectivity/bluetooth_config.h"
    
    if (!WISP_HAS_BLE) {
        recordError("BLE not supported on this board (board supports: " + String(WISP_BLUETOOTH_TYPE_STRING) + ")");
        return false;
    }
    
    if (data.empty()) {
        recordError("Cannot send empty BLE data");
        return false;
    }
    
    if (data.length() > 244) { // BLE MTU limit minus headers
        recordError("BLE data too large (max 244 bytes)");
        return false;
    }
    
    // TODO: Send data via BLE GATT characteristic
    recordError("BLE data transmission not fully implemented yet");
    return false;
}

std::string WispCuratedAPI::receiveBLEData() {
    #include "../connectivity/bluetooth_config.h"
    
    if (!WISP_HAS_BLE) {
        recordError("BLE not supported on this board (board supports: " + String(WISP_BLUETOOTH_TYPE_STRING) + ")");
        return "";
    }
    
    // TODO: Receive data from BLE GATT characteristic
    return "";
}

bool WispCuratedAPI::isBLEConnected() {
    #include "../connectivity/bluetooth_config.h"
    
    if (!WISP_HAS_BLE) {
        return false;
    }
    
    #include "../../system/services/bt.h"
    return BluetoothManager::isReady();
}

bool WispCuratedAPI::startBTEServer(const std::string& deviceName) {
    #include "../connectivity/bluetooth_config.h"
    
    if (!WISP_HAS_BTE) {
        recordError("Bluetooth Classic not supported on this board (board supports: " + String(WISP_BLUETOOTH_TYPE_STRING) + ")");
        return false;
    }
    
    if (!appPermissions.canAccessNetwork) {
        recordError("App does not have network access permission for Bluetooth Classic");
        return false;
    }
    
    #include "../../system/services/bt.h"
    
    if (BluetoothManager::begin(String(deviceName.c_str()))) {
        print(String("Bluetooth Classic server started: ") + String(deviceName));
        return true;
    } else {
        recordError("Failed to start Bluetooth Classic server");
        return false;
    }
}

void WispCuratedAPI::stopBTEServer() {
    #include "../connectivity/bluetooth_config.h"
    
    if (!WISP_HAS_BTE) {
        // Silently return - no error since BTE not available
        return;
    }
    
    #include "../../system/services/bt.h"
    BluetoothManager::stop();
    print("Bluetooth Classic server stopped");
}

bool WispCuratedAPI::sendBTEData(const std::string& data) {
    #include "../connectivity/bluetooth_config.h"
    
    if (!WISP_HAS_BTE) {
        recordError("Bluetooth Classic not supported on this board (board supports: " + String(WISP_BLUETOOTH_TYPE_STRING) + ")");
        return false;
    }
    
    if (data.empty()) {
        recordError("Cannot send empty Bluetooth Classic data");
        return false;
    }
    
    #include "../../system/services/bt.h"
    
    if (!BluetoothManager::isReady()) {
        recordError("Bluetooth Classic not connected");
        return false;
    }
    
    BluetoothManager::send(String(data.c_str()));
    return true;
}

std::string WispCuratedAPI::receiveBTEData() {
    #include "../connectivity/bluetooth_config.h"
    
    if (!WISP_HAS_BTE) {
        recordError("Bluetooth Classic not supported on this board (board supports: " + String(WISP_BLUETOOTH_TYPE_STRING) + ")");
        return "";
    }
    
    #include "../../system/services/bt.h"
    
    if (!BluetoothManager::isReady()) {
        return "";
    }
    
    String data = BluetoothManager::readLine();
    return std::string(data.c_str());
}

bool WispCuratedAPI::isBTEConnected() {
    #include "../connectivity/bluetooth_config.h"
    
    if (!WISP_HAS_BTE) {
        return false;
    }
    
    #include "../../system/services/bt.h"
    return BluetoothManager::isReady();
}

bool WispCuratedAPI::sendBluetoothData(const std::string& data) {
    #include "../connectivity/bluetooth_config.h"
    
    if (!WISP_HAS_ANY_BLUETOOTH) {
        recordError("No Bluetooth support on this board");
        return false;
    }
    
    // Auto-detect and use available Bluetooth type
    if (WISP_HAS_BTE) {
        return sendBTEData(data);
    } else if (WISP_HAS_BLE) {
        return sendBLEData(data);
    }
    
    recordError("No Bluetooth type available for data transmission");
    return false;
}

std::string WispCuratedAPI::receiveBluetoothData() {
    #include "../connectivity/bluetooth_config.h"
    
    if (!WISP_HAS_ANY_BLUETOOTH) {
        recordError("No Bluetooth support on this board");
        return "";
    }
    
    // Auto-detect and use available Bluetooth type
    if (WISP_HAS_BTE) {
        return receiveBTEData();
    } else if (WISP_HAS_BLE) {
        return receiveBLEData();
    }
    
    return "";
}

bool WispCuratedAPI::isBluetoothConnected() {
    #include "../connectivity/bluetooth_config.h"
    
    if (!WISP_HAS_ANY_BLUETOOTH) {
        return false;
    }
    
    // Check connection status for available Bluetooth types
    if (WISP_HAS_BTE && isBTEConnected()) {
        return true;
    }
    
    if (WISP_HAS_BLE && isBLEConnected()) {
        return true;
    }
    
    return false;
}

std::string WispCuratedAPI::getBluetoothStatus() {
    #include "../connectivity/bluetooth_config.h"
    #include "../../system/services/bt.h"
    
    if (!WISP_HAS_ANY_BLUETOOTH) {
        return "Bluetooth not supported on this board";
    }
    
    String status = "Bluetooth (";
    status += String(WISP_BLUETOOTH_TYPE_STRING);
    status += "): ";
    
    if (isBluetoothEnabled()) {
        status += BluetoothManager::getStatusReport();
    } else {
        status += "disabled";
    }
    
    return std::string(status.c_str());
}

// === MISSING GRAPHICS API IMPLEMENTATIONS ===

ResourceHandle WispCuratedAPI::loadSprite(const std::string& filePath) {
    // TODO: Implement sprite loading
    recordError("loadSprite not implemented yet");
    return 0; // Invalid handle
}

void WispCuratedAPI::unloadSprite(ResourceHandle handle) {
    // TODO: Implement sprite unloading
    if (handle == 0) return;
    recordError("unloadSprite not implemented yet");
}

bool WispCuratedAPI::drawRect(float x, float y, float width, float height, WispColor color, uint8_t depth) {
    // TODO: Implement rectangle drawing
    recordError("drawRect not implemented yet");
    return false;
}

bool WispCuratedAPI::drawText(const std::string& text, float x, float y, WispColor color, uint8_t depth) {
    // TODO: Implement text drawing  
    recordError("drawText not implemented yet");
    return false;
}

bool WispCuratedAPI::validateResourceHandle(ResourceHandle resource) {
    // TODO: Implement resource validation
    return resource != 0; // Simple validation for now
}

// === AUDIO API IMPLEMENTATIONS ===

ResourceHandle WispCuratedAPI::loadAudio(const std::string& filePath) {
    if (!checkAudioQuota()) {
        recordError("Audio quota exceeded");
        return 0;
    }
    
    // TODO: Implement audio loading through engine
    recordError("loadAudio not fully implemented yet");
    return 0; // Invalid handle for now
}

void WispCuratedAPI::unloadAudio(ResourceHandle handle) {
    if (handle == 0) return;
    
    // TODO: Implement audio unloading through engine
    recordError("unloadAudio not fully implemented yet");
}

bool WispCuratedAPI::playAudio(ResourceHandle audio, const WispAudioParams& params) {
    if (!checkAudioQuota()) {
        recordError("Audio quota exceeded");
        return false;
    }
    
    if (!validateResourceHandle(audio)) {
        recordError("Invalid audio handle");
        return false;
    }
    
    quota.startAudio();
    
    // TODO: Forward to WispEngine::Audio system
    // WispEngine::Audio::playBGM(...) or WispEngine::Audio::playSFX(...)
    recordError("playAudio not fully implemented yet");
    return false;
}

void WispCuratedAPI::stopAudio(ResourceHandle audio) {
    if (!validateResourceHandle(audio)) {
        recordError("Invalid audio handle");
        return;
    }
    
    // TODO: Forward to WispEngine::Audio::stopBGM() or stopAllSFX()
    recordError("stopAudio not fully implemented yet");
}
