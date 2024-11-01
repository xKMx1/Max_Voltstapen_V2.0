#include "motors.h"

#define PWMA_PIN     47 
#define AIN1_PIN     35  
#define AIN2_PIN     48  
#define PWMB_PIN     38
#define BIN1_PIN     36  
#define BIN2_PIN     37  

#define PWM_FREQ     1000 // 1 kHz dla PWM

void init_pwm() {
    mcpwm_config_t pwm_config = {
        .frequency = PWM_FREQ,
        .cmpr_a = 0,       // Początkowy współczynnik wypełnienia 0% dla silnika A
        .cmpr_b = 0,
        .duty_mode = MCPWM_DUTY_MODE_0,
        .counter_mode = MCPWM_UP_COUNTER,
    };

    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, PWMA_PIN);
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0A, PWMB_PIN);
    mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_0, &pwm_config);
}

// Funkcja inicjalizująca piny GPIO
void init_gpio() {
    // Ustawienie pinów dla silnika A
    esp_rom_gpio_pad_select_gpio(AIN1_PIN);
    esp_rom_gpio_pad_select_gpio(AIN2_PIN);
    gpio_set_direction(AIN1_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(AIN2_PIN, GPIO_MODE_OUTPUT);

    // Ustawienie pinów dla silnika B
    esp_rom_gpio_pad_select_gpio(BIN1_PIN);
    esp_rom_gpio_pad_select_gpio(BIN2_PIN);
    gpio_set_direction(BIN1_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(BIN2_PIN, GPIO_MODE_OUTPUT);
}

// Funkcja ustawiająca kierunek obrotu dla silnika A
void set_motorA_direction(bool forward) {
    gpio_set_level(AIN1_PIN, forward ? 1 : 0);
    gpio_set_level(AIN2_PIN, forward ? 0 : 1);
}

// Funkcja ustawiająca kierunek obrotu dla silnika B
void set_motorB_direction(bool forward) {
    gpio_set_level(BIN1_PIN, forward ? 1 : 0);
    gpio_set_level(BIN2_PIN, forward ? 0 : 1);
}

// Funkcja ustawiająca prędkość silnika A
void set_motorA_speed(float duty_cycle) {
    duty_cycle = (duty_cycle < 0) ? 0 : (duty_cycle > 100) ? 100 : duty_cycle;
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty_cycle);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
}

// Funkcja ustawiająca prędkość silnika B
void set_motorB_speed(float duty_cycle) {
    duty_cycle = (duty_cycle < 0) ? 0 : (duty_cycle > 100) ? 100 : duty_cycle;
    mcpwm_set_duty(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A, duty_cycle);
    mcpwm_set_duty_type(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
}

