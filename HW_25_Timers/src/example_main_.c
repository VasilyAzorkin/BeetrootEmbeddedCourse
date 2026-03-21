// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/gpio.h"
// #include "driver/gptimer.h"
// #include "esp_log.h"

// #define LED1_GPIO 11
// #define LED2_GPIO 12

// static const char *TAG = "timers";
// volatile uint32_t t1_hits = 0;
// volatile uint32_t t2_hits = 0;

// static volatile bool led1_state = false;
// static volatile bool led2_state = false;

// /* Timer callbacks */

// static bool IRAM_ATTR timer1_cb(gptimer_handle_t timer,
//                                 const gptimer_alarm_event_data_t *edata,
//                                 void *user_ctx)
// {
//     led1_state = !led1_state;
//     gpio_set_level(LED1_GPIO, led1_state);
//     t1_hits++;
//     return true;
// }

// static bool IRAM_ATTR timer2_cb(gptimer_handle_t timer,
//                                 const gptimer_alarm_event_data_t *edata,
//                                 void *user_ctx)
// {
//     led2_state = !led2_state;
//     gpio_set_level(LED2_GPIO, led2_state);
//     t2_hits++;
//     return true;
// }

// static void configure_gpio(void)
// {
//     gpio_config_t io_conf = {
//         .pin_bit_mask = (1ULL << LED1_GPIO) |
//                         (1ULL << LED2_GPIO),
//         .mode = GPIO_MODE_OUTPUT,
//         .pull_up_en = GPIO_PULLUP_DISABLE,
//         .pull_down_en = GPIO_PULLDOWN_DISABLE,
//         .intr_type = GPIO_INTR_DISABLE
//     };
//     gpio_config(&io_conf);
// }

// static void create_and_start_timer(uint32_t period_us,
//                                    gptimer_alarm_cb_t callback,
//                                    gptimer_handle_t *out_timer)
// {
//     gptimer_config_t timer_config = {
//         .clk_src = GPTIMER_CLK_SRC_DEFAULT,
//         .direction = GPTIMER_COUNT_UP,
//         .resolution_hz = 1 * 1000 * 1000  // 1 MHz = 1 µs
//     };
//     ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, out_timer));

//     gptimer_event_callbacks_t cbs = {
//         .on_alarm = callback
//     };
//     ESP_ERROR_CHECK(gptimer_register_event_callbacks(*out_timer, &cbs, NULL));

//     gptimer_alarm_config_t alarm_config = {
//         .alarm_count = period_us,
//         .reload_count = 0,
//         .flags.auto_reload_on_alarm = true,
//     };
//     ESP_ERROR_CHECK(gptimer_set_alarm_action(*out_timer, &alarm_config));

//     gptimer_stop(*out_timer);

//     ESP_ERROR_CHECK(gptimer_enable(*out_timer));
//     ESP_ERROR_CHECK(gptimer_start(*out_timer));
// }

// void app_main(void)
// {
//     configure_gpio();

//     gptimer_handle_t timer1 = NULL;
//     gptimer_handle_t timer2 = NULL;

//     create_and_start_timer(500000,  timer1_cb, &timer1); // 0.5 s
//     create_and_start_timer(1000000, timer2_cb, &timer2); // 1 s

//     ESP_LOGI(TAG, "4 hardware timers started");

//     while (true) {
//         vTaskDelay(pdMS_TO_TICKS(1000));    // Sleep to reduce CPU usage
//         ESP_LOGI(TAG, "Timer 1 hits: %lu", t1_hits);
//         ESP_LOGI(TAG, "Timer 2 hits: %lu", t2_hits);

//     }
// }