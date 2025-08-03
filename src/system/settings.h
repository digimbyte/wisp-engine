// settings.h - ESP32-C6/S3 Settings System using ESP-IDF
// Native ESP32 settings management with NVS and mbedTLS
#pragma once
#include "esp32_common.h"  // Pure ESP-IDF native headers
#include "nvs_flash.h"
#include "nvs.h"
#include <mbedtls/md5.h>

class Settings {
public:
  nvs_handle_t nvs_handle;
  std::string deviceId;

  void begin() {
    esp_err_t err = nvs_open("appcfg", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
      ESP_LOGE("Settings", "Error opening NVS: %s", esp_err_to_name(err));
      return;
    }
    
    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, "device_id", NULL, &required_size);
    if (err == ESP_OK && required_size > 0) {
      char* device_id_buf = (char*)malloc(required_size);
      nvs_get_str(nvs_handle, "device_id", device_id_buf, &required_size);
      deviceId = std::string(device_id_buf);
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
  std::string generateDeviceId() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char raw[32];
    snprintf(raw, sizeof(raw), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    unsigned char hash[16];
    mbedtls_md5((const unsigned char*)raw, strlen(raw), hash);

    char hex[33];
    for (int i = 0; i < 16; ++i) sprintf(hex + i * 2, "%02X", hash[i]);
    hex[32] = '\0';

    return std::string(hex);
  }

  std::string encrypt(const std::string& data) {
    std::string out = "";
    for (size_t i = 0; i < data.length(); ++i) {
      out += (char)(data[i] ^ deviceId[i % deviceId.length()]);
    }
    return out;
  }

  std::string decrypt(const std::string& data) {
    return encrypt(data); // XOR is symmetric
  }

  // --- WIFI ---
  std::string getWiFiSSID() {
    size_t required_size = 0;
    esp_err_t err = nvs_get_str(nvs_handle, "wifi_ssid", NULL, &required_size);
    if (err == ESP_OK && required_size > 0) {
      char* buf = (char*)malloc(required_size);
      nvs_get_str(nvs_handle, "wifi_ssid", buf, &required_size);
      std::string encrypted = std::string(buf);
      free(buf);
      return decrypt(encrypted);
    }
    return "";
  }

  void setWiFiSSID(const std::string& ssid) {
    std::string encrypted = encrypt(ssid);
    nvs_set_str(nvs_handle, "wifi_ssid", encrypted.c_str());
    nvs_commit(nvs_handle);
  }

  std::string getWiFiPassword() {
    size_t required_size = 0;
    esp_err_t err = nvs_get_str(nvs_handle, "wifi_pass", NULL, &required_size);
    if (err == ESP_OK && required_size > 0) {
      char* buf = (char*)malloc(required_size);
      nvs_get_str(nvs_handle, "wifi_pass", buf, &required_size);
      std::string encrypted = std::string(buf);
      free(buf);
      return decrypt(encrypted);
    }
    return "";
  }

  void setWiFiPassword(const std::string& password) {
    std::string encrypted = encrypt(password);
    nvs_set_str(nvs_handle, "wifi_pass", encrypted.c_str());
    nvs_commit(nvs_handle);
  }

  // --- BLUETOOTH ---
  std::string getBluetoothName() {
    size_t required_size = 0;
    esp_err_t err = nvs_get_str(nvs_handle, "bt_name", NULL, &required_size);
    if (err == ESP_OK && required_size > 0) {
      char* buf = (char*)malloc(required_size);
      nvs_get_str(nvs_handle, "bt_name", buf, &required_size);
      std::string encrypted = std::string(buf);
      free(buf);
      return decrypt(encrypted);
    }
    return "PetDevice";
  }

  void setBluetoothName(const std::string& name) {
    std::string encrypted = encrypt(name);
    nvs_set_str(nvs_handle, "bt_name", encrypted.c_str());
    nvs_commit(nvs_handle);
  }

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
