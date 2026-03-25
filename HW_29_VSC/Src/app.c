#include "app.h"
#include "main.h"
#include "stm32f4xx_hal_gpio.h"


void setup(){
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}

void loop(){
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    HAL_Delay(500);
}