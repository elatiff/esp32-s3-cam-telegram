/**
 * @file led_control.h
 * @brief LED control for status indication and flash
 */

#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LED state enum
 */
typedef enum {
    LED_STATE_OFF = 0,
    LED_STATE_ON,
    LED_STATE_BLINK_SLOW,
    LED_STATE_BLINK_FAST
} led_state_t;

/**
 * @brief Initialize LED control
 * @return ESP_OK on success
 */
esp_err_t led_control_init(void);

/**
 * @brief Set status LED state
 * @param state LED state
 */
void led_set_status(led_state_t state);

/**
 * @brief Turn flash LED on/off
 * @param on true to turn on
 */
void led_set_flash(bool on);

/**
 * @brief Flash the LED briefly for capture indication
 */
void led_flash_capture(void);

/**
 * @brief Indicate WiFi connected
 */
void led_indicate_wifi_connected(void);

/**
 * @brief Indicate WiFi disconnected
 */
void led_indicate_wifi_disconnected(void);

/**
 * @brief Indicate detection event
 */
void led_indicate_detection(void);

#ifdef __cplusplus
}
#endif

#endif // LED_CONTROL_H
