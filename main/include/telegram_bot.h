/**
 * @file telegram_bot.h
 * @brief Telegram Bot API client for sending messages and photos
 */

#ifndef TELEGRAM_BOT_H
#define TELEGRAM_BOT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Telegram bot client
 * @param bot_token Bot token from BotFather
 * @param chat_id Target chat ID
 * @return ESP_OK on success
 */
esp_err_t telegram_bot_init(const char *bot_token, const char *chat_id);

/**
 * @brief Send text message to Telegram
 * @param message Message text
 * @return ESP_OK on success
 */
esp_err_t telegram_bot_send_message(const char *message);

/**
 * @brief Send photo to Telegram
 * @param photo_data JPEG image data
 * @param photo_size Size of image data
 * @param caption Optional caption (can be NULL)
 * @return ESP_OK on success
 */
esp_err_t telegram_bot_send_photo(const uint8_t *photo_data, size_t photo_size, const char *caption);

/**
 * @brief Check if cooldown period has passed since last notification
 * @param cooldown_sec Cooldown period in seconds
 * @return true if notification can be sent
 */
bool telegram_bot_can_send(int cooldown_sec);

/**
 * @brief Reset notification cooldown timer
 */
void telegram_bot_reset_cooldown(void);

/**
 * @brief Deinitialize Telegram bot client
 */
void telegram_bot_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // TELEGRAM_BOT_H
