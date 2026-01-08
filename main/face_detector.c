/**
 * @file face_detector.c
 * @brief Face detection implementation
 * 
 * This module provides simplified face detection using the ESP-DL library.
 * For more advanced face recognition, you would need to add the human_face_detect
 * component from esp-dl.
 */

#include "face_detector.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>

static const char *TAG = "face_detector";

static bool s_initialized = false;
static int s_min_face_size = 48;

// Simple skin tone detection as fallback when esp-dl face detection isn't available
// This is a simplified approach - for production use esp-dl's human_face_detect

typedef struct {
    int x, y, w, h;
} face_box_t;

#define MAX_FACES 5

esp_err_t face_detector_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing face detector...");
    
    // Note: For full face detection with esp-dl, you would need to:
    // 1. Add esp-dl component to your project
    // 2. Include "human_face_detect_msr01.hpp" or "human_face_detect_mnp01.hpp"
    // 3. Create the detector model instance
    
    s_initialized = true;
    ESP_LOGI(TAG, "Face detector initialized (simplified mode)");
    
    return ESP_OK;
}

/**
 * @brief Simple skin tone detection for RGB565 image
 * This is a simplified approach - checks if pixel is in skin tone range
 */
static bool is_skin_tone_rgb565(uint16_t pixel)
{
    uint8_t r = (pixel >> 11) & 0x1F;
    uint8_t g = (pixel >> 5) & 0x3F;
    uint8_t b = pixel & 0x1F;
    
    // Expand to 8-bit
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    
    // Simple skin tone detection in RGB space
    // These thresholds work reasonably well for various skin tones
    if (r > 60 && g > 40 && b > 20 &&
        r > g && r > b &&
        abs(r - g) > 10 &&
        r - g < 100) {
        return true;
    }
    
    return false;
}

/**
 * @brief Find potential face regions using skin tone detection
 * This is a simplified approach for demonstration
 */
static int find_skin_regions(const uint8_t *rgb565_data, int width, int height,
                             face_box_t *faces, int max_faces)
{
    if (!rgb565_data || !faces) {
        return 0;
    }
    
    // Create a grid of skin-tone detection
    int grid_size = 8;
    int grid_w = width / grid_size;
    int grid_h = height / grid_size;
    
    // Allocate grid
    uint8_t *grid = heap_caps_calloc(grid_w * grid_h, 1, MALLOC_CAP_DEFAULT);
    if (!grid) {
        return 0;
    }
    
    // Analyze each grid cell
    for (int gy = 0; gy < grid_h; gy++) {
        for (int gx = 0; gx < grid_w; gx++) {
            int skin_count = 0;
            int total = 0;
            
            for (int y = gy * grid_size; y < (gy + 1) * grid_size && y < height; y++) {
                for (int x = gx * grid_size; x < (gx + 1) * grid_size && x < width; x++) {
                    int idx = (y * width + x) * 2;
                    uint16_t pixel = (rgb565_data[idx + 1] << 8) | rgb565_data[idx];
                    if (is_skin_tone_rgb565(pixel)) {
                        skin_count++;
                    }
                    total++;
                }
            }
            
            // Mark cell if more than 50% is skin-like
            if (skin_count > total / 2) {
                grid[gy * grid_w + gx] = 1;
            }
        }
    }
    
    // Find connected regions (simplified: just look for clusters)
    int face_count = 0;
    
    for (int gy = 0; gy < grid_h - 3 && face_count < max_faces; gy++) {
        for (int gx = 0; gx < grid_w - 3 && face_count < max_faces; gx++) {
            // Look for a minimum cluster of skin-tone cells
            int cluster = 0;
            for (int dy = 0; dy < 4; dy++) {
                for (int dx = 0; dx < 4; dx++) {
                    if (grid[(gy + dy) * grid_w + gx + dx]) {
                        cluster++;
                    }
                }
            }
            
            // If at least 10 cells are skin-like, consider it a potential face
            if (cluster >= 10) {
                faces[face_count].x = gx * grid_size;
                faces[face_count].y = gy * grid_size;
                faces[face_count].w = 4 * grid_size;
                faces[face_count].h = 4 * grid_size;
                
                // Check if this overlaps with an existing detection
                bool overlap = false;
                for (int i = 0; i < face_count; i++) {
                    if (abs(faces[i].x - faces[face_count].x) < s_min_face_size &&
                        abs(faces[i].y - faces[face_count].y) < s_min_face_size) {
                        overlap = true;
                        break;
                    }
                }
                
                if (!overlap && faces[face_count].w >= s_min_face_size) {
                    face_count++;
                    // Skip checked area
                    gx += 3;
                }
            }
        }
    }
    
    free(grid);
    return face_count;
}

face_result_t face_detector_detect(camera_fb_t *fb)
{
    face_result_t result = {
        .detected = false,
        .face_count = 0,
        .x = 0,
        .y = 0,
        .width = 0,
        .height = 0
    };
    
    if (!s_initialized || !fb) {
        return result;
    }
    
    // For JPEG images, we can't do face detection without decoding
    // This simplified version only works with RGB565 format
    if (fb->format != PIXFORMAT_RGB565) {
        ESP_LOGD(TAG, "Skipping face detection - image is not RGB565");
        return result;
    }
    
    face_box_t faces[MAX_FACES];
    int count = find_skin_regions(fb->buf, fb->width, fb->height, faces, MAX_FACES);
    
    if (count > 0) {
        result.detected = true;
        result.face_count = count;
        result.x = faces[0].x;
        result.y = faces[0].y;
        result.width = faces[0].w;
        result.height = faces[0].h;
        
        ESP_LOGI(TAG, "Detected %d potential face(s), first at (%d,%d) %dx%d",
                 count, result.x, result.y, result.width, result.height);
    }
    
    return result;
}

void face_detector_set_min_size(int size)
{
    s_min_face_size = size;
    ESP_LOGI(TAG, "Minimum face size set to %d", size);
}

void face_detector_deinit(void)
{
    // Clean up if needed
    s_initialized = false;
    ESP_LOGI(TAG, "Face detector deinitialized");
}
