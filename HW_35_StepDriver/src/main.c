#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// includes for ledc (servo)
#include <driver/ledc.h>

// include for logs
#include <esp_log.h>

// includes for adc
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>

// ledc consts
#define SERVO_PIN       15
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_SPEED_MODE LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES   LEDC_TIMER_10_BIT
#define SERVO_PWM_FREQ  50
#define SERVO_PERIOD_US 20000
#define SERVO_MIN_US    500
#define SERVO_MAX_US    2400

// adc consts
static const int POT_MAX_ANGLE = 270;
static const int POT_MAX_ANGLE_MV = 3176; // measured by multimeter
static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;
static adc_bitwidth_t ADC_BITWIDTH = ADC_BITWIDTH_12;
static adc_atten_t ADC_ATTEN = ADC_ATTEN_DB_12;
static adc_unit_t ADC_UNIT = ADC_UNIT_1;
static adc_channel_t ADC_CHANNEL = ADC_CHANNEL_3;

// common consts
static const int MIN_ANGLE = 0;
static const int MAX_ANGLE = 180;

void adc_init(){
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg));

    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
        .chan = ADC_CHANNEL
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle));     
}

int adc_read_mv(){
    int raw = 0;
    int mv = 0;

    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw));
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &mv));
    return mv;    
}

int adc_mv_to_angle(int mv){
    return mv * POT_MAX_ANGLE/POT_MAX_ANGLE_MV;
}

void servo_init(){
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_SPEED_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = SERVO_PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ledc_channel_config_t channel_config = {
        .channel = LEDC_CHANNEL,
        .speed_mode = LEDC_SPEED_MODE,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = SERVO_PIN,
        .duty = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

void rotate_servo_by_angle(int angle){
    int servo_angle = angle;
    if (servo_angle > MAX_ANGLE) servo_angle = MAX_ANGLE;
    float pulse_us = SERVO_MIN_US + (float)servo_angle *  (SERVO_MAX_US - SERVO_MIN_US) / (MAX_ANGLE - MIN_ANGLE);

    uint32_t duty = (uint32_t)(pulse_us * 1024) / SERVO_PERIOD_US;

    ESP_LOGI("POT-SERVO", "pot_angle:%d\tservo_angle:%d\n", angle, servo_angle);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
}

void app_main(){
    adc_init();
    servo_init();

    int mv = adc_read_mv();
    int angle = adc_mv_to_angle(mv);

    rotate_servo_by_angle(angle);
    vTaskDelay(pdMS_TO_TICKS(1000));

    while (1)
    {
        int mv = adc_read_mv();
        int angle = adc_mv_to_angle(mv);
        rotate_servo_by_angle(angle);
        vTaskDelay(pdMS_TO_TICKS(200));
    }    
}

