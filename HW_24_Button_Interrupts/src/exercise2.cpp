// #include <Arduino.h>

// constexpr uint8_t BTN_PIN = 4;
// constexpr uint8_t DEBOUNCE_DELAY = 50;

// uint32_t count = 0;
// volatile bool button_pressed = false;
// uint32_t prev = 0;

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
//     if (button_pressed && (millis() - prev > DEBOUNCE_DELAY))
//     {
//       button_pressed = false;
//       prev = millis();
//       count++;
//       printf("Count=%d\n", count);
//     }
// }
