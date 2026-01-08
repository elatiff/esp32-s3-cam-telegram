/**
 * @file face_detector.h
 * @brief Face detection using ESP-DL
 */

#ifndef FACE_DETECTOR_H
#define FACE_DETECTOR_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_camera.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Face detection result
 */
typedef struct {
    bool detected;      ///< Face detected flag
    int face_count;     ///< Number of faces detected
    int x;              ///< X coordinate of first face
    int y;              ///< Y coordinate of first face
    int width;          ///< Width of first face
    int height;         ///< Height of first face
} face_result_t;

/**
 * @brief Initialize face detector
 * @return ESP_OK on success
 */
esp_err_t face_detector_init(void);

/**
 * @brief Detect faces in camera frame
 * @param fb Camera frame buffer (RGB565 or JPEG)
 * @return Face detection result
 */
face_result_t face_detector_detect(camera_fb_t *fb);

/**
 * @brief Set minimum face size for detection
 * @param size Minimum face size in pixels
 */
void face_detector_set_min_size(int size);

/**
 * @brief Deinitialize face detector
 */
void face_detector_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // FACE_DETECTOR_H
