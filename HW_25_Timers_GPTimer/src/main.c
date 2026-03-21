#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"

#define LED_GREEN_GPIO  4
#define LED_RED_GPIO    5
#define MOTOR_BASE_GPIO 7

static const char*      TAG                 = "timers";
static const char*      ENABLED_TXT         = "enabled";
static const char*      DISABLED_TXT        = "disabled";

static const uint32_t   DISABLEd_DELAY_SEC  = 12;  
static const uint32_t   ENABLEd_DELAY_SEC   = 3;  

static volatile bool motor_enable = false;

gptimer_handle_t timer_to_start = NULL;
gptimer_handle_t timer_to_stop = NULL;

/* Timer callbacks */

static bool IRAM_ATTR timer_to_start_handle(gptimer_handle_t timer,
                                const gptimer_alarm_event_data_t *edata,
                                void *user_ctx)
{
    motor_enable = true;
    return true;
}

static bool IRAM_ATTR timer_to_stop_handle(gptimer_handle_t timer,
                                const gptimer_alarm_event_data_t *edata,
                                void *user_ctx)
{
    motor_enable = false;
    return true;
}

static void handle_motor_system(){
    if(motor_enable){
        gptimer_stop(timer_to_start);
        gptimer_start(timer_to_stop);
    }
    else{
        gptimer_stop(timer_to_stop);
        gptimer_start(timer_to_start);
    }
    gpio_set_level(MOTOR_BASE_GPIO, motor_enable);
    gpio_set_level(LED_GREEN_GPIO, !motor_enable);
    gpio_set_level(LED_RED_GPIO, motor_enable);
    ESP_LOGI(TAG, "Motor %s", motor_enable ? ENABLED_TXT : DISABLED_TXT);
}

static void configure_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GREEN_GPIO) |
                        (1ULL << LED_RED_GPIO) |
                        (1ULL << MOTOR_BASE_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

static void create_and_start_timer(uint32_t period_us,
                                   gptimer_alarm_cb_t callback,
                                   gptimer_handle_t *out_timer)
{
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000  // 1 MHz = 1 µs
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, out_timer));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = callback
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(*out_timer, &cbs, NULL));

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = period_us,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(*out_timer, &alarm_config));
   
    ESP_ERROR_CHECK(gptimer_enable(*out_timer));
    ESP_ERROR_CHECK(gptimer_start(*out_timer));
}

void app_main(void)
{
    configure_gpio();

    create_and_start_timer(DISABLEd_DELAY_SEC * 1000000, timer_to_start_handle, &timer_to_start);
    create_and_start_timer(ENABLEd_DELAY_SEC * 1000000, timer_to_stop_handle, &timer_to_stop);

    ESP_LOGI(TAG, "Timers started");

    while (true) {
        handle_motor_system();
        vTaskDelay(pdMS_TO_TICKS(1000));    // Sleep to reduce CPU usage
    }
}