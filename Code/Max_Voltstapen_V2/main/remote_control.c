#include "remote_control.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "string.h"
#include <stdio.h>

#define WIFI_SSID "SKAR"       // Zamień na swoją nazwę sieci WiFi
#define WIFI_PASS "kristalA152-+0"   // Zamień na hasło swojej sieci WiFi

static float kp = 0.08;  // Domyślna wartość dla kp
static float kd = 0.08;  // Domyślna wartość dla kd

// Funkcja handler obsługująca endpoint "/update_params"
static esp_err_t kp_kd_handler(httpd_req_t *req) {
    char buf[100];
    int ret = httpd_req_recv(req, buf, req->content_len);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    buf[ret] = '\0';

    // Parsowanie parametrów kp i kd z body requestu (np. "kp=0.1&kd=0.15")
    sscanf(buf, "kp=%f&kd=%f", &kp, &kd);

    // Wysłanie odpowiedzi
    char response[100];
    sprintf(response, "Updated kp to %.2f and kd to %.2f", kp, kd);
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    
    return ESP_OK;
}

// Getter dla parametrów kp i kd
float get_kp(void) { return kp; }
float get_kd(void) { return kd; }

// Setter dla parametrów kp i kd
void set_kp(float new_kp) { kp = new_kp; }
void set_kd(float new_kd) { kd = new_kd; }

// Funkcja uruchamiająca serwer HTTP
void start_web_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t kp_kd_uri = {
            .uri      = "/update_params",
            .method   = HTTP_POST,
            .handler  = kp_kd_handler
        };
        httpd_register_uri_handler(server, &kp_kd_uri);
    }
}

// Funkcja inicjalizująca WiFi
void init_wifi(void) {
    // Inicjalizacja NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicjalizacja sieci za pomocą esp_netif
    esp_netif_init();
    esp_netif_create_default_wifi_sta();  // Tworzenie interfejsu WiFi w trybie stacji

    // Konfiguracja WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // Tryb klienta WiFi
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("WiFi", "Connecting to %s...", WIFI_SSID);

    // Oczekiwanie na połączenie i pobranie adresu IP
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip_info;  // Użycie poprawnego typu
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {  // Użycie poprawnej funkcji
        printf("ESP32 IP Address: " IPSTR "\n", IP2STR(&ip_info.ip));
    }
}
