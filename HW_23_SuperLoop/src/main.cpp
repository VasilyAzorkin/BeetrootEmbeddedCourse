#include <Arduino.h>

constexpr uint8_t   YELLOW_PIN    = 4;
constexpr uint8_t   GREEN_PIN     = 5;
constexpr uint8_t   RED_PIN       = 6;

constexpr uint16_t  YELLOW_DELAY  = 200;
constexpr uint16_t  GREEN_DELAY   = 500;
constexpr uint16_t  RED_DELAY     = 1000;

uint64_t yellow_prev              = 0;
uint64_t green_prev               = 0;
uint64_t red_prev                 = 0;

bool yellow_on                    = true;
bool green_on                     = true;
bool red_on                       = true;

void toggleIfNeeded(uint8_t pin, uint16_t delay, uint64_t* prev, bool* on);

void setup() {
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
}

void loop() {
  toggleIfNeeded(YELLOW_PIN, YELLOW_DELAY, &yellow_prev, &yellow_on);
  toggleIfNeeded(GREEN_PIN, GREEN_DELAY, &green_prev, &green_on);
  toggleIfNeeded(RED_PIN, RED_DELAY, &red_prev, &red_on);
}

void toggleIfNeeded(uint8_t pin, uint16_t delay, uint64_t* prev, bool* on){
  if (millis() - *prev >= delay){
    *on = !*on;
    digitalWrite(pin, *on);
    *prev = millis();
  }
}