#include <Arduino.h>

constexpr uint8_t BTN_PIN = 4;
constexpr uint8_t READ_DELAY = 10;

uint32_t count = 0;
uint64_t prev = 0;

enum class BtnState { PRESSED, RELEASED };
BtnState btnState = BtnState::RELEASED;

void setup()
{
  Serial0.begin(115200);
  pinMode(BTN_PIN, INPUT);
}

void loop()
{  
    if (millis() - prev > READ_DELAY){
      if (digitalRead(BTN_PIN) == LOW){
        btnState = BtnState::PRESSED;
      }
      else if (digitalRead(BTN_PIN) == HIGH && btnState == BtnState::PRESSED){
        count++;
        printf("Count=%d\n", count);
        btnState = BtnState::RELEASED;  
      }
      else
        btnState = BtnState::RELEASED;
      prev = millis();
    }        
}
