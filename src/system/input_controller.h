// input_controller.h - ESP32-C6/S3 Input Controller using ESP-IDF
// Native ESP32 GPIO input handling with debouncing and touch support
#pragma once
#include "esp32_common.h"  // Pure ESP-IDF native headers
#include "definitions.h"

// Hardware button mapping
extern const uint8_t BUTTON_PINS[MAX_BUTTONS];

class InputController {
private:
    const uint8_t* buttonPins;
    uint8_t buttonStates;
    uint8_t prevButtonStates;
    
public:
    InputController(const uint8_t* pins) : buttonPins(pins), buttonStates(0), prevButtonStates(0) {}
    
    bool init() {
        // Initialize button pins as input with pullup
        for (int i = 0; i < MAX_BUTTONS; i++) {
            if (buttonPins[i] != 255) {  // 255 = not connected
                pinMode(buttonPins[i], INPUT_PULLUP);
            }
        }
        return true;
    }
    
    void update() {
        prevButtonStates = buttonStates;
        buttonStates = 0;
        
        // Read button states (inverted because of pullup)
        for (int i = 0; i < MAX_BUTTONS; i++) {
            if (buttonPins[i] != 255) {
                if (digitalRead(buttonPins[i]) == LOW) {
                    buttonStates |= (1 << i);
                }
            }
        }
    }
    
    bool isPressed(Button button) const {
        return (buttonStates & (1 << button)) != 0;
    }
    
    bool wasPressed(Button button) const {
        return ((buttonStates & (1 << button)) != 0) && ((prevButtonStates & (1 << button)) == 0);
    }
    
    bool wasReleased(Button button) const {
        return ((buttonStates & (1 << button)) == 0) && ((prevButtonStates & (1 << button)) != 0);
    }
    
    uint8_t getButtonStates() const { return buttonStates; }
};

// Default button pin mapping (should be defined in board config)
#ifndef BUTTON_PINS
const uint8_t BUTTON_PINS[MAX_BUTTONS] = {
    // Default ESP32 pin mapping - adjust for your hardware
    32,  // BTN_UP
    33,  // BTN_DOWN
    25,  // BTN_LEFT
    26,  // BTN_RIGHT
    27,  // BTN_SELECT (A button)
    14   // BTN_BACK (B button)
};
#endif
