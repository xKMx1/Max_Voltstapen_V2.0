#include "sensors.h"

adc_oneshot_unit_handle_t adc1_handle;
adc_oneshot_unit_handle_t adc2_handle;

#define NUM_CHANNELS_ADC1 4
#define NUM_CHANNELS_ADC2 4

adc_channel_t adc1_channels[NUM_CHANNELS_ADC1] = {ADC_CHANNEL_5, ADC_CHANNEL_4, ADC_CHANNEL_3, ADC_CHANNEL_6};
adc_channel_t adc2_channels[NUM_CHANNELS_ADC2] = {ADC_CHANNEL_7, ADC_CHANNEL_6, ADC_CHANNEL_5, ADC_CHANNEL_4};

const uint8_t sensorCount = NUM_CHANNELS_ADC1 + NUM_CHANNELS_ADC2;

struct CalibrationData calibration = {0, NULL, NULL};

uint32_t lastPosition = (sensorCount - 1) * 1000 / 2;

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

int readLine(int* sensorValues){
    for(uint8_t i = 0; i < sensorCount; i++){
        printf("%d: %d, ", i, sensorValues[i]);
    }
    printf("\n");

    bool isOnLine = false;
    int avg = 0;
    int sum = 0;

    for(uint8_t i = 0; i < sensorCount; i++){
        int value = sensorValues[i];

        if(value > 200) { isOnLine = true; }

        if(value > 50){
            avg += (int32_t)value * (i * 1000);
            sum += value;
        }
    }

    if(!isOnLine){
        if(lastPosition  < (sensorCount - 1) * 1000 / 2) {return 0;}
        else {return((sensorCount - 1) * 1000);}
    }

    lastPosition = avg / sum;

    return lastPosition;
}

void readSensValue(int* raw_values){   // raw_values is 8 element array

    for (int i = 0; i < NUM_CHANNELS_ADC1; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, adc1_channels[i], &raw_values[i]));
    }

    for (int i = 0; i < NUM_CHANNELS_ADC2; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc2_handle, adc2_channels[i], &raw_values[NUM_CHANNELS_ADC1 + i]));
    }

    // for(int i = 0; i < NUM_CHANNELS_ADC1 + NUM_CHANNELS_ADC2; i++){
    //     printf("%d: %d, ", i, raw_values[i]);
    // }
    // printf("NIEZKALIBROWANE\n");
}

void readSensValueCalibrated(int* calibratedValues){
    int rawValues[sensorCount] = {};

    for (int i = 0; i < NUM_CHANNELS_ADC1; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, adc1_channels[i], &rawValues[i]));
    }

    for (int i = 0; i < NUM_CHANNELS_ADC2; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc2_handle, adc2_channels[i], &rawValues[NUM_CHANNELS_ADC1 + i]));
    }

    for(uint8_t i = 0; i < sensorCount; i++){
        uint16_t calMin = calibration.minimum[i];
        uint16_t calMax = calibration.maximum[i];

        uint16_t denominator = calMax - calMin;
        int16_t value = 0;

        if(denominator != 0){
            value = (((int16_t)rawValues[i]) - calMin) * 1000 / denominator;
        }

        if (value < 0) { value = 0; }
        else if (value > 1000) { value = 1000; }

        calibratedValues[i] = value;
    }
    // for(uint8_t i = 0; i < sensorCount; i++){
    //     printf(" %d: %d,", i, calibratedValues[i]);
    // }
    // printf("\n");
}

void calibrate(struct CalibrationData* calibration){
    int sensorValues[sensorCount];
    uint16_t maxSensorValues[sensorCount];
    uint16_t minSensorValues[sensorCount];

    if(!calibration->initialized){
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
            calibration->minimum[i] = 1000 * sensorCount;
        }

        calibration->initialized = true;
    }

    for(uint8_t j = 0; j < 10; j++){
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
}