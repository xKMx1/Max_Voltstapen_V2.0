#include <stdio.h>
#include "sensors.h"
#include "motors.h"
#include "controller.h"
#include "wifi_manager.h"
#include "encoders.h"
#include "logger.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"  // Dodane
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "esp_http_server.h"

const int PWMA = 47;
const int PWMB = 38;
const int AIN1 = 35;
const int AIN2 = 48;
const int BIN1 = 36;
const int BIN2 = 37;

static const char *TAG = "main";

// Jedna definicja zmiennej logging_enabled
bool logging_enabled = false;

const char* html_form = "<!DOCTYPE html><html><head><meta charset='utf-8'><title>PD Sterowanie</title>"
"<style>"
"body { font-family: sans-serif; margin: 30px; }"
"label { display: block; margin-top: 15px; font-weight: bold; }"
"input[type=range], input[type=number] { margin-right: 10px; }"
"input[type=checkbox] { margin-right: 10px; }"
"button { padding: 10px 20px; margin: 20px 10px 0 0; font-size: 16px; }"
"</style></head><body>"

"<h2>Sterowanie Robotem</h2>"

"<label for='kp'>Kp:</label>"
"<input type='range' id='kp' min='0' max='0.2' step='0.001' value='0.0075'>"
"<input type='number' id='kp_num' min='0' max='0.2' step='0.001' value='0.0075'>"

"<label for='kd'>Kd:</label>"
"<input type='range' id='kd' min='0' max='100' step='0.1' value='7.0'>"
"<input type='number' id='kd_num' min='0' max='100' step='0.1' value='7.0'>"

"<label for='speed'>Speed:</label>"
"<input type='range' id='speed' min='0' max='255' step='1' value='30'>"
"<input type='number' id='speed_num' min='0' max='255' step='1' value='30'>"

"<label for='logging'>"
"<input type='checkbox' id='logging'> Włącz logowanie danych"
"</label>"

"<div>"
"<button onclick='sendControl(1)'>Start</button>"
"<button onclick='sendControl(0)'>Stop</button>"
"</div>"

"<script>"
"function syncInputs(slider, number) {"
"  slider.oninput = () => number.value = slider.value;"
"  number.oninput = () => slider.value = number.value;"
"}"

"syncInputs(kp, kp_num);"
"syncInputs(kd, kd_num);"
"syncInputs(speed, speed_num);"

"function sendControl(run) {"
"  const params = new URLSearchParams();"
"  params.append('kp', kp.value);"
"  params.append('kd', kd.value);"
"  params.append('speed', speed.value);"
"  params.append('logging', logging.checked ? '1' : '0');"
"  params.append('run', run);"
"  fetch('/control?' + params.toString()).then(r => r.text()).then(console.log);"
"}"
"</script></body></html>";

esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, html_form, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t control_handler(httpd_req_t *req) {
    char query[150];  // Zwiększony rozmiar
    char buf[250];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char param[16];

        if (httpd_query_key_value(query, "kp", param, sizeof(param)) == ESP_OK) {
            kp = atof(param);
        }

        if (httpd_query_key_value(query, "kd", param, sizeof(param)) == ESP_OK) {
            kd = atof(param);
        }

        if (httpd_query_key_value(query, "speed", param, sizeof(param)) == ESP_OK) {
            default_speed = atof(param);
        }

        if (httpd_query_key_value(query, "logging", param, sizeof(param)) == ESP_OK) {
            logging_enabled = (atoi(param) == 1);
            enable_logger(logging_enabled);
        }

        if (httpd_query_key_value(query, "run", param, sizeof(param)) == ESP_OK) {
            robot_running = atoi(param); // 1 = start, 0 = stop
        }

        snprintf(buf, sizeof(buf),
            "Nowe wartosci:<br>kp: %.4f<br>kd: %.4f<br>speed: %.1f<br>logging: %s<br>running: %s",
            kp, kd, default_speed, logging_enabled ? "enabled" : "disabled", 
            robot_running ? "true" : "false");

        httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send(req, "Brak danych", HTTPD_RESP_USE_STRLEN);
    }

    return ESP_OK;
}

void start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_LOGI(TAG, "Uruchamiam serwer HTTP...");

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Serwer HTTP działa!");

        httpd_uri_t root = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_get_handler,
            .user_ctx = NULL
        };

        httpd_uri_t control = {
            .uri = "/control",
            .method = HTTP_GET,
            .handler = control_handler,
            .user_ctx = NULL
        };

        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &control);

    } else {
        ESP_LOGE(TAG, "Nie udało się uruchomić serwera HTTP!");
    }
}

void wifi_task(void *pvParameter) {
    // Zmień na station mode - łączenie z domową siecią
    wifi_init_sta();      // Zamiast wifi_init_softap()
    
    // Sprawdź czy połączono
    if (is_wifi_connected()) {
        ESP_LOGI(TAG, "WiFi połączone, uruchamiam serwer HTTP...");
        start_webserver();    // Start serwera HTTP
    } else {
        ESP_LOGE(TAG, "Nie udało się połączyć z WiFi!");
    }
    
    vTaskDelete(NULL);    // Kończy task po uruchomieniu serwera
}

void app_main(void) {
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    // Inicjalizacja podstawowych komponentów
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Inicjalizacja WiFi task (Access Point)
    xTaskCreate(&wifi_task, "wifi_task", 4096, NULL, 5, NULL);

    // Inicjalizacja hardware
    init_gpio();
    init_pwm();
    initADC();
    init_encoders();
    start_encoder_monitoring();
    
    // Inicjalizacja loggera
    if (init_logger() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize logger");
    }

    // Kalibracja czujników
    unsigned long currLoopT = 0;
    unsigned long prevLoopT = 0;
    float deltaLoopT = 0;

    int slowDown = 0;
    int sensorValues[8] = {0};
    float linePos = 0;

    // Kalibracja
    ESP_LOGI(TAG, "Starting sensor calibration...");
    for (uint16_t i = 0; i < 1000; i++) {
        calibrate(&calibration);
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGI(TAG, "Calibration done");

    // Główna pętla
    while (1) {
        currLoopT = esp_timer_get_time();
        deltaLoopT = ((float)(currLoopT - prevLoopT)) / 1.0e6;

        readSensValueCalibrated(sensorValues);
        linePos = readLine(sensorValues, &slowDown) - 3500.f;

        if (robot_running) {
            controller(linePos, deltaLoopT, &slowDown);
        } else {
            set_motorA_speed(0);
            set_motorB_speed(0);
        }

        prevLoopT = currLoopT;

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}