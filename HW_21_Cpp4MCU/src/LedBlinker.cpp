#include "LedBlinker.h"

void Led::init(uint8_t ledPin, uint16_t delay){
    _ledState = LedState::On;
    _ledPin = ledPin;

    pinMode(ledPin, OUTPUT);
    digitalWrite(_ledPin, LOW);
    _nextMillis = millis() + delay;
    _delay = delay;
}

void Led::set(LedState state){
    if (_ledState != state){
        _ledState = state;
        if (_ledState == LedState::On)
            digitalWrite(_ledPin, LOW);
        else
            digitalWrite(_ledPin, HIGH);
    }
}

LedState Led::changeState(){
    if (millis() > _nextMillis){
        _nextMillis = millis() + _delay;
        if(_ledState == LedState::On)
            return LedState::Off;
        return LedState::On;
    }
    return _ledState;
}