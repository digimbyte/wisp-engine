// bluetooth_manager.h - ESP32-C6/S3 Bluetooth Manager using ESP-IDF
// ESP32-C6: Bluetooth LE only, ESP32-S3: Classic + LE support
#pragma once
#include "../esp32_common.h"  // Pure ESP-IDF native headers

// Include Bluetooth headers only when needed
#ifdef CONFIG_BT_ENABLED
    #include "esp_bt.h"
    #include "esp_bt_main.h"
    #ifdef CONFIG_BT_CLASSIC_ENABLED
        #include "esp_spp_api.h"
        #include "esp_gap_bt_api.h"
    #endif
    #ifdef CONFIG_BT_BLE_ENABLED
        #include "esp_gap_ble_api.h"
        #include "esp_gatts_api.h"
    #endif
#endif

#include <Settings.h>

static const char* TAG = "BluetoothManager";

namespace BluetoothManager {

static bool btActive = false;
static bool clientConnected = false;
static String deviceName = "WispEngine";

#ifdef CONFIG_BT_ENABLED

#ifdef CONFIG_BT_CLASSIC_ENABLED
// ESP32-S3 Classic Bluetooth (SPP) implementation
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    switch (event) {
    case ESP_SPP_INIT_EVT:
        WISP_DEBUG_INFO(TAG, "ESP_SPP_INIT_EVT");
        esp_bt_dev_set_device_name(deviceName.c_str());
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        esp_spp_start_srv(ESP_SPP_SEC_AUTHENTICATE, ESP_SPP_ROLE_SLAVE, 0, "WispEngine");
        break;
    case ESP_SPP_OPEN_EVT:
    case ESP_SPP_SRV_OPEN_EVT:
        WISP_DEBUG_INFO(TAG, "Bluetooth Classic client connected");
        clientConnected = true;
        break;
    case ESP_SPP_CLOSE_EVT:
        WISP_DEBUG_INFO(TAG, "Bluetooth Classic client disconnected");
        clientConnected = false;
        break;
    case ESP_SPP_DATA_IND_EVT:
        WISP_DEBUG_INFO(TAG, "Received data");
        break;
    default:
        break;
    }
}

inline bool beginClassic(const String& name) {
    deviceName = name;
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        WISP_DEBUG_ERROR(TAG, "Initialize controller failed");
        return false;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        WISP_DEBUG_ERROR(TAG, "Enable controller failed");
        return false;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(TAG, "Initialize bluedroid failed: %s", esp_err_to_name(ret));
        return false;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(TAG, "Enable bluedroid failed: %s", esp_err_to_name(ret));
        return false;
    }

    if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) {
        ESP_LOGE(TAG, "spp register failed: %s", esp_err_to_name(ret));
        return false;
    }

    if ((ret = esp_spp_init(ESP_SPP_MODE_CB)) != ESP_OK) {
        ESP_LOGE(TAG, "spp init failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    btActive = true;
    ESP_LOGI(TAG, "Bluetooth Classic initialized as '%s'", deviceName.c_str());
    return true;
}
#endif // CONFIG_BT_CLASSIC_ENABLED

#ifdef CONFIG_BT_BLE_ENABLED
// ESP32-C6/S3 Bluetooth LE implementation
inline bool beginLE(const String& name) {
    deviceName = name;
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "Initialize controller failed: %s", esp_err_to_name(ret));
        return false;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_BLE)) != ESP_OK) {
        ESP_LOGE(TAG, "Enable controller failed: %s", esp_err_to_name(ret));
        return false;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(TAG, "Initialize bluedroid failed: %s", esp_err_to_name(ret));
        return false;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(TAG, "Enable bluedroid failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    btActive = true;
    ESP_LOGI(TAG, "Bluetooth LE initialized as '%s'", deviceName.c_str());
    return true;
}
#endif // CONFIG_BT_BLE_ENABLED

#endif // CONFIG_BT_ENABLED

inline bool beginFromSettings(Settings& settings) {
    if (!btActive) {
        deviceName = settings.getBluetoothName();
        return begin(deviceName);
    }
    return btActive;
}

inline bool begin(const String& name = "WispEngine") {
    if (!btActive) {
#if defined(CONFIG_BT_ENABLED)
    #if defined(CONFIG_BT_CLASSIC_ENABLED)
        // ESP32-S3: Try Classic Bluetooth first
        return beginClassic(name);
    #elif defined(CONFIG_BT_BLE_ENABLED)
        // ESP32-C6: Use Bluetooth LE only
        return beginLE(name);
    #else
        ESP_LOGW(TAG, "Bluetooth support compiled out");
        return false;
    #endif
#else
        ESP_LOGW(TAG, "Bluetooth not enabled in configuration");
        return false;
#endif
    }
    return btActive;
}

inline void stop() {
    if (btActive) {
#ifdef CONFIG_BT_ENABLED
    #ifdef CONFIG_BT_CLASSIC_ENABLED
        esp_spp_deinit();
    #endif
        esp_bluedroid_disable();
        esp_bluedroid_deinit();
        esp_bt_controller_disable();
        esp_bt_controller_deinit();
#endif
        btActive = false;
        clientConnected = false;
        ESP_LOGI(TAG, "Bluetooth stopped");
    }
}

inline bool isReady() {
    return btActive && clientConnected;
}

inline void send(const String& msg) {
    if (btActive && clientConnected) {
        ESP_LOGI(TAG, "BT Send: %s", msg.c_str());
        // Note: Actual data transmission would need handle from connection events
    }
}

inline String readLine() {
    // Note: Would need to implement data buffering from events
    return "";
}

inline bool available() {
    // Note: Would need to implement data buffering from events
    return false;
}

inline void flush() {
    // ESP-IDF doesn't have explicit flush for BT
}

inline String getStatusReport() {
    if (!btActive) return "Bluetooth not started";
    String report = "Bluetooth ready: ";
    report += (clientConnected ? "client connected" : "no client");
    return report;
}

} // namespace BluetoothManager
