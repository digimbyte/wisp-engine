// system_init.h - Comprehensive System Initialization for Wisp Engine
#pragma once

#include <stdint.h>
#include <stdbool.h>

// System initialization results
typedef enum {
    WISP_INIT_OK = 0,
    WISP_INIT_ERROR_WIRELESS,
    WISP_INIT_ERROR_FLASH,
    WISP_INIT_ERROR_RGB,
    WISP_INIT_ERROR_SD,
    WISP_INIT_ERROR_LCD,
    WISP_INIT_ERROR_LVGL,
    WISP_INIT_ERROR_SETTINGS,
    WISP_INIT_ERROR_UNKNOWN
} wisp_init_result_t;

// System component flags
typedef enum {
    WISP_COMPONENT_NONE     = 0x00,
    WISP_COMPONENT_WIRELESS = 0x01,
    WISP_COMPONENT_FLASH    = 0x02,
    WISP_COMPONENT_RGB      = 0x04,
    WISP_COMPONENT_SD       = 0x08,
    WISP_COMPONENT_LCD      = 0x10,
    WISP_COMPONENT_LVGL     = 0x20,
    WISP_COMPONENT_SETTINGS = 0x40,
    WISP_COMPONENT_ALL      = 0xFF
} wisp_component_flags_t;

// System status structure
typedef struct {
    bool wireless_ready;
    bool flash_ready;
    bool rgb_ready;
    bool sd_ready;
    bool lcd_ready;
    bool lvgl_ready;
    bool settings_ready;
    uint32_t flash_size_mb;
    uint8_t backlight_level;
    uint32_t init_time_ms;
} wisp_system_status_t;

#ifdef __cplusplus
extern "C" {
#endif

// === MAIN INITIALIZATION FUNCTIONS ===

/**
 * @brief Initialize the complete Wisp Engine system
 * @param components Bitmask of components to initialize
 * @return Initialization result
 */
wisp_init_result_t wisp_system_init(wisp_component_flags_t components);

/**
 * @brief Initialize system with default components
 * @return Initialization result
 */
wisp_init_result_t wisp_system_setup(void);

/**
 * @brief Shutdown the system gracefully
 */
void wisp_system_shutdown(void);

/**
 * @brief Get current system status
 * @return Pointer to system status structure
 */
const wisp_system_status_t* wisp_get_system_status(void);

// === INDIVIDUAL COMPONENT INITIALIZATION ===

/**
 * @brief Initialize wireless communication module
 * @return true if successful, false otherwise
 */
bool wisp_wireless_init(void);

/**
 * @brief Test and print flash memory information
 * @return Flash size in MB, 0 if failed
 */
uint32_t wisp_flash_searching(void);

/**
 * @brief Initialize RGB LED functions
 * @return true if successful, false otherwise
 */
bool wisp_rgb_init(void);

/**
 * @brief Display RGB LED examples
 */
void wisp_rgb_example(void);

/**
 * @brief Initialize SD/TF card
 * @return true if successful, false otherwise
 */
bool wisp_sd_init(void);

/**
 * @brief Initialize LCD display
 * @return true if successful, false otherwise
 */
bool wisp_lcd_init(void);

/**
 * @brief Set LCD backlight brightness
 * @param level Brightness level (0-100)
 */
void wisp_backlight_set(uint8_t level);

/**
 * @brief Initialize LVGL graphics library
 * @return true if successful, false otherwise
 */
bool wisp_lvgl_init(void);

/**
 * @brief Run LVGL example 1
 */
void wisp_lvgl_example1(void);

/**
 * @brief Initialize settings manager
 * @return true if successful, false otherwise
 */
bool wisp_settings_init(void);

// === MAIN LOOP FUNCTIONS ===

/**
 * @brief Main system loop - call this continuously
 * Should be called from main application loop
 */
void wisp_system_loop(void);

/**
 * @brief System task delay (wrapper for vTaskDelay)
 * @param delay_ms Delay in milliseconds
 */
void wisp_delay_ms(uint32_t delay_ms);

/**
 * @brief Handle LVGL timer events
 * Should be called regularly (every 10ms recommended)
 */
void wisp_lvgl_timer_handler(void);

// === UTILITY FUNCTIONS ===

/**
 * @brief Check if a specific component is initialized
 * @param component Component to check
 * @return true if initialized, false otherwise
 */
bool wisp_is_component_ready(wisp_component_flags_t component);

/**
 * @brief Get component initialization status as string
 * @param component Component to check
 * @return Status string
 */
const char* wisp_get_component_status_string(wisp_component_flags_t component);

/**
 * @brief Print system status to console
 */
void wisp_print_system_status(void);

/**
 * @brief Run system diagnostics
 * @return true if all tests pass, false otherwise
 */
bool wisp_run_diagnostics(void);

#ifdef __cplusplus
}
#endif
