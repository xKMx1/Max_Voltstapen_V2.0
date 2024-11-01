#include "controller.h"

const float WHEELBASE = 17.5;
const float default_speed = 20;

void controller(float line_deviation){
    float kp = 1;
    // float kd = 1;

    float error = 0.f - line_deviation;

    float turning_speed = kp * error;

    float duty_left = (2 * default_speed - turning_speed * WHEELBASE) / 2; 
	float duty_right = 2 * default_speed - duty_left;  

    // set_motorA_speed(duty_left);
    // set_motorB_speed(duty_right);
}