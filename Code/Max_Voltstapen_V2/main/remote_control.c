#include "remote_control.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "string.h"
#include "stdio.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "nvs_flash.h"

static float kp = 0.08; // Domyślna wartość dla kp
static float kd = 0.08; // Domyślna wartość dla kd
static const char *TAG = "WiFi";

static const char *html_form = "\
<!DOCTYPE html>\
<html>\
<head><title>PID Control</title></head>\
<body>\
<form action='/update' method='post'>\
    kp: <input type='text' name='kp' value='%.2f'><br>\
    kd: <input type='text' name='kd' value='%.2f'><br>\
    <input type='submit' value='Update'>\
</form>\
</body>\
</html>";

static esp_err_t root_handler(httpd_req_t *req) {
    char response[512];
    snprintf(response, sizeof(response), html_form, kp, kd);
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t update_handler(httpd_req_t *req) {
    char buf[100];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        return ESP_FAIL;
    }
    buf[ret] = '\0';
    sscanf(buf, "kp=%f&kd=%f", &kp, &kd);
    ESP_LOGI(TAG, "Updated kp=%.2f, kd=%.2f", kp, kd);
    httpd_resp_send(req, "Updated successfully", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

void start_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    httpd_start(&server, &config);
    httpd_uri_t uri_get = { .uri = "/", .method = HTTP_GET, .handler = root_handler, .user_ctx = NULL };
    httpd_uri_t uri_post = { .uri = "/update", .method = HTTP_POST, .handler = update_handler, .user_ctx = NULL };
    httpd_register_uri_handler(server, &uri_get);
    httpd_register_uri_handler(server, &uri_post);
}

void init_wifi(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Initializing WiFi...");
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "SKAR",
            .password = "kristalA152-+0"
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
vTaskDelay(pdMS_TO_TICKS(5000)); // Poczekaj 5 sek.

esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
esp_netif_ip_info_t ip_info;
esp_netif_get_ip_info(netif, &ip_info);

if (ip_info.ip.addr == 0) {
    ESP_LOGE(TAG, "WiFi connection failed!");
} else {
    ESP_LOGI(TAG, "Connected! IP Address: " IPSTR, IP2STR(&ip_info.ip));
}

    ESP_LOGI(TAG, "WiFi initialized");

    // esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    // esp_netif_ip_info_t ip_info;
    // esp_netif_get_ip_info(netif, &ip_info);
    // ESP_LOGI(TAG, "IP Address: " IPSTR, IP2STR(&ip_info.ip));

}

float get_kp(void) { return kp; }
float get_kd(void) { return kd; }
void set_kp(float new_kp) { kp = new_kp; }
void set_kd(float new_kd) { kd = new_kd; }
