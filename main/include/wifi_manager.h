/**
 * @file wifi_manager.h
 * @brief WiFi connection manager for ESP32-S3-CAM
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize WiFi in station mode
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Connect to configured WiFi network
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_connect(void);

/**
 * @brief Check if WiFi is connected
 * @return true if connected
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Get current IP address as string
 * @return IP address string or NULL if not connected
 */
const char* wifi_manager_get_ip(void);

/**
 * @brief Disconnect from WiFi
 */
void wifi_manager_disconnect(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H
