#include <stdio.h>

#include "sensors.h"
#include "motors.h"
#include "controller.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const int PWMA = 47;
const int PWMB = 38;
const int AIN1 = 35;
const int AIN2 = 48;
const int BIN1 = 36;
const int BIN2 = 37;

void app_main(void)
{
    init_gpio();
    init_pwm();
    initADC();
    while(1){
        set_motorA_direction(true);
        set_motorB_direction(false);

        

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        read_sens_value();
    }
}