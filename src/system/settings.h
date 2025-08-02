// settings.h - ESP32-C6/S3 Settings System using ESP-IDF
// Native ESP32 settings management with NVS and mbedTLS
#pragma once
#include "esp32_common.h"  // Pure ESP-IDF native headers
#include <Preferences.h>
#include <mbedtls/md5.h>

class Settings {
public:
  Preferences prefs;
  String deviceId;

  void begin() {
    prefs.begin("appcfg", false);
    deviceId = prefs.getString("device_id", "");
    if (deviceId.isEmpty()) {
      deviceId = generateDeviceId();
      prefs.putString("device_id", deviceId);
    }
  }

  void end() {
    prefs.end();
  }

  // --- HASH UTILITY ---
  String generateDeviceId() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char raw[32];
    snprintf(raw, sizeof(raw), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    unsigned char hash[16];
    mbedtls_md5((const unsigned char*)raw, strlen(raw), hash);

    char hex[33];
    for (int i = 0; i < 16; ++i) sprintf(hex + i * 2, "%02X", hash[i]);
    hex[32] = '\0';

    return String(hex);
  }

  String encrypt(const String& data) {
    String out = "";
    for (size_t i = 0; i < data.length(); ++i) {
      out += (char)(data[i] ^ deviceId[i % deviceId.length()]);
    }
    return out;
  }

  String decrypt(const String& data) {
    return encrypt(data); // XOR is symmetric
  }

  // --- WIFI ---
  String getWiFiSSID() {
    return decrypt(prefs.getString("wifi_ssid", ""));
  }

  void setWiFiSSID(const String& ssid) {
    prefs.putString("wifi_ssid", encrypt(ssid));
  }

  String getWiFiPassword() {
    return decrypt(prefs.getString("wifi_pass", ""));
  }

  void setWiFiPassword(const String& password) {
    prefs.putString("wifi_pass", encrypt(password));
  }

  // --- BLUETOOTH ---
  String getBluetoothName() {
    return decrypt(prefs.getString("bt_name", "PetDevice"));
  }

  void setBluetoothName(const String& name) {
    prefs.putString("bt_name", encrypt(name));
  }

  // --- UI THEME ---
  uint16_t getThemePrimaryColor() {
    return prefs.getUShort("theme_primary", 0xFFFF); // default white
  }

  void setThemePrimaryColor(uint16_t color) {
    prefs.putUShort("theme_primary", color);
  }

  uint16_t getThemeAccentColor() {
    return prefs.getUShort("theme_accent", 0x07E0); // default green
  }

  void setThemeAccentColor(uint16_t color) {
    prefs.putUShort("theme_accent", color);
  }

  // --- FEATURE FLAGS ---
  bool isDebugModeEnabled() {
    return prefs.getBool("debug_mode", false);
  }

  void setDebugMode(bool enabled) {
    prefs.putBool("debug_mode", enabled);
  }
};
