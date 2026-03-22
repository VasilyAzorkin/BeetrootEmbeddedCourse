#include <stdio.h>

#define             BTN_PIN                 4
#define             DEBOUNCE_DELAY          50
#define             READ_DELAY              10
#define             EX1_TAG                 "Exercise 1"
#define             EX2_TAG                 "Exercise 2"
#define             EX3_TAG                 "Exercise 3"
#define             EX4_TAG                 "Exercise 4"

void    init_gpio_input(uint8_t pin);
void    init_gpio_input_isr(uint8_t pin, void* isr);

typedef enum{
    PRESSED,
    RELEASED
} btn_state;