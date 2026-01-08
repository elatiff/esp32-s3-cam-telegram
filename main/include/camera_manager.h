/**
 * @file camera_manager.h
 * @brief Camera initialization and capture for ESP32-S3-CAM
 */

#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include "esp_err.h"
#include "esp_camera.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize camera with configured settings
 * @return ESP_OK on success
 */
esp_err_t camera_manager_init(void);

/**
 * @brief Capture a single frame
 * @return Pointer to camera frame buffer or NULL on failure
 */
camera_fb_t* camera_manager_capture(void);

/**
 * @brief Return frame buffer to driver
 * @param fb Frame buffer to return
 */
void camera_manager_return_fb(camera_fb_t *fb);

/**
 * @brief Set camera resolution
 * @param framesize Frame size enum
 * @return ESP_OK on success
 */
esp_err_t camera_manager_set_framesize(framesize_t framesize);

/**
 * @brief Set camera quality (JPEG compression)
 * @param quality Quality value (10-63, lower is better quality)
 * @return ESP_OK on success
 */
esp_err_t camera_manager_set_quality(int quality);

/**
 * @brief Enable/disable horizontal flip
 * @param enable true to enable flip
 */
void camera_manager_set_hmirror(bool enable);

/**
 * @brief Enable/disable vertical flip
 * @param enable true to enable flip
 */
void camera_manager_set_vflip(bool enable);

/**
 * @brief Deinitialize camera
 */
void camera_manager_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // CAMERA_MANAGER_H
