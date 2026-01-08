/**
 * @file motion_detector.c
 * @brief Motion detection implementation using frame comparison
 */

#include "motion_detector.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static const char *TAG = "motion_detector";

static uint8_t *s_prev_frame = NULL;
static size_t s_frame_size = 0;
static int s_width = 0;
static int s_height = 0;
static int s_threshold = 15;
static float s_change_threshold = 5.0f;
static bool s_has_baseline = false;

esp_err_t motion_detector_init(int width, int height, int threshold, float change_threshold)
{
    s_width = width;
    s_height = height;
    s_threshold = threshold;
    s_change_threshold = change_threshold;
    s_frame_size = width * height;
    
    // Allocate in PSRAM if available
    s_prev_frame = heap_caps_malloc(s_frame_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_prev_frame) {
        // Fallback to regular RAM
        s_prev_frame = malloc(s_frame_size);
    }
    
    if (!s_prev_frame) {
        ESP_LOGE(TAG, "Failed to allocate memory for motion detection");
        return ESP_ERR_NO_MEM;
    }
    
    s_has_baseline = false;
    ESP_LOGI(TAG, "Motion detector initialized: %dx%d, threshold=%d, change=%.1f%%",
             width, height, threshold, change_threshold);
    
    return ESP_OK;
}

motion_result_t motion_detector_process(const uint8_t *grayscale_data, size_t size)
{
    motion_result_t result = {
        .detected = false,
        .change_percentage = 0.0f,
        .changed_pixels = 0
    };
    
    if (!s_prev_frame || !grayscale_data) {
        ESP_LOGE(TAG, "Invalid state or data");
        return result;
    }
    
    if (size != s_frame_size) {
        ESP_LOGW(TAG, "Frame size mismatch: expected %u, got %u", s_frame_size, size);
        return result;
    }
    
    // If no baseline, set it and return
    if (!s_has_baseline) {
        memcpy(s_prev_frame, grayscale_data, size);
        s_has_baseline = true;
        ESP_LOGI(TAG, "Motion detection baseline set");
        return result;
    }
    
    // Compare frames
    uint32_t changed = 0;
    for (size_t i = 0; i < size; i++) {
        int diff = abs((int)grayscale_data[i] - (int)s_prev_frame[i]);
        if (diff > s_threshold) {
            changed++;
        }
    }
    
    // Calculate percentage
    result.changed_pixels = changed;
    result.change_percentage = (float)changed / (float)size * 100.0f;
    result.detected = (result.change_percentage >= s_change_threshold);
    
    // Update baseline with rolling average to adapt to lighting changes
    for (size_t i = 0; i < size; i++) {
        // Blend: 90% old + 10% new for gradual adaptation
        s_prev_frame[i] = (uint8_t)((s_prev_frame[i] * 9 + grayscale_data[i]) / 10);
    }
    
    if (result.detected) {
        ESP_LOGI(TAG, "Motion detected: %.2f%% changed (%u pixels)", 
                 result.change_percentage, result.changed_pixels);
    }
    
    return result;
}

void motion_detector_reset(void)
{
    s_has_baseline = false;
    ESP_LOGI(TAG, "Motion detector baseline reset");
}

void motion_detector_set_threshold(int threshold)
{
    s_threshold = threshold;
    ESP_LOGI(TAG, "Motion threshold set to %d", threshold);
}

void motion_detector_deinit(void)
{
    if (s_prev_frame) {
        heap_caps_free(s_prev_frame);
        s_prev_frame = NULL;
    }
    s_frame_size = 0;
    s_has_baseline = false;
    ESP_LOGI(TAG, "Motion detector deinitialized");
}

esp_err_t rgb565_to_grayscale(const uint8_t *rgb565_data, size_t rgb565_size,
                               uint8_t *gray_data, size_t gray_size)
{
    if (!rgb565_data || !gray_data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t pixel_count = rgb565_size / 2;
    if (gray_size < pixel_count) {
        return ESP_ERR_INVALID_SIZE;
    }
    
    for (size_t i = 0; i < pixel_count; i++) {
        // RGB565: RRRRRGGGGGGBBBBB
        uint16_t pixel = (rgb565_data[i * 2 + 1] << 8) | rgb565_data[i * 2];
        
        uint8_t r = (pixel >> 11) & 0x1F;
        uint8_t g = (pixel >> 5) & 0x3F;
        uint8_t b = pixel & 0x1F;
        
        // Expand to 8-bit
        r = (r << 3) | (r >> 2);
        g = (g << 2) | (g >> 4);
        b = (b << 3) | (b >> 2);
        
        // Grayscale conversion using luminance formula
        gray_data[i] = (uint8_t)((r * 77 + g * 150 + b * 29) >> 8);
    }
    
    return ESP_OK;
}
