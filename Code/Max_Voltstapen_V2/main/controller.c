#include "controller.h"
#include "motors.h"

const float WHEELBASE = 17.5;
const float default_speed = 20;

void controller(float line_deviation){          // line deviation from 3500 to 7000 -> target value = 0
    float kp = 1;
    // float kd = 1;

    float error = 3.5f - line_deviation/1000;
    // static float last_error = 0.f;

    // float derivative = (error - last_error) * deltaT;
    float turning_speed = kp * error;                   // + kd * lastError

    float duty_left = (2 * default_speed - turning_speed * WHEELBASE) / 2;          //TODO zmniejszyc warotsci zeby wykorzystac caly przedzial dla silnikow
	float duty_right = 2 * default_speed - duty_left;  

    // printf("A: %f, B: %f, error:%f\n", duty_left, duty_right, error);
    set_motorA_direction(1);
    set_motorB_direction(1);
    set_motorA_speed(duty_right);
    set_motorB_speed(duty_left);
}