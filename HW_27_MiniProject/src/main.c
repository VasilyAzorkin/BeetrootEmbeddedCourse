#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char* TAG = "TrafficLight";

#define RED_LED_PIN             GPIO_NUM_10
#define YELLOW_LED_PIN          GPIO_NUM_16
#define GREEN_LED_PIN           GPIO_NUM_15
#define BUTTON_PIN              GPIO_NUM_4

#define RED_TIME_MS             3000
#define YELLOW_TIME_MS          1000
#define GREEN_TIME_MS           3000
#define GREEN_BLINK_TIME_MS     2000
#define YELLOW_AND_RED_TIME_MS  1000
#define DEBOUNCE_TIME_MS        50
#define TOGGLE_BLINK_MS         200

typedef enum {
    RED,
    YELLOW_AND_RED,
    GREEN,
    GREEN_BLINK,
    YELLOW,
    YELLOW_BLINK
} TrafficState;

volatile int64_t last_isr_time_ms = 0;
volatile bool blink_mode_toggle = false;

TrafficState current_state = RED;
int64_t state_timestamp_ms = 0;

bool yellow_blink_level = false;
bool green_level_low = false;
uint32_t prev_blink_ms = 0;

static void IRAM_ATTR button_isr_handler(void* arg)
{
    int64_t now_ms = esp_timer_get_time() / 1000;

    if (now_ms - last_isr_time_ms >= DEBOUNCE_TIME_MS) {
        last_isr_time_ms = now_ms;
        blink_mode_toggle = true;
    }
}

void all_leds_off()
{
    gpio_set_level(RED_LED_PIN, 1);
    gpio_set_level(YELLOW_LED_PIN, 1);
    gpio_set_level(GREEN_LED_PIN, 1);
}

void handle_state(TrafficState state)
{
    state_timestamp_ms = esp_timer_get_time() / 1000;
    all_leds_off();
    current_state = state;

    switch (current_state) {
        case RED:
            gpio_set_level(RED_LED_PIN, 0);
            ESP_LOGI(TAG, "State: RED");
            break;
        case YELLOW_AND_RED:
            gpio_set_level(RED_LED_PIN, 0);
            gpio_set_level(YELLOW_LED_PIN, 0);
            ESP_LOGI(TAG, "State: YELLOW AND RED");
            break;
        case GREEN:
            gpio_set_level(GREEN_LED_PIN, 0);
            ESP_LOGI(TAG, "State: GREEN");
            break;
        case GREEN_BLINK:
            prev_blink_ms = state_timestamp_ms;
            green_level_low = true;
            gpio_set_level(GREEN_LED_PIN, green_level_low);
            ESP_LOGI(TAG, "State: GREEN BLINK");
            break;
        case YELLOW:
            gpio_set_level(YELLOW_LED_PIN, 0);
            ESP_LOGI(TAG, "State: YELLOW");
            break;
        case YELLOW_BLINK:
            prev_blink_ms = state_timestamp_ms;
            yellow_blink_level = true;
            gpio_set_level(YELLOW_LED_PIN, yellow_blink_level);
            ESP_LOGI(TAG, "State: YELLOW BLINK");
            break;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Traffic Light Controller Starting...");

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RED_LED_PIN)
                      | (1ULL << YELLOW_LED_PIN)
                      | (1ULL << GREEN_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&btn_conf);
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);

    all_leds_off();
    handle_state(RED);

    while (1) {
        int64_t now_ms = esp_timer_get_time() / 1000;

        // Blink mode toggle handling
        if (blink_mode_toggle){
            blink_mode_toggle = false;
            handle_state(current_state == YELLOW_BLINK ? RED : YELLOW_BLINK);
        }


        switch (current_state) {
            case RED:
                if (now_ms - state_timestamp_ms >= RED_TIME_MS) {
                    handle_state(YELLOW_AND_RED);
                }
                break;
            case YELLOW_AND_RED:
                if (now_ms - state_timestamp_ms >= YELLOW_AND_RED_TIME_MS) {
                    handle_state(GREEN);
                }
                break;
            case GREEN:
                if (now_ms - state_timestamp_ms >= GREEN_TIME_MS) {
                    handle_state(GREEN_BLINK);
                }
                break;
            case GREEN_BLINK:
                if (now_ms - state_timestamp_ms >= GREEN_BLINK_TIME_MS) {
                    handle_state(YELLOW);
                    break;
                }
                if (now_ms - prev_blink_ms >= TOGGLE_BLINK_MS){
                    prev_blink_ms = now_ms;
                    green_level_low = !green_level_low;
                    gpio_set_level(GREEN_LED_PIN, green_level_low);
                }                
                break;
            case YELLOW:
                if (now_ms - state_timestamp_ms >= YELLOW_TIME_MS) {
                    handle_state(RED);
                }
                break;
            case YELLOW_BLINK:
                if (now_ms - prev_blink_ms >= TOGGLE_BLINK_MS){
                    prev_blink_ms = now_ms;
                    yellow_blink_level = !yellow_blink_level;
                    gpio_set_level(YELLOW_LED_PIN, yellow_blink_level);
                }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms delay to avoid busy loop
    }
}