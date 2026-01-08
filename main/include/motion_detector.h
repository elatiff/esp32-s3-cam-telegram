/**
 * @file motion_detector.h
 * @brief Motion detection using frame comparison
 */

#ifndef MOTION_DETECTOR_H
#define MOTION_DETECTOR_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Motion detection result
 */
typedef struct {
    bool detected;           ///< Motion detected flag
    float change_percentage; ///< Percentage of changed pixels
    uint32_t changed_pixels; ///< Number of changed pixels
} motion_result_t;

/**
 * @brief Initialize motion detector
 * @param width Image width
 * @param height Image height
 * @param threshold Pixel difference threshold (0-255)
 * @param change_threshold Percentage of pixels that must change
 * @return ESP_OK on success
 */
esp_err_t motion_detector_init(int width, int height, int threshold, float change_threshold);

/**
 * @brief Process a frame for motion detection
 * @param grayscale_data Grayscale image data
 * @param size Size of image data
 * @return Motion detection result
 */
motion_result_t motion_detector_process(const uint8_t *grayscale_data, size_t size);

/**
 * @brief Reset motion detector baseline
 */
void motion_detector_reset(void);

/**
 * @brief Set detection threshold
 * @param threshold New threshold value
 */
void motion_detector_set_threshold(int threshold);

/**
 * @brief Deinitialize motion detector
 */
void motion_detector_deinit(void);

/**
 * @brief Convert RGB565 to grayscale
 * @param rgb565_data Input RGB565 data
 * @param rgb565_size Size of input data
 * @param gray_data Output grayscale buffer
 * @param gray_size Size of output buffer
 * @return ESP_OK on success
 */
esp_err_t rgb565_to_grayscale(const uint8_t *rgb565_data, size_t rgb565_size,
                               uint8_t *gray_data, size_t gray_size);

#ifdef __cplusplus
}
#endif

#endif // MOTION_DETECTOR_H
