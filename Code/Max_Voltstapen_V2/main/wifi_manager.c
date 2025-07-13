#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#include "string.h"
#include "freertos/event_groups.h"

static const char* TAG = "wifi_manager";
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi started, trying to connect...");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected, trying to reconnect...");
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected! Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void) {
    wifi_event_group = xEventGroupCreate();
    
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "2.4G-ithurtswhenIP",
            // .ssid = "AndroidAP9F82",
            .password = "9pc249dorw4vyuoy",
            // .password = "kvhy8620",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", wifi_config.sta.ssid);
    
    // Czekaj na połączenie (maksymalnie 10 sekund)
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, pdMS_TO_TICKS(10000));
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Successfully connected to WiFi!");
    } else {
        ESP_LOGE(TAG, "Failed to connect to WiFi");
    }
}

bool is_wifi_connected(void) {
    if (wifi_event_group == NULL) return false;
    EventBits_t bits = xEventGroupGetBits(wifi_event_group);
    return (bits & WIFI_CONNECTED_BIT) != 0;
}

// Funkcje ze starego kodu - zachowaj kompatybilność
void wifi_init_softap(void) {
    wifi_init_sta(); // Przekieruj na station mode
}

void wifi_connect_sta(const char* ssid, const char* password) {
    ESP_LOGI(TAG, "wifi_connect_sta called, but using predefined network");
    // Można by zaimplementować dynamiczną zmianę sieci, ale na razie używamy stałej
}