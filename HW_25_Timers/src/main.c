#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "esp_log.h"

#define BASE_GPIO           7

#define TIMER_BASE_CLK      APB_CLK_FREQ
#define TIMER_DIVIDER       16
#define TIMER_SCALE         (TIMER_BASE_CLK / TIMER_DIVIDER)
#define TIMER_CURRENT       TIMER_0
#define TIMER_GROUP_CURRENT TIMER_GROUP_0

#define TIMER_INTERVAL_SEC  15

static const char *TAG = "timer";

static volatile bool motor_enable = false;

static bool IRAM_ATTR timer_callback(void *args){
    motor_enable = !motor_enable;
    return true;
}

static void configure_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BASE_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

static void setup_timer(void){
    
    timer_config_t timer_config = {
        .alarm_en = TIMER_ALARM_EN,
        .counter_dir = TIMER_COUNT_UP,
        .divider = TIMER_DIVIDER,
        .counter_en = TIMER_PAUSE,
        .auto_reload = true
    };

    timer_init(TIMER_GROUP_CURRENT, TIMER_CURRENT, &timer_config);
    timer_set_counter_value(TIMER_GROUP_CURRENT, TIMER_CURRENT, 0);

    timer_set_alarm_value(TIMER_GROUP_CURRENT, TIMER_CURRENT, TIMER_INTERVAL_SEC * TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP_CURRENT, TIMER_CURRENT);
    
    timer_isr_callback_add(TIMER_GROUP_CURRENT, TIMER_CURRENT, timer_callback, NULL, 0);
    
    timer_start(TIMER_GROUP_CURRENT, TIMER_CURRENT);
}

void app_main(void)
{
    configure_gpio();
    setup_timer();

    ESP_LOGI(TAG, "TIMER started");

    while (true) {
        gpio_set_level(BASE_GPIO, motor_enable);
        ESP_LOGI(TAG, "Motor %s", motor_enable ? "enabled" : "disabled");
        vTaskDelay(pdMS_TO_TICKS(1000));    // Sleep to reduce CPU usage
    }
}