# Wisp Eng- **Bootloader & Engine**: Acts as both the system bootloader and the runtime app engine.
- **App Loading**: Loads apps from SD card; supports both bundled ROMs and unbundled raw files.
- **Lua Scripting**: App logic is written in Lua, interpreted at runtime.
- **Audio System**: Multi-output audio supporting piezo buzzer, I2S DAC, and Bluetooth A2DP streaming.
- **Connectivity**: Built-in support for WiFi and Bluetooth.
- **Persistence**: Uses I2C EEPROM for saving app state and settings.
- **Lightweight Rendering**: Optimized for the ESP32-C6's capabilities.
- **Variable Frame Rate**: The engine can function at different frame rates: 8, 10, 12, 14, or 16 FPS. These specific values are chosen to balance smooth animation with the ESP32-C6's limited processing power and to ensure consistent timing for app logic across a range of performance profiles.ign Doc

## Overview

**Wisp Engine** is a lightweight app engine and bootloader for the ESP32-C6, inspired by the Game Boy Advance. It loads apps from an SD card, interprets Lua scripts for app logic, and supports both bundled ROMs and raw file formats. The engine provides WiFi and Bluetooth connectivity to apps through sandboxed APIs, ensuring secure access, and uses I2C EEPROM for state persistence.

---

## Features

- **Bootloader & Engine**: Acts as both the system bootloader and the runtime game engine.
- **Game Loading**: Loads games from SD card; supports both bundled ROMs and unbundled raw files.
- **Lua Scripting**: Game logic is written in Lua, interpreted at runtime.
- **Connectivity**: Built-in support for WiFi and Bluetooth.
- **Persistence**: Uses I2C EEPROM for saving game state and settings.
- **Lightweight Rendering**: Optimized for the ESP32-C6’s capabilities.
- **Variable Frame Rate**: The engine can function at different frame rates: 8, 10, 12, 14, or 16 FPS. These specific values are chosen to balance smooth animation with the ESP32-C6's limited processing power and to ensure consistent timing for game logic across a range of performance profiles.

---


---

## Texture Format

Wisp Engine uses a custom compressed binary texture format designed for efficient storage and flexible rendering:

- **Structure**: Each texture entry contains a color index and a state, defined by (col, row, state), forming an (x, y, z) composition. The z (state) value is optional and can be omitted for simple textures.
- **Color Index**: The color index is determined by looking up the (col, row) coordinates in a 2D 256-color palette grid, enabling fast palette swapping for effects and themes.
- **State (z)**: The primary purpose of the state value (z) is depth sorting, with a range of 0-12. It can also be used for animation frames or alternate tile states. Lighting effects are achieved via palette swaps, and transparency is not supported (cutout style only, meaning pixels are either fully opaque or fully transparent, with no partial transparency).
- **Compression**: The format is binary and compressed for minimal storage footprint on the ESP32-C6.

### Example Binary Layout

| Byte(s) | Field        | Description                       |
|---------|--------------|-----------------------------------|
| 0       | col (x)      | Column position                   |
| 1       | row (y)      | Row position                      |
| 2       | state (z)    | Depth (0-12), also for animation/alt states (optional) |

For simple textures, the state byte can be omitted, resulting in a 2-byte entry (col, row).  
At runtime, the engine distinguishes between 2-byte and 3-byte entries either by a header flag in the texture file format or by specifying the entry size in the file metadata, ensuring correct parsing of each texture entry.

### Palette Swapping

The 256-color palette is stored as a grid, allowing for dynamic palette swapping at runtime. This supports effects such as color cycling, theming, and efficient memory usage.

---

## Audio System

Wisp Engine provides a flexible multi-output audio system designed for the ESP32's capabilities:

### **Audio Outputs**
1. **Piezo Buzzer**: Simple tones, beeps, and basic chiptune-style music
2. **I2S DAC**: High-quality PCM audio output for external speakers/headphones  
3. **Bluetooth A2DP**: Stream audio to Bluetooth speakers/headphones
4. **Internal DAC**: Use ESP32's built-in DAC for direct analog output
5. **PWM Audio**: Software-generated PWM audio on any GPIO pin

*All outputs can be used simultaneously or individually based on app requirements and hardware availability.*

### **Audio Formats & Capabilities**
- **Tone Generation**: Square wave, sawtooth, triangle, sine, and noise waveforms
- **PCM Playback**: 8/16-bit mono/stereo samples at configurable sample rates (8kHz - 48kHz)
- **Chiptune Synthesis**: Multi-channel tracker-style music (up to 16 channels)
- **MIDI Support**: Basic MIDI file playback and real-time MIDI generation
- **Streaming Audio**: MP3, OGG, and WAV file streaming from SD card
- **Sound Effects**: Short samples with pitch/volume modulation and real-time effects
- **Real-time Audio Processing**: Filters, reverb, delay, and distortion effects
- **Audio Recording**: Record audio input (if microphone is connected)
- **Cross-fade & Mixing**: Seamless transitions between audio sources

### **Memory-Efficient Design**
- **Streaming Audio**: Large audio files streamed from SD card to save RAM
- **Compressed Samples**: Custom compression for sound effects and music
- **Procedural Audio**: Mathematical sound generation to minimize storage
- **Audio Pools**: Pre-allocated buffers to prevent memory fragmentation

### **Hardware Configuration**
```cpp
// All audio outputs supported simultaneously
#define AUDIO_PIEZO_PIN     21    // Piezo buzzer (PWM)
#define AUDIO_PWM_LEFT      18    // PWM audio left channel
#define AUDIO_PWM_RIGHT     19    // PWM audio right channel

// I2S DAC configuration
#define AUDIO_I2S_BCLK      26    // Bit clock
#define AUDIO_I2S_LRC       25    // Left/Right clock
#define AUDIO_I2S_DIN       22    // Data input

// Internal DAC pins (ESP32 only)
#define AUDIO_DAC_LEFT      25    // GPIO 25 (DAC1)
#define AUDIO_DAC_RIGHT     26    // GPIO 26 (DAC2)

// Audio input (optional microphone)
#define AUDIO_INPUT_PIN     36    // ADC input for recording

// Engine settings (app-configurable)
#define AUDIO_MAX_SAMPLE_RATE 48000
#define AUDIO_MAX_CHANNELS    16
#define AUDIO_MAX_BUFFER_SIZE 2048
```

---
## Architecture

- **Bootloader**: Minimal system initialization with basic audio (piezo only) for boot feedback
- **App Discovery**: Scans SD card for apps and reads their configuration headers
- **Dynamic Audio Setup**: Audio system is configured per-app based on requirements in config.json
- **ROM Format**: Bundled apps are packaged as ROMs (single archive files containing all app assets and scripts); the engine can also load apps as raw files (individual Lua scripts and asset files in a directory). See the "Next Steps" section for format definitions.
- **App Configuration**: Each app declares its audio, performance, and system requirements
- **Lua Interpreter**: Executes app logic scripts from the loaded app with app-specific APIs exposed
- **Renderer**: Minimal 2D renderer optimized for ESP32-C6.
- **I/O Handlers**: Manages input, display, audio, and communication interfaces.
- **Persistence Layer**: Reads/writes app state to I2C EEPROM; this layer is abstracted and exposed to apps via a secure API, allowing apps to save and load their own state.

---

## File Structure Example

Apps can be loaded either as a folder containing raw files or as a single bundled ROM file. Only one format is used per app.

**Raw files format:**

```
/sdcard/
    /apps/
        /myapp/
            config.json      # App configuration and requirements
            main.lua         # Primary app script
            audio_setup.lua  # Optional audio configuration script
            assets/
                sounds/
                sprites/
                palettes/
            data/
        myapp.rom           # Alternative: bundled ROM format
```

**App Configuration (config.json):**

```json
{
  "name": "My App",
  "version": "1.0.0",
  "audio": {
    "outputs": ["i2s", "bluetooth"],
    "sampleRate": 44100,
    "channels": 8,
    "streaming": true,
    "effects": true
  },
  "performance": {
    "fps": 24,
    "ram": 65536
  },
  "system": {
    "wifi": true,
    "bluetooth": true,
    "eeprom": true
  },
  "entry": {
    "main": "main.lua",
    "config": "audio_setup.lua"
  }
}
```

---

## Next Steps

1. Define ROM and raw file formats with configuration headers.
2. Complete SD card mounting and app discovery logic.
3. Integrate Lua interpreter with dynamic API exposure based on app config.
4. Implement minimal 2D rendering routines.
5. Add I2C EEPROM persistence.
6. Expose WiFi/Bluetooth APIs to Lua based on app requirements.
7. Implement audio streaming from SD card for large audio files.
8. Add audio compression/decompression for efficient storage.
9. Create app development tools and templates.
10. Add app validation and security checks.
