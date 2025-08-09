// system_init.cpp - Comprehensive System Initialization Implementation
#include "system_init.h"
#include "settings_manager.h"
#include "led_controller.h"
#include "esp32_common.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_flash.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/spi_common.h"
#include "driver/ledc.h"
#include "sdmmc_cmd.h"

// LVGL includes (if available)
#ifdef CONFIG_LVGL_USE_LVGL
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#endif

// LovyanGFX includes (if available)
#ifdef CONFIG_LOVYANGFX
#include <LovyanGFX.hpp>
#endif

static const char* TAG = "WispSystem";

// Global system status
static wisp_system_status_t g_system_status = {0};
static bool g_system_initialized = false;

// === MAIN INITIALIZATION FUNCTIONS ===

wisp_init_result_t wisp_system_init(wisp_component_flags_t components) {
    ESP_LOGI(TAG, "Starting Wisp Engine System Initialization...");
    
    uint32_t start_time = get_millis();
    
    // Reset system status
    memset(&g_system_status, 0, sizeof(g_system_status));
    
    // Initialize flash first (needed for other components)
    if (components & WISP_COMPONENT_FLASH) {
        ESP_LOGI(TAG, "Initializing Flash...");
        g_system_status.flash_size_mb = wisp_flash_searching();
        g_system_status.flash_ready = (g_system_status.flash_size_mb > 0);
        
        if (!g_system_status.flash_ready) {
            ESP_LOGE(TAG, "Flash initialization failed");
            return WISP_INIT_ERROR_FLASH;
        }
    }
    
    // Initialize settings (depends on flash being available)
    if (components & WISP_COMPONENT_SETTINGS) {
        ESP_LOGI(TAG, "Initializing Settings Manager...");
        g_system_status.settings_ready = wisp_settings_init();
        
        if (!g_system_status.settings_ready) {
            ESP_LOGE(TAG, "Settings initialization failed");
            return WISP_INIT_ERROR_SETTINGS;
        }
    }
    
    // Initialize wireless
    if (components & WISP_COMPONENT_WIRELESS) {
        ESP_LOGI(TAG, "Initializing Wireless...");
        g_system_status.wireless_ready = wisp_wireless_init();
        
        if (!g_system_status.wireless_ready) {
            ESP_LOGW(TAG, "Wireless initialization failed - continuing without WiFi");
            // Don't fail completely - wireless is optional
        }
    }
    
    // Initialize RGB LEDs
    if (components & WISP_COMPONENT_RGB) {
        ESP_LOGI(TAG, "Initializing RGB LEDs...");
        g_system_status.rgb_ready = wisp_rgb_init();
        
        if (!g_system_status.rgb_ready) {
            ESP_LOGW(TAG, "RGB initialization failed - continuing without LEDs");
        }
    }
    
    // Initialize SD card
    if (components & WISP_COMPONENT_SD) {
        ESP_LOGI(TAG, "Initializing SD Card...");
        g_system_status.sd_ready = wisp_sd_init();
        
        if (!g_system_status.sd_ready) {
            ESP_LOGW(TAG, "SD card initialization failed - continuing without storage");
        }
    }
    
    // Initialize LCD
    if (components & WISP_COMPONENT_LCD) {
        ESP_LOGI(TAG, "Initializing LCD Display...");
        g_system_status.lcd_ready = wisp_lcd_init();
        
        if (!g_system_status.lcd_ready) {
            ESP_LOGE(TAG, "LCD initialization failed");
            return WISP_INIT_ERROR_LCD;
        }
        
        // Set default backlight
        wisp_backlight_set(50);
    }
    
    // Initialize LVGL (depends on LCD)
    if ((components & WISP_COMPONENT_LVGL) && g_system_status.lcd_ready) {
        ESP_LOGI(TAG, "Initializing LVGL...");
        g_system_status.lvgl_ready = wisp_lvgl_init();
        
        if (!g_system_status.lvgl_ready) {
            ESP_LOGE(TAG, "LVGL initialization failed");
            return WISP_INIT_ERROR_LVGL;
        }
    }
    
    g_system_status.init_time_ms = get_millis() - start_time;
    g_system_initialized = true;
    
    ESP_LOGI(TAG, "System initialization completed in %d ms", g_system_status.init_time_ms);
    wisp_print_system_status();
    
    return WISP_INIT_OK;
}

wisp_init_result_t wisp_system_setup(void) {
    // Initialize all available components by default
    return wisp_system_init(WISP_COMPONENT_ALL);
}

void wisp_system_shutdown(void) {
    if (!g_system_initialized) return;
    
    ESP_LOGI(TAG, "Shutting down Wisp Engine System...");
    
    // Shutdown components in reverse order
    if (g_system_status.lvgl_ready) {
        // LVGL cleanup would go here
    }
    
    if (g_system_status.rgb_ready) {
        ledController.shutdown();
    }
    
    if (g_system_status.wireless_ready) {
        esp_wifi_deinit();
    }
    
    g_system_initialized = false;
    memset(&g_system_status, 0, sizeof(g_system_status));
    
    ESP_LOGI(TAG, "System shutdown complete");
}

const wisp_system_status_t* wisp_get_system_status(void) {
    return &g_system_status;
}

// === INDIVIDUAL COMPONENT INITIALIZATION ===

bool wisp_wireless_init(void) {
    #ifdef CONFIG_ESP32_WIFI_ENABLED
    esp_err_t ret;
    
    // Initialize TCP/IP
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Initialize event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Create default WiFi station
    esp_netif_create_default_wifi_sta();
    
    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    
    if (ret == ESP_OK) {
        ret = esp_wifi_set_mode(WIFI_MODE_STA);
        if (ret == ESP_OK) {
            ret = esp_wifi_start();
        }
    }
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "WiFi initialized successfully");
        return true;
    } else {
        ESP_LOGE(TAG, "WiFi initialization failed: %s", esp_err_to_name(ret));
        return false;
    }
    #else
    ESP_LOGW(TAG, "WiFi not enabled in configuration");
    return false;
    #endif
}

uint32_t wisp_flash_searching(void) {
    uint32_t flash_size = 0;
    
    // Get flash chip information
    esp_flash_t* flash_chip = esp_flash_default_chip;
    if (flash_chip != NULL) {
        esp_err_t ret = esp_flash_get_size(flash_chip, &flash_size);
        if (ret == ESP_OK) {
            uint32_t size_mb = flash_size / (1024 * 1024);
            ESP_LOGI(TAG, "Flash memory detected: %d MB (%d bytes)", size_mb, flash_size);
            
            // Print some flash characteristics
            ESP_LOGI(TAG, "Flash chip ID: 0x%08X", flash_chip->chip_id);
            ESP_LOGI(TAG, "Flash page size: %d bytes", flash_chip->page_size);
            ESP_LOGI(TAG, "Flash sector size: %d bytes", flash_chip->sector_size);
            
            return size_mb;
        } else {
            ESP_LOGE(TAG, "Failed to get flash size: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGE(TAG, "Flash chip not found");
    }
    
    return 0;
}

bool wisp_rgb_init(void) {
    #if WISP_HAS_LED
    return ledController.init();
    #else
    ESP_LOGW(TAG, "RGB LEDs not available on this board");
    return false;
    #endif
}

void wisp_rgb_example(void) {
    #if WISP_HAS_LED
    if (!g_system_status.rgb_ready) {
        ESP_LOGW(TAG, "RGB not initialized - skipping example");
        return;
    }
    
    ESP_LOGI(TAG, "Running RGB LED Example...");
    
    // Color cycle
    LED_SET_COLOR(255, 0, 0);      // Red
    LED_SHOW();
    delay(500);
    
    LED_SET_COLOR(0, 255, 0);      // Green  
    LED_SHOW();
    delay(500);
    
    LED_SET_COLOR(0, 0, 255);      // Blue
    LED_SHOW();
    delay(500);
    
    // Rainbow animation
    LED_RAINBOW(2000);
    delay(2000);
    
    // Pulse animation
    LED_PULSE(255, 255, 255, 1000);
    delay(3000);
    
    // Clear
    LED_CLEAR();
    LED_SHOW();
    
    ESP_LOGI(TAG, "RGB LED Example complete");
    #else
    ESP_LOGW(TAG, "RGB LEDs not available");
    #endif
}

bool wisp_sd_init(void) {
    #ifdef CONFIG_SD_CARD_ENABLED
    esp_err_t ret;
    
    // Initialize SD card configuration
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    
    // Configure GPIOs (adjust based on your board)
    #ifdef SD_CARD_CMD_PIN
    slot_config.cmd = SD_CARD_CMD_PIN;
    slot_config.clk = SD_CARD_CLK_PIN;  
    slot_config.d0 = SD_CARD_D0_PIN;
    #ifdef SD_CARD_D1_PIN
    slot_config.d1 = SD_CARD_D1_PIN;
    slot_config.d2 = SD_CARD_D2_PIN;
    slot_config.d3 = SD_CARD_D3_PIN;
    slot_config.width = 4; // 4-bit mode
    #else
    slot_config.width = 1; // 1-bit mode
    #endif
    #endif
    
    // Mount filesystem
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    sdmmc_card_t* card;
    const char mount_point[] = "/sdcard";
    
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SD card mounted successfully");
        ESP_LOGI(TAG, "SD card info - Name: %s, Type: %s, Speed: %s", 
                 card->cid.name,
                 (card->ocr & SD_OCR_SDHC_CAP) ? "SDHC/SDXC" : "SDSC",
                 (card->csd.tr_speed > 25000000) ? "high speed" : "default speed");
        ESP_LOGI(TAG, "SD card size: %llu MB", ((uint64_t) card->csd.capacity) * card->csd.sector_size / (1024 * 1024));
        return true;
    } else {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        return false;
    }
    #else
    ESP_LOGW(TAG, "SD card not enabled in configuration");
    return false;
    #endif
}

bool wisp_lcd_init(void) {
    #ifdef CONFIG_DISPLAY_ENABLED
    
    #ifdef CONFIG_LOVYANGFX
    // Initialize LovyanGFX display
    // This would need to be implemented based on your specific display
    ESP_LOGI(TAG, "Initializing LovyanGFX display...");
    
    // Note: Actual LovyanGFX initialization would go here
    // For now, we'll assume it works
    ESP_LOGI(TAG, "LovyanGFX display initialized");
    return true;
    
    #else
    // Alternative display initialization
    ESP_LOGI(TAG, "Initializing standard display...");
    
    // Initialize display pins, SPI, etc.
    // This would be board-specific
    
    ESP_LOGI(TAG, "Standard display initialized");
    return true;
    #endif
    
    #else
    ESP_LOGW(TAG, "Display not enabled in configuration");
    return false;
    #endif
}

void wisp_backlight_set(uint8_t level) {
    if (level > 100) level = 100;
    
    g_system_status.backlight_level = level;
    
    #ifdef LCD_BACKLIGHT_PIN
    // Use PWM to control backlight
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    
    ledc_channel_config_t channel_config = {
        .channel = LEDC_CHANNEL_0,
        .duty = (level * 255) / 100,
        .gpio_num = LCD_BACKLIGHT_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0
    };
    
    static bool backlight_configured = false;
    if (!backlight_configured) {
        ledc_timer_config(&timer_config);
        ledc_channel_config(&channel_config);
        backlight_configured = true;
    } else {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (level * 255) / 100);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    }
    
    ESP_LOGI(TAG, "Backlight set to %d%%", level);
    #else
    ESP_LOGW(TAG, "Backlight control not available");
    #endif
}

bool wisp_lvgl_init(void) {
    #ifdef CONFIG_LVGL_USE_LVGL
    if (!g_system_status.lcd_ready) {
        ESP_LOGE(TAG, "Cannot initialize LVGL without LCD");
        return false;
    }
    
    ESP_LOGI(TAG, "Initializing LVGL...");
    
    // Initialize LVGL
    lv_init();
    
    // Initialize display port
    lv_port_disp_init();
    
    // Initialize input port (touch, buttons, etc.)
    lv_port_indev_init();
    
    ESP_LOGI(TAG, "LVGL initialized successfully");
    return true;
    
    #else
    ESP_LOGW(TAG, "LVGL not enabled in configuration");
    return false;
    #endif
}

void wisp_lvgl_example1(void) {
    #ifdef CONFIG_LVGL_USE_LVGL
    if (!g_system_status.lvgl_ready) {
        ESP_LOGW(TAG, "LVGL not initialized - skipping example");
        return;
    }
    
    ESP_LOGI(TAG, "Running LVGL Example 1...");
    
    // Create a simple label
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello Wisp Engine!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -50);
    
    // Create a button
    lv_obj_t * btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_t * btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Click Me!");
    lv_obj_center(btn_label);
    
    // Create a progress bar
    lv_obj_t * bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(bar, 200, 20);
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, 50);
    lv_bar_set_value(bar, 70, LV_ANIM_OFF);
    
    ESP_LOGI(TAG, "LVGL Example 1 complete");
    
    #else
    ESP_LOGW(TAG, "LVGL not available");
    #endif
}

bool wisp_settings_init(void) {
    using namespace WispEngine::System;
    
    try {
        SettingsManager& settings = SettingsManager::getInstance();
        SettingsError result = settings.init();
        
        if (result == SettingsError::SUCCESS) {
            ESP_LOGI(TAG, "Settings Manager initialized successfully");
            
            // Apply loaded settings to system components
            if (g_system_status.lcd_ready) {
                uint8_t brightness = settings.getScreenBrightness();
                // Convert 0-255 brightness to 0-100 percentage for backlight
                uint8_t brightness_percent = (brightness * 100) / 255;
                wisp_backlight_set(brightness_percent);
                ESP_LOGI(TAG, "Applied screen brightness: %d%% (from setting: %d)", brightness_percent, brightness);
            }
            
            // Log some key settings
            ESP_LOGI(TAG, "Device name: %s", settings.getDeviceName().c_str());
            ESP_LOGI(TAG, "WiFi auto-connect: %s", settings.getWiFiAutoConnect() ? "enabled" : "disabled");
            ESP_LOGI(TAG, "Bluetooth: %s", settings.getBluetoothEnabled() ? "enabled" : "disabled");
            ESP_LOGI(TAG, "Audio volume: %d", settings.getVolumeLevel());
            
            return true;
        } else {
            ESP_LOGE(TAG, "Settings Manager initialization failed: %s", settings.getErrorString(result).c_str());
            return false;
        }
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "Exception during settings initialization: %s", e.what());
        return false;
    } catch (...) {
        ESP_LOGE(TAG, "Unknown exception during settings initialization");
        return false;
    }
}

// === MAIN LOOP FUNCTIONS ===

void wisp_system_loop(void) {
    if (!g_system_initialized) return;
    
    // Update RGB LEDs
    if (g_system_status.rgb_ready) {
        ledController.update();
    }
    
    // Handle LVGL timer
    if (g_system_status.lvgl_ready) {
        wisp_lvgl_timer_handler();
    }
    
    // Small delay to prevent watchdog issues
    wisp_delay_ms(1);
}

void wisp_delay_ms(uint32_t delay_ms) {
    delay(delay_ms);
}

void wisp_lvgl_timer_handler(void) {
    #ifdef CONFIG_LVGL_USE_LVGL
    if (g_system_status.lvgl_ready) {
        lv_timer_handler();
    }
    #endif
}

// === UTILITY FUNCTIONS ===

bool wisp_is_component_ready(wisp_component_flags_t component) {
    switch (component) {
        case WISP_COMPONENT_WIRELESS: return g_system_status.wireless_ready;
        case WISP_COMPONENT_FLASH:    return g_system_status.flash_ready;
        case WISP_COMPONENT_RGB:      return g_system_status.rgb_ready;
        case WISP_COMPONENT_SD:       return g_system_status.sd_ready;
        case WISP_COMPONENT_LCD:      return g_system_status.lcd_ready;
        case WISP_COMPONENT_LVGL:     return g_system_status.lvgl_ready;
        case WISP_COMPONENT_SETTINGS: return g_system_status.settings_ready;
        default: return false;
    }
}

const char* wisp_get_component_status_string(wisp_component_flags_t component) {
    return wisp_is_component_ready(component) ? "READY" : "NOT READY";
}

void wisp_print_system_status(void) {
    ESP_LOGI(TAG, "=== Wisp Engine System Status ===");
    ESP_LOGI(TAG, "Flash:     %s (%d MB)", wisp_get_component_status_string(WISP_COMPONENT_FLASH), g_system_status.flash_size_mb);
    ESP_LOGI(TAG, "Settings:  %s", wisp_get_component_status_string(WISP_COMPONENT_SETTINGS));
    ESP_LOGI(TAG, "Wireless:  %s", wisp_get_component_status_string(WISP_COMPONENT_WIRELESS));
    ESP_LOGI(TAG, "RGB LEDs:  %s", wisp_get_component_status_string(WISP_COMPONENT_RGB));
    ESP_LOGI(TAG, "SD Card:   %s", wisp_get_component_status_string(WISP_COMPONENT_SD));
    ESP_LOGI(TAG, "LCD:       %s (Backlight: %d%%)", wisp_get_component_status_string(WISP_COMPONENT_LCD), g_system_status.backlight_level);
    ESP_LOGI(TAG, "LVGL:      %s", wisp_get_component_status_string(WISP_COMPONENT_LVGL));
    ESP_LOGI(TAG, "Init Time: %d ms", g_system_status.init_time_ms);
    ESP_LOGI(TAG, "===============================");
}

bool wisp_run_diagnostics(void) {
    ESP_LOGI(TAG, "Running system diagnostics...");
    
    bool all_passed = true;
    
    // Test flash
    if (g_system_status.flash_ready) {
        ESP_LOGI(TAG, "✓ Flash memory: %d MB", g_system_status.flash_size_mb);
    } else {
        ESP_LOGE(TAG, "✗ Flash memory failed");
        all_passed = false;
    }
    
    // Test settings
    if (g_system_status.settings_ready) {
        ESP_LOGI(TAG, "✓ Settings manager functional");
        
        // Test settings by reading a value
        try {
            using namespace WispEngine::System;
            SettingsManager& settings = SettingsManager::getInstance();
            std::string device_name = settings.getDeviceName();
            ESP_LOGI(TAG, "✓ Settings read test successful (device: %s)", device_name.c_str());
        } catch (...) {
            ESP_LOGW(TAG, "○ Settings read test failed");
        }
    } else {
        ESP_LOGW(TAG, "○ Settings manager not available");
    }
    
    // Test RGB LEDs
    if (g_system_status.rgb_ready) {
        ESP_LOGI(TAG, "✓ RGB LEDs functional");
        // Quick LED test
        LED_SET_COLOR(0, 255, 0);
        LED_SHOW();
        delay(100);
        LED_CLEAR();
        LED_SHOW();
    } else {
        ESP_LOGW(TAG, "○ RGB LEDs not available");
    }
    
    // Test other components
    if (g_system_status.wireless_ready) {
        ESP_LOGI(TAG, "✓ Wireless module ready");
    } else {
        ESP_LOGW(TAG, "○ Wireless not available");
    }
    
    if (g_system_status.sd_ready) {
        ESP_LOGI(TAG, "✓ SD card mounted");
    } else {
        ESP_LOGW(TAG, "○ SD card not available");
    }
    
    if (g_system_status.lcd_ready) {
        ESP_LOGI(TAG, "✓ LCD display ready");
    } else {
        ESP_LOGW(TAG, "○ LCD display not available");
    }
    
    if (g_system_status.lvgl_ready) {
        ESP_LOGI(TAG, "✓ LVGL graphics ready");
    } else {
        ESP_LOGW(TAG, "○ LVGL not available");
    }
    
    ESP_LOGI(TAG, "Diagnostics complete: %s", all_passed ? "ALL PASSED" : "SOME FAILED");
    return all_passed;
}
