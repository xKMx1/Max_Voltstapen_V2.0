#ifndef SENSORS
#define SENSORS

#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"

#include <math.h>

struct CalibrationData
{
    bool initialized;
    uint16_t *minimum;
    uint16_t *maximum;
};

extern struct CalibrationData calibration;


//TODO create calibrationdata object in sensors.c

void initADC();
void readSensValue(int* raw_values);
void readSensValueCalibrated(int* rawValues);
void calibrate(struct CalibrationData* calibration);
float readLine(int* sensorValues, int* slowDown);

#endif