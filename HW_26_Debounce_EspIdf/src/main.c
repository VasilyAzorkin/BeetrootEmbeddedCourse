#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "common.h"

volatile uint32_t   count           =   0;
volatile bool       button_pressed  =   false;

uint64_t            prev            =   0;

btn_state           btnState        =   RELEASED;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void exercise1();
void exercise2();
void exercise3();
void exercise4();

void IRAM_ATTR button_isr_handler_ex1(void *arg){
    portENTER_CRITICAL_ISR(&mux);
    count++;
    portEXIT_CRITICAL_ISR(&mux);
    button_pressed = true;
}

void IRAM_ATTR button_isr_handler_ex2_ex3(void *arg){
    button_pressed = true;
}

void app_main() {
    // exercise1();
    // exercise2();
    // exercise3();
    exercise4();        
}


void exercise1(){
    init_gpio_input_isr(BTN_PIN, button_isr_handler_ex1);
    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_LOGI(EX1_TAG, "Button and ISR initialized");
    while (true)
    {
        if(button_pressed){
            button_pressed = false;
            ESP_LOGI(EX1_TAG, "Count=%d", count);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void exercise2(){
    init_gpio_input_isr(BTN_PIN, button_isr_handler_ex2_ex3);
    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_LOGI(EX2_TAG, "Button and ISR initialized");
    
    uint64_t currentTime = 0;
    while (true)
    {
        currentTime = esp_timer_get_time();
        if(button_pressed && (currentTime - prev) > DEBOUNCE_DELAY ){
            button_pressed = false;
            prev = esp_timer_get_time();
            ESP_LOGI(EX2_TAG, "Count=%d", ++count);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void exercise3(){
    init_gpio_input_isr(BTN_PIN, button_isr_handler_ex2_ex3);
    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_LOGI(EX3_TAG, "Button and ISR initialized");
    
    while (true)
    {
        if(button_pressed && gpio_get_level(BTN_PIN) == 0){
            button_pressed = false;
            ESP_LOGI(EX3_TAG, "Count=%d", ++count);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void exercise4(){
    init_gpio_input(BTN_PIN);

    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_LOGI(EX4_TAG, "Button initialized");

    uint64_t currentTime = 0;
    while (true){
        currentTime = esp_timer_get_time();
        if (currentTime - prev > READ_DELAY){
            if (gpio_get_level(BTN_PIN) == 0){
                btnState = PRESSED;
            }
            else if (gpio_get_level(BTN_PIN) == 1 && btnState == PRESSED){
                btnState = RELEASED;
                ESP_LOGI(EX4_TAG, "Count=%d", ++count);
            }
            else
                btnState = RELEASED;
            
            prev = esp_timer_get_time();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}