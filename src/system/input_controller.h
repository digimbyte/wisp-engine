// input_controller.h - ESP32-C6/S3 Input Controller using ESP-IDF
// Native ESP32 GPIO input handling with debouncing and touch support
#pragma once
#include "esp32_common.h"  // Pure ESP-IDF native headers
#include "definitions.h"

// Board-specific configuration includes - ESP-IDF native detection
#if defined(CONFIG_ESP32_C6_LCD_147) || defined(CONFIG_IDF_TARGET_ESP32C6)
    #include "../boards/esp32-c6_config.h"
#elif defined(CONFIG_ESP32_S3_ROUND) || defined(CONFIG_IDF_TARGET_ESP32S3)
    #include "../boards/esp32-s3_config.h"
#else
    #error "Board configuration not defined. Define CONFIG_ESP32_C6_LCD_147 or CONFIG_ESP32_S3_ROUND"
#endif

class InputController {
private:
    const uint8_t* buttonPins;
    uint8_t buttonStates;
    uint8_t prevButtonStates;
    
public:
    // Constructor uses board-specific BUTTON_PINS array
    InputController() : buttonPins(BUTTON_PINS), buttonStates(0), prevButtonStates(0) {}
    
    bool init() {
        // Initialize button pins as input with pullup (ESP-IDF native)
        for (int i = 0; i < 9; i++) {  // 9 buttons: LEFT, RIGHT, UP, DOWN, A, B, C, SELECT, START
            if (buttonPins[i] != 255) {  // 255 = not connected
                gpio_config_t io_conf = {
                    .pin_bit_mask = (1ULL << buttonPins[i]),
                    .mode = GPIO_MODE_INPUT,
                    .pull_up_en = GPIO_PULLUP_ENABLE,
                    .pull_down_en = GPIO_PULLDOWN_DISABLE,
                    .intr_type = GPIO_INTR_DISABLE
                };
                gpio_config(&io_conf);
            }
        }
        return true;
    }
    
    void update() {
        prevButtonStates = buttonStates;
        buttonStates = 0;
        
        // Read button states (inverted because of pullup) - ESP-IDF native
        for (int i = 0; i < 9; i++) {
            if (buttonPins[i] != 255) {
                if (gpio_get_level((gpio_num_t)buttonPins[i]) == 0) {  // LOW = pressed
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
