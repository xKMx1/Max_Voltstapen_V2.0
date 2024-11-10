idf_component_register(SRCS "remote_control.c" "controller.c" "motors.c" "sensors.c" "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_adc esp_timer esp_wifi esp_http_server nvs_flash)