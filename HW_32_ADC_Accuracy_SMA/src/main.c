#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"

#define TAG             "LDR_SMA"

#define LED_PIN         9
#define LDR_PIN         4

#define ADC_ATTEN       ADC_ATTEN_DB_12
#define ADC_BITWIDTH    ADC_BITWIDTH_12

#define LED_VALUE_ON    2000
#define LED_VALUE_OFF   2500

#define SMA_WINDOW_SIZE 16

static int sma_buffer[SMA_WINDOW_SIZE];
static int sma_index = 0;
static int sma_count = 0;
static int sma_sum = 0;

static int sma_add_sample(int new_sample)
{
    // Remove oldest sample if buffer is full
    if (sma_count == SMA_WINDOW_SIZE) {
        sma_sum -= sma_buffer[sma_index];
    } else {
        sma_count++;
    }

    // Add new sample
    sma_buffer[sma_index] = new_sample;
    sma_sum += new_sample;

    // Advance circular index
    sma_index = (sma_index + 1) % SMA_WINDOW_SIZE; 
    return sma_sum / sma_count;
}

static adc_oneshot_unit_handle_t adc_init(int pin_num, adc_channel_t *const adc_channel){
    adc_unit_t adc_unit;
    adc_oneshot_io_to_channel(LDR_PIN, &adc_unit, adc_channel);

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = adc_unit
    };
    
    adc_oneshot_unit_handle_t adc_handle;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t channel_cfg = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, *adc_channel, &channel_cfg));
    return adc_handle;
}

static void init_led(){
    const gpio_config_t io_cfg = {
        .pin_bit_mask = 1ULL << LED_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_cfg));
}

void app_main() {
    adc_channel_t adc_channel;    
    adc_oneshot_unit_handle_t adc_handle = adc_init(LDR_PIN, &adc_channel);
    ESP_LOGI(TAG, "ADC LDR measuring started");

    init_led();
    bool led_on = false;
    while (true)
    {
        int raw_adc_value = 0;

        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, adc_channel, &raw_adc_value));
        int averaged_value = sma_add_sample(raw_adc_value);

        ESP_LOGI(TAG, "Raw:%d\tSMA1:%d", raw_adc_value, averaged_value);
        if(averaged_value > LED_VALUE_OFF) led_on = false;
        else if (averaged_value < LED_VALUE_ON) led_on = true;
        gpio_set_level(LED_PIN, led_on);

        vTaskDelay(pdMS_TO_TICKS(200));
    }    
}