// device_management.h - ESP32-C6/S3 Device Management using ESP-IDF
// Native ESP32 device identification, WiFi, and system management
#pragma once
#include "../system/esp32_common.h"  // Pure ESP-IDF native headers
#include "nvs_flash.h"
#include "nvs.h"
#include <mbedtls/md5.h>
#include <esp_system.h>
#include <esp_spi_flash.h>
#include <esp_chip_info.h>
#include <esp_wifi.h>
#include <string>

namespace DeviceManagement {

inline std::string generateDeviceId() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char raw[32];
  snprintf(raw, sizeof(raw), "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  unsigned char hash[16];
  mbedtls_md5((const unsigned char*)raw, strlen(raw), hash);

  char hex[33];
  for (int i = 0; i < 16; ++i) sprintf(hex + i * 2, "%02X", hash[i]);
  hex[32] = '\0';

  char hex[17];
  snprintf(hex, sizeof(hex), "%016llX", chipId);
  return hex;
}

inline std::string ensureDeviceId() {
  Preferences p;
  p.begin("appcfg", false);
  std::string id = p.getString("device_id", "");
  if (id.empty()) {
    id = generateDeviceId();
    p.putString("device_id", id);
  }
  p.end();
  return id;
}

inline std::string getChipInfo() {
  esp_chip_info_t chip;
  esp_chip_info(&chip);

  std::string info = "ESP32-";
  info += std::to_string(chip.model);
  info += " Rev";
  info += std::to_string(chip.revision);
  info += " Cores: ";
  info += std::to_string(chip.cores);
  return info;
}

inline std::string getMacAddress() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return buf;
}

inline uint32_t getUptimeMs() {
  return get_millis();
}

inline size_t getFreeHeap() {
  return esp_get_free_heap_size();
}

inline size_t getPsramSize() {
#ifdef BOARD_HAS_PSRAM
  return esp_himem_get_phys_size();
#else
  return 0;
#endif
}

inline std::string getResetReason() {
  return std::to_string(esp_reset_reason());
}

inline void resetToFactory() {
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("appcfg", NVS_READWRITE, &nvs_handle);
  if (err == ESP_OK) {
    nvs_erase_all(nvs_handle);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
  }
  esp_restart();
}

} // namespace DeviceManagement
