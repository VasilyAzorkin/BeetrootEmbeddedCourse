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

Stopwatch::Stopwatch(){
    _prev = 0;
    _total = 0;
    _count = 0;
}

void Stopwatch::count(){
    if (_count == 0){
        _count++;
        _prev = micros();
        return;
    }
    _total += micros() - _prev;
    _count++;
    _prev = micros();
}

float Stopwatch::getAverage(){
    if (_count == 0)
        return 0;
    return (float)_total / _count;
}

uint32_t Stopwatch::getCount(){
    return _count;
}