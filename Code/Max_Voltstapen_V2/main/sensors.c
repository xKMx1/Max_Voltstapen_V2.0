#include "sensors.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

adc_oneshot_unit_handle_t adc1_handle;
adc_oneshot_unit_handle_t adc2_handle;

#define NUM_CHANNELS_ADC1 4
#define NUM_CHANNELS_ADC2 4

#define XMAX 53.0                               // distance from middle to the end of the sensor board

adc_channel_t adc1_channels[NUM_CHANNELS_ADC1] = {ADC_CHANNEL_5, ADC_CHANNEL_4, ADC_CHANNEL_3, ADC_CHANNEL_6};
adc_channel_t adc2_channels[NUM_CHANNELS_ADC2] = {ADC_CHANNEL_7, ADC_CHANNEL_6, ADC_CHANNEL_5, ADC_CHANNEL_4};

const uint8_t sensorCount = NUM_CHANNELS_ADC1 + NUM_CHANNELS_ADC2;
const uint8_t singleCalibrationRunInterval = 10;

struct CalibrationData calibration = {0, NULL, NULL};

uint32_t lastPosition = (sensorCount - 1) * 1000 / 2;   // last position, starting in the middle

void initADC(){

    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    adc_oneshot_unit_init_cfg_t init_config2 = {
        .unit_id = ADC_UNIT_2,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config2, &adc2_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    for (int i = 0; i < NUM_CHANNELS_ADC1; i++) {
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, adc1_channels[i], &config));
    }

    for (int i = 0; i < NUM_CHANNELS_ADC2; i++) {
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, adc2_channels[i], &config));
    }
}

float readLine(int* sensorValues, int* slowDown){
    for(uint8_t i = 0; i < sensorCount; i++){
        printf("%d: %d, ", i, sensorValues[i]);
    }
    printf(" \n");
    // printf(" \n");
    *slowDown = 0;
    bool isOnLine = false;
    int avg = 0;
    int sum = 0;

    for(uint8_t i = 0; i < sensorCount; i++){
        int value = sensorValues[i];

        if(value > 200){
            isOnLine = true; 
        }

        if(value > 50){ 
            avg += (int32_t)value * (i * 1000);
            sum += value;
        }
        esp_task_wdt_reset();
    }
    
    if(!isOnLine){
        if(lastPosition  < (sensorCount - 1) * 1000 / 2) {lastPosition =  0;}
        else {lastPosition = ((sensorCount - 1) * 1000);}
    }
    // else{
    if(sum != 0){
        lastPosition = avg / sum;
    }
    // }

    printf("%ld", lastPosition);


    float deviation = ((XMAX / 3500.f) * lastPosition) - XMAX;            // distance from middle axis of the robot to the sensor which has line underneath
    float angle_error = asin(deviation / (sqrt((deviation * deviation) + (29756.25f))));   // in radians, max 0,2314
    
    // return angle_error;
    return lastPosition;
}

// Read raw values from KTIR sensros
void readSensValue(int* raw_values){   // raw_values is 8 element array

    for (int i = 0; i < NUM_CHANNELS_ADC1; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, adc1_channels[i], &raw_values[i]));
        ESP_ERROR_CHECK(adc_oneshot_read(adc2_handle, adc2_channels[i], &raw_values[NUM_CHANNELS_ADC1 + i]));
    }

    // TODO check if doing 3 adc readings at the same time works

    // for (int i = 0; i < NUM_CHANNELS_ADC2; i++) {
    //     ESP_ERROR_CHECK(adc_oneshot_read(adc2_handle, adc2_channels[i], &raw_values[NUM_CHANNELS_ADC1 + i]));
    // }
}

// Read values from KTIR sensors and convert them accordingly into 0-1000 range
void readSensValueCalibrated(int* calibratedValues){
    int rawValues[sensorCount] = {};
    int wageMask[] = {1, 1, 1, 1,      1, 1, 1, 1};

    for (int i = 0; i < NUM_CHANNELS_ADC1; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, adc1_channels[i], &rawValues[i]));
        ESP_ERROR_CHECK(adc_oneshot_read(adc2_handle, adc2_channels[i], &rawValues[NUM_CHANNELS_ADC1 + i]));
    }

    // TODO check if doing 3 adc readings at the same time works

    // for (int i = 0; i < NUM_CHANNELS_ADC2; i++) {
    //     ESP_ERROR_CHECK(adc_oneshot_read(adc2_handle, adc2_channels[i], &rawValues[NUM_CHANNELS_ADC1 + i]));
    // }

    for(uint8_t i = 0; i < sensorCount; i++){
        int calMin = calibration.minimum[i];
        int calMax = calibration.maximum[i];

        int denominator = calMax - calMin;
        int value = 0;

        if(denominator != 0){
            value = (((int16_t)rawValues[i]) - calMin) * 1000 / denominator;
        }

        if (value < 0) { value = 0; }
        else if (value > 1000) { value = 1000; }

        calibratedValues[i] = value;      // save calculated value to the array
    }                                    // TODO change how wageMask works
}

// 
void calibrate(struct CalibrationData* calibration){
    int sensorValues[sensorCount];
    uint16_t maxSensorValues[sensorCount];
    uint16_t minSensorValues[sensorCount];

    if(!calibration->initialized){               // initialize calibration data with sample values
        uint16_t* oldMax = calibration->maximum;

        calibration->maximum = (uint16_t*)realloc(calibration->maximum, sizeof(uint16_t) * sensorCount);
        if (calibration->maximum == NULL){       // Memory allocation failed; don't continue.
            free(oldMax); // deallocate any memory used by old array
            return;
        }

        uint16_t* oldMin = calibration->minimum;

        calibration->minimum = (uint16_t*)realloc(calibration->minimum, sizeof(uint16_t) * sensorCount);
        if (calibration->minimum == NULL){       // Memory allocation failed; don't continue.
            free(oldMin); // deallocate any memory used by old array
            return;
        }

        for (uint8_t i = 0; i < sensorCount; i++){
            calibration->maximum[i] = 0;
            calibration->minimum[i] = 1000 * (sensorCount - 1);
        }

        calibration->initialized = true;
    }

    for(uint8_t j = 0; j < singleCalibrationRunInterval; j++){      // find max and min during singleCalibrationRunInverval times
        readSensValue(sensorValues);

        for (uint8_t i = 0; i < sensorCount; i++){
            // set the max we found THIS time
            if ((j == 0) || (sensorValues[i] > maxSensorValues[i]))
            {
                maxSensorValues[i] = sensorValues[i];
            }

            // set the min we found THIS time
            if ((j == 0) || (sensorValues[i] < minSensorValues[i]))
            {
                minSensorValues[i] = sensorValues[i];
            }
        }
    }

    for (uint8_t i = 0; i < sensorCount; i++){
        // Update maximum only if the min of 10 readings was still higher than it
        // (we got 10 readings in a row higher than the existing maximum).
        if (minSensorValues[i] > calibration->maximum[i]){
            calibration->maximum[i] = minSensorValues[i];
        }

        // Update minimum only if the max of 10 readings was still lower than it
        // (we got 10 readings in a row lower than the existing minimum).
        if (maxSensorValues[i] < calibration->minimum[i]){
            calibration->minimum[i] = maxSensorValues[i];
        }
    }

    // TODO dealocate arrays
}