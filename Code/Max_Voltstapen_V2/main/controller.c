#include "controller.h"

const float WHEELBASE = 17.5;
const float default_speed = 20;

void controller(float line_deviation){          // line deviation from -3500 to 3500 -> target value = 0
    float kp = 1;
    // float kd = 1;

    float error = 0.f - line_deviation;
    // static float last_error = 0.f;

    // float derivative = (error - last_error) * deltaT;
    float turning_speed = kp * error;                   // + kd * lastError

    float duty_left = (2 * default_speed - turning_speed * WHEELBASE) / 2;          //TODO zmniejszyc warotsci zeby wykorzystac caly przedzial dla silnikow
	float duty_right = 2 * default_speed - duty_left;  

    // set_motorA_speed(duty_left);
    // set_motorB_speed(duty_right);
}