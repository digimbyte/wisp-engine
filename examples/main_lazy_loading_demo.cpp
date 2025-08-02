// main_lazy_loading_demo.cpp
// Complete demonstration of the lazy loading game engine

#include "src/engine/core.h"
#include "src/engine/game_loop_manager.h"
#include "src/engine/lazy_resource_manager.h"
#include "src/engine/graphics_engine.h"
#include "examples/platformer_game_lazy.cpp"

// ESP32 specific includes
#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <SPIFFS.h>
#include <SD.h>

// Display configuration for ESP32
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    
public:
    LGFX(void) {
        // Configure SPI bus
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
        
        // Configure panel
        auto panel_cfg = _panel_instance.config();
        panel_cfg.pin_cs = 15;
        panel_cfg.pin_rst = 4;
        panel_cfg.pin_busy = -1;
        panel_cfg.memory_width = 240;
        panel_cfg.memory_height = 320;
        panel_cfg.panel_width = 240;
        panel_cfg.panel_height = 320;
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

// Global engine components
LGFX display;
ColorRenderer paletteRenderer;
GraphicsEngine graphics;
LazyResourceManager resourceManager;
GameLoopManager gameLoop(&resourceManager, &graphics);
EngineCore engineCore;

// Performance monitoring
struct SystemStats {
    uint32_t totalFrames;
    uint32_t totalLoadedResources;
    uint32_t totalLoadedChunks;
    uint32_t peakMemoryUsage;
    uint32_t totalLoadTime;
    uint32_t startTime;
    
    SystemStats() : totalFrames(0), totalLoadedResources(0), totalLoadedChunks(0),
                    peakMemoryUsage(0), totalLoadTime(0), startTime(0) {}
} stats;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== Wisp Engine Lazy Loading Demo ===");
    Serial.println("Initializing systems...");
    
    stats.startTime = millis();
    
    // Initialize storage
    if (!SPIFFS.begin(true)) {
        Serial.println("ERROR: SPIFFS initialization failed");
        return;
    }
    
    if (!SD.begin()) {
        Serial.println("WARNING: SD card not available, using SPIFFS only");
    }
    
    // Initialize display
    display.init();
    display.setRotation(1); // Landscape
    display.fillScreen(TFT_BLACK);
    display.setTextColor(TFT_WHITE);
    display.drawString("Wisp Engine Loading...", 10, 10);
    
    // Initialize engine core
    if (!engineCore.initialize()) {
        Serial.println("ERROR: Failed to initialize engine core");
        return;
    }
    
    // Initialize graphics engine
    graphics.init(&display, &paletteRenderer);
    
    // Generate test color LUT for demo
    graphics.generateTestLUT();
    
    // Configure resource manager for ESP32
    resourceManager.setMemoryBudget(128 * 1024); // 128KB total budget
    
    // Configure game loop for optimal performance
    gameLoop.setTargetFPS(60.0f);
    gameLoop.setLoadStrategy(LOAD_ADJACENT);
    gameLoop.setAdaptiveLoading(true);
    gameLoop.setPerformanceBudget(10000); // 10ms max loading per frame
    
    Serial.println("Systems initialized successfully!");
    
    // Create and start demo game
    static PlatformerGame game(&resourceManager, &gameLoop, &graphics);
    
    if (gameLoop.loadLevel(LEVEL_WORLD_1_1, &game)) {
        Serial.println("Demo game loaded!");
        
        // Show initial stats
        display.fillScreen(TFT_BLACK);
        display.drawString("Game Loaded - Starting...", 10, 100);
        delay(1000);
        
    } else {
        Serial.println("ERROR: Failed to load demo game");
        display.fillScreen(TFT_RED);
        display.drawString("LOAD ERROR", 10, 100);
        while(1) delay(1000);
    }
}

void loop() {
    uint32_t frameStart = micros();
    
    // Main game loop tick
    gameLoop.tick();
    
    // Update statistics
    stats.totalFrames++;
    uint32_t currentMemory = resourceManager.getCurrentMemoryUsage();
    if (currentMemory > stats.peakMemoryUsage) {
        stats.peakMemoryUsage = currentMemory;
    }
    
    // Performance monitoring and adaptation
    const auto& metrics = gameLoop.getMetrics();
    
    // Print detailed stats every 10 seconds
    static uint32_t lastDetailedStats = 0;
    if (millis() - lastDetailedStats > 10000) {
        printDetailedStats();
        lastDetailedStats = millis();
    }
    
    // Print quick stats every 2 seconds
    static uint32_t lastQuickStats = 0;
    if (millis() - lastQuickStats > 2000) {
        printQuickStats();
        lastQuickStats = millis();
    }
    
    // Memory pressure handling
    if (resourceManager.getMemoryPressure() > 0.9f) {
        Serial.println("HIGH MEMORY PRESSURE - Triggering garbage collection");
        resourceManager.garbageCollect();
    }
    
    // Watchdog feed
    yield();
    
    // Frame rate limiting (if not using VSync)
    uint32_t frameTime = micros() - frameStart;
    if (frameTime < 16667) { // Target 60fps
        delayMicroseconds(16667 - frameTime);
    }
}

void printQuickStats() {
    const auto& metrics = gameLoop.getMetrics();
    
    Serial.print("FPS: ");
    Serial.print(metrics.fps, 1);
    Serial.print(" | Memory: ");
    Serial.print(resourceManager.getCurrentMemoryUsage() / 1024);
    Serial.print("KB (");
    Serial.print((int)resourceManager.getMemoryPressure());
    Serial.print("%) | Loaded: ");
    Serial.print(resourceManager.getLoadedResources().size());
    Serial.println(" resources");
}

void printDetailedStats() {
    Serial.println("\n=== DETAILED SYSTEM STATISTICS ===");
    
    // Runtime stats
    uint32_t uptime = millis() - stats.startTime;
    Serial.print("Uptime: ");
    Serial.print(uptime / 1000);
    Serial.println(" seconds");
    
    Serial.print("Total Frames: ");
    Serial.println(stats.totalFrames);
    
    Serial.print("Average FPS: ");
    Serial.println((float)stats.totalFrames / (uptime / 1000.0f), 2);
    
    // Memory stats
    Serial.print("Current Memory: ");
    Serial.print(resourceManager.getCurrentMemoryUsage());
    Serial.print(" / ");
    Serial.print(resourceManager.getMaxMemoryUsage());
    Serial.print(" bytes (");
    Serial.print((int)resourceManager.getMemoryPressure());
    Serial.println("%)");
    
    Serial.print("Peak Memory Usage: ");
    Serial.print(stats.peakMemoryUsage);
    Serial.println(" bytes");
    
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());
    
    // Performance breakdown
    const auto& metrics = gameLoop.getMetrics();
    Serial.print("Last Frame - Logic: ");
    Serial.print(metrics.logicTime);
    Serial.print("μs, Render: ");
    Serial.print(metrics.renderTime);
    Serial.print("μs, Loading: ");
    Serial.print(metrics.loadingTime);
    Serial.println("μs");
    
    // Resource stats
    Serial.print("Loaded Resources: ");
    Serial.println(resourceManager.getLoadedResources().size());
    
    // Game loop state
    Serial.print("Game State: ");
    switch (gameLoop.getState()) {
        case GAME_LOADING: Serial.println("LOADING"); break;
        case GAME_RUNNING: Serial.println("RUNNING"); break;
        case GAME_STREAMING: Serial.println("STREAMING"); break;
        case GAME_PAUSED: Serial.println("PAUSED"); break;
        case GAME_TRANSITIONING: Serial.println("TRANSITIONING"); break;
    }
    
    // ESP32 specific stats
    Serial.print("CPU Frequency: ");
    Serial.print(getCpuFrequencyMhz());
    Serial.println(" MHz");
    
    Serial.print("Flash Size: ");
    Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
    Serial.println(" MB");
    
    Serial.print("PSRAM Size: ");
    Serial.print(ESP.getPsramSize() / 1024 / 1024);
    Serial.println(" MB");
    
    Serial.println("===================================\n");
}

// Error handling and recovery
void handleSystemError(const String& error) {
    Serial.print("SYSTEM ERROR: ");
    Serial.println(error);
    
    // Try to recover gracefully
    if (error.indexOf("memory") >= 0) {
        Serial.println("Attempting memory recovery...");
        resourceManager.garbageCollect();
        
        // Force minimal loading mode
        gameLoop.setLoadStrategy(LOAD_MINIMAL);
        gameLoop.setPerformanceBudget(5000); // Reduce budget
        
    } else if (error.indexOf("performance") >= 0) {
        Serial.println("Attempting performance recovery...");
        gameLoop.setAdaptiveLoading(true);
        gameLoop.setTargetFPS(30.0f); // Reduce target FPS
    }
    
    // Show error on display
    graphics.clearBuffers(0xF800); // Red background
    // TODO: Render error message using graphics engine
    graphics.present();
}

// Development utilities
void debugDumpMemoryMap() {
    Serial.println("\n=== MEMORY MAP ===");
    
    auto loadedResources = resourceManager.getLoadedResources();
    for (uint16_t resourceId : loadedResources) {
        // TODO: Print resource details
        Serial.print("Resource ");
        Serial.print(resourceId);
        Serial.println(" loaded");
    }
    
    Serial.println("==================\n");
}

void testLazyLoadingBehavior() {
    Serial.println("\n=== LAZY LOADING TEST ===");
    
    // Test resource loading/unloading
    Serial.println("Testing resource lifecycle...");
    
    // Force load a resource
    void* testResource = resourceManager.getResource(SPRITE_PLAYER_IDLE);
    if (testResource) {
        Serial.println("✓ Resource loaded on demand");
    } else {
        Serial.println("✗ Resource loading failed");
    }
    
    // Test memory pressure handling
    Serial.println("Testing memory pressure...");
    uint32_t initialMemory = resourceManager.getCurrentMemoryUsage();
    
    // Try to load many resources
    for (int i = 0; i < 10; i++) {
        resourceManager.preloadResource(SPRITE_PLAYER_RUN + i, 255); // Low priority
    }
    
    uint32_t finalMemory = resourceManager.getCurrentMemoryUsage();
    Serial.print("Memory change: ");
    Serial.print(finalMemory - initialMemory);
    Serial.println(" bytes");
    
    Serial.println("=======================\n");
}

// Called if setup() fails
void emergencyMode() {
    Serial.println("ENTERING EMERGENCY MODE");
    
    // Basic display output
    display.fillScreen(TFT_RED);
    display.setTextColor(TFT_WHITE);
    display.drawString("EMERGENCY MODE", 10, 100);
    display.drawString("Check Serial Output", 10, 120);
    
    while (1) {
        // Basic heartbeat
        Serial.println("Emergency mode active...");
        delay(5000);
    }
}
