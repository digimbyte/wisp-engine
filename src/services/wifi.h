// wifi_manager.h
#pragma once
#include <WiFi.h>
#include <Settings.h>

namespace WiFiManager {

inline bool connectFromSettings(Settings& settings, uint32_t timeoutMs = 10000) {
  String ssid = settings.getWiFiSSID();
  String pass = settings.getWiFiPassword();

  if (ssid.isEmpty()) return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(100);
  }

  return WiFi.status() == WL_CONNECTED;
}

inline void startAccessPoint(const String& apName = "PetDevice_AP", const String& password = "") {
  WiFi.mode(WIFI_AP);
  if (password.length() >= 8) {
    WiFi.softAP(apName.c_str(), password.c_str());
  } else {
    WiFi.softAP(apName.c_str());
  }
}

inline bool isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

inline String getLocalIP() {
  return WiFi.localIP().toString();
}

inline int getRSSI() {
  return WiFi.RSSI();
}

inline void disconnect(bool resetMode = true) {
  WiFi.disconnect(true);
  if (resetMode) WiFi.mode(WIFI_OFF);
}

inline void scanNetworks(std::vector<String>& results) {
  results.clear();
  int count = WiFi.scanNetworks();
  for (int i = 0; i < count; ++i) {
    results.push_back(WiFi.SSID(i));
  }
  WiFi.scanDelete();
}

inline String getConnectionReport() {
  if (!isConnected()) return "WiFi not connected";
  String report = "Connected to ";
  report += WiFi.SSID();
  report += " (";
  report += WiFi.localIP().toString();
  report += ") RSSI: ";
  report += WiFi.RSSI();
  report += "dBm";
  return report;
}

} // namespace WiFiManager
