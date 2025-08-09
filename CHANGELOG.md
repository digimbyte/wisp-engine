# Changelog

All notable changes to the Wisp Engine project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Repository restructuring to AAA standards
- Comprehensive documentation organization
- Professional README.md with badges and structured content
- Enhanced .gitignore with comprehensive coverage
- Waveshare board documentation enhancements

### Changed
- Moved all documentation to `docs/` directory
- Reorganized configuration files to `configs/` directory
- Moved build tools to `tools/` directory
- Updated repository structure for better maintainability

### Removed
- Empty placeholder files (todo.md, test_namespaces*.cpp)
- Build artifacts from version control
- Temporary analysis files

### Fixed
- Inconsistent file naming conventions
- Scattered documentation files
- Missing professional repository structure

## [0.9.0] - 2025-08-09 - Pre-AAA State

### Added
- Core Wisp Engine system implementation
- Universal LED controller with multiple LED type support
- System initialization framework matching ESP-IDF patterns
- Audio API with background music and sound effects
- Database system for asset management
- Memory management and optimization features
- Multi-platform ESP32 support (ESP32, ESP32-S3, ESP32-C6)

### Features
- High-performance 2D graphics engine
- Frame rate management with adaptive scaling
- Component-based architecture
- FreeRTOS integration
- Hardware abstraction layer
- Comprehensive example collection

### Supported Hardware
- Waveshare ESP32-C6-LCD-1.47
- Waveshare ESP32-S3-LCD-2
- Waveshare ESP32-S3-Touch-AMOLED-2.06
- Generic ESP32 development boards

### Development Tools
- ESP-IDF integration
- PlatformIO support
- CMake build system
- Asset compilation pipeline

---

## Version Format

This project uses [Semantic Versioning](https://semver.org/):
- **MAJOR.MINOR.PATCH** (e.g., 1.2.3)
- **MAJOR**: Incompatible API changes
- **MINOR**: Backwards-compatible functionality additions
- **PATCH**: Backwards-compatible bug fixes

## Release Types

- **🚀 Major Release**: Significant new features, API changes
- **✨ Minor Release**: New features, improvements, backwards-compatible
- **🔧 Patch Release**: Bug fixes, security updates, documentation
- **🏗️ Pre-release**: Alpha, Beta, RC versions for testing

## Contributing to Changelog

When contributing, please:
1. Add entries to the `[Unreleased]` section
2. Use appropriate category (Added, Changed, Deprecated, Removed, Fixed, Security)
3. Write clear, concise descriptions
4. Include relevant issue/PR numbers
5. Follow the established format

## Categories

- **Added**: New features
- **Changed**: Changes in existing functionality
- **Deprecated**: Soon-to-be removed features
- **Removed**: Now removed features
- **Fixed**: Bug fixes
- **Security**: Security vulnerability fixes
