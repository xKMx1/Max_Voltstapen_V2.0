#include "encoders.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include <stdint.h>

#define ENCODER_LEFT_A GPIO_NUM_13
#define ENCODER_LEFT_B GPIO_NUM_14
#define ENCODER_RIGHT_A GPIO_NUM_12
#define ENCODER_RIGHT_B GPIO_NUM_11

static volatile int32_t encoder_left = 0;
static volatile int32_t encoder_right = 0;
static const char *TAG = "encoders";

static void IRAM_ATTR encoder_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    
    // LEFT
    if (gpio_num == ENCODER_LEFT_A) {
        int a = gpio_get_level(ENCODER_LEFT_A);
        int b = gpio_get_level(ENCODER_LEFT_B);
        encoder_left += (a == b) ? 1 : -1;
    }
    // RIGHT
    else if (gpio_num == ENCODER_RIGHT_A) {
        int a = gpio_get_level(ENCODER_RIGHT_A);
        int b = gpio_get_level(ENCODER_RIGHT_B);
        encoder_right += (a == b) ? -1 : 1;
    }
}

void init_encoders() {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << ENCODER_LEFT_A) | (1ULL << ENCODER_LEFT_B) | 
                        (1ULL << ENCODER_RIGHT_A) | (1ULL << ENCODER_RIGHT_B),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    
    gpio_config(&io_conf);
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(ENCODER_LEFT_A, encoder_isr_handler, (void*) ENCODER_LEFT_A);
    gpio_isr_handler_add(ENCODER_RIGHT_A, encoder_isr_handler, (void*) ENCODER_RIGHT_A);
    
    ESP_LOGI(TAG, "Enkodery zainicjalizowane");
}

int32_t get_encoder_left() {
    return encoder_left;
}

int32_t get_encoder_right() {
    return encoder_right;
}

void reset_encoders() {
    encoder_left = 0;
    encoder_right = 0;
}

// Funkcja do printowania danych z enkoderów
void print_encoder_data() {
    ESP_LOGI(TAG, "Lewy enkoder: %ld, Prawy enkoder: %ld", encoder_left, encoder_right);
}

// Task do ciągłego monitorowania enkoderów
void encoder_monitor_task(void *pvParameter) {
    while (1) {
        print_encoder_data();
        vTaskDelay(pdMS_TO_TICKS(500)); // Printuj co 500ms
    }
}

// Funkcja do uruchomienia monitorowania enkoderów
void start_encoder_monitoring() {
    xTaskCreate(encoder_monitor_task, "encoder_monitor", 2048, NULL, 5, NULL);
    ESP_LOGI(TAG, "Monitoring enkoderów uruchomiony");
}