// settings.h - ESP32-C6/S3 Settings System using ESP-IDF
// Native ESP32 settings management with NVS and mbedTLS
#pragma once

// Include board-specific feature flags
#ifdef PLATFORM_C6
  #include "../boards/esp32-c6_config.h"
#elif PLATFORM_S3
  #include "../boards/esp32-s3_config.h"
#else
  // Default feature flags for unknown platforms
  #define WISP_HAS_WIFI           0
  #define WISP_HAS_BLUETOOTH      0
  #define WISP_HAS_BLUETOOTH_CLASSIC 0
  #define WISP_HAS_WIFI_DIRECT    0
  #define WISP_HAS_EXTERNAL_STORAGE 0
#endif

#include "esp32_common.h"  // Pure ESP-IDF native headers
#include "nvs_flash.h"
#include "nvs.h"
#include <mbedtls/md5.h>

class Settings {
public:
  nvs_handle_t nvs_handle;
  char deviceId[17]; // 16 hex chars + null terminator

  void begin() {
    esp_err_t err = nvs_open("appcfg", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
      WISP_DEBUG_ERROR("Settings", "Error opening NVS");
      return;
    }
    
    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, "device_id", NULL, &required_size);
    if (err == ESP_OK && required_size > 0) {
      char* device_id_buf = (char*)malloc(required_size);
      nvs_get_str(nvs_handle, "device_id", device_id_buf, &required_size);
      strncpy(deviceId, device_id_buf, sizeof(deviceId) - 1);
      deviceId[sizeof(deviceId) - 1] = '\0';
      free(device_id_buf);
    } else {
      deviceId = generateDeviceId();
      nvs_set_str(nvs_handle, "device_id", deviceId.c_str());
      nvs_commit(nvs_handle);
    }
  }

  void end() {
    nvs_close(nvs_handle);
  }

  // --- HASH UTILITY ---
  void generateDeviceId(char* output, size_t outputSize) {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char raw[32];
    snprintf(raw, sizeof(raw), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    unsigned char hash[16];
    mbedtls_md5((const unsigned char*)raw, strlen(raw), hash);

    char hex[33];
    for (int i = 0; i < 16; ++i) sprintf(hex + i * 2, "%02X", hash[i]);
    hex[32] = '\0';

    snprintf(output, outputSize, "%s", hex);
  }

  void encrypt(const char* data, char* output, size_t outputSize) {
    size_t dataLen = strlen(data);
    size_t deviceIdLen = strlen(deviceId);
    size_t copyLen = (dataLen < outputSize - 1) ? dataLen : outputSize - 1;
    
    for (size_t i = 0; i < copyLen; ++i) {
      output[i] = data[i] ^ deviceId[i % deviceIdLen];
    }
    output[copyLen] = '\0';
  }

  void decrypt(const char* data, char* output, size_t outputSize) {
    encrypt(data, output, outputSize); // XOR is symmetric
  }

  // --- WIFI ---
  void getWiFiSSID(char* output, size_t outputSize) {
    size_t required_size = 0;
    esp_err_t err = nvs_get_str(nvs_handle, "wifi_ssid", NULL, &required_size);
    if (err == ESP_OK && required_size > 0) {
      char* buf = (char*)malloc(required_size);
      nvs_get_str(nvs_handle, "wifi_ssid", buf, &required_size);
      decrypt(buf, output, outputSize);
      free(buf);
    } else {
      output[0] = '\0';
    }
  }

  void setWiFiSSID(const char* ssid) {
    char encrypted[128];
    encrypt(ssid, encrypted, sizeof(encrypted));
    nvs_set_str(nvs_handle, "wifi_ssid", encrypted);
    nvs_commit(nvs_handle);
  }

  void getWiFiPassword(char* output, size_t outputSize) {
    size_t required_size = 0;
    esp_err_t err = nvs_get_str(nvs_handle, "wifi_pass", NULL, &required_size);
    if (err == ESP_OK && required_size > 0) {
      char* buf = (char*)malloc(required_size);
      nvs_get_str(nvs_handle, "wifi_pass", buf, &required_size);
      decrypt(buf, output, outputSize);
      free(buf);
    } else {
      output[0] = '\0';
    }
  }

  void setWiFiPassword(const char* password) {
    char encrypted[128];
    encrypt(password, encrypted, sizeof(encrypted));
    nvs_set_str(nvs_handle, "wifi_pass", encrypted);
    nvs_commit(nvs_handle);
  }

  // --- BLUETOOTH ---
  void getBluetoothName(char* output, size_t outputSize) {
    size_t required_size = 0;
    esp_err_t err = nvs_get_str(nvs_handle, "bt_name", NULL, &required_size);
    if (err == ESP_OK && required_size > 0) {
      char* buf = (char*)malloc(required_size);
      nvs_get_str(nvs_handle, "bt_name", buf, &required_size);
      decrypt(buf, output, outputSize);
      free(buf);
    } else {
      strncpy(output, "PetDevice", outputSize - 1);
      output[outputSize - 1] = '\0';
    }
  }

  void setBluetoothName(const char* name) {
    char encrypted[128];
    encrypt(name, encrypted, sizeof(encrypted));
    nvs_set_str(nvs_handle, "bt_name", encrypted);
    nvs_commit(nvs_handle);
  }

  // --- WIFI ENABLE/DISABLE ---
#if WISP_HAS_WIFI
  bool getWiFiEnabled() {
    uint8_t enabled = 1; // Default: enabled if hardware supports it
    nvs_get_u8(nvs_handle, "wifi_enabled", &enabled);
    return enabled != 0;
  }

  void setWiFiEnabled(bool enabled) {
    nvs_set_u8(nvs_handle, "wifi_enabled", enabled ? 1 : 0);
    nvs_commit(nvs_handle);
  }
#else
  // Stub methods for boards without WiFi
  bool getWiFiEnabled() { return false; }
  void setWiFiEnabled(bool enabled) { /* No-op */ }
#endif

  // --- BLUETOOTH ENABLE/DISABLE ---
#if WISP_HAS_BLUETOOTH
  bool getBluetoothEnabled() {
    uint8_t enabled = 1; // Default: enabled if hardware supports it
    nvs_get_u8(nvs_handle, "bt_enabled", &enabled);
    return enabled != 0;
  }

  void setBluetoothEnabled(bool enabled) {
    nvs_set_u8(nvs_handle, "bt_enabled", enabled ? 1 : 0);
    nvs_commit(nvs_handle);
  }

  String getBluetoothDeviceName() {
    char name[64];
    getBluetoothName(name, sizeof(name));
    return String(name);
  }

  void setBluetoothDeviceName(const String& name) {
    setBluetoothName(name.c_str());
  }
#else
  // Stub methods for boards without Bluetooth
  bool getBluetoothEnabled() { return false; }
  void setBluetoothEnabled(bool enabled) { /* No-op */ }
  String getBluetoothDeviceName() { return "No Bluetooth"; }
  void setBluetoothDeviceName(const String& name) { /* No-op */ }
#endif

  // --- UI THEME ---
  uint16_t getThemePrimaryColor() {
    uint16_t color = 0xFFFF; // default white
    nvs_get_u16(nvs_handle, "theme_primary", &color);
    return color;
  }

  void setThemePrimaryColor(uint16_t color) {
    nvs_set_u16(nvs_handle, "theme_primary", color);
    nvs_commit(nvs_handle);
  }

  uint16_t getThemeAccentColor() {
    uint16_t color = 0x07E0; // default green
    nvs_get_u16(nvs_handle, "theme_accent", &color);
    return color;
  }

  void setThemeAccentColor(uint16_t color) {
    nvs_set_u16(nvs_handle, "theme_accent", color);
    nvs_commit(nvs_handle);
  }

  // --- FEATURE FLAGS ---
  bool isDebugModeEnabled() {
    uint8_t debug_mode = 0;
    nvs_get_u8(nvs_handle, "debug_mode", &debug_mode);
    return debug_mode != 0;
  }

  void setDebugMode(bool enabled) {
    nvs_set_u8(nvs_handle, "debug_mode", enabled ? 1 : 0);
    nvs_commit(nvs_handle);
  }
};
