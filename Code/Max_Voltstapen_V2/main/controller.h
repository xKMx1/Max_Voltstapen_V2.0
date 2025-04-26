#ifndef CONTROLLER
#define CONTROLLER

#include <stdbool.h> 

extern float kp;
extern float kd;
extern float default_speed;
extern bool robot_running;


void controller(float line_deviation, float deltaT, int* slowDown);

#endif