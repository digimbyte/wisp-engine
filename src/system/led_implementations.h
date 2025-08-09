// led_implementations.h - Board-specific LED implementation classes
#pragma once

#include "led_controller.h"
#include "esp32_common.h"

#ifdef RGB_LED_TYPE

// Simple GPIO LED Implementation (single on/off LED)
class SimpleGPIOLED : public LEDImplementation {
private:
    int pin;
    bool inverted; // Some boards have inverted LED logic
    LEDColor currentColor;

public:
    SimpleGPIOLED(bool invert = false) : pin(-1), inverted(invert) {}

    bool init(int ledPin, int count) override {
        if (ledPin < 0) return false;
        
        pin = ledPin;
        pinMode(pin, OUTPUT);
        digitalWrite(pin, inverted ? HIGH : LOW); // Start with LED off
        return true;
    }

    void shutdown() override {
        if (pin >= 0) {
            digitalWrite(pin, inverted ? HIGH : LOW);
            pin = -1;
        }
    }

    void setLED(int index, const LEDColor& color) override {
        // For simple LED, any non-black color = on
        bool ledOn = (color.r > 0 || color.g > 0 || color.b > 0);
        currentColor = color;
        digitalWrite(pin, (ledOn != inverted) ? HIGH : LOW);
    }

    void setAll(const LEDColor& color) override {
        setLED(0, color); // Simple LED only has one "LED"
    }

    void show() override {
        // No-op for GPIO LED - changes are immediate
    }

    void clear() override {
        setLED(0, LEDColor::BLACK);
    }

    bool supportsRGB() const override { return false; }
    bool supportsAnimations() const override { return true; }
    bool supportsBrightness() const override { return false; }
};

// PWM RGB LED Implementation (3 separate PWM channels)
class PWMRGBLED : public LEDImplementation {
private:
    int pinR, pinG, pinB;
    int channelR, channelG, channelB;
    LEDColor currentColor;
    bool initialized;

public:
    PWMRGBLED() : pinR(-1), pinG(-1), pinB(-1), 
                 channelR(-1), channelG(-1), channelB(-1),
                 initialized(false) {}

    bool init(int ledPin, int count) override {
        // For PWM RGB, we need 3 pins defined in board config
        #ifdef RGB_LED_PIN_R
            pinR = RGB_LED_PIN_R;
            pinG = RGB_LED_PIN_G;
            pinB = RGB_LED_PIN_B;
        #else
            // Single pin provided - assume it's red channel only
            pinR = ledPin;
            pinG = -1;
            pinB = -1;
        #endif

        // Find available PWM channels
        channelR = 0; // Use LEDC channels 0, 1, 2
        channelG = 1;
        channelB = 2;

        // Configure PWM channels
        if (pinR >= 0) {
            ledcSetup(channelR, 1000, 8); // 1kHz, 8-bit resolution
            ledcAttachPin(pinR, channelR);
        }
        if (pinG >= 0) {
            ledcSetup(channelG, 1000, 8);
            ledcAttachPin(pinG, channelG);
        }
        if (pinB >= 0) {
            ledcSetup(channelB, 1000, 8);
            ledcAttachPin(pinB, channelB);
        }

        initialized = true;
        clear();
        return true;
    }

    void shutdown() override {
        if (!initialized) return;
        
        if (channelR >= 0) ledcDetachPin(pinR);
        if (channelG >= 0) ledcDetachPin(pinG);
        if (channelB >= 0) ledcDetachPin(pinB);
        
        initialized = false;
    }

    void setLED(int index, const LEDColor& color) override {
        if (!initialized) return;
        
        currentColor = color;
        if (channelR >= 0) ledcWrite(channelR, color.r);
        if (channelG >= 0) ledcWrite(channelG, color.g);
        if (channelB >= 0) ledcWrite(channelB, color.b);
    }

    void setAll(const LEDColor& color) override {
        setLED(0, color); // PWM RGB only has one "LED"
    }

    void show() override {
        // No-op for PWM - changes are immediate
    }

    void clear() override {
        setLED(0, LEDColor::BLACK);
    }

    bool supportsRGB() const override { return (pinG >= 0 && pinB >= 0); }
    bool supportsAnimations() const override { return true; }
    bool supportsBrightness() const override { return true; }
};

// WS2812/NeoPixel LED Implementation
class WS2812LED : public LEDImplementation {
private:
    int pin;
    int count;
    std::vector<LEDColor> ledBuffer;
    bool initialized;

    // Simple bit-banging implementation (more compatible than RMT)
    void sendByte(uint8_t byte) {
        for (int i = 7; i >= 0; i--) {
            if (byte & (1 << i)) {
                // Send '1' bit: ~800ns high, ~450ns low
                digitalWrite(pin, HIGH);
                delayMicroseconds(1); // ~1us high
                digitalWrite(pin, LOW);
                // Short delay for low period (ESP32 overhead provides this)
            } else {
                // Send '0' bit: ~400ns high, ~850ns low  
                digitalWrite(pin, HIGH);
                // Very brief high (ESP32 overhead provides ~400ns)
                digitalWrite(pin, LOW);
                delayMicroseconds(1); // ~1us low
            }
        }
    }

public:
    WS2812LED() : pin(-1), count(0), initialized(false) {}

    bool init(int ledPin, int ledCount) override {
        if (ledPin < 0 || ledCount <= 0) return false;
        
        pin = ledPin;
        count = ledCount;
        ledBuffer.resize(count);
        
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
        
        initialized = true;
        clear();
        show();
        return true;
    }

    void shutdown() override {
        if (initialized) {
            clear();
            show();
            initialized = false;
        }
    }

    void setLED(int index, const LEDColor& color) override {
        if (!initialized || index < 0 || index >= count) return;
        ledBuffer[index] = color;
    }

    void setAll(const LEDColor& color) override {
        if (!initialized) return;
        for (int i = 0; i < count; i++) {
            ledBuffer[i] = color;
        }
    }

    void show() override {
        if (!initialized) return;
        
        // Disable interrupts during transmission
        noInterrupts();
        
        for (int i = 0; i < count; i++) {
            // WS2812 expects GRB order
            sendByte(ledBuffer[i].g);
            sendByte(ledBuffer[i].r);
            sendByte(ledBuffer[i].b);
        }
        
        // Re-enable interrupts
        interrupts();
        
        // Reset latch (>50us low)
        digitalWrite(pin, LOW);
        delayMicroseconds(60);
    }

    void clear() override {
        setAll(LEDColor::BLACK);
    }

    bool supportsRGB() const override { return true; }
    bool supportsAnimations() const override { return true; }
    bool supportsBrightness() const override { return true; }
};

// APA102/DotStar LED Implementation (SPI-based)
class APA102LED : public LEDImplementation {
private:
    int pinClock, pinData;
    int count;
    std::vector<LEDColor> ledBuffer;
    bool initialized;

    void sendByte(uint8_t byte) {
        for (int i = 7; i >= 0; i--) {
            digitalWrite(pinData, (byte & (1 << i)) ? HIGH : LOW);
            digitalWrite(pinClock, HIGH);
            digitalWrite(pinClock, LOW);
        }
    }

public:
    APA102LED() : pinClock(-1), pinData(-1), count(0), initialized(false) {}

    bool init(int ledPin, int ledCount) override {
        // For APA102, we need clock and data pins
        #ifdef RGB_LED_CLOCK_PIN
            pinClock = RGB_LED_CLOCK_PIN;
            pinData = RGB_LED_PIN; // Use main pin as data
        #else
            // Fallback: assume consecutive pins
            pinData = ledPin;
            pinClock = ledPin + 1;
        #endif

        if (pinData < 0 || pinClock < 0 || ledCount <= 0) return false;
        
        count = ledCount;
        ledBuffer.resize(count);
        
        pinMode(pinData, OUTPUT);
        pinMode(pinClock, OUTPUT);
        digitalWrite(pinData, LOW);
        digitalWrite(pinClock, LOW);
        
        initialized = true;
        clear();
        show();
        return true;
    }

    void shutdown() override {
        if (initialized) {
            clear();
            show();
            initialized = false;
        }
    }

    void setLED(int index, const LEDColor& color) override {
        if (!initialized || index < 0 || index >= count) return;
        ledBuffer[index] = color;
    }

    void setAll(const LEDColor& color) override {
        if (!initialized) return;
        for (int i = 0; i < count; i++) {
            ledBuffer[i] = color;
        }
    }

    void show() override {
        if (!initialized) return;
        
        // Start frame (32 zeros)
        for (int i = 0; i < 4; i++) sendByte(0x00);
        
        // LED data
        for (int i = 0; i < count; i++) {
            sendByte(0xFF); // Full brightness frame
            sendByte(ledBuffer[i].b); // APA102 expects BGR order
            sendByte(ledBuffer[i].g);
            sendByte(ledBuffer[i].r);
        }
        
        // End frame (32 ones) - actually need count/2 additional clock pulses
        for (int i = 0; i < ((count + 15) / 16); i++) sendByte(0xFF);
    }

    void clear() override {
        setAll(LEDColor::BLACK);
    }

    bool supportsRGB() const override { return true; }
    bool supportsAnimations() const override { return true; }
    bool supportsBrightness() const override { return true; }
};

#endif // RGB_LED_TYPE
