// #include <Arduino.h>

// #define  BTN_PIN                       4

// volatile uint32_t  count           =   0;
// volatile bool      button_pressed  =   false;

// void IRAM_ATTR onButtonInterrupt(){
//   count++;
//   button_pressed  = true;
// }

// void setup() {
//   Serial0.begin(115200);
//   pinMode(BTN_PIN, INPUT);
//   attachInterrupt(digitalPinToInterrupt(BTN_PIN), onButtonInterrupt, FALLING);
// }

// void loop() {
//   if(button_pressed){
//     printf("Count=%d\n", count);
//     button_pressed = false;
//   }
// }
