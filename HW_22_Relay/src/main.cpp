#include <Arduino.h>

constexpr uint8_t RELAY_IN_PIN        = 17;
constexpr uint8_t RELAY_OUT_PIN       = 18;    
constexpr uint16_t SIGNAL_DELAY_MS    = 2000;

uint64_t prev;
bool relay_off;

void IRAM_ATTR onRelayInterrupt(){
  printf("RELAY ON!");
}

void setup() {
  Serial0.begin(115200);
  pinMode(RELAY_IN_PIN, OUTPUT);
  pinMode(RELAY_OUT_PIN, INPUT_PULLDOWN);
  //attachInterrupt(digitalPinToInterrupt(RELAY_OUT_PIN), onRelayInterrupt, RISING);
  prev = millis();
  relay_off = true;
  digitalWrite(RELAY_IN_PIN, relay_off);
}

void loop() {
  
  if(millis() - prev >= SIGNAL_DELAY_MS){
    relay_off = !relay_off;
    digitalWrite(RELAY_IN_PIN, relay_off);
    printf("relay_off: %d\n", relay_off);
    if (digitalRead(RELAY_OUT_PIN) == HIGH){
      printf("RELAY ON!");
    }
    else 
      printf("RELAY OFF!");  
    prev = millis();
  }
}