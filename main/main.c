/**
 * @file main.c
 * @brief ESP32-S3-CAM Face/Motion Detection with Telegram Notification
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "wifi_manager.h"
#include "camera_manager.h"
#include "motion_detector.h"
#include "face_detector.h"
#include "telegram_bot.h"
#include "led_control.h"
#include "img_converters.h" // Needed for RGB565->JPEG conversion

static const char *TAG = "main";

// Detection task handle
static TaskHandle_t s_detection_task_handle = NULL;

// Detection event configuration
typedef enum {
    DETECTION_EVENT_MOTION,
    DETECTION_EVENT_FACE,
    DETECTION_EVENT_BOTH
} detection_event_type_t;

typedef struct {
    detection_event_type_t type;
    camera_fb_t *fb;      // Used if camera provides JPEG natively
    uint8_t *jpg_buf;     // Used if we did software conversion
    size_t jpg_len;       // Length of software converted JPEG
} detection_event_t;

static QueueHandle_t s_detection_queue = NULL;

// Statistics
static uint32_t s_motion_count = 0;
static uint32_t s_face_count = 0;
static uint32_t s_telegram_sent = 0;

/**
 * @brief Print system information at startup
 */
static void print_system_info(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32-S3-CAM Telegram Bot v1.0.0");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "Free PSRAM: %lu bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "----------------------------------------");
    
#if CONFIG_ENABLE_FACE_DETECTION
    ESP_LOGI(TAG, "Face Detection: ENABLED");
#else
    ESP_LOGI(TAG, "Face Detection: DISABLED");
#endif

#if CONFIG_ENABLE_MOTION_DETECTION
    ESP_LOGI(TAG, "Motion Detection: ENABLED");
    ESP_LOGI(TAG, "  - Threshold: %d", CONFIG_MOTION_THRESHOLD);
    ESP_LOGI(TAG, "  - Pixel Threshold: %d%%", CONFIG_MOTION_PIXEL_THRESHOLD);
#else
    ESP_LOGI(TAG, "Motion Detection: DISABLED");
#endif

    ESP_LOGI(TAG, "Detection Interval: %d ms", CONFIG_DETECTION_INTERVAL_MS);
    ESP_LOGI(TAG, "Telegram Cooldown: %d sec", CONFIG_TELEGRAM_COOLDOWN_SEC);
    ESP_LOGI(TAG, "========================================");
}

/**
 * @brief Task to handle Telegram notifications
 */
static void telegram_notification_task(void *pvParameters)
{
    detection_event_t event;
    
    ESP_LOGI(TAG, "Telegram notification task started");
    
    while (1) {
        if (xQueueReceive(s_detection_queue, &event, portMAX_DELAY) == pdTRUE) {
            // Check cooldown
            if (!telegram_bot_can_send(CONFIG_TELEGRAM_COOLDOWN_SEC)) {
                ESP_LOGW(TAG, "Telegram cooldown active, skipping notification");
                // Cleanup
                if (event.fb) camera_manager_return_fb(event.fb);
                if (event.jpg_buf) free(event.jpg_buf);
                continue;
            }
            
            // Build message
            char message[256];
            switch (event.type) {
                case DETECTION_EVENT_MOTION:
                    snprintf(message, sizeof(message),
                             "🚨 <b>Motion Detected!</b>\n"
                             "📅 Time: Detection #%lu\n"
                             "📸 Image attached",
                             ++s_motion_count);
                    break;
                    
                case DETECTION_EVENT_FACE:
                    snprintf(message, sizeof(message),
                             "👤 <b>Face Detected!</b>\n"
                             "📅 Time: Detection #%lu\n"
                             "📸 Image attached",
                             ++s_face_count);
                    break;
                    
                case DETECTION_EVENT_BOTH:
                    snprintf(message, sizeof(message),
                             "🚨👤 <b>Motion + Face Detected!</b>\n"
                             "📅 Motion: #%lu, Face: #%lu\n"
                             "📸 Image attached",
                             ++s_motion_count, ++s_face_count);
                    break;
            }
            
            // Flash LED
            led_flash_capture();
            
            // Determine which buffer to use
            uint8_t *buf_to_send = NULL;
            size_t len_to_send = 0;
            
            if (event.fb) {
                buf_to_send = event.fb->buf;
                len_to_send = event.fb->len;
            } else if (event.jpg_buf) {
                buf_to_send = event.jpg_buf;
                len_to_send = event.jpg_len;
            }
            
            if (buf_to_send && len_to_send > 0) {
                // Send photo with caption
                esp_err_t err = telegram_bot_send_photo(
                    buf_to_send,
                    len_to_send,
                    message
                );
                
                if (err == ESP_OK) {
                    s_telegram_sent++;
                    ESP_LOGI(TAG, "✅ Telegram notification sent (total: %lu)", s_telegram_sent);
                } else {
                    ESP_LOGE(TAG, "❌ Failed to send Telegram notification");
                }
            } else {
                // Should not happen, but fallback to text
                telegram_bot_send_message(message);
            }
            
            // Cleanup resources
            if (event.fb) {
                camera_manager_return_fb(event.fb);
            }
            if (event.jpg_buf) {
                free(event.jpg_buf); // Generated by frame2jpg
            }
            
            // Indicate detection on LED
            led_indicate_detection();
        }
    }
}

/**
 * @brief Main detection task
 */
static void detection_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Detection task started on core %d", xPortGetCoreID());
    
    // Allocate grayscale buffer for motion detection
    const int motion_width = 320;
    const int motion_height = 240;
    const size_t gray_size = motion_width * motion_height;
    
    uint8_t *gray_buffer = heap_caps_malloc(gray_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!gray_buffer) {
        gray_buffer = malloc(gray_size);
    }
    
    if (!gray_buffer) {
        ESP_LOGE(TAG, "Failed to allocate grayscale buffer");
        vTaskDelete(NULL);
        return;
    }
    
    // Initialize motion detector
#if CONFIG_ENABLE_MOTION_DETECTION
    motion_detector_init(motion_width, motion_height, 
                         CONFIG_MOTION_THRESHOLD, 
                         (float)CONFIG_MOTION_PIXEL_THRESHOLD);
#endif

    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Warmup frames
    for (int i = 0; i < 5; i++) {
        camera_fb_t *fb = camera_manager_capture();
        if (fb) camera_manager_return_fb(fb);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    ESP_LOGI(TAG, "Detection loop starting...");
    
    while (1) {
        // Capture frame
        camera_fb_t *fb = camera_manager_capture();
        if (!fb) {
            ESP_LOGW(TAG, "Camera capture failed");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        
        bool motion_detected = false;
        bool face_detected = false;
        
        // 1. Motion Detection
#if CONFIG_ENABLE_MOTION_DETECTION
        if (fb->format == PIXFORMAT_RGB565) {
            // Convert to grayscale for motion detection
            // RGB565 stride is 2 bytes/pixel
            if (rgb565_to_grayscale(fb->buf, fb->len, gray_buffer, gray_size) == ESP_OK) {
                motion_result_t result = motion_detector_process(gray_buffer, gray_size);
                motion_detected = result.detected;
            }
        } else if (fb->format == PIXFORMAT_JPEG) {
            // Skip motion detection for JPEG for now, 
            // or decode to RGB888 then gray... expense.
            // Assuming RGB565 for GC2145
        }
#endif

        // 2. Face Detection
#if CONFIG_ENABLE_FACE_DETECTION
        if (fb->format == PIXFORMAT_RGB565) {
            face_result_t result = face_detector_detect(fb);
            face_detected = result.detected;
        }
#endif

        // 3. Handle Detection
        if (motion_detected || face_detected) {
            detection_event_t event = {
                .type = (motion_detected && face_detected) ? DETECTION_EVENT_BOTH :
                        motion_detected ? DETECTION_EVENT_MOTION : DETECTION_EVENT_FACE,
                .fb = NULL,
                .jpg_buf = NULL,
                .jpg_len = 0
            };

            // Prepare image for Telegram
            if (fb->format == PIXFORMAT_JPEG) {
                // Native JPEG, pass the FB directly
                // IMPORTANT: We do NOT return the FB here, we pass ownership to the queue
                event.fb = fb;
                fb = NULL; // Mark as null so we don't free it at end of loop
            } 
            else if (fb->format == PIXFORMAT_RGB565) {
                // Convert to JPEG
                uint8_t *jpg_buf = NULL;
                size_t jpg_len = 0;
                
                if (frame2jpg(fb, 80, &jpg_buf, &jpg_len)) {
                    event.jpg_buf = jpg_buf;
                    event.jpg_len = jpg_len;
                    // We keep 'fb' to be returned at end of loop
                } else {
                    ESP_LOGE(TAG, "JPEG conversion failed");
                }
            }
            
            // Queue event
            if (event.fb || event.jpg_buf) {
                if (xQueueSend(s_detection_queue, &event, 0) != pdTRUE) {
                    ESP_LOGW(TAG, "Queue full, dropping event");
                    if (event.fb) camera_manager_return_fb(event.fb);
                    if (event.jpg_buf) free(event.jpg_buf);
                    // If we passed fb ownership to event, we need to recover it 
                    // if queue failed, but here we just freed it.
                    // If we kept fb (RGB case), it will be returned below.
                }
            }
        }
        
        // Return frame buffer (if ownership not passed)
        if (fb) {
            camera_manager_return_fb(fb);
        }
        
        vTaskDelay(pdMS_TO_TICKS(CONFIG_DETECTION_INTERVAL_MS));
    }
}

/**
 * @brief Application entry point
 */
void app_main(void)
{
    esp_err_t ret;
    
    print_system_info();
    
    ret = led_control_init();
    led_set_status(LED_STATE_BLINK_SLOW);
    
    // Initialize WiFi
    ret = wifi_manager_init();
    if (ret != ESP_OK) return;
    
    ret = wifi_manager_connect();
    if (ret != ESP_OK) {
        led_set_status(LED_STATE_BLINK_FAST);
        return;
    }
    
    led_indicate_wifi_connected();
    
    // Initialize devices
    if (camera_manager_init() != ESP_OK) return;
    
#if CONFIG_ENABLE_FACE_DETECTION
    face_detector_init();
#endif
    
    if (telegram_bot_init(CONFIG_TELEGRAM_BOT_TOKEN, CONFIG_TELEGRAM_CHAT_ID) != ESP_OK) return;
    
    // Send startup notification
    char message[128];
    const char *ip = wifi_manager_get_ip();
    snprintf(message, sizeof(message), 
             "🟢 <b>ESP32-S3-CAM Online!</b>\n"
             "📡 WiFi Connected\n"
             "🌐 IP: %s\n"
             "🔍 Ready", 
             ip ? ip : "Unknown");
             
    telegram_bot_send_message(message);
    
    // Tasks
    s_detection_queue = xQueueCreate(5, sizeof(detection_event_t));
    
    xTaskCreatePinnedToCore(telegram_notification_task, "telegram_task", 6 * 1024, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(detection_task, "detection_task", 8 * 1024, NULL, 10, &s_detection_task_handle, 1);
    
    ESP_LOGI(TAG, "System running...");
}
