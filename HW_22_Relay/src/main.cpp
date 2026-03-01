#include <Arduino.h>

constexpr uint8_t RELAY_IN_PIN        = 17;
constexpr uint8_t RELAY_OUT_PIN       = 18;    
constexpr uint16_t SIGNAL_DELAY_MS    = 1000;
constexpr uint8_t DEBOUNCING_DELAY    = 10;

uint64_t debounce_last = 0;
uint64_t prev = 0;
uint16_t count = 0;
volatile uint32_t total_delay = 0;
volatile uint16_t curr_delay;
volatile bool fired = false;

void IRAM_ATTR onRelayInterrupt(){
  
  if (millis() - debounce_last > DEBOUNCING_DELAY){
    debounce_last = millis();
    curr_delay = millis() - prev;
    fired = true;
  }
}

void setup() {
  Serial0.begin(115200);
  pinMode(RELAY_IN_PIN, OUTPUT);
  pinMode(RELAY_OUT_PIN, PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(RELAY_OUT_PIN), onRelayInterrupt, CHANGE);
}

void loop() {
  if (fired){
    fired = false;
    total_delay += curr_delay;
    count++;
    printf("current:%d\ttotal:%d\tcount:%d\tavg:%d\n", curr_delay, total_delay, count, total_delay/count);
  }

  if(millis() - prev >= SIGNAL_DELAY_MS){
    const int relay_state = digitalRead(RELAY_OUT_PIN);
    prev = millis();
    digitalWrite(RELAY_IN_PIN, relay_state);
    logRelayState();  
  }
}

void logRelayState()
{
  String relay_state_text;
  if (digitalRead(RELAY_OUT_PIN) == HIGH)
  {
    relay_state_text = "OFF";
  }
  else
  {
    relay_state_text = "ON";
  }
  printf("\nRelay state: %s\n\n", relay_state_text);
}
