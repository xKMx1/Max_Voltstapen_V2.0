#include <stdio.h>

#include "sensors.h"
#include "motors.h"
#include "controller.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

const int PWMA = 47;
const int PWMB = 38;
const int AIN1 = 35;
const int AIN2 = 48;
const int BIN1 = 36;
const int BIN2 = 37;

void app_main(void)
{
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));  // Register main task with Task Watchdog

    init_gpio();
    init_pwm();
    initADC();

    int sensorValues[8] = {0};
    int16_t linePos = 0;

    for (uint16_t i = 0; i < 2000; i++) {
        calibrate(&calibration);
    }
    printf("Calibration done\n\n");

    while (1) {
        readSensValueCalibrated(sensorValues);
        linePos = readLine(sensorValues);
        printf("Line: %d\n", linePos);

        controller(linePos);
        esp_task_wdt_reset();  // Reset the Task Watchdog timer

        vTaskDelay(pdMS_TO_TICKS(10));  // Add delay to prevent the WDT trigger
    }
}
