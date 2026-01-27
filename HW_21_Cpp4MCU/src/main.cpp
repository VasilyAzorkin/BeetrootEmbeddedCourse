#include <Arduino.h>
#include <LedBlinker.h>

constexpr uint8_t RED_LED = 10;
constexpr uint16_t BLINK_DELAY = 1000;
constexpr uint32_t LOG_BY_COUNT = 100000;

Led led;
Stopwatch sw;

void setup(){
  Serial0.begin(115200);
  led.init(RED_LED, BLINK_DELAY);  
}

void loop(){
  if (sw.getCount() % LOG_BY_COUNT == 0){
    Serial0.printf("Avg loop execution:%f\tCount:%d\n", sw.getAverage(), sw.getCount());
  }
  sw.count();
  LedState state = led.changeState();
  led.set(state);
}