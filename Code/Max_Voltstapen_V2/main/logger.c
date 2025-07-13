#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include "esp_netif.h"

#define LOG_TAG "Logger"
#define LOG_QUEUE_SIZE 50
#define HTTP_TIMEOUT_MS 500

typedef struct {
    float timestamp;
    float line_error;
    float duty_left;
    float duty_right;
    int32_t encoder_left;
    int32_t encoder_right;
} LogEntry;

static QueueHandle_t log_queue = NULL;
static bool logger_enabled = false;
static esp_http_client_handle_t http_client = NULL;

static bool is_wifi_connected() {
    wifi_ap_record_t ap_info;
    return esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK;
}

static void logger_task(void *arg) {
    LogEntry entry;
    char post_data[256];
    
    esp_http_client_config_t config = {
        .url = "http://192.168.0.15:8080", 
        .method = HTTP_METHOD_POST,
        .timeout_ms = HTTP_TIMEOUT_MS,
        .keep_alive_enable = true,
    };
    
    http_client = esp_http_client_init(&config);
    if (http_client == NULL) {
        ESP_LOGE(LOG_TAG, "Failed to initialize HTTP client");
        vTaskDelete(NULL);
        return;
    }
    
    esp_http_client_set_header(http_client, "Content-Type", "application/json");

    while (1) {
        if (xQueueReceive(log_queue, &entry, portMAX_DELAY)) {
            // Sprawdź czy WiFi jest aktywne i logger włączony
            if (!logger_enabled || !is_wifi_connected()) {
                continue;
            }
            
            // Formatuj dane JSON
            int len = snprintf(post_data, sizeof(post_data),
                     "{\"timestamp\":%.3f,\"line_error\":%.2f,\"duty_left\":%.2f,\"duty_right\":%.2f,\"enc_l\":%ld,\"enc_r\":%ld}",
                     entry.timestamp, entry.line_error, entry.duty_left, entry.duty_right, 
                     entry.encoder_left, entry.encoder_right);

            if (len >= sizeof(post_data)) {
                ESP_LOGW(LOG_TAG, "Log data truncated");
                continue;
            }

            // Wyślij dane
            esp_http_client_set_post_field(http_client, post_data, len);
            esp_err_t err = esp_http_client_perform(http_client);
            
            if (err == ESP_OK) {
                int status_code = esp_http_client_get_status_code(http_client);
                if (status_code == 200) {
                    ESP_LOGD(LOG_TAG, "Data logged successfully");
                } else {
                    ESP_LOGW(LOG_TAG, "Server returned status: %d", status_code);
                }
            } else {
                ESP_LOGW(LOG_TAG, "HTTP request failed: %s", esp_err_to_name(err));
                // Opcjonalnie: wyłącz logger na chwilę po błędzie
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }
}

esp_err_t init_logger() {
    if (log_queue != NULL) {
        ESP_LOGW(LOG_TAG, "Logger already initialized");
        return ESP_OK;
    }

    log_queue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(LogEntry));
    if (log_queue == NULL) {
        ESP_LOGE(LOG_TAG, "Failed to create log queue");
        return ESP_FAIL;
    }

    BaseType_t ret = xTaskCreate(logger_task, "logger_task", 4096, NULL, 5, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(LOG_TAG, "Failed to create logger task");
        vQueueDelete(log_queue);
        log_queue = NULL;
        return ESP_FAIL;
    }

    ESP_LOGI(LOG_TAG, "Logger initialized successfully");
    return ESP_OK;
}

void enable_logger(bool enable) {
    logger_enabled = enable;
    ESP_LOGI(LOG_TAG, "Logger %s", enable ? "enabled" : "disabled");
}

void log_data(float timestamp, float line_error, float duty_left, float duty_right, 
              int32_t encoder_left, int32_t encoder_right) {
    if (log_queue == NULL || !logger_enabled) {
        return;
    }

    LogEntry entry = {
        .timestamp = timestamp,
        .line_error = line_error,
        .duty_left = duty_left,
        .duty_right = duty_right,
        .encoder_left = encoder_left,
        .encoder_right = encoder_right,
    };

    // Próbuj wysłać do kolejki, jeśli pełna - pomiń najstarszy wpis
    if (xQueueSend(log_queue, &entry, 0) != pdTRUE) {
        LogEntry dummy;
        xQueueReceive(log_queue, &dummy, 0);  // Usuń najstarszy
        xQueueSend(log_queue, &entry, 0);     // Dodaj nowy
        ESP_LOGW(LOG_TAG, "Log queue full, dropping oldest entry");
    }
}

void cleanup_logger() {
    if (http_client) {
        esp_http_client_cleanup(http_client);
        http_client = NULL;
    }
    
    if (log_queue) {
        vQueueDelete(log_queue);
        log_queue = NULL;
    }
    
    logger_enabled = false;
}