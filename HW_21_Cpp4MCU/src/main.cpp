#include <Arduino.h>
#include <LedBlinker.h>

constexpr uint8_t RED_LED = 10;
constexpr uint16_t BLINK_DELAY = 1000;
Led led;

void setup(){
  led.init(RED_LED, BLINK_DELAY);  
}

void loop(){
  LedState state = led.changeState();
  led.set(state);
}