#ifndef ENCODERS_H
#define ENCODERS_H

#include <stdint.h>

void init_encoders();
int32_t get_encoder_left();
int32_t get_encoder_right();
void reset_encoders();
void print_encoder_data();
void start_encoder_monitoring();

#endif
