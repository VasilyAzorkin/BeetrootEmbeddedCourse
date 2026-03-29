#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define MAX_BITWIDTH_VALUE 4095
#define MAX_VOLTAGE_IN     3.3

static const char *TAG = "ADC";

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;
static adc_bitwidth_t ADC_BITWIDTH = ADC_BITWIDTH_12;
static adc_atten_t ADC_ATTENUATION = ADC_ATTEN_DB_12;
static adc_unit_t ADC_UNIT = ADC_UNIT_1;
static adc_channel_t ADC_CHANNEL = ADC_CHANNEL_3; // GPIO4
bool cali_enabled = false;

typedef struct {
    int raw;
    float manual;
    int cali;
    float error;
} adc_value_t;

void adc_init(void)
{
    ESP_LOGI("ADC", "ADC calibration enabled (real Vref from eFuse)");
    // ADC unit
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT,                  // Використовуємо ADC1
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

    // ADC channel
    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTENUATION, // до ~3.3V
    };
    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg)
    );

    // Калібрування (використає реальну Vref з eFuse)
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTENUATION,
        .bitwidth = ADC_BITWIDTH,
    };

    if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle) == ESP_OK) {
        cali_enabled = true;
        ESP_LOGI("ADC", "ADC calibration enabled (real Vref from eFuse)");
    } else {
        ESP_LOGW("ADC", "ADC calibration NOT available");
    }
}

esp_err_t adc_read_voltage_mv(adc_value_t* val)
{
    val->raw = 0;
    val->manual = 0.0;
    val->cali = 0;

    esp_err_t result = adc_oneshot_read(adc_handle, ADC_CHANNEL, &val->raw);
    if (result != ESP_OK) return result;
    
    val->manual = val->raw * MAX_VOLTAGE_IN  * 1000 / MAX_BITWIDTH_VALUE; 
    
    if (cali_enabled) {
        result = adc_cali_raw_to_voltage(cali_handle, val->raw, &val->cali);
        if (result != ESP_OK) return result;
        
        val->error = (val->manual - val->cali) * 100 / val->cali;
        return ESP_OK; 
    }

    return ESP_FAIL;
}


void app_main(void)
{
    ESP_LOGI("ADC", "ADC calibration enabled (real Vref from eFuse)");
    adc_init();

    adc_value_t val;
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI(TAG, "RAW\tManual\tCali\tErr");
    while (1) {
        ESP_ERROR_CHECK(adc_read_voltage_mv(&val));
        ESP_LOGI(TAG, "%d\t%0.2f\t%d\t%0.2f", val.raw, val.manual, val.cali, val.error);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}