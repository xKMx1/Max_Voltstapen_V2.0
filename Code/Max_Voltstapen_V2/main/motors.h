#ifndef MOTORS
#define MOTORS

#include "driver/mcpwm.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void init_pwm();
void init_gpio();
void set_motorA_direction(bool forward);
void set_motorB_direction(bool forward);
void set_motorA_speed(float duty_cycle);
void set_motorB_speed(float duty_cycle);

#endif