#include "sensors.h"

adc_oneshot_unit_handle_t adc1_handle;
adc_oneshot_unit_handle_t adc2_handle;

#define NUM_CHANNELS_ADC1 4
#define NUM_CHANNELS_ADC2 4

adc_channel_t adc1_channels[NUM_CHANNELS_ADC1] = {ADC_CHANNEL_5, ADC_CHANNEL_4, ADC_CHANNEL_3, ADC_CHANNEL_6};
adc_channel_t adc2_channels[NUM_CHANNELS_ADC2] = {ADC_CHANNEL_7, ADC_CHANNEL_6, ADC_CHANNEL_5, ADC_CHANNEL_4};

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

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_4, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_5, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &config));

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL_4, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL_5, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL_6, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL_7, &config));

    for (int i = 0; i < NUM_CHANNELS_ADC1; i++) {
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, adc1_channels[i], &config));
    }

    for (int i = 0; i < NUM_CHANNELS_ADC2; i++) {
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, adc2_channels[i], &config));
    }
}

void appr_pos(){
    
}

void read_sens_value(){
    int adc_raw_values[NUM_CHANNELS_ADC1 + NUM_CHANNELS_ADC2] = {};

    for (int i = 0; i < NUM_CHANNELS_ADC1; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, adc1_channels[i], &adc_raw_values[i]));
    }

    for (int i = 0; i < NUM_CHANNELS_ADC2; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc2_handle, adc2_channels[i], &adc_raw_values[NUM_CHANNELS_ADC1 + i]));
    }

    for(int i = 0; i < NUM_CHANNELS_ADC1 + NUM_CHANNELS_ADC2; i++){
        printf("%d: %d", i, adc_raw_values[i]);
    }
}

