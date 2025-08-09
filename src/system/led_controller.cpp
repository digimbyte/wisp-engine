// led_controller.cpp - Generic LED Controller Implementation
#include "led_controller.h"
#include "led_implementations.h"
#include "esp_log.h"
#include <cmath>
#include <algorithm>
#include <vector>

static const char* TAG = "LEDController";

// Color presets
const LEDColor LEDColor::BLACK   = LEDColor(0, 0, 0);
const LEDColor LEDColor::WHITE   = LEDColor(255, 255, 255);
const LEDColor LEDColor::RED     = LEDColor(255, 0, 0);
const LEDColor LEDColor::GREEN   = LEDColor(0, 255, 0);
const LEDColor LEDColor::BLUE    = LEDColor(0, 0, 255);
const LEDColor LEDColor::YELLOW  = LEDColor(255, 255, 0);
const LEDColor LEDColor::CYAN    = LEDColor(0, 255, 255);
const LEDColor LEDColor::MAGENTA = LEDColor(255, 0, 255);
const LEDColor LEDColor::ORANGE  = LEDColor(255, 165, 0);
const LEDColor LEDColor::PURPLE  = LEDColor(128, 0, 128);
const LEDColor LEDColor::PINK    = LEDColor(255, 192, 203);

// Static member initialization
LEDController* LEDController::instance = nullptr;

LEDController::LEDController() 
    : ledType(WISP_LED_TYPE), ledPin(RGB_LED_PIN), ledCount(RGB_LED_COUNT),
      initialized(false), globalBrightness(1.0f), isDirty(true),
      animationActive(false), lastUpdateTime(0), implementation(nullptr),
      customCallback(nullptr), customUserData(nullptr) {
    
    globalColor = LEDColor::BLACK;
    ledColors.resize(ledCount);
}

LEDController::~LEDController() {
    shutdown();
}

LEDController& LEDController::getInstance() {
    if (!instance) {
        instance = new LEDController();
    }
    return *instance;
}

bool LEDController::init() {
    if (initialized) {
        ESP_LOGW(TAG, "LED Controller already initialized");
        return true;
    }
    
    if (!WISP_HAS_LED) {
        ESP_LOGW(TAG, "LED not available on this board");
        return false;
    }
    
    // Create appropriate implementation based on LED type
    if (!initImplementation()) {
        return false;
    }

    initialized = true;
    clear();
    show();
    
    ESP_LOGI(TAG, "LED Controller initialized successfully");
    return true;
}

void LEDController::shutdown() {
    if (!initialized) return;
    
    // Stop any active animations
    stopAnimation();
    
    // Clear all LEDs
    clear();
    show();
    
    // Shutdown implementation
    shutdownImplementation();
    
    initialized = false;
    ESP_LOGI(TAG, "LED Controller shutdown");
}

bool LEDController::initImplementation() {
    delete implementation; // Clean up any existing implementation
    implementation = nullptr;

    #ifdef RGB_LED_TYPE
    switch (ledType) {
        case LEDType::SIMPLE_GPIO:
            implementation = new SimpleGPIOLED();
            break;
        
        case LEDType::PWM_RGB:
            implementation = new PWMRGBLED();
            break;
        
        case LEDType::WS2812_RGB:
            implementation = new WS2812LED();
            break;
        
        case LEDType::APA102_RGB:
            implementation = new APA102LED();
            break;
        
        case LEDType::NONE:
        default:
            return false;
    }

    if (implementation && implementation->init(ledPin, ledCount)) {
        ESP_LOGI(TAG, "LED implementation initialized: pin=%d, count=%d, type=%s", 
                 ledPin, ledCount, RGB_LED_TYPE);
        return true;
    }
    
    delete implementation;
    implementation = nullptr;
    #endif
    
    return false;
}

void LEDController::shutdownImplementation() {
    if (implementation) {
        implementation->shutdown();
        delete implementation;
        implementation = nullptr;
    }
}

bool LEDController::initRMT() {
    #if !WISP_HAS_RGB_LED
    return false;
    #endif
    
    // Create RMT TX channel configuration for WS2812
    rmt_tx_channel_config_t tx_chan_config = {
        .gpio_num = (gpio_num_t)ledPin,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10000000, // 10MHz resolution
        .mem_block_symbols = 64,   // Memory block size
        .trans_queue_depth = 4,    // Transaction queue depth
    };
    
    esp_err_t ret = rmt_new_tx_channel(&tx_chan_config, &rmtTxChannel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "RMT TX channel creation failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Enable the RMT TX channel
    ret = rmt_enable(rmtTxChannel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "RMT TX channel enable failed: %s", esp_err_to_name(ret));
        rmt_del_channel(rmtTxChannel);
        rmtTxChannel = nullptr;
        return false;
    }
    
    ESP_LOGI(TAG, "RMT TX channel initialized for WS2812 on GPIO %d", ledPin);
    return true;
}

void LEDController::shutdownRMT() {
    #if WISP_HAS_RGB_LED
    if (rmtTxChannel) {
        rmt_disable(rmtTxChannel);
        rmt_del_channel(rmtTxChannel);
        rmtTxChannel = nullptr;
    }
    #endif
}

void LEDController::setLED(int index, const LEDColor& color) {
    if (!initialized || !implementation || index < 0 || index >= ledCount) return;
    
    ledColors[index] = color;
    LEDColor scaledColor = color.scale(globalBrightness);
    implementation->setLED(index, scaledColor);
    isDirty = true;
}

void LEDController::setLED(int index, uint8_t r, uint8_t g, uint8_t b) {
    setLED(index, LEDColor(r, g, b));
}

void LEDController::setAll(const LEDColor& color) {
    if (!initialized || !implementation) return;
    
    globalColor = color;
    for (int i = 0; i < ledCount; i++) {
        ledColors[i] = color;
    }
    
    LEDColor scaledColor = color.scale(globalBrightness);
    implementation->setAll(scaledColor);
    isDirty = true;
}

void LEDController::setAll(uint8_t r, uint8_t g, uint8_t b) {
    setAll(LEDColor(r, g, b));
}

LEDColor LEDController::getLED(int index) const {
    if (!initialized || index < 0 || index >= ledCount) {
        return LEDColor::BLACK;
    }
    return ledColors[index];
}

void LEDController::clear() {
    if (!initialized || !implementation) return;
    
    setAll(LEDColor::BLACK);
    implementation->clear();
}

void LEDController::show() {
    if (!initialized || !implementation) return;
    
    implementation->show();
    isDirty = false;
}

void LEDController::setBrightness(float brightness) {
    globalBrightness = std::max(0.0f, std::min(1.0f, brightness));
    isDirty = true;
}

void LEDController::fadeAllTo(const LEDColor& target, uint32_t durationMs) {
    if (!initialized) return;
    
    LEDAnimation anim;
    anim.type = LED_ANIM_FADE;
    anim.startColor = globalColor; // Use current global color as start
    anim.endColor = target;
    anim.duration = durationMs;
    anim.repeat = false;
    
    startAnimation(anim);
}

void LEDController::fadeAllTo(uint8_t r, uint8_t g, uint8_t b, uint32_t durationMs) {
    fadeAllTo(LEDColor(r, g, b), durationMs);
}

// === NEW ANIMATION METHODS ===

void LEDController::fadeTo(int index, const LEDColor& target, uint32_t durationMs) {
    if (!initialized || index < 0 || index >= ledCount) return;
    
    // Create a custom single-LED fade animation
    LEDAnimation anim;
    anim.type = LED_ANIM_CUSTOM;
    anim.startColor = ledColors[index];
    anim.endColor = target;
    anim.duration = durationMs;
    anim.repeat = false;
    
    // Store the target LED index for the custom callback
    static int targetIndex = index;
    customUserData = &targetIndex;
    
    // Set custom callback for single LED fade
    customCallback = [](int ledIndex, uint32_t time, void* userData) -> LEDColor {
        int* targetIdx = (int*)userData;
        if (ledIndex == *targetIdx) {
            // Calculate progress for this specific LED
            LEDController& controller = LEDController::getInstance();
            float progress = controller.calculateAnimationProgress();
            return controller.currentAnimation.startColor.blend(controller.currentAnimation.endColor, progress);
        }
        // Return current color for other LEDs
        return LEDController::getInstance().getLED(ledIndex);
    };
    
    startAnimation(anim);
}

void LEDController::fadeTo(int index, uint8_t r, uint8_t g, uint8_t b, uint32_t durationMs) {
    fadeTo(index, LEDColor(r, g, b), durationMs);
}

void LEDController::crossFade(const LEDColor& colorA, const LEDColor& colorB, uint32_t durationMs) {
    if (!initialized) return;
    
    LEDAnimation anim;
    anim.type = LED_ANIM_FADE;
    anim.startColor = colorA;
    anim.endColor = colorB;
    anim.duration = durationMs;
    anim.repeat = false;
    anim.reverse = true;  // Will reverse back to colorA when done
    
    startAnimation(anim);
}

// Enhanced brightness fade with animation support
void LEDController::fadeBrightness(float targetBrightness, uint32_t durationMs) {
    if (!initialized) return;
    
    float startBrightness = globalBrightness;
    float endBrightness = std::max(0.0f, std::min(1.0f, targetBrightness));
    
    // Create custom brightness fade animation
    LEDAnimation anim;
    anim.type = LED_ANIM_CUSTOM;
    anim.duration = durationMs;
    anim.repeat = false;
    
    // Store brightness values in a static struct for the callback
    static struct {
        float start;
        float end;
    } brightnessData;
    
    brightnessData.start = startBrightness;
    brightnessData.end = endBrightness;
    customUserData = &brightnessData;
    
    // Set custom callback for brightness fade
    customCallback = [](int ledIndex, uint32_t time, void* userData) -> LEDColor {
        auto* data = (decltype(brightnessData)*)userData;
        LEDController& controller = LEDController::getInstance();
        
        float progress = controller.calculateAnimationProgress();
        float currentBrightness = data->start + (data->end - data->start) * progress;
        
        // Update global brightness and return scaled color
        controller.globalBrightness = currentBrightness;
        return controller.ledColors[ledIndex].scale(currentBrightness);
    };
    
    startAnimation(anim);
}

// === ADDITIONAL UTILITY METHODS ===

void LEDController::reset() {
    if (!initialized) return;
    
    stopAnimation();
    clear();
    setBrightness(1.0f);
    globalColor = LEDColor::BLACK;
    show();
    
    ESP_LOGI(TAG, "LED Controller reset to defaults");
}

void LEDController::setCurrentColor(const LEDColor& color) {
    if (!initialized) return;
    
    globalColor = color;
    setAll(color);
}

void LEDController::setCurrentColor(uint8_t r, uint8_t g, uint8_t b) {
    setCurrentColor(LEDColor(r, g, b));
}

LEDColor LEDController::getCurrentColor() const {
    return globalColor;
}

// Enhanced fade that uses current state as starting point
void LEDController::fadeToFromCurrent(const LEDColor& target, uint32_t durationMs) {
    if (!initialized) return;
    
    LEDAnimation anim;
    anim.type = LED_ANIM_FADE;
    anim.startColor = globalColor;  // Start from current global color
    anim.endColor = target;
    anim.duration = durationMs;
    anim.repeat = false;
    
    startAnimation(anim);
}

void LEDController::fadeToFromCurrent(uint8_t r, uint8_t g, uint8_t b, uint32_t durationMs) {
    fadeToFromCurrent(LEDColor(r, g, b), durationMs);
}

// Instant transition (no animation)
void LEDController::transitionTo(const LEDColor& target) {
    if (!initialized) return;
    
    stopAnimation();
    setAll(target);
    show();
}

void LEDController::transitionTo(uint8_t r, uint8_t g, uint8_t b) {
    transitionTo(LEDColor(r, g, b));
}

void LEDController::startAnimation(LEDAnimationType type, uint32_t durationMs, bool repeat) {
    LEDAnimation anim;
    anim.type = type;
    anim.duration = durationMs;
    anim.repeat = repeat;
    
    startAnimation(anim);
}

void LEDController::startAnimation(const LEDAnimation& animation) {
    if (!initialized) return;
    
    currentAnimation = animation;
    currentAnimation.startTime = get_millis();
    animationActive = true;
    
    ESP_LOGI(TAG, "Started animation type %d, duration %dms, repeat %s",
             (int)animation.type, (int)animation.duration, 
             animation.repeat ? "true" : "false");
}

void LEDController::stopAnimation() {
    animationActive = false;
    ESP_LOGI(TAG, "Animation stopped");
}

void LEDController::pulse(const LEDColor& color, uint32_t durationMs, bool repeat) {
    LEDAnimation anim;
    anim.type = LED_ANIM_PULSE;
    anim.startColor = LEDColor::BLACK;
    anim.endColor = color;
    anim.duration = durationMs;
    anim.repeat = repeat;
    
    startAnimation(anim);
}

void LEDController::breathe(const LEDColor& color, uint32_t durationMs, bool repeat) {
    LEDAnimation anim;
    anim.type = LED_ANIM_BREATHE;
    anim.startColor = LEDColor::BLACK;
    anim.endColor = color;
    anim.duration = durationMs;
    anim.repeat = repeat;
    
    startAnimation(anim);
}

void LEDController::rainbow(uint32_t durationMs, bool repeat) {
    LEDAnimation anim;
    anim.type = LED_ANIM_RAINBOW;
    anim.duration = durationMs;
    anim.repeat = repeat;
    
    startAnimation(anim);
}

void LEDController::colorWipe(const LEDColor& color, uint32_t durationMs) {
    LEDAnimation anim;
    anim.type = LED_ANIM_COLOR_WIPE;
    anim.endColor = color;
    anim.duration = durationMs;
    anim.repeat = false;
    
    startAnimation(anim);
}

void LEDController::theaterChase(const LEDColor& color, uint32_t durationMs, bool repeat) {
    LEDAnimation anim;
    anim.type = LED_ANIM_THEATER_CHASE;
    anim.endColor = color;
    anim.duration = durationMs;
    anim.repeat = repeat;
    
    startAnimation(anim);
}

void LEDController::fire(uint32_t durationMs, bool repeat) {
    LEDAnimation anim;
    anim.type = LED_ANIM_FIRE;
    anim.duration = durationMs;
    anim.repeat = repeat;
    
    startAnimation(anim);
}

void LEDController::update() {
    if (!initialized) return;
    
    uint32_t currentTime = get_millis();
    
    // Update animation
    if (animationActive) {
        updateAnimation();
        isDirty = true;
    }
    
    // Auto-update display if dirty
    if (isDirty) {
        show();
    }
    
    lastUpdateTime = currentTime;
}

void LEDController::updateAnimation() {
    if (!animationActive) return;
    
    float progress = calculateAnimationProgress();
    
    // Check if animation is complete
    if (progress >= 1.0f) {
        if (currentAnimation.repeat) {
            // Restart animation
            currentAnimation.startTime = get_millis();
            progress = 0.0f;
        } else {
            // Stop animation
            stopAnimation();
            return;
        }
    }
    
    // Update LED colors based on animation type
    for (int i = 0; i < ledCount; i++) {
        LEDColor newColor = calculateAnimationColor(i, progress);
        ledColors[i] = newColor.scale(globalBrightness);
    }
}

float LEDController::calculateAnimationProgress() {
    uint32_t elapsed = get_millis() - currentAnimation.startTime;
    float progress = (float)elapsed / currentAnimation.duration;
    
    // Apply speed multiplier
    progress *= currentAnimation.speed;
    
    return std::min(progress, 1.0f);
}

LEDColor LEDController::calculateAnimationColor(int ledIndex, float progress) {
    switch (currentAnimation.type) {
        case LED_ANIM_FADE: {
            return currentAnimation.startColor.blend(currentAnimation.endColor, progress);
        }
        
        case LED_ANIM_PULSE: {
            float intensity = (sin(progress * 2 * M_PI) + 1.0f) * 0.5f;
            return currentAnimation.endColor.scale(intensity);
        }
        
        case LED_ANIM_BREATHE: {
            float intensity = (sin(progress * M_PI - M_PI_2) + 1.0f) * 0.5f;
            return currentAnimation.endColor.scale(intensity);
        }
        
        case LED_ANIM_RAINBOW: {
            uint8_t wheelPos = (uint8_t)(progress * 255);
            return wheel(wheelPos);
        }
        
        case LED_ANIM_RAINBOW_CHASE: {
            float temp = progress * 255 + ledIndex * 255 / ledCount;
            uint8_t wheelPos = (uint8_t)((int)temp % 255);
            return wheel(wheelPos);
        }
        
        case LED_ANIM_COLOR_WIPE: {
            int activeIndex = (int)(progress * ledCount);
            return (ledIndex <= activeIndex) ? currentAnimation.endColor : LEDColor::BLACK;
        }
        
        case LED_ANIM_THEATER_CHASE: {
            int phase = (int)(progress * 3) % 3;
            return (ledIndex % 3 == phase) ? currentAnimation.endColor : LEDColor::BLACK;
        }
        
        case LED_ANIM_FIRE: {
            // Simulate fire flicker
            float flicker = (random(100)) / 100.0f;
            float intensity = 0.5f + flicker * 0.5f;
            LEDColor fireColor(255, (uint8_t)(100 + flicker * 155), 0);
            return fireColor.scale(intensity);
        }
        
        case LED_ANIM_CUSTOM: {
            if (customCallback) {
                return customCallback(ledIndex, get_millis(), customUserData);
            }
            break;
        }
        
        default:
            break;
    }
    
    return LEDColor::BLACK;
}

void LEDController::sendData() {
    #if !WISP_HAS_RGB_LED
    return;
    #endif
    
    if (!rmtTxChannel) {
        ESP_LOGE(TAG, "RMT TX channel not initialized");
        return;
    }
    
    // Convert colors to RMT symbols
    std::vector<rmt_symbol_word_t> rmt_data;
    rmt_data.reserve(ledCount * 24 + 1); // 24 bits per LED + reset pulse
    
    for (int i = 0; i < ledCount; i++) {
        LEDColor color = ledColors[i].scale(globalBrightness);
        uint32_t colorGRB = colorToGRB(color);
        
        // Send 24 bits (G8R8B8) 
        for (int bit = 23; bit >= 0; bit--) {
            rmt_symbol_word_t symbol;
            bool bitValue = (colorGRB >> bit) & 1;
            
            if (bitValue) {
                // Send '1' bit: high for T1H, low for T1L
                symbol.level0 = 1;
                symbol.duration0 = 7;  // ~700ns at 10MHz
                symbol.level1 = 0;
                symbol.duration1 = 6;  // ~600ns at 10MHz
            } else {
                // Send '0' bit: high for T0H, low for T0L  
                symbol.level0 = 1;
                symbol.duration0 = 3;  // ~300ns at 10MHz
                symbol.level1 = 0;
                symbol.duration1 = 9;  // ~900ns at 10MHz
            }
            
            rmt_data.push_back(symbol);
        }
    }
    
    // Add reset pulse
    rmt_symbol_word_t resetSymbol;
    resetSymbol.level0 = 0;
    resetSymbol.duration0 = 500; // ~50us at 10MHz 
    resetSymbol.level1 = 0;
    resetSymbol.duration1 = 0;
    rmt_data.push_back(resetSymbol);
    
    // Prepare transmission configuration
    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // No loop
    };
    
    // Send data via new RMT TX API
    esp_err_t ret = rmt_transmit(rmtTxChannel, rmt_data.data(), rmt_data.size() * sizeof(rmt_symbol_word_t), &tx_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "RMT transmit failed: %s", esp_err_to_name(ret));
    }
}

uint32_t LEDController::colorToGRB(const LEDColor& color) const {
    return ((uint32_t)color.g << 16) | ((uint32_t)color.r << 8) | color.b;
}

// Static utility functions
LEDColor LEDController::HSV(float h, float s, float v) {
    int i = (int)(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);
    
    float r, g, b;
    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
        default: r = g = b = 0; break;
    }
    
    return LEDColor((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
}

LEDColor LEDController::wheel(uint8_t pos) {
    if (pos < 85) {
        return LEDColor(pos * 3, 255 - pos * 3, 0);
    } else if (pos < 170) {
        pos -= 85;
        return LEDColor(255 - pos * 3, 0, pos * 3);
    } else {
        pos -= 170;
        return LEDColor(0, pos * 3, 255 - pos * 3);
    }
}

LEDColor LEDController::gamma32(const LEDColor& color) {
    // Simple gamma correction (2.8)
    static const uint8_t gamma8[] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
        2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
        5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
       10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
       17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
       25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
       37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
       51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
       69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
       90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
      115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
      144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
      177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
      215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };
    
    return LEDColor(gamma8[color.r], gamma8[color.g], gamma8[color.b]);
}

// Status indication functions
void LEDController::showStatus(const LEDColor& color, uint32_t durationMs) {
    setAll(color);
    show();
    if (durationMs > 0) {
        delay(durationMs);
        clear();
        show();
    }
}

void LEDController::showError(uint32_t durationMs) {
    pulse(LEDColor::RED, durationMs, false);
}

void LEDController::showWarning(uint32_t durationMs) {
    pulse(LEDColor::ORANGE, durationMs, false);
}

void LEDController::showSuccess(uint32_t durationMs) {
    pulse(LEDColor::GREEN, durationMs, false);
}

void LEDController::showInfo(uint32_t durationMs) {
    pulse(LEDColor::BLUE, durationMs, false);
}

void LEDController::showBootSequence() {
    if (!initialized) return;
    
    // Rainbow sweep during boot
    rainbow(2000, false);
    delay(2000);
    
    // Final green pulse to indicate ready
    pulse(LEDColor::GREEN, 500, false);
    delay(500);
    clear();
    show();
}

void LEDController::showLowBattery() {
    // Slow red breathing effect
    breathe(LEDColor::RED, 3000, true);
}

void LEDController::setCustomAnimation(AnimationCallback callback, void* userData, 
                                     uint32_t durationMs, bool repeat) {
    customCallback = callback;
    customUserData = userData;
    
    LEDAnimation anim;
    anim.type = LED_ANIM_CUSTOM;
    anim.duration = durationMs;
    anim.repeat = repeat;
    
    startAnimation(anim);
}

// Global instance
LEDController& ledController = LEDController::getInstance();
