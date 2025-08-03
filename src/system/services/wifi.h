// wifi_manager.h
#pragma once
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <Settings.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace WiFiManager {

inline bool connectFromSettings(Settings& settings, uint32_t timeoutMs = 10000) {
  String ssid = settings.getWiFiSSID();
  String pass = settings.getWiFiPassword();

  if (ssid.empty()) return false;

  wifi_config_t wifi_config = {};
  strncpy((char*)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid));
  strncpy((char*)wifi_config.sta.password, pass.c_str(), sizeof(wifi_config.sta.password));
  
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  esp_wifi_start();
  esp_wifi_connect();

  uint32_t start = xTaskGetTickCount() * portTICK_PERIOD_MS;
  while (xTaskGetTickCount() * portTICK_PERIOD_MS - start < timeoutMs) {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
      return true; // Connected
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }

  return false; // Connection failed
}

inline void startAccessPoint(const String& apName = "PetDevice_AP", const String& password = "") {
  wifi_config_t ap_config = {};
  strncpy((char*)ap_config.ap.ssid, apName.c_str(), sizeof(ap_config.ap.ssid));
  if (password.length() >= 8) {
    strncpy((char*)ap_config.ap.password, password.c_str(), sizeof(ap_config.ap.password));
    ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
  } else {
    ap_config.ap.authmode = WIFI_AUTH_OPEN;
  }
  ap_config.ap.max_connection = 4;
  
  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_set_config(WIFI_IF_AP, &ap_config);
  esp_wifi_start();
}

inline bool isConnected() {
  wifi_ap_record_t ap_info;
  return esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK;
}

inline String getLocalIP() {
  esp_netif_ip_info_t ip_info;
  esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
    return String(ip4addr_ntoa((ip4_addr_t*)&ip_info.ip));
  }
  return "0.0.0.0";
}

inline int getRSSI() {
  wifi_ap_record_t ap_info;
  if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
    return ap_info.rssi;
  }
  return -100; // Default weak signal
}

inline void disconnect(bool resetMode = true) {
  esp_wifi_disconnect();
  if (resetMode) {
    esp_wifi_stop();
    esp_wifi_set_mode(WIFI_MODE_NULL);
  }
}

inline void scanNetworks(std::vector<String>& results) {
  results.clear();
  
  wifi_scan_config_t scan_config = {};
  esp_wifi_scan_start(&scan_config, true); // Blocking scan
  
  uint16_t ap_count = 0;
  esp_wifi_scan_get_ap_num(&ap_count);
  
  if (ap_count > 0) {
    wifi_ap_record_t* ap_records = (wifi_ap_record_t*)malloc(ap_count * sizeof(wifi_ap_record_t));
    if (ap_records) {
      esp_wifi_scan_get_ap_records(&ap_count, ap_records);
      for (int i = 0; i < ap_count; i++) {
        results.push_back(String((char*)ap_records[i].ssid));
      }
      free(ap_records);
    }
  }
}

inline String getConnectionReport() {
  if (!isConnected()) return "WiFi not connected";
  
  wifi_ap_record_t ap_info;
  String report = "Connected to ";
  if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
    report += String((char*)ap_info.ssid);
  } else {
    report += "Unknown";
  }
  report += " (";
  report += getLocalIP();
  report += ") RSSI: ";
  report += getRSSI();
  report += "dBm";
  return report;
}

} // namespace WiFiManager
