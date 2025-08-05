// audio_engine.h - ESP32-C6/S3 Audio Engine using ESP-IDF native drivers
// Native ESP32 implementation with LEDC PWM and I2S support
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include <driver/ledc.h>
#include <driver/i2s_std.h>
// Use legacy I2S API to avoid deprecation warnings until migration
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#include <driver/i2s.h>  // Legacy I2S API for i2s_driver_uninstall
#pragma GCC diagnostic pop
#include <driver/gpio.h>

#define MAX_AUDIO_CHANNELS 16
#define AUDIO_BUFFER_SIZE 2048
#define AUDIO_SAMPLE_RATE 44100
#define PIEZO_PWM_CHANNEL 0
#define PWM_LEFT_CHANNEL 1
#define PWM_RIGHT_CHANNEL 2
#define PIEZO_PWM_FREQ 1000
#define PWM_AUDIO_FREQ 312500
#define PIEZO_PWM_RESOLUTION 8
#define PWM_AUDIO_RESOLUTION 8

// Audio output modes - can be combined with bitwise OR
enum AudioOutput {
  AUDIO_PIEZO       = 0x01,
  AUDIO_I2S_DAC     = 0x02,
  AUDIO_BLUETOOTH   = 0x04,
  AUDIO_INTERNAL_DAC= 0x08,
  AUDIO_PWM         = 0x10,
  AUDIO_ALL         = 0xFF  // Enable all available outputs
};

// Waveform types for tone generation
enum Waveform {
  WAVE_SQUARE,
  WAVE_TRIANGLE,
  WAVE_SAWTOOTH,
  WAVE_SINE,
  WAVE_NOISE
};

// Audio channel state
struct AudioChannel {
  bool active;
  uint8_t volume;           // 0-255
  uint16_t frequency;       // Hz
  Waveform waveform;
  uint32_t phase;           // For waveform generation
  uint16_t duration;        // Remaining duration in ms
  uint32_t lastUpdate;
  
  // Sample playback
  const int16_t* sampleData;
  uint32_t sampleLength;
  uint32_t samplePos;
  bool looping;
  float pitch;              // Playback speed multiplier
};

// Audio effect for simple sound synthesis
struct AudioEffect {
  uint16_t frequency;
  uint16_t duration;
  Waveform waveform;
  uint8_t volume;
  bool fadeOut;
};

class AudioEngine {
public:
  AudioChannel channels[MAX_AUDIO_CHANNELS];
  uint8_t enabledOutputs = AUDIO_PIEZO;  // Bitmask of enabled outputs
  uint8_t masterVolume = 255;
  uint32_t sampleRate = AUDIO_SAMPLE_RATE;
  bool enabled = true;
  
  // Output-specific states
  bool piezoEnabled = false;
  bool i2sEnabled = false;
  bool bluetoothEnabled = false;
  bool internalDacEnabled = false;
  bool pwmEnabled = false;
  
  // I2S channel handle for new ESP-IDF 5.x API
  i2s_chan_handle_t i2s_tx_handle = nullptr;
  
  // Bluetooth A2DP (disabled for basic compilation)
  #ifdef AUDIO_BLUETOOTH_ENABLED
  BluetoothA2DPSink* a2dpSink = nullptr;
  #endif
  
  // Audio buffers
  int16_t mixBuffer[AUDIO_BUFFER_SIZE * 2]; // Stereo
  int16_t outputBuffer[AUDIO_BUFFER_SIZE * 2];
  uint8_t dacBuffer[AUDIO_BUFFER_SIZE * 2];

  void init(uint8_t outputs = AUDIO_ALL, uint32_t rate = AUDIO_SAMPLE_RATE) {
    enabledOutputs = outputs;
    sampleRate = rate;
    
    // Initialize channels
    for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
      channels[i].active = false;
      channels[i].volume = 128;
      channels[i].phase = 0;
      channels[i].lastUpdate = get_millis();
    }
    
    // Initialize all requested outputs
    if (outputs & AUDIO_PIEZO) initPiezo();
    if (outputs & AUDIO_I2S_DAC) initI2S();
    if (outputs & AUDIO_BLUETOOTH) initBluetooth();
    if (outputs & AUDIO_INTERNAL_DAC) initInternalDAC();
    if (outputs & AUDIO_PWM) initPWM();
  }

  void initPiezo() {
    ledcSetup(PIEZO_PWM_CHANNEL, PIEZO_PWM_FREQ, PIEZO_PWM_RESOLUTION);
    ledcAttachPin(AUDIO_PIEZO_PIN, PIEZO_PWM_CHANNEL);
    piezoEnabled = true;
  }

  void initPWM() {
    // PWM audio output for higher quality than piezo
    ledcSetup(PWM_LEFT_CHANNEL, PWM_AUDIO_FREQ, PWM_AUDIO_RESOLUTION);
    ledcSetup(PWM_RIGHT_CHANNEL, PWM_AUDIO_FREQ, PWM_AUDIO_RESOLUTION);
    ledcAttachPin(AUDIO_PWM_LEFT, PWM_LEFT_CHANNEL);
    ledcAttachPin(AUDIO_PWM_RIGHT, PWM_RIGHT_CHANNEL);
    pwmEnabled = true;
  }

  void initInternalDAC() {
    // Use ESP32's built-in DAC (GPIO 25, 26)
    #ifdef DAC_CHANNEL_1_GPIO_NUM
    dacEnable(DAC_CHANNEL_1); // GPIO 25
    dacEnable(DAC_CHANNEL_2); // GPIO 26
    internalDacEnabled = true;
    #endif
  }

  void initI2S() {
    // New ESP-IDF 5.x I2S API
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    esp_err_t ret = i2s_new_channel(&chan_cfg, &i2s_tx_handle, nullptr);
    if (ret != ESP_OK) {
      ESP_LOGE("AudioEngine", "Failed to create I2S channel: %s", esp_err_to_name(ret));
      return;
    }

    i2s_std_config_t std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sampleRate),
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
      .gpio_cfg = {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = AUDIO_I2S_BCLK,
        .ws = AUDIO_I2S_LRC,
        .dout = AUDIO_I2S_DIN,
        .din = I2S_GPIO_UNUSED,
        .invert_flags = {
          .mclk_inv = false,
          .bclk_inv = false,
          .ws_inv = false
        }
      }
    };

    ret = i2s_channel_init_std_mode(i2s_tx_handle, &std_cfg);
    if (ret != ESP_OK) {
      ESP_LOGE("AudioEngine", "Failed to initialize I2S channel: %s", esp_err_to_name(ret));
      return;
    }

    ret = i2s_channel_enable(i2s_tx_handle);
    if (ret == ESP_OK) {
      i2sEnabled = true;
      ESP_LOGI("AudioEngine", "I2S initialized successfully");
    } else {
      ESP_LOGE("AudioEngine", "Failed to enable I2S channel: %s", esp_err_to_name(ret));
    }
  }

  void initBluetooth() {
    // Initialize Bluetooth A2DP for wireless audio streaming
    bluetoothEnabled = true;
    // Note: Full A2DP implementation would go here
  }

  // Play a simple tone on specified channel
  void playTone(uint8_t channel, uint16_t frequency, uint16_t duration, 
                Waveform wave = WAVE_SQUARE, uint8_t volume = 128) {
    if (channel >= MAX_AUDIO_CHANNELS || !enabled) return;
    
    AudioChannel& ch = channels[channel];
    ch.active = true;
    ch.frequency = frequency;
    ch.duration = duration;
    ch.waveform = wave;
    ch.volume = volume;
    ch.phase = 0;
    ch.lastUpdate = get_millis();
    ch.sampleData = nullptr; // Clear any sample playback
  }

  // Play a pre-defined sound effect
  void playEffect(uint8_t channel, const AudioEffect& effect) {
    playTone(channel, effect.frequency, effect.duration, 
             effect.waveform, effect.volume);
  }

  // Play a PCM sample
  void playSample(uint8_t channel, const int16_t* data, uint32_t length, 
                  bool loop = false, float pitch = 1.0f, uint8_t volume = 128) {
    if (channel >= MAX_AUDIO_CHANNELS || !enabled) return;
    
    AudioChannel& ch = channels[channel];
    ch.active = true;
    ch.sampleData = data;
    ch.sampleLength = length;
    ch.samplePos = 0;
    ch.looping = loop;
    ch.pitch = pitch;
    ch.volume = volume;
    ch.duration = 0; // Indefinite for samples
  }

  // Stop audio on specified channel
  void stop(uint8_t channel) {
    if (channel >= MAX_AUDIO_CHANNELS) return;
    channels[channel].active = false;
  }

  // Stop all audio
  void stopAll() {
    for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
      channels[i].active = false;
    }
    silenceAllOutputs();
  }

  // Enable/disable specific outputs dynamically
  void enableOutput(uint8_t output) {
    if (!(enabledOutputs & output)) {
      enabledOutputs |= output;
      // Initialize the newly enabled output
      if (output == AUDIO_PIEZO && !piezoEnabled) initPiezo();
      if (output == AUDIO_I2S_DAC && !i2sEnabled) initI2S();
      if (output == AUDIO_BLUETOOTH && !bluetoothEnabled) initBluetooth();
      if (output == AUDIO_INTERNAL_DAC && !internalDacEnabled) initInternalDAC();
      if (output == AUDIO_PWM && !pwmEnabled) initPWM();
    }
  }

  void disableOutput(uint8_t output) {
    enabledOutputs &= ~output;
  }

  // Set sample rate dynamically
  void setSampleRate(uint32_t rate) {
    if (rate != sampleRate) {
      sampleRate = rate;
      // Reinitialize I2S with new sample rate
      if (i2sEnabled) {
        i2s_driver_uninstall(I2S_NUM_0);
        initI2S();
      }
    }
  }

  // Get available outputs based on hardware
  uint8_t getAvailableOutputs() {
    uint8_t available = AUDIO_PIEZO | AUDIO_PWM; // Always available
    
    #ifdef DAC_CHANNEL_1_GPIO_NUM
    available |= AUDIO_INTERNAL_DAC;
    #endif
    
    // Check for I2S hardware availability
    available |= AUDIO_I2S_DAC;
    
    // Check for Bluetooth capability
    #ifdef CONFIG_BT_ENABLED
    available |= AUDIO_BLUETOOTH;
    #endif
    
    return available;
  }

  // Update audio engine (call from main loop)
  void update() {
    if (!enabled) return;
    
    uint32_t now = get_millis();
    
    // Update channel states
    for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
      AudioChannel& ch = channels[i];
      if (!ch.active) continue;
      
      // Handle duration-based sounds
      if (ch.duration > 0) {
        uint32_t elapsed = now - ch.lastUpdate;
        if (elapsed >= ch.duration) {
          ch.active = false;
          continue;
        }
      }
    }
    
    // Generate and output audio to all enabled outputs
    generateMixedAudio();
    outputToAllDevices();
  }

private:
  void silenceAllOutputs() {
    if (piezoEnabled) {
      ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PIEZO_PWM_CHANNEL, 0);
      ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PIEZO_PWM_CHANNEL);
    }
    if (pwmEnabled) {
      ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PWM_LEFT_CHANNEL, 128);  // Middle value for PWM
      ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PWM_LEFT_CHANNEL);
      ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PWM_RIGHT_CHANNEL, 128);
      ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PWM_RIGHT_CHANNEL);
    }
    if (internalDacEnabled) {
      #ifdef DAC_CHANNEL_1_GPIO_NUM
      dacWrite(DAC_CHANNEL_1, 128);
      dacWrite(DAC_CHANNEL_2, 128);
      #endif
    }
  }

  void generateMixedAudio() {
    // Clear mix buffer (stereo)
    memset(mixBuffer, 0, sizeof(mixBuffer));
    
    // Mix all active channels
    for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
      AudioChannel& ch = channels[i];
      if (!ch.active) continue;
      
      for (int s = 0; s < AUDIO_BUFFER_SIZE; s++) {
        int16_t sampleL = 0, sampleR = 0;
        
        if (ch.sampleData) {
          // PCM sample playback
          if (ch.samplePos < ch.sampleLength) {
            sampleL = sampleR = ch.sampleData[(uint32_t)(ch.samplePos * ch.pitch)];
            ch.samplePos++;
            if (ch.samplePos >= ch.sampleLength && ch.looping) {
              ch.samplePos = 0;
            }
          } else if (!ch.looping) {
            ch.active = false;
            break;
          }
        } else {
          // Waveform generation
          sampleL = sampleR = generateWaveform(ch.waveform, ch.phase, ch.frequency);
          ch.phase += (ch.frequency * 65536) / sampleRate;
        }
        
        // Apply volume and mix
        sampleL = (sampleL * ch.volume * masterVolume) >> 16;
        sampleR = (sampleR * ch.volume * masterVolume) >> 16;
        
        mixBuffer[s * 2] = constrain(mixBuffer[s * 2] + sampleL, -32767, 32767);
        mixBuffer[s * 2 + 1] = constrain(mixBuffer[s * 2 + 1] + sampleR, -32767, 32767);
      }
    }
  }

  void outputToAllDevices() {
    // Output to I2S DAC
    if (enabledOutputs & AUDIO_I2S_DAC && i2sEnabled && i2s_tx_handle) {
      size_t bytesWritten;
      i2s_channel_write(i2s_tx_handle, mixBuffer, sizeof(mixBuffer), &bytesWritten, 0);
    }
    
    // Output to PWM
    if (enabledOutputs & AUDIO_PWM && pwmEnabled) {
      outputToPWM();
    }
    
    // Output to internal DAC
    if (enabledOutputs & AUDIO_INTERNAL_DAC && internalDacEnabled) {
      outputToInternalDAC();
    }
    
    // Output to piezo (simplified)
    if (enabledOutputs & AUDIO_PIEZO && piezoEnabled) {
      outputToPiezo();
    }
    
    // Output to Bluetooth
    if (enabledOutputs & AUDIO_BLUETOOTH && bluetoothEnabled) {
      outputToBluetooth();
    }
  }

  void outputToPWM() {
    // Convert stereo mix to PWM values
    for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
      uint8_t leftPWM = ((mixBuffer[i * 2] + 32768) >> 8);
      uint8_t rightPWM = ((mixBuffer[i * 2 + 1] + 32768) >> 8);
      ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PWM_LEFT_CHANNEL, leftPWM);
      ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PWM_LEFT_CHANNEL);
      ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PWM_RIGHT_CHANNEL, rightPWM);
      ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PWM_RIGHT_CHANNEL);
      delayMicroseconds(1000000 / sampleRate); // Timing for PWM updates
    }
  }

  void outputToInternalDAC() {
    #ifdef DAC_CHANNEL_1_GPIO_NUM
    for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
      uint8_t leftDAC = ((mixBuffer[i * 2] + 32768) >> 8);
      uint8_t rightDAC = ((mixBuffer[i * 2 + 1] + 32768) >> 8);
      dacWrite(DAC_CHANNEL_1, leftDAC);
      dacWrite(DAC_CHANNEL_2, rightDAC);
    }
    #endif
  }

  void outputToPiezo() {
    // Simple piezo output - use mono mix
    for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
      AudioChannel& ch = channels[i];
      if (ch.active) {
        uint32_t pwmValue = (ch.volume * masterVolume) >> 8;
        pwmValue = (pwmValue * 255) >> 8;
        
        // Set frequency by configuring timer
        ledc_timer_config_t ledc_timer = {
          .speed_mode = LEDC_LOW_SPEED_MODE,
          .duty_resolution = LEDC_TIMER_8_BIT,
          .timer_num = LEDC_TIMER_0,
          .freq_hz = ch.frequency,
          .clk_cfg = LEDC_AUTO_CLK,
          .deconfigure = false
        };
        ledc_timer_config(&ledc_timer);
        
        ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PIEZO_PWM_CHANNEL, pwmValue);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PIEZO_PWM_CHANNEL);
        return;
      }
    }
    ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PIEZO_PWM_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PIEZO_PWM_CHANNEL);
  }

  void outputToBluetooth() {
    // Stream mixed audio to Bluetooth A2DP
    // Implementation would depend on specific Bluetooth library
  }

  int16_t generateWaveform(Waveform wave, uint32_t phase, uint16_t frequency) {
    uint16_t phaseIndex = phase >> 16; // Get high 16 bits
    
    switch (wave) {
      case WAVE_SQUARE:
        return (phaseIndex < 32768) ? 16383 : -16383;
      
      case WAVE_TRIANGLE:
        if (phaseIndex < 32768) {
          return (phaseIndex >> 1) - 16383;
        } else {
          return 16383 - ((phaseIndex - 32768) >> 1);
        }
      
      case WAVE_SAWTOOTH:
        return phaseIndex - 32768;
      
      case WAVE_SINE:
        // Simple sine approximation
        return sin(2.0 * PI * phaseIndex / 65536.0) * 16383;
      
      case WAVE_NOISE:
        return random(-16383, 16383);
      
      default:
        return 0;
    }
  }
};

// Pre-defined sound effects
namespace SoundEffects {
  static const AudioEffect BEEP = {800, 100, WAVE_SQUARE, 128, false};
  static const AudioEffect CLICK = {1200, 50, WAVE_SQUARE, 100, false};
  static const AudioEffect ERROR = {300, 200, WAVE_SQUARE, 150, false};
  static const AudioEffect SUCCESS = {600, 150, WAVE_TRIANGLE, 120, false};
  static const AudioEffect NOTIFICATION = {440, 300, WAVE_SINE, 100, true};
}
