// bootloader.cpp - Wisp Engine Native ESP32-C6/S3 Bootloader using ESP-IDF
// 
// ESP32 ARCHITECTURE NOTES:
// - Uses ESP-IDF framework (NOT Arduino) with compatibility layer
// - Targets ESP32-C6 (512KB HP-SRAM, 16KB LP-SRAM) and ESP32-S3 variants
// - All timing uses esp_timer, all GPIO uses native ESP-IDF drivers
// - Arduino compatibility provided through esp32_common.h
// 
// Clean micro-folder architecture with namespace organization

// Core engine with clean namespaces
#include "engine/namespaces.h"

// System components (existing)
#include "system/esp32_common.h"
#include "system/definitions.h"
#include "system/app_manager.h"
#include "system/input_controller.h"
#include "utils/panels/menu.h"
#include "core/timekeeper.h"

// External libraries
#include <LovyanGFX.hpp>

// Namespace organization
using namespace WispEngine;
namespace Core = WispEngine::Core;
namespace Graphics = WispEngine::Graphics;
namespace Audio = WispEngine::Audio;
namespace Database = WispEngine::Database;

// --- HARDWARE CONFIGURATION ---
// Display configuration (move to board-specific config)
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    
public:
    LGFX(void) {
        // Configure based on board type
#ifdef BOARD_ESP32_S3
        configureST7789_S3();
#elif defined(BOARD_ESP32_C6)
        configureST7789_C6();
#else
        configureDefaultST7789();
#endif
    }
    
private:
    void configureDefaultST7789() {
        auto cfg = _bus_instance.config();
        cfg.spi_host = VSPI_HOST;
        cfg.spi_mode = 0;
        cfg.freq_write = 40000000;
        cfg.freq_read = 16000000;
        cfg.spi_3wire = true;
        cfg.use_lock = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO;
        cfg.pin_sclk = 18;
        cfg.pin_mosi = 23;
        cfg.pin_miso = -1;
        cfg.pin_dc = 2;
        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
        
        auto panel_cfg = _panel_instance.config();
        panel_cfg.pin_cs = 15;
        panel_cfg.pin_rst = 4;
        panel_cfg.pin_busy = -1;
        panel_cfg.memory_width = SCREEN_WIDTH;
        panel_cfg.memory_height = SCREEN_HEIGHT;
        panel_cfg.panel_width = SCREEN_WIDTH;
        panel_cfg.panel_height = SCREEN_HEIGHT;
        panel_cfg.offset_x = 0;
        panel_cfg.offset_y = 0;
        panel_cfg.offset_rotation = 0;
        panel_cfg.dummy_read_pixel = 8;
        panel_cfg.dummy_read_bits = 1;
        panel_cfg.readable = true;
        panel_cfg.invert = false;
        panel_cfg.rgb_order = false;
        panel_cfg.dlen_16bit = false;
        panel_cfg.bus_shared = true;
        _panel_instance.config(panel_cfg);
        
        setPanel(&_panel_instance);
    }
};

// --- GLOBAL ENGINE COMPONENTS ---
LGFX display;
WispCuratedAPI curatedAPI;  // Use existing WispCuratedAPI
AppLoader appLoader;
AppManager appManager;
InputController* inputController;

// Boot state
bool bootComplete = false;
bool menuInitialized = false;

// --- SETUP PHASE ---
void bootloaderSetup() {
    ESP_LOGI("WISP", "=== Wisp Engine Native C++ Bootloader ===");
    ESP_LOGI("WISP", "Starting boot sequence...");
    
    // Initialize debug system first (use namespace bridge)
    Core::Debug::init(Core::Debug::DEBUG_MODE_ENABLED, Core::Debug::SAFETY_ENABLED);
    ESP_LOGI("WISP", "Debug system initialized");
    
    // Initialize display first
    display.init();
    display.setBrightness(255);
    display.setColorDepth(16);
    display.fillScreen(0x0000); // Black
    
    ESP_LOGI("WISP", "Display initialized");
    
    // Show initial boot message
    display.setTextColor(0xFFFF);
    display.setTextDatum(top_center);
    display.drawString("WISP ENGINE", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20);
    display.drawString("Booting...", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    
    // Initialize storage systems (with existing SD health checks)
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE("WISP", "SPIFFS initialization failed");
        display.fillScreen(0xF800); // Red error screen
        display.setTextColor(0xFFFF);
        display.drawString("STORAGE ERROR", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        while(1) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGI("WISP", "SPIFFS initialized successfully");
    
    // Try to initialize SD card (non-breaking, optional)
    bool sdAvailable = SD.begin();
    if (sdAvailable) {
        ESP_LOGI("WISP", "SD card initialized successfully");
    } else {
        ESP_LOGI("WISP", "SD card not available - using SPIFFS only");
    }
    
    ESP_LOGI("WISP", "Save system skipped for basic bootloader test");
    
    // Initialize input controller
    inputController = new InputController(BUTTON_PINS);
    if (!inputController->init()) {
        ESP_LOGE("WISP", "Input controller initialization failed");
        while(1) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Initialize timing system (use namespace bridge)
    Core::Timing::init();
    
    // Initialize app manager (use existing)
    AppLoader appLoaderInstance;
    if (!appManager.init(&appLoaderInstance, &appLoop)) {
        ESP_LOGE("WISP", "App manager initialization failed");
        display.fillScreen(0xF800); // Red error screen
        display.setTextColor(0xFFFF);
        display.drawString("APP MANAGER ERROR", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        while(1) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Scan for available .wisp applications
    appManager.scanForApps();
    
    ESP_LOGI("WISP", "App manager initialized successfully");
}

// --- MAIN LOOP ---
void bootloaderLoop() {
    // Frame timing control (use namespace bridge)
    if (!Core::Timing::frameReady()) return;
    Core::Timing::tick();
    
    // System heartbeat (use namespace bridge)
    Core::Debug::heartbeat();
    
    // Update input
    inputController->update();
    WispInputState inputState = convertToWispInput();  // Use existing input state
    
    if (!bootComplete) {
        // Simple boot - just mark as complete after initialization
        bootComplete = true;
        
        // Initialize menu system (use existing WispMenu)
        if (!menuInitialized) {
            if (WispMenu::init(&curatedAPI)) {
                menuInitialized = true;
                ESP_LOGI("WISP", "Menu system initialized");
            } else {
                ESP_LOGE("WISP", "Menu system initialization failed");
            }
        }
        
        // Show menu with available .wisp apps
        WispMenu::activate();
        
    } else {
        // Post-boot: either menu or app is running
        
        if (WispMenu::isActive()) {
            // Menu is active
            WispMenu::update(inputState);
            WispMenu::render();
            
        } else if (appManager.isAppRunning()) {
            // App is running via app manager
            appManager.update();
            
        } else {
            // No app or menu - show error or return to menu
            ESP_LOGI("WISP", "No active app or menu - returning to menu");
            WispMenu::activate();
        }
    }
    
    // Emergency menu activation (hold SELECT + BACK)
    static uint32_t emergencyMenuTimer = 0;
    if (inputState.select && inputState.buttonB) {
        if (emergencyMenuTimer == 0) {
            emergencyMenuTimer = millis();
        } else if (millis() - emergencyMenuTimer > 2000) {
            // 2 seconds of holding both buttons
            ESP_LOGI("WISP", "Emergency menu activation");
            WispMenu::activate();
            emergencyMenuTimer = 0;
        }
    } else {
        emergencyMenuTimer = 0;
    }
    
    // Performance monitoring
    static uint32_t lastStatsTime = 0;
    if (millis() - lastStatsTime > 5000) { // Every 5 seconds
        printPerformanceStats();
        lastStatsTime = millis();
    }
}

// --- UTILITY FUNCTIONS ---
WispInputState convertToWispInput() {
    WispInputState state;
    
    // Map hardware buttons to Wisp input state
    state.left = inputController->isPressed(BTN_LEFT);
    state.right = inputController->isPressed(BTN_RIGHT);
    state.up = inputController->isPressed(BTN_UP);
    state.down = inputController->isPressed(BTN_DOWN);
    state.buttonA = inputController->isPressed(BTN_SELECT);
    state.buttonB = inputController->isPressed(BTN_BACK);
    state.buttonC = false; // Not available on this hardware
    state.select = inputController->isPressed(BTN_SELECT);
    state.start = false; // Not available on this hardware
    
    // Analog input (if available)
    state.analogX = 0;
    state.analogY = 0;
    
    // Touch input (if available)
    state.touched = false;
    state.touchX = 0;
    state.touchY = 0;
    
    return state;
}

bool launchApp(const String& appPath) {
    ESP_LOGI("WISP", "Launching app: %s", appPath.c_str());
    
    // Use app manager to load the .wisp file (existing system)
    if (appManager.loadApp(appPath)) {
        ESP_LOGI("WISP", "App launched successfully via app manager");
        return true;
    } else {
        ESP_LOGE("WISP", "App launch failed");
        return false;
    }
}

void printPerformanceStats() {
    uint32_t freeHeap = esp_get_free_heap_size();
    ESP_LOGI("STATS", "FPS: %.1f | Memory: %luKB | Free Heap: %lu bytes", 
             Core::Timing::getFPS(), freeHeap / 1024, freeHeap);
}

// Emergency error handler
void handleCriticalError(const String& error) {
    ESP_LOGE("WISP", "CRITICAL ERROR: %s", error.c_str());
    
    // Activate emergency mode in debug system (use namespace bridge)
    Core::Debug::activateEmergencyMode(error);
    
    // Show error on display
    display.fillScreen(0xF800); // Red background
    display.setTextColor(0xFFFF);
    display.setTextDatum(top_center);
    display.drawString("SYSTEM ERROR", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 40);
    display.drawString(error, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 10);
    display.drawString("Hold RESET to restart", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 20);
    
    // Stop all systems
    appManager.stopApp();
    
    // Shutdown debug system to flush logs
    Core::Debug::shutdown();
    
    // Infinite loop until reset
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
