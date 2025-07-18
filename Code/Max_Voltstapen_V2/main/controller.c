#include "controller.h"
#include "motors.h"
#include "wifi_manager.h"
#include <math.h>

#include "logger.h"
#include "encoders.h"
#include "esp_timer.h"

const float WHEELBASE = 0.175;
float spinning_speed = 25;   

float kp = 0.0075f;
float kd = 7.0f;
float default_speed = 30.f;
bool robot_running = false;

void controller(float line_deviation, float deltaT, int* slowDown) {

    // float kp = 0.012f;
    // float kd = 0.9f;
    // default_speed = 30;
    // napiecie na kurwie - 7,46

    float error = /* 0.f - */ line_deviation;
    static float last_error = 0.f;

    float derivative = (error - last_error) * deltaT;
    
    float turning_speed = kp * error + kd * derivative;
    
    float duty_left = default_speed + turning_speed;
    float duty_right = default_speed - turning_speed;    

    if (duty_left > 255) duty_left = 255;
    if (duty_right > 255) duty_right = 255;
    if (duty_left < -255) duty_left = -255;
    if (duty_right < -255) duty_right = -255;

    if (duty_left < 0) {
        set_motorB_direction(1);
        duty_left = -duty_left;   
    } else {
        set_motorB_direction(0);
    }
    
    if (duty_right < 0) {
        set_motorA_direction(0);  
        duty_right = -duty_right; 
    } else {
        set_motorA_direction(1);
    }
    
    // Ustawienie prędkości silników
    set_motorA_speed(duty_right);
    set_motorB_speed(duty_left);

    float timestamp = (float)(esp_timer_get_time() / 1.0e6);
    log_data(timestamp, error, duty_left, duty_right, get_encoder_left(), get_encoder_right());
    
    last_error = error;
}