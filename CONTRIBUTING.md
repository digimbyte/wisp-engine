# Contributing to Wisp Engine

Thank you for your interest in contributing to Wisp Engine! This document provides guidelines and information for contributors.

## 🤝 Code of Conduct

This project adheres to a Code of Conduct that we expect all contributors to follow. Please read [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) before contributing.

## 🚀 Getting Started

### Prerequisites
- **ESP-IDF v5.0+** or **PlatformIO Core 6.0+**
- **Git** with proper configuration
- **C/C++** development experience
- **ESP32** hardware for testing (recommended)

### Setting Up Development Environment

1. **Fork and Clone**
   ```bash
   git fork https://github.com/your-username/wisp-engine.git
   git clone https://github.com/your-username/wisp-engine.git
   cd wisp-engine
   ```

2. **Set Up ESP-IDF**
   ```bash
   # Install ESP-IDF (if not already installed)
   # Follow: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/
   
   # Set up the environment
   . $HOME/esp/esp-idf/export.sh  # Linux/macOS
   # or
   %userprofile%\esp\esp-idf\export.bat  # Windows
   ```

3. **Install Dependencies**
   ```bash
   # For ESP-IDF
   idf.py install
   
   # For PlatformIO
   pip install platformio
   ```

4. **Build and Test**
   ```bash
   cd examples/
   idf.py build
   # or
   pio run
   ```

## 📋 Types of Contributions

We welcome various types of contributions:

### 🐛 Bug Reports
- Use the GitHub issue tracker
- Follow the bug report template
- Include system information and reproduction steps
- Provide minimal test cases when possible

### 💡 Feature Requests
- Discuss large changes in issues before implementing
- Follow the feature request template
- Consider backwards compatibility
- Include use cases and examples

### 🔧 Code Contributions
- Bug fixes
- New features
- Performance improvements
- Documentation updates
- Test additions

### 📚 Documentation
- API documentation
- Tutorials and guides
- Code examples
- README improvements

### 🧪 Testing
- Unit tests
- Integration tests
- Hardware validation
- Performance benchmarks

## 🏗️ Development Workflow

### Branch Strategy
We use a modified Git Flow approach:

- **`main`** - Stable releases only
- **`develop`** - Development integration branch
- **`feature/feature-name`** - Feature development
- **`fix/issue-number`** - Bug fixes
- **`docs/topic`** - Documentation updates

### Making Changes

1. **Create a Branch**
   ```bash
   git checkout develop
   git pull origin develop
   git checkout -b feature/your-feature-name
   ```

2. **Make Changes**
   - Follow coding standards (see below)
   - Add tests for new functionality
   - Update documentation as needed
   - Keep commits atomic and descriptive

3. **Test Your Changes**
   ```bash
   # Build for multiple targets
   idf.py set-target esp32c6
   idf.py build
   
   idf.py set-target esp32s3
   idf.py build
   
   # Run tests (when available)
   idf.py flash monitor  # Test on hardware
   ```

4. **Commit Changes**
   ```bash
   git add .
   git commit -m "feat: add new LED animation system"
   ```

5. **Push and Create PR**
   ```bash
   git push origin feature/your-feature-name
   # Create Pull Request on GitHub
   ```

## 📝 Coding Standards

### Code Style
Follow ESP-IDF coding conventions with these additions:

#### C++ Guidelines
```cpp
// Use meaningful names
class WispEngine {
private:
    uint32_t frame_count_;  // Use trailing underscore for private members
    
public:
    // Use verb-noun naming for functions
    void initialize_system();
    bool load_sprite(const char* filename);
    
    // Use ALL_CAPS for constants
    static constexpr uint32_t MAX_SPRITES = 256;
};

// Use namespace for grouping
namespace wisp {
namespace audio {
    void play_sound(uint32_t sound_id);
}
}
```

#### File Organization
```cpp
// Header files (.h)
#pragma once  // Use pragma once instead of include guards

#include <standard_headers>
#include "esp_system.h"
#include "local_headers.h"

// Forward declarations
class ClassName;

// Interface first, implementation details last
```

#### Memory Management
```cpp
// Prefer RAII and smart pointers when applicable
// Use ESP-IDF heap functions for dynamic allocation
void* buffer = heap_caps_malloc(size, MALLOC_CAP_DMA);
// Always check for allocation failure
if (!buffer) {
    ESP_LOGE(TAG, "Failed to allocate buffer");
    return ESP_ERR_NO_MEM;
}

// Clean up resources
heap_caps_free(buffer);
```

### Documentation
- Use Doxygen comments for public APIs
- Include examples in documentation
- Document parameters, return values, and exceptions
- Keep documentation up-to-date with code changes

```cpp
/**
 * @brief Load and initialize a sprite from file
 * 
 * @param filename Path to sprite file (must be null-terminated)
 * @param sprite_id Unique identifier for the sprite (0-255)
 * @return esp_err_t ESP_OK on success, error code on failure
 * 
 * @note The sprite file must be in WISP format (.spr)
 * @warning This function is not thread-safe
 * 
 * Example usage:
 * @code
 * esp_err_t err = wisp_load_sprite("player.spr", PLAYER_SPRITE_ID);
 * if (err != ESP_OK) {
 *     ESP_LOGE(TAG, "Failed to load sprite: %s", esp_err_to_name(err));
 * }
 * @endcode
 */
esp_err_t wisp_load_sprite(const char* filename, uint8_t sprite_id);
```

### Error Handling
- Use ESP-IDF error codes (`esp_err_t`)
- Check all return values
- Log errors with appropriate levels
- Provide recovery mechanisms when possible

```cpp
esp_err_t initialize_display() {
    esp_err_t err = spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(err));
        return err;
    }
    
    // Continue with initialization...
    return ESP_OK;
}
```

## 🧪 Testing

### Test Structure
```
test/
├── unit/              # Unit tests
│   ├── test_engine.cpp
│   └── test_audio.cpp
├── integration/       # Integration tests
│   ├── test_full_system.cpp
│   └── test_hardware.cpp
├── mocks/            # Mock objects for testing
│   └── mock_spi.h
└── fixtures/         # Test data
    ├── test_sprite.spr
    └── test_audio.wav
```

### Writing Tests
```cpp
#include "unity.h"
#include "wisp_engine.h"

void setUp(void) {
    // Set up test fixtures
}

void tearDown(void) {
    // Clean up after tests
}

void test_sprite_loading(void) {
    // Arrange
    const char* test_sprite = "test_fixtures/sprite.spr";
    
    // Act
    esp_err_t result = wisp_load_sprite(test_sprite, 1);
    
    // Assert
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_TRUE(wisp_is_sprite_loaded(1));
}

int runUnityTests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sprite_loading);
    return UNITY_END();
}
```

## 🔍 Code Review Process

### Automated Checks
All PRs must pass:
- **Build Tests** - Compile for all supported platforms
- **Code Style** - Automated formatting checks
- **Static Analysis** - Memory safety and code quality
- **Documentation** - API documentation completeness

### Manual Review
Maintainers will review:
- **Code Quality** - Readability, maintainability, performance
- **Architecture** - Fits well with existing design
- **Testing** - Adequate test coverage
- **Documentation** - Clear and complete

### Review Criteria
- [ ] Follows coding standards
- [ ] Includes appropriate tests
- [ ] Documentation is updated
- [ ] Backwards compatibility considered
- [ ] Performance impact assessed
- [ ] Security implications reviewed

## 📚 Documentation Guidelines

### README Updates
- Keep installation instructions current
- Update feature lists for new capabilities
- Maintain accurate examples
- Update supported hardware lists

### API Documentation
- Document all public functions and classes
- Include usage examples
- Note thread safety considerations
- Document performance characteristics

### User Guides
- Write step-by-step instructions
- Include screenshots when helpful
- Test all procedures
- Keep language clear and concise

## 🏷️ Commit Message Format

Use conventional commits format:

```
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

### Types
- **feat**: New feature
- **fix**: Bug fix
- **docs**: Documentation only changes
- **style**: Code style changes (formatting, etc.)
- **refactor**: Code refactoring
- **test**: Adding or modifying tests
- **chore**: Maintenance tasks

### Examples
```
feat(audio): add MP3 decoder support

Implement MP3 decoding using libmad library for background music playback.
Supports variable bitrates and sample rates up to 48kHz.

Fixes #123
```

```
fix(display): resolve SPI timing issues on ESP32-C6

The SPI clock frequency was too high for the display controller,
causing corruption in the rendered graphics.

Closes #456
```

## 🚀 Release Process

### Version Numbering
We follow [Semantic Versioning](https://semver.org/):
- **Major** (X.y.z): Breaking changes
- **Minor** (x.Y.z): New features, backwards compatible
- **Patch** (x.y.Z): Bug fixes, backwards compatible

### Release Checklist
- [ ] All tests passing
- [ ] Documentation updated
- [ ] CHANGELOG.md updated
- [ ] Version numbers incremented
- [ ] Git tag created
- [ ] Release notes written

## 🆘 Getting Help

### Communication Channels
- **GitHub Issues** - Bug reports and feature requests
- **GitHub Discussions** - General questions and ideas
- **ESP32 Forums** - Hardware-specific questions

### Mentorship
New contributors are welcome! If you're new to:
- **ESP32 development** - We can help with hardware setup
- **C++ embedded** - We can guide you through best practices
- **Open source** - We'll help you navigate the contribution process

## 📄 License

By contributing to Wisp Engine, you agree that your contributions will be licensed under the MIT License.

## 🙏 Recognition

Contributors will be recognized in:
- **CONTRIBUTORS.md** - Permanent record of contributors
- **Release Notes** - Major contributions highlighted
- **README.md** - Core team and major contributors

Thank you for helping make Wisp Engine better! 🚀
