#include <Arduino.h>

enum class LedState:uint8_t{
    On,
    Off
};

class BlinkConfiguration{
    public:
        static const uint8_t SHORT_BTN_PUSH_PERIOD = 100;
        static const uint8_t SHORT_BTN_PUSH_BLINK_COUNT = 3;
};

class Led{
    public:
        void init(uint8_t ledPin, uint16_t delay);
        void set(LedState state);
        LedState changeState();

    private:
        LedState _ledState;
        uint8_t _ledPin;
        uint64_t _nextMillis;
        uint16_t _delay;
};