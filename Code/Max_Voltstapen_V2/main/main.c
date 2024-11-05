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

    int sensorValues[8] = {0};
    int16_t linePos = 0;

    for(uint16_t i = 0; i < 2000; i++){
        calibrate(&calibration);
    }
    printf("done\n\n");

    while(1){
        readSensValueCalibrated(sensorValues);
        // for(uint8_t i = 0; i < 8; i++){
        //     printf("%d: %d, ", i, sensorValues[i]);
        // }
        // printf("\n");

        linePos = readLine(sensorValues);               
        printf("Line: %d ", linePos);

        controller(linePos);
    }
}