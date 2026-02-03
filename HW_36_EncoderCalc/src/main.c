#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <driver/pulse_cnt.h>
#include <CalcState.h>

#define ENC_A 4
#define ENC_B 5
#define ENC_BUTT 15

int32_t encoder_value = 0; // software counter
int32_t first, second, result;
CalcState state = CalcState_Ready;
char operation;
const char operations[4] = {'+', '-', '*', '/'};
volatile bool btn_pressed = false;

static const uint64_t debounce_delay_us = 300000; // 

volatile bool buttonPressed = false;
volatile uint64_t lastInterruptTime = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;


static void IRAM_ATTR button_pressed_handler(void *arg){
    uint64_t currentTime = esp_timer_get_time();

    portENTER_CRITICAL_ISR(&mux);
    if (currentTime - lastInterruptTime > debounce_delay_us){
        lastInterruptTime = currentTime;
        btn_pressed = true;
    }
    portEXIT_CRITICAL_ISR(&mux);
}

void app_main(void)
{
    // GPIO pullups
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << ENC_A) | (1ULL << ENC_B),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io);

    gpio_config_t btn_conf = {
        .pin_bit_mask = 1ULL << ENC_BUTT,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&btn_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(ENC_BUTT, button_pressed_handler, NULL);

    // PCNT unit
    pcnt_unit_handle_t unit;
    pcnt_unit_config_t unit_cfg = {
        .high_limit = 100,
        .low_limit = -100,
    };
    pcnt_new_unit(&unit_cfg, &unit);

    // PCNT channel
    pcnt_channel_handle_t chan;
    pcnt_chan_config_t chan_cfg = {
        .edge_gpio_num = ENC_A,  // pulse
        .level_gpio_num = ENC_B, // direction
    };
    pcnt_new_channel(unit, &chan_cfg, &chan);

    // Count on rising edge only
    pcnt_channel_set_edge_action(
        chan,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE
    );

    // Direction by B level
    pcnt_channel_set_level_action(
        chan,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE
    );

    pcnt_unit_enable(unit);
    pcnt_unit_clear_count(unit);
    pcnt_unit_start(unit);

    printf("Start turning encoder\n");

    while (1) {
        int count = 0;
        pcnt_unit_get_count(unit, &count);
        
        if (count != 0) {
            encoder_value += count;            // accumulate deltas
            if (state != CalcState_GotFirst){
                if (state == CalcState_Ready)
                    printf("First operand: %ld\n", encoder_value);
                else
                    printf("Second operand: %ld\n", encoder_value);
            }
            else{
                uint8_t index =  ((encoder_value % 4) + 4) % 4;
                operation = operations[index];
                printf("Current operation: %c\n", operation);
            }
            pcnt_unit_clear_count(unit);
        }

        if (btn_pressed){
            btn_pressed = false;
            UpdateState();
            if (state == CalcState_GotResult){
                printf("Result: %ld %c %ld = %ld\n", first, operation, second, result);
                UpdateState();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void reinit(){
    first = 0, second = 0, result = 0, operation = ' ';
};



void UpdateState(){
    switch (state)
    {
    case CalcState_Ready :
        first = encoder_value;
        state = CalcState_GotFirst;
        encoder_value = 0;
        break;    
    case CalcState_GotFirst:
        state = CalcState_GotOperation;
        break;
    case CalcState_GotOperation:
        second = encoder_value;
        state = CalcState_GotResult;
        if (operation == '-') result = first - second;
        else if (operation == '+') result = first + second;
        else if (operation == '*') result = first * second;
        else result = first / second;
        break;
    case CalcState_GotResult:
        reinit();
        state = CalcState_Ready;
    default:
        break;
    }
}