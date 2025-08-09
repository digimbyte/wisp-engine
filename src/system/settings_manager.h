// settings_manager.h - Persistent Settings Manager for Wisp Engine
#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include "system/definitions.h"
#include <string>
#include <map>
#include <cstring>

#ifdef ESP_PLATFORM
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#endif

namespace WispEngine {
namespace System {

enum class SettingsError {
    SUCCESS = 0,
    FILE_NOT_FOUND,
    FLASH_READ_ONLY,
    OUT_OF_SPACE,
    CORRUPTED_DATA,
    FLASH_ERROR,
    PARSE_ERROR,
    ACCESS_DENIED,
    UNKNOWN_ERROR
};

class SettingsManager {
private:
    static SettingsManager* instance;
    static const char* SETTINGS_FILE_PATH;
    static const char* NVS_NAMESPACE;
    static const size_t MAX_VALUE_SIZE = 512;
    
    struct SettingsData {
        // Network settings
        std::string wifi_ssid;
        std::string wifi_password;
        bool wifi_auto_connect = true;
        uint8_t wifi_power = 20;
        
        // Bluetooth settings
        bool bluetooth_enabled = true;
        bool bluetooth_audio_streaming = false;
        std::string bluetooth_device_name;
        std::string bluetooth_device_address;
        
        // Hotspot settings
        bool hotspot_enabled = false;
        std::string hotspot_name = "WispEngine";
        std::string hotspot_password = "wisp1234";
        
        // System settings
        std::string device_name = "wisp-engine";
        bool mdns_enabled = true;
        uint8_t system_version = 1;
        
        // Engine settings
        uint8_t screen_brightness = 255;
        bool auto_sleep_enabled = true;
        uint16_t auto_sleep_minutes = 30;
        bool boot_animation_enabled = true;
        
        // Audio settings
        uint8_t volume_level = 128;
        bool audio_enabled = true;
        uint8_t audio_sample_rate = 44; // 44.1kHz
    } settings;
    
    bool initialized = false;
    bool flash_readonly = false;
    bool use_nvs = true; // Prefer NVS over SPIFFS
    size_t available_space = 0;
    SettingsError last_error = SettingsError::SUCCESS;
    
public:
    static SettingsManager& getInstance() {
        if (!instance) {
            instance = new SettingsManager();
        }
        return *instance;
    }
    
    // Initialize settings system - must be called on boot
    SettingsError init() {
        if (initialized) return SettingsError::SUCCESS;
        
        last_error = SettingsError::SUCCESS;
        
#ifdef ESP_PLATFORM
        // Try to initialize NVS first (preferred)
        if (initNVS() == SettingsError::SUCCESS) {
            use_nvs = true;
            initialized = true;
            ESP_LOGI("Settings", "Using NVS for settings storage");
        } else {
            // Fallback to SPIFFS
            if (initSPIFFS() == SettingsError::SUCCESS) {
                use_nvs = false;
                initialized = true;
                ESP_LOGI("Settings", "Using SPIFFS for settings storage");
            } else {
                ESP_LOGE("Settings", "Failed to initialize both NVS and SPIFFS");
                return SettingsError::FLASH_ERROR;
            }
        }
        
        // Load existing settings or create defaults
        loadSettings();
        
        ESP_LOGI("Settings", "SettingsManager initialized successfully");
#else
        // Non-ESP platform - use file system
        initialized = true;
        loadSettings();
#endif
        
        return SettingsError::SUCCESS;
    }
    
    // Load settings from persistent storage
    SettingsError loadSettings() {
        if (!initialized) return SettingsError::FLASH_ERROR;
        
        if (use_nvs) {
            return loadFromNVS();
        } else {
            return loadFromFile();
        }
    }
    
    // Save settings to persistent storage
    SettingsError saveSettings() {
        if (!initialized) return SettingsError::FLASH_ERROR;
        if (flash_readonly) return SettingsError::FLASH_READ_ONLY;
        
        if (use_nvs) {
            return saveToNVS();
        } else {
            return saveToFile();
        }
    }
    
    // Network Settings Getters/Setters
    std::string getWiFiSSID() const { return settings.wifi_ssid; }
    void setWiFiSSID(const std::string& ssid) { settings.wifi_ssid = ssid; }
    
    std::string getWiFiPassword() const { return settings.wifi_password; }
    void setWiFiPassword(const std::string& password) { settings.wifi_password = password; }
    
    bool getWiFiAutoConnect() const { return settings.wifi_auto_connect; }
    void setWiFiAutoConnect(bool enabled) { settings.wifi_auto_connect = enabled; }
    
    uint8_t getWiFiPower() const { return settings.wifi_power; }
    void setWiFiPower(uint8_t power) { settings.wifi_power = (power > 20) ? 20 : power; }
    
    // Bluetooth Settings
    bool getBluetoothEnabled() const { return settings.bluetooth_enabled; }
    void setBluetoothEnabled(bool enabled) { settings.bluetooth_enabled = enabled; }
    
    bool getBluetoothAudioStreaming() const { return settings.bluetooth_audio_streaming; }
    void setBluetoothAudioStreaming(bool enabled) { settings.bluetooth_audio_streaming = enabled; }
    
    std::string getBluetoothDeviceName() const { return settings.bluetooth_device_name; }
    void setBluetoothDeviceName(const std::string& name) { settings.bluetooth_device_name = name; }
    
    // Hotspot Settings
    bool getHotspotEnabled() const { return settings.hotspot_enabled; }
    void setHotspotEnabled(bool enabled) { settings.hotspot_enabled = enabled; }
    
    std::string getHotspotName() const { return settings.hotspot_name; }
    void setHotspotName(const std::string& name) { settings.hotspot_name = name; }
    
    std::string getHotspotPassword() const { return settings.hotspot_password; }
    void setHotspotPassword(const std::string& password) { settings.hotspot_password = password; }
    
    // System Settings
    std::string getDeviceName() const { return settings.device_name; }
    void setDeviceName(const std::string& name) { settings.device_name = name; }
    
    bool getMDNSEnabled() const { return settings.mdns_enabled; }
    void setMDNSEnabled(bool enabled) { settings.mdns_enabled = enabled; }
    
    // Engine Settings
    uint8_t getScreenBrightness() const { return settings.screen_brightness; }
    void setScreenBrightness(uint8_t brightness) { settings.screen_brightness = brightness; }
    
    bool getAutoSleepEnabled() const { return settings.auto_sleep_enabled; }
    void setAutoSleepEnabled(bool enabled) { settings.auto_sleep_enabled = enabled; }
    
    uint16_t getAutoSleepMinutes() const { return settings.auto_sleep_minutes; }
    void setAutoSleepMinutes(uint16_t minutes) { settings.auto_sleep_minutes = minutes; }
    
    // Audio Settings
    uint8_t getVolumeLevel() const { return settings.volume_level; }
    void setVolumeLevel(uint8_t volume) { settings.volume_level = volume; }
    
    bool getAudioEnabled() const { return settings.audio_enabled; }
    void setAudioEnabled(bool enabled) { settings.audio_enabled = enabled; }
    
    // Status and diagnostics
    SettingsError getLastError() const { return last_error; }
    bool isFlashReadOnly() const { return flash_readonly; }
    size_t getAvailableSpace() const { return available_space; }
    bool isUsingNVS() const { return use_nvs; }
    
    // Utility functions
    SettingsError resetToDefaults() {
        settings = SettingsData{}; // Reset to default values
        return saveSettings();
    }
    
    SettingsError exportSettings(std::string& output) {
        output = generateConfigString();
        return SettingsError::SUCCESS;
    }
    
    std::string getErrorString(SettingsError error) const {
        switch (error) {
            case SettingsError::SUCCESS: return "Success";
            case SettingsError::FILE_NOT_FOUND: return "Settings file not found";
            case SettingsError::FLASH_READ_ONLY: return "Flash storage is read-only";
            case SettingsError::OUT_OF_SPACE: return "Insufficient storage space";
            case SettingsError::CORRUPTED_DATA: return "Settings data corrupted";
            case SettingsError::FLASH_ERROR: return "Flash storage error";
            case SettingsError::PARSE_ERROR: return "Settings parse error";
            case SettingsError::ACCESS_DENIED: return "Storage access denied";
            default: return "Unknown error";
        }
    }
    
private:
    SettingsManager() = default;
    ~SettingsManager() = default;
    
#ifdef ESP_PLATFORM
    SettingsError initNVS() {
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            // NVS partition was truncated and needs to be erased
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        
        if (ret != ESP_OK) {
            ESP_LOGE("Settings", "Failed to initialize NVS: %s", esp_err_to_name(ret));
            return SettingsError::FLASH_ERROR;
        }
        
        // Test NVS access
        nvs_handle_t nvs_handle;
        ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
        if (ret == ESP_OK) {
            nvs_close(nvs_handle);
            return SettingsError::SUCCESS;
        } else if (ret == ESP_ERR_NVS_NOT_FOUND) {
            // Namespace doesn't exist yet - that's OK
            return SettingsError::SUCCESS;
        } else {
            ESP_LOGE("Settings", "Failed to test NVS access: %s", esp_err_to_name(ret));
            return SettingsError::ACCESS_DENIED;
        }
    }
    
    SettingsError initSPIFFS() {
        esp_vfs_spiffs_conf_t conf = {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = true
        };
        
        esp_err_t ret = esp_vfs_spiffs_register(&conf);
        if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                ESP_LOGE("Settings", "Failed to mount SPIFFS filesystem");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                ESP_LOGE("Settings", "Failed to find SPIFFS partition");
            } else {
                ESP_LOGE("Settings", "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
            }
            return SettingsError::FLASH_ERROR;
        }
        
        // Check available space
        size_t total = 0, used = 0;
        ret = esp_spiffs_info(NULL, &total, &used);
        if (ret == ESP_OK) {
            available_space = total - used;
            ESP_LOGI("Settings", "SPIFFS: %d KB total, %d KB used, %d KB available", 
                    total / 1024, used / 1024, available_space / 1024);
        }
        
        return SettingsError::SUCCESS;
    }
    
    SettingsError loadFromNVS() {
        nvs_handle_t nvs_handle;
        esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            // First run - create defaults
            ESP_LOGI("Settings", "No existing settings found, using defaults");
            return saveToNVS(); // Save defaults
        } else if (err != ESP_OK) {
            ESP_LOGE("Settings", "Error opening NVS handle: %s", esp_err_to_name(err));
            return SettingsError::ACCESS_DENIED;
        }
        
        // Load settings from NVS
        size_t required_size;
        
        // String settings
        required_size = settings.wifi_ssid.capacity();
        char* buffer = new char[MAX_VALUE_SIZE];
        
        required_size = MAX_VALUE_SIZE;
        if (nvs_get_str(nvs_handle, "wifi_ssid", buffer, &required_size) == ESP_OK) {
            settings.wifi_ssid = std::string(buffer, required_size - 1);
        }
        
        required_size = MAX_VALUE_SIZE;
        if (nvs_get_str(nvs_handle, "wifi_pass", buffer, &required_size) == ESP_OK) {
            settings.wifi_password = std::string(buffer, required_size - 1);
        }
        
        required_size = MAX_VALUE_SIZE;
        if (nvs_get_str(nvs_handle, "hotspot_name", buffer, &required_size) == ESP_OK) {
            settings.hotspot_name = std::string(buffer, required_size - 1);
        }
        
        required_size = MAX_VALUE_SIZE;
        if (nvs_get_str(nvs_handle, "device_name", buffer, &required_size) == ESP_OK) {
            settings.device_name = std::string(buffer, required_size - 1);
        }
        
        delete[] buffer;
        
        // Boolean and numeric settings
        uint8_t bool_val;
        if (nvs_get_u8(nvs_handle, "wifi_auto", &bool_val) == ESP_OK) {
            settings.wifi_auto_connect = bool_val != 0;
        }
        if (nvs_get_u8(nvs_handle, "wifi_power", &settings.wifi_power) != ESP_OK) {
            settings.wifi_power = 20; // Default
        }
        if (nvs_get_u8(nvs_handle, "bt_enabled", &bool_val) == ESP_OK) {
            settings.bluetooth_enabled = bool_val != 0;
        }
        if (nvs_get_u8(nvs_handle, "bt_audio", &bool_val) == ESP_OK) {
            settings.bluetooth_audio_streaming = bool_val != 0;
        }
        if (nvs_get_u8(nvs_handle, "hotspot_en", &bool_val) == ESP_OK) {
            settings.hotspot_enabled = bool_val != 0;
        }
        if (nvs_get_u8(nvs_handle, "brightness", &settings.screen_brightness) != ESP_OK) {
            settings.screen_brightness = 255; // Default
        }
        if (nvs_get_u8(nvs_handle, "volume", &settings.volume_level) != ESP_OK) {
            settings.volume_level = 128; // Default
        }
        
        nvs_close(nvs_handle);
        ESP_LOGI("Settings", "Settings loaded from NVS successfully");
        return SettingsError::SUCCESS;
    }
    
    SettingsError saveToNVS() {
        nvs_handle_t nvs_handle;
        esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE("Settings", "Error opening NVS handle for write: %s", esp_err_to_name(err));
            return SettingsError::ACCESS_DENIED;
        }
        
        // Save string settings
        nvs_set_str(nvs_handle, "wifi_ssid", settings.wifi_ssid.c_str());
        nvs_set_str(nvs_handle, "wifi_pass", settings.wifi_password.c_str());
        nvs_set_str(nvs_handle, "hotspot_name", settings.hotspot_name.c_str());
        nvs_set_str(nvs_handle, "hotspot_pass", settings.hotspot_password.c_str());
        nvs_set_str(nvs_handle, "device_name", settings.device_name.c_str());
        nvs_set_str(nvs_handle, "bt_dev_name", settings.bluetooth_device_name.c_str());
        
        // Save boolean and numeric settings
        nvs_set_u8(nvs_handle, "wifi_auto", settings.wifi_auto_connect ? 1 : 0);
        nvs_set_u8(nvs_handle, "wifi_power", settings.wifi_power);
        nvs_set_u8(nvs_handle, "bt_enabled", settings.bluetooth_enabled ? 1 : 0);
        nvs_set_u8(nvs_handle, "bt_audio", settings.bluetooth_audio_streaming ? 1 : 0);
        nvs_set_u8(nvs_handle, "hotspot_en", settings.hotspot_enabled ? 1 : 0);
        nvs_set_u8(nvs_handle, "mdns_en", settings.mdns_enabled ? 1 : 0);
        nvs_set_u8(nvs_handle, "brightness", settings.screen_brightness);
        nvs_set_u8(nvs_handle, "auto_sleep", settings.auto_sleep_enabled ? 1 : 0);
        nvs_set_u16(nvs_handle, "sleep_min", settings.auto_sleep_minutes);
        nvs_set_u8(nvs_handle, "volume", settings.volume_level);
        nvs_set_u8(nvs_handle, "audio_en", settings.audio_enabled ? 1 : 0);
        nvs_set_u8(nvs_handle, "version", settings.system_version);
        
        // Commit changes
        err = nvs_commit(nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE("Settings", "Error committing NVS changes: %s", esp_err_to_name(err));
            nvs_close(nvs_handle);
            return SettingsError::FLASH_ERROR;
        }
        
        nvs_close(nvs_handle);
        ESP_LOGI("Settings", "Settings saved to NVS successfully");
        return SettingsError::SUCCESS;
    }
#endif
    
    SettingsError loadFromFile() {
        // Load from settings.ini file
        std::string filepath = "/spiffs/settings.ini";
        
#ifdef ESP_PLATFORM
        FILE* f = fopen(filepath.c_str(), "r");
        if (f == NULL) {
            ESP_LOGI("Settings", "No settings file found, creating defaults");
            return saveToFile(); // Create default file
        }
        
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            parseLine(line);
        }
        
        fclose(f);
        ESP_LOGI("Settings", "Settings loaded from file successfully");
#endif
        return SettingsError::SUCCESS;
    }
    
    SettingsError saveToFile() {
        std::string filepath = "/spiffs/settings.ini";
        
#ifdef ESP_PLATFORM
        FILE* f = fopen(filepath.c_str(), "w");
        if (f == NULL) {
            ESP_LOGE("Settings", "Cannot create settings file");
            flash_readonly = true;
            return SettingsError::FLASH_READ_ONLY;
        }
        
        std::string config = generateConfigString();
        if (fwrite(config.c_str(), 1, config.length(), f) != config.length()) {
            ESP_LOGE("Settings", "Error writing settings file");
            fclose(f);
            return SettingsError::FLASH_ERROR;
        }
        
        fclose(f);
        ESP_LOGI("Settings", "Settings saved to file successfully");
#endif
        return SettingsError::SUCCESS;
    }
    
    void parseLine(const std::string& line) {
        size_t pos = line.find('=');
        if (pos == std::string::npos) return;
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        // Remove whitespace and newlines
        key.erase(key.find_last_not_of(" \t\r\n") + 1);
        value.erase(value.find_last_not_of(" \t\r\n") + 1);
        
        // Parse key-value pairs
        if (key == "wifi_ssid") settings.wifi_ssid = value;
        else if (key == "wifi_password") settings.wifi_password = value;
        else if (key == "wifi_auto_connect") settings.wifi_auto_connect = (value == "true" || value == "1");
        else if (key == "wifi_power") settings.wifi_power = std::stoi(value);
        else if (key == "bluetooth_enabled") settings.bluetooth_enabled = (value == "true" || value == "1");
        else if (key == "bluetooth_audio") settings.bluetooth_audio_streaming = (value == "true" || value == "1");
        else if (key == "hotspot_enabled") settings.hotspot_enabled = (value == "true" || value == "1");
        else if (key == "hotspot_name") settings.hotspot_name = value;
        else if (key == "hotspot_password") settings.hotspot_password = value;
        else if (key == "device_name") settings.device_name = value;
        else if (key == "screen_brightness") settings.screen_brightness = std::stoi(value);
        else if (key == "volume_level") settings.volume_level = std::stoi(value);
    }
    
    std::string generateConfigString() {
        std::string config;
        config += "# Wisp Engine Settings Configuration\n";
        config += "# Generated automatically - do not edit manually\n\n";
        
        config += "[Network]\n";
        config += "wifi_ssid=" + settings.wifi_ssid + "\n";
        config += "wifi_password=" + settings.wifi_password + "\n";
        config += "wifi_auto_connect=" + std::string(settings.wifi_auto_connect ? "true" : "false") + "\n";
        config += "wifi_power=" + std::to_string(settings.wifi_power) + "\n\n";
        
        config += "[Bluetooth]\n";
        config += "bluetooth_enabled=" + std::string(settings.bluetooth_enabled ? "true" : "false") + "\n";
        config += "bluetooth_audio=" + std::string(settings.bluetooth_audio_streaming ? "true" : "false") + "\n";
        config += "bluetooth_device_name=" + settings.bluetooth_device_name + "\n\n";
        
        config += "[Hotspot]\n";
        config += "hotspot_enabled=" + std::string(settings.hotspot_enabled ? "true" : "false") + "\n";
        config += "hotspot_name=" + settings.hotspot_name + "\n";
        config += "hotspot_password=" + settings.hotspot_password + "\n\n";
        
        config += "[System]\n";
        config += "device_name=" + settings.device_name + "\n";
        config += "mdns_enabled=" + std::string(settings.mdns_enabled ? "true" : "false") + "\n";
        config += "screen_brightness=" + std::to_string(settings.screen_brightness) + "\n";
        config += "auto_sleep_enabled=" + std::string(settings.auto_sleep_enabled ? "true" : "false") + "\n";
        config += "auto_sleep_minutes=" + std::to_string(settings.auto_sleep_minutes) + "\n\n";
        
        config += "[Audio]\n";
        config += "volume_level=" + std::to_string(settings.volume_level) + "\n";
        config += "audio_enabled=" + std::string(settings.audio_enabled ? "true" : "false") + "\n";
        config += "audio_sample_rate=" + std::to_string(settings.audio_sample_rate) + "\n\n";
        
        config += "system_version=" + std::to_string(settings.system_version) + "\n";
        
        return config;
    }
};

// Static member definitions
SettingsManager* SettingsManager::instance = nullptr;
const char* SettingsManager::SETTINGS_FILE_PATH = "/spiffs/settings.ini";
const char* SettingsManager::NVS_NAMESPACE = "wisp_settings";

} // namespace System
} // namespace WispEngine

#endif // SETTINGS_MANAGER_H
