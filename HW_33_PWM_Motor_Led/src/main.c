#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "esp_adc/adc_oneshot.h"

#include "driver/ledc.h"

/// ADC defines and constants: begin
#define     MAX_BITWIDTH_VALUE_IN   4095
#define     MAX_MV_IN               3176

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;
static adc_bitwidth_t ADC_BITWIDTH = ADC_BITWIDTH_12;
static adc_atten_t ADC_ATTENUATION = ADC_ATTEN_DB_12;
static adc_unit_t ADC_UNIT = ADC_UNIT_1;
static adc_channel_t ADC_CHANNEL = ADC_CHANNEL_5; //GPIO 6

static const char *TAG_ADC =    "ADC";

/// ADC defines and constants: end


/// LED consts and defines: begin
#define     LED_PIN                 4
#define     LED_TIMER_ID            LEDC_TIMER_0
#define     LED_CHANNEL_ID          LEDC_CHANNEL_0
#define     LED_LEDC_MODE           LEDC_LOW_SPEED_MODE
#define     LED_PWM_FREQUENCY_HZ    5000
#define     LED_PWM_RESOLUTION      LEDC_TIMER_12_BIT
#define     LED_MAX_DUTY            4095

static const char *PWM_LED =    "LED";
/// LED consts and defines: end

/// MOTOR consts and defines: begin
#define     MOTOR_PIN               5
#define     MOTOR_TIMER_ID          LEDC_TIMER_1
#define     MOTOR_CHANNEL_ID        LEDC_CHANNEL_1
#define     MOTOR_LEDC_MODE         LEDC_LOW_SPEED_MODE
#define     MOTOR_PWM_FREQUENCY_HZ  20000
#define     MOTOR_PWM_RESOLUTION    LEDC_TIMER_11_BIT
#define     MOTOR_MAX_DUTY          2047

static const char *PWM_MOTOR =  "MOTOR";
/// MOTOR consts and defines: end


void pwm_led_init(void){
    ledc_timer_config_t timer_config = {
        .speed_mode       = LED_LEDC_MODE,
        .timer_num        = LED_TIMER_ID,
        .duty_resolution  = LED_PWM_RESOLUTION,
        .freq_hz          = LED_PWM_FREQUENCY_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ledc_channel_config_t channel_config = {
        .speed_mode     = LED_LEDC_MODE,
        .channel        = LED_CHANNEL_ID,
        .timer_sel      = LED_TIMER_ID,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LED_PIN,
        .duty           = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

void pwm_motor_init(void){
    ledc_timer_config_t timer_config = {
        .speed_mode       = MOTOR_LEDC_MODE,
        .timer_num        = MOTOR_TIMER_ID,
        .duty_resolution  = MOTOR_PWM_RESOLUTION,
        .freq_hz          = MOTOR_PWM_FREQUENCY_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ledc_channel_config_t channel_config = {
        .speed_mode     = MOTOR_LEDC_MODE,
        .channel        = MOTOR_CHANNEL_ID,
        .timer_sel      = MOTOR_TIMER_ID,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = MOTOR_PIN,
        .duty           = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

void adc_init(void)
{
    ESP_LOGI(TAG_ADC, "ADC calibration enabled (real Vref from eFuse)");
    // ADC unit
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT,                  // Використовуємо ADC1
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

    // ADC channel
    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTENUATION, 
    };
    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg)
    );

    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTENUATION,
        .bitwidth = ADC_BITWIDTH,
    };

    ESP_ERROR_CHECK(
         adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle)
    );  

    ESP_LOGI(TAG_ADC, "ADC calibration enabled (real Vref from eFuse)");
}

void app_main() {
    adc_init();
    pwm_led_init();
    pwm_motor_init();
    int voltage_raw_in = 0;
    int voltage_cali_in = 0;

    while (true)
    {
        adc_oneshot_read(adc_handle, ADC_CHANNEL, &voltage_raw_in);
        adc_cali_raw_to_voltage(cali_handle, voltage_raw_in, &voltage_cali_in);
        ESP_LOGI(TAG_ADC, "adc value:%d", voltage_cali_in);
        int led_pwm_value = voltage_cali_in * LED_MAX_DUTY / MAX_MV_IN;
        ESP_LOGI(PWM_LED, "ledc value:%d", led_pwm_value);
        ledc_set_duty(LED_LEDC_MODE, LED_CHANNEL_ID, led_pwm_value);
        ledc_update_duty(LED_LEDC_MODE, LED_CHANNEL_ID);
        int motor_pwm_value = voltage_cali_in * MOTOR_MAX_DUTY / MAX_MV_IN;
        ESP_LOGI(PWM_MOTOR, "ledc value:%d", motor_pwm_value);
        ledc_set_duty(MOTOR_LEDC_MODE, MOTOR_CHANNEL_ID, motor_pwm_value);
        ledc_update_duty(MOTOR_LEDC_MODE, MOTOR_CHANNEL_ID);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    
}