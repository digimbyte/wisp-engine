# Wisp Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-ESP32-blue.svg)](https://espressif.com/en/products/socs/esp32)
[![Framework](https://img.shields.io/badge/framework-ESP--IDF-green.svg)](https://docs.espressif.com/projects/esp-idf/)
[![PlatformIO](https://img.shields.io/badge/build-PlatformIO-orange.svg)](https://platformio.org/)

A comprehensive, high-performance game engine and system framework designed specifically for ESP32 microcontrollers. Wisp Engine provides a complete development environment for creating graphics-intensive applications, games, and interactive systems on resource-constrained embedded devices.

## ✨ Key Features

### 🎮 Game Engine Core
- **High-performance 2D graphics engine** with sprite rendering and animation
- **Advanced memory management** with optimized asset loading
- **Real-time audio system** supporting background music and sound effects
- **Efficient database system** for game assets and user data
- **Frame rate management** with adaptive performance scaling

### 🔌 Hardware Integration
- **Universal LED controller** supporting multiple LED types (GPIO, PWM RGB, WS2812, APA102)
- **Display abstraction layer** for various LCD/OLED panels
- **Touch input processing** with gesture recognition
- **Audio codec integration** with I2S interface support
- **Wireless connectivity** (WiFi, Bluetooth, IEEE 802.15.4)

### 🎯 ESP32 Optimized
- **Multi-platform support** (ESP32, ESP32-S3, ESP32-C6)
- **FreeRTOS integration** with proper task management
- **Memory-efficient design** for constrained environments
- **Hardware acceleration** utilizing ESP32-specific features
- **Low-power modes** for battery-powered applications

### 🛠️ Developer Experience
- **Clean, modern C++ API** with comprehensive documentation
- **Component-based architecture** for modular development
- **Rich example collection** covering common use cases
- **Professional build system** with CMake and PlatformIO support
- **Extensive board support** including popular development boards

## 🚀 Quick Start

### Prerequisites
- **ESP-IDF v5.0+** or **PlatformIO** with ESP32 platform
- **Supported ESP32 variant**: ESP32, ESP32-S3, or ESP32-C6
- **Development board** with display (recommended)

### Installation

#### Option 1: ESP-IDF
```bash
# Clone the repository
git clone https://github.com/your-username/wisp-engine.git
cd wisp-engine

# Navigate to examples
cd examples/

# Set target platform
idf.py set-target esp32c6  # or esp32s3, esp32

# Configure project
idf.py menuconfig

# Build and flash
idf.py build flash monitor
```

#### Option 2: PlatformIO
```bash
# Clone the repository
git clone https://github.com/your-username/wisp-engine.git
cd wisp-engine

# Build and upload
pio run --target upload --target monitor
```

### Basic Usage

```cpp
#include "system/system_init.h"
#include "system/led_controller.h"

extern "C" void app_main(void) {
    // Initialize the complete Wisp system
    if (wisp_system_setup() == WISP_INIT_OK) {
        LED_SUCCESS();  // Green flash indicates success
        
        // Main application loop
        while (1) {
            wisp_system_loop();     // Handle system updates
            wisp_delay_ms(10);      // 10ms delay (100 FPS)
        }
    } else {
        LED_ERROR();  // Red flash indicates error
    }
}
```

## 📋 System Architecture

Wisp Engine follows ESP-IDF patterns and provides these core components:

```cpp
// System initialization sequence (matches ESP-IDF style)
setup() {
    wisp_wireless_init();      // WiFi/Bluetooth initialization
    wisp_flash_searching();    // Flash memory detection
    wisp_rgb_init();           // LED system setup
    wisp_rgb_example();        // LED demonstration
    wisp_sd_init();            // TF/SD card initialization
    wisp_lcd_init();           // Display initialization
    wisp_backlight_set(50);    // Set backlight brightness
    wisp_lvgl_init();          // LVGL graphics library
    wisp_lvgl_example1();      // Display demo content
}

while(1) {
    wisp_delay_ms(10);         // vTaskDelay(pdMS_TO_TICKS(10))
    wisp_lvgl_timer_handler(); // lv_timer_handler()
}
```

### Component Overview

| Component | Description |
|-----------|-------------|
| **Engine Core** | Main game loop, frame management, resource loading |
| **Graphics** | 2D rendering, sprite system, animation engine |
| **Audio** | Background music, sound effects, audio streaming |
| **Input** | Touch processing, button handling, gesture recognition |
| **Storage** | Asset database, save system, configuration management |
| **Networking** | WiFi connectivity, Bluetooth, data synchronization |
| **LED System** | Universal LED control, status indicators, animations |

## 🎮 Examples

The repository includes comprehensive examples for different use cases:

- **[Basic System](examples/wisp_main_demo.cpp)** - Complete system initialization
- **[LED Control](examples/led_examples/)** - LED animations and status indicators  
- **[Graphics Demo](examples/sprite_test/)** - 2D graphics and animation
- **[Audio Playback](examples/audio_test/)** - Background music and sound effects
- **[Database Usage](examples/database_test/)** - Asset storage and retrieval
- **[Network Features](examples/network_test/)** - WiFi and Bluetooth connectivity

## 🔧 Supported Hardware

### Development Boards
- **Waveshare ESP32-C6-LCD-1.47** - Compact board with 1.47" display
- **Waveshare ESP32-S3-LCD-2** - 2" IPS display with camera interface
- **Waveshare ESP32-S3-Touch-AMOLED-2.06** - Watch-style with AMOLED display
- **Generic ESP32 boards** with custom configuration

### LED Types
- **Simple GPIO** - Single on/off LEDs
- **PWM RGB** - RGB LEDs via PWM channels
- **WS2812** - NeoPixel addressable RGB strips
- **APA102** - DotStar addressable RGB strips

### Display Support
- **IPS LCD** panels (SPI interface)
- **AMOLED** displays (QSPI interface)
- **Touch controllers** (capacitive and resistive)
- **Various resolutions** from 172×320 to 410×502

## 📚 Documentation

Comprehensive documentation is available in the [docs/](docs/) directory:

- **[API Reference](docs/api/)** - Complete API documentation
- **[User Guides](docs/guides/)** - Step-by-step tutorials
- **[Board Setup](docs/boards/)** - Hardware-specific guides
- **[Development](docs/development/)** - Architecture and internals

### Quick Links
- [🚀 Quick Start Guide](docs/guides/quick_start.md)
- [📦 Installation Instructions](docs/guides/installation.md)
- [🔧 Hardware Setup](docs/guides/board_setup.md)
- [🎮 Engine API Reference](docs/api/engine_api.md)
- [🎵 Audio API Reference](docs/api/audio_api.md)
- [💡 LED Controller API](docs/api/led_api.md)

## 🏗️ Building from Source

### Build Requirements
- **ESP-IDF 5.0+** or **PlatformIO Core 6.0+**
- **CMake 3.16+** (for ESP-IDF builds)
- **Python 3.6+** with pip
- **Git** for version control

### Build Configuration
```bash
# Configure for specific board
idf.py menuconfig

# Key configuration options:
# - Component config → Wisp Engine → Board Type
# - Component config → Wisp Engine → Display Settings  
# - Component config → Wisp Engine → Audio Settings
# - Component config → Wisp Engine → Memory Options
```

### Memory Optimization
The engine includes several memory optimization features:

- **Asset compression** - Automatic sprite and audio compression
- **Memory pooling** - Efficient allocation strategies
- **Lazy loading** - Load assets on demand
- **Cache management** - LRU cache for frequently used assets

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

### Development Workflow
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

### Coding Standards
- Follow [ESP-IDF Coding Style](docs/development/coding_standards.md)
- Use meaningful variable and function names
- Add documentation for public APIs
- Include examples for new features

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Espressif Systems** for the excellent ESP-IDF framework
- **LVGL Team** for the graphics library
- **PlatformIO** for the development platform
- **Waveshare** for their development boards and documentation

## 📞 Support

- **Documentation**: [docs/](docs/)
- **Issues**: [GitHub Issues](https://github.com/your-username/wisp-engine/issues)
- **Discussions**: [GitHub Discussions](https://github.com/your-username/wisp-engine/discussions)
- **ESP32 Community**: [ESP32 Forums](https://esp32.com/)

## 🔄 Version History

See [CHANGELOG.md](CHANGELOG.md) for detailed version history and release notes.

---

**Wisp Engine** - Bringing AAA game development capabilities to ESP32 microcontrollers.

Made with ❤️ by the Wisp Engine team.
