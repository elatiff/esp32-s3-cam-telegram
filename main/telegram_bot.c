/**
 * @file telegram_bot.c
 * @brief Telegram Bot API implementation for ESP32
 */

#include "telegram_bot.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "telegram_bot";

#define TELEGRAM_API_HOST "api.telegram.org"
#define MAX_HTTP_OUTPUT_BUFFER 2048
#define HTTP_TIMEOUT_MS 30000

static char s_bot_token[64] = {0};
static char s_chat_id[32] = {0};
static time_t s_last_notification_time = 0;
static bool s_initialized = false;

// Root CA certificate for Telegram API (api.telegram.org)
// This is the ISRG Root X1 certificate used by Let's Encrypt
extern const uint8_t telegram_root_cert_pem_start[] asm("_binary_telegram_root_cert_pem_start");
extern const uint8_t telegram_root_cert_pem_end[] asm("_binary_telegram_root_cert_pem_end");

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER: %s: %s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

esp_err_t telegram_bot_init(const char *bot_token, const char *chat_id)
{
    if (!bot_token || !chat_id) {
        ESP_LOGE(TAG, "Invalid bot token or chat ID");
        return ESP_ERR_INVALID_ARG;
    }
    
    strncpy(s_bot_token, bot_token, sizeof(s_bot_token) - 1);
    strncpy(s_chat_id, chat_id, sizeof(s_chat_id) - 1);
    
    s_initialized = true;
    ESP_LOGI(TAG, "Telegram bot initialized");
    
    return ESP_OK;
}

esp_err_t telegram_bot_send_message(const char *message)
{
    if (!s_initialized) {
        ESP_LOGE(TAG, "Telegram bot not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!message) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Sending message to Telegram...");
    
    // Build URL
    char url[256];
    snprintf(url, sizeof(url), "https://%s/bot%s/sendMessage",
             TELEGRAM_API_HOST, s_bot_token);
    
    // Build JSON body
    char body[512];
    snprintf(body, sizeof(body), 
             "{\"chat_id\":\"%s\",\"text\":\"%s\",\"parse_mode\":\"HTML\"}",
             s_chat_id, message);
    
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = HTTP_TIMEOUT_MS,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }
    
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, body, strlen(body));
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Message sent, HTTP status = %d", status);
        
        if (status != 200) {
            ESP_LOGW(TAG, "Telegram API returned non-200 status");
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    return err;
}

esp_err_t telegram_bot_send_photo(const uint8_t *photo_data, size_t photo_size, const char *caption)
{
    if (!s_initialized) {
        ESP_LOGE(TAG, "Telegram bot not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!photo_data || photo_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Sending photo to Telegram (%u bytes)...", photo_size);
    
    // Build URL
    char url[256];
    snprintf(url, sizeof(url), "https://%s/bot%s/sendPhoto",
             TELEGRAM_API_HOST, s_bot_token);
    
    // Create multipart form data
    // Boundary for multipart
    const char *boundary = "----ESP32CamBoundary";
    
    // Calculate total size
    char header_part[512];
    snprintf(header_part, sizeof(header_part),
             "--%s\r\n"
             "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n"
             "%s\r\n"
             "--%s\r\n"
             "Content-Disposition: form-data; name=\"photo\"; filename=\"photo.jpg\"\r\n"
             "Content-Type: image/jpeg\r\n\r\n",
             boundary, s_chat_id, boundary);
    
    char caption_part[256] = "";
    if (caption && strlen(caption) > 0) {
        snprintf(caption_part, sizeof(caption_part),
                 "\r\n--%s\r\n"
                 "Content-Disposition: form-data; name=\"caption\"\r\n\r\n"
                 "%s",
                 boundary, caption);
    }
    
    char footer_part[64];
    snprintf(footer_part, sizeof(footer_part), "\r\n--%s--\r\n", boundary);
    
    size_t header_len = strlen(header_part);
    size_t caption_len = strlen(caption_part);
    size_t footer_len = strlen(footer_part);
    size_t total_len = header_len + photo_size + caption_len + footer_len;
    
    // Allocate buffer for multipart data
    uint8_t *body = heap_caps_malloc(total_len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!body) {
        body = malloc(total_len);
    }
    
    if (!body) {
        ESP_LOGE(TAG, "Failed to allocate memory for HTTP body");
        return ESP_ERR_NO_MEM;
    }
    
    // Build body
    size_t offset = 0;
    memcpy(body + offset, header_part, header_len);
    offset += header_len;
    memcpy(body + offset, photo_data, photo_size);
    offset += photo_size;
    memcpy(body + offset, caption_part, caption_len);
    offset += caption_len;
    memcpy(body + offset, footer_part, footer_len);
    
    // Create content type header
    char content_type[64];
    snprintf(content_type, sizeof(content_type), "multipart/form-data; boundary=%s", boundary);
    
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = HTTP_TIMEOUT_MS,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        free(body);
        return ESP_FAIL;
    }
    
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", content_type);
    esp_http_client_set_post_field(client, (const char *)body, total_len);
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Photo sent, HTTP status = %d", status);
        
        if (status != 200) {
            ESP_LOGW(TAG, "Telegram API returned non-200 status");
            err = ESP_FAIL;
        } else {
            // Update last notification time
            time(&s_last_notification_time);
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    free(body);
    
    return err;
}

bool telegram_bot_can_send(int cooldown_sec)
{
    time_t now;
    time(&now);
    
    if (s_last_notification_time == 0) {
        return true;
    }
    
    return (now - s_last_notification_time) >= cooldown_sec;
}

void telegram_bot_reset_cooldown(void)
{
    s_last_notification_time = 0;
}

void telegram_bot_deinit(void)
{
    memset(s_bot_token, 0, sizeof(s_bot_token));
    memset(s_chat_id, 0, sizeof(s_chat_id));
    s_initialized = false;
    ESP_LOGI(TAG, "Telegram bot deinitialized");
}
