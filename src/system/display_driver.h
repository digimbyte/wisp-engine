// display_driver.h - ESP32-C6/S3 Display Driver using LovyanGFX
// Native ESP32 display implementation with multi-board support
#pragma once
#include "esp32_common.h"  
#include <LovyanGFX.hpp>

// Include both board configurations for now (will be refined by build system)
#include "../../boards/esp32-c6_config.h"
#include "../../boards/esp32-s3_config.h"

static const char* DISPLAY_TAG = "DisplayDriver";

// Multi-board LGFX display driver
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    
public:
    LGFX(void) {
        // Configure based on board type - ESP-IDF native detection
#if defined(CONFIG_ESP32_C6_LCD_147) || defined(CONFIG_IDF_TARGET_ESP32C6)
        configureST7789_C6();
#elif defined(CONFIG_ESP32_S3_ROUND) || defined(CONFIG_IDF_TARGET_ESP32S3)
        configureST7789_S3();
#else
        configureDefaultST7789();
#endif
    }
    
private:
    void configureST7789_C6() {
        // ESP32-C6-LCD-1.47 configuration
        auto cfg = _bus_instance.config();
        cfg.spi_host = SPI2_HOST;
        cfg.spi_mode = 0;
        cfg.freq_write = 40000000;    // 40MHz write
        cfg.freq_read = 16000000;     // 16MHz read
        cfg.spi_3wire = true;
        cfg.use_lock = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO;
        cfg.pin_sclk = DISPLAY_SPI_CLK_PIN;
        cfg.pin_mosi = DISPLAY_SPI_MOSI_PIN;
        cfg.pin_miso = -1;            // No MISO for display
        cfg.pin_dc = DISPLAY_DC_PIN;
        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
        
        auto pcfg = _panel_instance.config();
        pcfg.pin_cs = DISPLAY_SPI_CS_PIN;
        pcfg.pin_rst = DISPLAY_RST_PIN;
        pcfg.pin_busy = -1;
        pcfg.memory_width = 240;
        pcfg.memory_height = 320;
        pcfg.panel_width = DISPLAY_WIDTH;
        pcfg.panel_height = DISPLAY_HEIGHT;
        pcfg.offset_x = 34;           // Offset for 1.47" display
        pcfg.offset_y = 0;
        pcfg.offset_rotation = 0;
        pcfg.dummy_read_pixel = 8;
        pcfg.dummy_read_bits = 1;
        pcfg.readable = false;
        pcfg.invert = true;           // Color inversion for ST7789
        pcfg.rgb_order = false;
        pcfg.dlen_16bit = false;
        pcfg.bus_shared = false;
        _panel_instance.config(pcfg);
        
        this->setPanel(&_panel_instance);
    }
    
    void configureST7789_S3() {
        // ESP32-S3 Round Display configuration
        auto cfg = _bus_instance.config();
        cfg.spi_host = SPI2_HOST;
        cfg.spi_mode = 0;
        cfg.freq_write = 40000000;    // 40MHz write
        cfg.freq_read = 16000000;     // 16MHz read
        cfg.spi_3wire = true;
        cfg.use_lock = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO;
        cfg.pin_sclk = DISPLAY_SPI_CLK_PIN;
        cfg.pin_mosi = DISPLAY_SPI_MOSI_PIN;
        cfg.pin_miso = -1;            // No MISO for display
        cfg.pin_dc = DISPLAY_DC_PIN;
        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
        
        auto pcfg = _panel_instance.config();
        pcfg.pin_cs = DISPLAY_SPI_CS_PIN;
        pcfg.pin_rst = DISPLAY_RST_PIN;
        pcfg.pin_busy = -1;
        pcfg.memory_width = 240;
        pcfg.memory_height = 240;
        pcfg.panel_width = DISPLAY_WIDTH;
        pcfg.panel_height = DISPLAY_HEIGHT;
        pcfg.offset_x = 0;
        pcfg.offset_y = 0;
        pcfg.offset_rotation = 0;
        pcfg.dummy_read_pixel = 8;
        pcfg.dummy_read_bits = 1;
        pcfg.readable = false;
        pcfg.invert = true;           // Color inversion for GC9A01/ST7789
        pcfg.rgb_order = false;
        pcfg.dlen_16bit = false;
        pcfg.bus_shared = false;
        _panel_instance.config(pcfg);
        
        this->setPanel(&_panel_instance);
    }
    
    void configureDefaultST7789() {
        // Fallback configuration
        ESP_LOGW(DISPLAY_TAG, "Using default ST7789 configuration - may not work on all boards");
        auto cfg = _bus_instance.config();
        cfg.spi_host = SPI2_HOST;
        cfg.spi_mode = 0;
        cfg.freq_write = 40000000;
        cfg.freq_read = 16000000;
        cfg.spi_3wire = true;
        cfg.use_lock = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO;
        cfg.pin_sclk = 18;
        cfg.pin_mosi = 19;
        cfg.pin_miso = -1;
        cfg.pin_dc = 16;
        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
        
        auto pcfg = _panel_instance.config();
        pcfg.pin_cs = 5;
        pcfg.pin_rst = 23;
        pcfg.pin_busy = -1;
        pcfg.memory_width = 240;
        pcfg.memory_height = 320;
        pcfg.panel_width = 240;
        pcfg.panel_height = 320;
        pcfg.offset_x = 0;
        pcfg.offset_y = 0;
        pcfg.offset_rotation = 0;
        pcfg.dummy_read_pixel = 8;
        pcfg.dummy_read_bits = 1;
        pcfg.readable = false;
        pcfg.invert = true;
        pcfg.rgb_order = false;
        pcfg.dlen_16bit = false;
        pcfg.bus_shared = false;
        _panel_instance.config(pcfg);
        
        this->setPanel(&_panel_instance);
    }
};
