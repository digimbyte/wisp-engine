// bluetooth_manager.h - ESP32-C6/S3 Bluetooth Manager using ESP-IDF
// Native ESP32 Bluetooth 5 LE implementation with classic Bluetooth support
#pragma once
#include "../system/esp32_common.h"  // Pure ESP-IDF native headers
#include <Settings.h>
#include <BluetoothSerial.h>

namespace BluetoothManager {

BluetoothSerial SerialBT;
static bool btActive = false;

inline bool beginFromSettings(Settings& settings) {
  if (!btActive) {
    String name = settings.getBluetoothName();
    btActive = SerialBT.begin(name.c_str());
  }
  return btActive;
}

inline bool begin(const String& name = "PetDevice") {
  if (!btActive) {
    btActive = SerialBT.begin(name.c_str());
  }
  return btActive;
}

inline void stop() {
  if (btActive) {
    SerialBT.end();
    btActive = false;
  }
}

inline bool isReady() {
  return btActive && SerialBT.hasClient();
}

inline void send(const String& msg) {
  if (btActive && SerialBT.hasClient()) {
    SerialBT.println(msg);
  }
}

inline String readLine() {
  String input = "";
  while (SerialBT.available()) {
    char c = SerialBT.read();
    if (c == '\n') break;
    input += c;
  }
  return input;
}

inline bool available() {
  return btActive && SerialBT.available();
}

inline void flush() {
  if (btActive) SerialBT.flush();
}

inline String getStatusReport() {
  if (!btActive) return "Bluetooth not started";
  String report = "Bluetooth ready: ";
  report += (SerialBT.hasClient() ? "client connected" : "no client");
  return report;
}

} // namespace BluetoothManager
