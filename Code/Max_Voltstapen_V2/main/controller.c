#include "controller.h"
#include "motors.h"
#include "remote_control.h"

const float WHEELBASE = 0.175;
float default_speed = 20;                                                             //20/800

void controller(float line_deviation, float deltaT, int* slowDown) {
    float kp = 650.0f;
    float kd = 30000.0f;
    default_speed = 15;

    float error = 0.f - line_deviation;
    static float last_error = 0.f;

    float derivative = (error - last_error) * deltaT;

    
    float turning_speed = kp * error + kd * derivative;
    printf("der: %f\n", kp*error);
    
    if(*slowDown > 3){ 
        default_speed = 8;
        // duty_left = 0;
        // duty_left = 40;
    }

    float duty_left = (2 * default_speed - turning_speed * WHEELBASE) / 2;
    float duty_right = 2 * default_speed - duty_left;

    printf("slowDOwn: %d\n", *slowDown);

    if(duty_left > 255) { duty_left = 255;};
    if(duty_right > 255) { duty_right = 255;};
    if(duty_left < 0) { duty_left = 0;};
    if(duty_right < 0) { duty_right = 0;};

    printf("error: %f, left: %f, right: %f\n", error, duty_left, duty_right);

    set_motorA_direction(1);
    set_motorB_direction(0);
    set_motorA_speed(duty_right);
    set_motorB_speed(duty_left);

    last_error = error;
}
