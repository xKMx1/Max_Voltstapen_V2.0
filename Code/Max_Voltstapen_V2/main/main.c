#include <stdio.h>
#include "sensors.h"
#include "motors.h"
#include "controller.h"
#include "remote_control.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"

const int PWMA = 47;
const int PWMB = 38;
const int AIN1 = 35;
const int AIN2 = 48;
const int BIN1 = 36;
const int BIN2 = 37;

void app_main(void) {
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    init_gpio();
    init_pwm();
    initADC();
    // init_wifi();

    unsigned long currLoopT = 0;
    unsigned long prevLoopT = 0;
    float deltaLoopT = 0;

    int slowDown = 0;

    int sensorValues[8] = {0};
    float linePos = 0;

    for (uint16_t i = 0; i < 1000; i++) {
        calibrate(&calibration);
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    printf("Calibration done\n\n");

    while (1) {
        // Dodatkowe logi
        currLoopT = esp_timer_get_time();
        deltaLoopT = ((float)(currLoopT - prevLoopT)) / 1.0e6;
        readSensValueCalibrated(sensorValues);
        linePos = readLine(sensorValues, &slowDown) - 3500.f;
        
        // printf("Line: %f\n", linePos);

        controller(linePos, deltaLoopT, &slowDown);

        prevLoopT = currLoopT;

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
