/**
 * @file led_control.c
 * @brief LED control implementation for status indication
 */

#include "led_control.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

static const char *TAG = "led_control";

// LED GPIO pins - adjust based on your board
#if CONFIG_CAMERA_MODULE_ESP32S3_CAM
#define LED_STATUS_GPIO     2   // Built-in LED on most boards
#define LED_FLASH_GPIO      48  // Flash LED
#elif CONFIG_CAMERA_MODULE_ESP32S3_EYE
#define LED_STATUS_GPIO     3
#define LED_FLASH_GPIO      48
#elif CONFIG_CAMERA_MODULE_XIAO_ESP32S3
#define LED_STATUS_GPIO     21
#define LED_FLASH_GPIO      -1  // No flash LED
#else
#define LED_STATUS_GPIO     2
#define LED_FLASH_GPIO      48
#endif

static led_state_t s_current_state = LED_STATE_OFF;
static TimerHandle_t s_blink_timer = NULL;
static bool s_blink_on = false;

static void blink_timer_callback(TimerHandle_t timer)
{
    s_blink_on = !s_blink_on;
    
    if (LED_STATUS_GPIO >= 0) {
        gpio_set_level(LED_STATUS_GPIO, s_blink_on ? 1 : 0);
    }
}

esp_err_t led_control_init(void)
{
    ESP_LOGI(TAG, "Initializing LED control...");
    
    // Configure status LED
    if (LED_STATUS_GPIO >= 0) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << LED_STATUS_GPIO),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_conf);
        gpio_set_level(LED_STATUS_GPIO, 0);
    }
    
    // Configure flash LED
    if (LED_FLASH_GPIO >= 0) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << LED_FLASH_GPIO),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_conf);
        gpio_set_level(LED_FLASH_GPIO, 0);
    }
    
    // Create blink timer
    s_blink_timer = xTimerCreate("led_blink", pdMS_TO_TICKS(500),
                                  pdTRUE, NULL, blink_timer_callback);
    
    ESP_LOGI(TAG, "LED control initialized");
    return ESP_OK;
}

void led_set_status(led_state_t state)
{
    s_current_state = state;
    
    // Stop blink timer if running
    if (s_blink_timer && xTimerIsTimerActive(s_blink_timer)) {
        xTimerStop(s_blink_timer, 0);
    }
    
    switch (state) {
        case LED_STATE_OFF:
            if (LED_STATUS_GPIO >= 0) {
                gpio_set_level(LED_STATUS_GPIO, 0);
            }
            break;
            
        case LED_STATE_ON:
            if (LED_STATUS_GPIO >= 0) {
                gpio_set_level(LED_STATUS_GPIO, 1);
            }
            break;
            
        case LED_STATE_BLINK_SLOW:
            xTimerChangePeriod(s_blink_timer, pdMS_TO_TICKS(500), 0);
            xTimerStart(s_blink_timer, 0);
            break;
            
        case LED_STATE_BLINK_FAST:
            xTimerChangePeriod(s_blink_timer, pdMS_TO_TICKS(100), 0);
            xTimerStart(s_blink_timer, 0);
            break;
    }
}

void led_set_flash(bool on)
{
    if (LED_FLASH_GPIO >= 0) {
        gpio_set_level(LED_FLASH_GPIO, on ? 1 : 0);
    }
}

void led_flash_capture(void)
{
    if (LED_FLASH_GPIO >= 0) {
        gpio_set_level(LED_FLASH_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(LED_FLASH_GPIO, 0);
    }
}

void led_indicate_wifi_connected(void)
{
    // Quick triple blink
    for (int i = 0; i < 3; i++) {
        if (LED_STATUS_GPIO >= 0) {
            gpio_set_level(LED_STATUS_GPIO, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_level(LED_STATUS_GPIO, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    
    // Then stay on
    led_set_status(LED_STATE_ON);
}

void led_indicate_wifi_disconnected(void)
{
    led_set_status(LED_STATE_BLINK_SLOW);
}

void led_indicate_detection(void)
{
    // Fast blink for 1 second
    led_state_t prev_state = s_current_state;
    led_set_status(LED_STATE_BLINK_FAST);
    vTaskDelay(pdMS_TO_TICKS(1000));
    led_set_status(prev_state);
}
