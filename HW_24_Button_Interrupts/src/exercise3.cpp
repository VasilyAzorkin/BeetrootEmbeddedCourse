// #include <Arduino.h>

// constexpr uint8_t BTN_PIN = 4;
// constexpr uint8_t DEBOUNCE_DELAY = 50;

// uint32_t count = 0;
// volatile bool button_pressed = false;

// void IRAM_ATTR onButtonInterrupt()
// {
//   button_pressed = true;
// }

// void setup()
// {
//   Serial0.begin(115200);
//   pinMode(BTN_PIN, INPUT);
//   attachInterrupt(digitalPinToInterrupt(BTN_PIN), onButtonInterrupt, FALLING);
// }

// void loop()
// {  
//     if (button_pressed && digitalRead(BTN_PIN) == LOW){
//       button_pressed = false;
//       count++;
//       printf("Count=%d\n", count);
//     }    
// }
