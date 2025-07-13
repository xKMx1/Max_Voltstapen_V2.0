#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

esp_err_t init_logger();

void enable_logger(bool enable);

void log_data(float timestamp, float line_error, float duty_left, float duty_right, 
              int32_t encoder_left, int32_t encoder_right);

void cleanup_logger();

#endif