#include "controller.h"
#include "motors.h"
#include "remote_control.h"
#include <math.h>

const float WHEELBASE = 0.175;
float default_speed = 50;   
float spinning_speed = 35;                                                          

void controller(float line_deviation, float deltaT, int* slowDown) {
    float kp = 0.02f;
    float kd = 6.0f;
    default_speed = 40;

    // float kp = 0.012f;
    // float kd = 0.9f;
    // default_speed = 30;
    // napiecie na kurwie - 7,46

    float error = /*0.f -*/ line_deviation;
    static float last_error = 0.f;

    float derivative = (error - last_error) * deltaT;
    
    float turning_speed = kp * error + kd * derivative;
    // printf("der: %f\n", kp*error);
    
    // if(*slowDown > 3){ 
    //     default_speed = 30;
    //     // duty_left = 0;
    //     // duty_left = 40;
    // }

    // float duty_left = (2 * default_speed - turning_speed * WHEELBASE) / 2;
    // float duty_right = 2 * default_speed - duty_left;

    float duty_left = default_speed + turning_speed;
    float duty_right = default_speed - turning_speed;    


    // printf("slowDOwn: %d\n", *slowDown);

    if(duty_left > 255) { duty_left = 255;};
    if(duty_right > 255) { duty_right = 255;};
    if(duty_left < 0) { duty_left = 0;};
    if(duty_right < 0) { duty_right = 0;};

    // printf("error: %f, left: %f, right: %f\n", error, duty_left, duty_right);

    if(fabs(error) < 3400){        
        set_motorA_direction(1);
        set_motorB_direction(0);
        set_motorA_speed(duty_right);
        set_motorB_speed(duty_left);
    }
    else if(error > 3400){
        set_motorA_direction(0);
        set_motorB_direction(0);
        set_motorA_speed(spinning_speed);
        set_motorB_speed(spinning_speed);
    }
    else if(error < -3400){
        set_motorA_direction(1);
        set_motorB_direction(1);
        set_motorA_speed(spinning_speed);
        set_motorB_speed(spinning_speed);
    }

    last_error = error;
}
