# Wisp Engine Documentation

Welcome to the comprehensive documentation for Wisp Engine - a high-performance game engine designed for ESP32 microcontrollers.

## 📚 Documentation Structure

### 🚀 Getting Started
Start here if you're new to Wisp Engine:

- **[Quick Start Guide](guides/quick_start.md)** - Get up and running in minutes
- **[Installation Instructions](guides/installation.md)** - Detailed setup procedures
- **[Board Setup](guides/board_setup.md)** - Hardware configuration guide
- **[Troubleshooting](guides/troubleshooting.md)** - Common issues and solutions

### 📖 API Reference
Complete API documentation for all Wisp Engine components:

- **[Engine Core API](api/engine_api.md)** - Main engine functions and classes
- **[Audio System API](api/audio_api.md)** - Background music and sound effects
- **[Graphics API](api/graphics_api.md)** - 2D rendering and sprites
- **[Bluetooth API](api/bluetooth_api.md)** - Wireless connectivity
- **[LED Controller API](api/led_api.md)** - Universal LED control system

### 🏗️ Development
In-depth technical documentation for developers:

- **[System Architecture](development/design_document.md)** - Overall system design
- **[System Flow](development/system_flow.md)** - Component interactions
- **[Engine Analysis](development/engine_analysis.md)** - Performance analysis
- **[System Integration](development/system_integration.md)** - ESP-IDF integration
- **[Memory Management](development/memory/)** - Memory optimization guides

### 🔧 Board Support
Hardware-specific documentation and guides:

- **[Waveshare Boards](boards/waveshare/)** - Waveshare development boards
  - [ESP32-C6-LCD-1.47](boards/waveshare/c6-147.md)
  - [ESP32-S3-LCD-2](boards/waveshare/S3-2inch.md)
  - [ESP32-S3-Touch-AMOLED-2.06](boards/waveshare/S3-watch.md)

## 🎯 Documentation by Use Case

### For New Users
1. [Quick Start Guide](guides/quick_start.md) - Basic setup
2. [Board Setup](guides/board_setup.md) - Hardware preparation
3. [Basic Examples](../examples/) - Simple code examples

### For Developers
1. [System Architecture](development/design_document.md) - Understanding the engine
2. [API Reference](api/) - Function documentation
3. [Development Guides](development/) - Advanced topics

### For Contributors
1. [Contributing Guidelines](../CONTRIBUTING.md) - How to contribute
2. [Coding Standards](development/coding_standards.md) - Code style guide
3. [Testing Guide](development/testing.md) - Writing tests

## 🔍 Finding Information

### By Topic
- **Audio**: [Audio API](api/audio_api.md) + [Audio Examples](../examples/audio_test/)
- **Graphics**: [Graphics API](api/graphics_api.md) + [Sprite Examples](../examples/sprite_test/)
- **LED Control**: [LED API](api/led_api.md) + [LED Examples](../examples/led_examples/)
- **Networking**: [Bluetooth API](api/bluetooth_api.md) + [Network Examples](../examples/network_test/)
- **Storage**: [Database Guide](development/database/) + [Save Examples](../examples/save_test/)

### By Hardware
- **ESP32-C6**: [C6-specific docs](boards/waveshare/c6-147.md) + [C6 configs](../configs/boards/)
- **ESP32-S3**: [S3-specific docs](boards/waveshare/S3-2inch.md) + [S3 configs](../configs/boards/)
- **Custom Boards**: [Board Configuration Guide](guides/board_setup.md)

### By Framework
- **ESP-IDF**: [ESP-IDF Integration](development/system_integration.md)
- **PlatformIO**: [PlatformIO Setup](guides/installation.md#platformio)
- **Arduino**: [Arduino Compatibility](guides/arduino_support.md)

## 🛠️ Development Tools

### Documentation Tools
- **Doxygen**: API documentation generation
- **Markdown**: Human-readable documentation format
- **PlantUML**: Diagram generation (where applicable)

### Example Code
All documentation includes working code examples that can be found in the [examples/](../examples/) directory:

```cpp
// Example from Audio API documentation
#include "engine/audio/audio_api.h"

void play_background_music() {
    wisp_audio_play_bgm("battle_theme.wbgm", 0.8f, true);
}
```

## 🔄 Documentation Updates

This documentation is actively maintained and updated with each release. Check the following for the latest information:

- **[Changelog](../CHANGELOG.md)** - Recent changes and additions
- **[Release Notes](https://github.com/your-username/wisp-engine/releases)** - Version-specific updates
- **[Issues](https://github.com/your-username/wisp-engine/issues)** - Known documentation issues

### Contributing to Documentation
We welcome improvements to documentation! See our [Contributing Guidelines](../CONTRIBUTING.md) for details on:
- Fixing typos and errors
- Adding missing information
- Creating new guides
- Improving examples

## 📞 Getting Help

If you can't find what you're looking for in this documentation:

1. **Check the [FAQ](guides/faq.md)** - Common questions and answers
2. **Search [GitHub Issues](https://github.com/your-username/wisp-engine/issues)** - Existing discussions
3. **Ask in [GitHub Discussions](https://github.com/your-username/wisp-engine/discussions)** - Community help
4. **Visit [ESP32 Forums](https://esp32.com/)** - Hardware-specific questions

## 📋 Documentation Standards

This documentation follows these standards:
- **Markdown format** for readability and maintainability  
- **Working code examples** that compile and run
- **Cross-references** between related topics
- **Clear section structure** with consistent formatting
- **Regular updates** to match code changes

---

**Last Updated**: 2025-08-09  
**Documentation Version**: 1.0.0  
**Wisp Engine Version**: 0.9.0+

Need help with something specific? Jump to the relevant section above or use the search function in your browser (Ctrl+F / Cmd+F) to find specific topics.
