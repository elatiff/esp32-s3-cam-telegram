/**
 * @file camera_manager.c
 * @brief Camera manager implementation for ESP32-S3-CAM
 */

#include "camera_manager.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "driver/gpio.h"
#include "img_converters.h"

static const char *TAG = "camera_manager";

// ===== Camera Pin Definitions for Different Modules =====

#if CONFIG_CAMERA_MODULE_ESP32S3_CAM
// ESP32-S3-CAM (FREENOVE/Generic) Pin Configuration
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM     4
#define SIOC_GPIO_NUM     5

#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM       8
#define Y3_GPIO_NUM       9
#define Y2_GPIO_NUM       11
#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM     13

#define LED_GPIO_NUM      48  // Built-in LED

#elif CONFIG_CAMERA_MODULE_ESP32S3_EYE
// ESP32-S3-EYE (Espressif) Pin Configuration
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM     4
#define SIOC_GPIO_NUM     5

#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM       8
#define Y3_GPIO_NUM       9
#define Y2_GPIO_NUM       11
#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM     13

#define LED_GPIO_NUM      3

#elif CONFIG_CAMERA_MODULE_XIAO_ESP32S3
// Seeed XIAO ESP32S3 Sense Pin Configuration
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39

#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

#define LED_GPIO_NUM      21

#else
#error "Camera module not selected in menuconfig!"
#endif

static bool s_camera_initialized = false;
static bool s_sensor_supports_jpeg = true;

esp_err_t camera_manager_init(void)
{
    if (s_camera_initialized) {
        ESP_LOGW(TAG, "Camera already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing camera...");

    // First try with JPEG format
    camera_config_t camera_config = {
        .pin_pwdn = PWDN_GPIO_NUM,
        .pin_reset = RESET_GPIO_NUM,
        .pin_xclk = XCLK_GPIO_NUM,
        .pin_sccb_sda = SIOD_GPIO_NUM,
        .pin_sccb_scl = SIOC_GPIO_NUM,
        .pin_d7 = Y9_GPIO_NUM,
        .pin_d6 = Y8_GPIO_NUM,
        .pin_d5 = Y7_GPIO_NUM,
        .pin_d4 = Y6_GPIO_NUM,
        .pin_d3 = Y5_GPIO_NUM,
        .pin_d2 = Y4_GPIO_NUM,
        .pin_d1 = Y3_GPIO_NUM,
        .pin_d0 = Y2_GPIO_NUM,
        .pin_vsync = VSYNC_GPIO_NUM,
        .pin_href = HREF_GPIO_NUM,
        .pin_pclk = PCLK_GPIO_NUM,

        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_VGA,    // 640x480
        .jpeg_quality = 12,              // 0-63, lower is better quality
        .fb_count = 2,                   // Double buffering
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_LATEST,
    };

    // Initialize camera with JPEG first
    esp_err_t err = esp_camera_init(&camera_config);
    
    if (err != ESP_OK) {
        // JPEG not supported (GC2145, etc.) - use RGB565 instead
        ESP_LOGW(TAG, "JPEG not supported by sensor, using RGB565 with software conversion");
        s_sensor_supports_jpeg = false;
        
        camera_config.pixel_format = PIXFORMAT_RGB565;
        camera_config.frame_size = FRAMESIZE_QVGA;  // Use smaller size for RGB565 (memory)
        camera_config.jpeg_quality = 0;  // Not used for RGB565
        
        err = esp_camera_init(&camera_config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
            return err;
        }
    }

    // Get sensor and apply initial settings
    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        ESP_LOGI(TAG, "Camera sensor: PID=0x%04x", s->id.PID);
        
        // Initial sensor settings
        s->set_brightness(s, 0);     // -2 to 2
        s->set_contrast(s, 0);       // -2 to 2
        s->set_saturation(s, 0);     // -2 to 2
        s->set_special_effect(s, 0); // 0 = No Effect
        s->set_whitebal(s, 1);       // 0 = Disable, 1 = Enable
        s->set_awb_gain(s, 1);       // 0 = Disable, 1 = Enable
        s->set_wb_mode(s, 0);        // 0 = Auto
        s->set_exposure_ctrl(s, 1);  // 0 = Disable, 1 = Enable
        s->set_aec2(s, 0);           // 0 = Disable, 1 = Enable
        s->set_ae_level(s, 0);       // -2 to 2
        s->set_aec_value(s, 300);    // 0 to 1200
        s->set_gain_ctrl(s, 1);      // 0 = Disable, 1 = Enable
        s->set_agc_gain(s, 0);       // 0 to 30
        s->set_gainceiling(s, (gainceiling_t)0); // 0 to 6
        s->set_bpc(s, 0);            // 0 = Disable, 1 = Enable
        s->set_wpc(s, 1);            // 0 = Disable, 1 = Enable
        s->set_raw_gma(s, 1);        // 0 = Disable, 1 = Enable
        s->set_lenc(s, 1);           // 0 = Disable, 1 = Enable
        s->set_hmirror(s, 0);        // 0 = Disable, 1 = Enable
        s->set_vflip(s, 0);          // 0 = Disable, 1 = Enable
        s->set_dcw(s, 1);            // 0 = Disable, 1 = Enable
        s->set_colorbar(s, 0);       // 0 = Disable, 1 = Enable
    }

    s_camera_initialized = true;
    ESP_LOGI(TAG, "Camera initialized successfully (JPEG support: %s)", 
             s_sensor_supports_jpeg ? "hardware" : "software");
    return ESP_OK;
}

camera_fb_t* camera_manager_capture(void)
{
    if (!s_camera_initialized) {
        ESP_LOGE(TAG, "Camera not initialized");
        return NULL;
    }

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return NULL;
    }

    ESP_LOGD(TAG, "Captured image: %dx%d, %u bytes", 
             fb->width, fb->height, fb->len);
    return fb;
}

void camera_manager_return_fb(camera_fb_t *fb)
{
    if (fb) {
        esp_camera_fb_return(fb);
    }
}

esp_err_t camera_manager_set_framesize(framesize_t framesize)
{
    sensor_t *s = esp_camera_sensor_get();
    if (!s) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (s->set_framesize(s, framesize) != 0) {
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t camera_manager_set_quality(int quality)
{
    sensor_t *s = esp_camera_sensor_get();
    if (!s) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (s->set_quality(s, quality) != 0) {
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

void camera_manager_set_hmirror(bool enable)
{
    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_hmirror(s, enable ? 1 : 0);
    }
}

void camera_manager_set_vflip(bool enable)
{
    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_vflip(s, enable ? 1 : 0);
    }
}

void camera_manager_deinit(void)
{
    if (s_camera_initialized) {
        esp_camera_deinit();
        s_camera_initialized = false;
        ESP_LOGI(TAG, "Camera deinitialized");
    }
}
