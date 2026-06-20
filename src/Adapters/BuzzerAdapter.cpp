#include "BuzzerAdapter.h"
#include <Arduino.h>

namespace Adapters {

BuzzerAdapter::BuzzerAdapter(int pin) : _pin(pin) {}

void BuzzerAdapter::init() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
}

void BuzzerAdapter::playAllowed() {
    digitalWrite(_pin, HIGH);
    delay(100);
    digitalWrite(_pin, LOW);
    delay(50);
    digitalWrite(_pin, HIGH);
    delay(100);
    digitalWrite(_pin, LOW);
}

void BuzzerAdapter::playDenied() {
    digitalWrite(_pin, HIGH);
    delay(500);
    digitalWrite(_pin, LOW);
}

void BuzzerAdapter::playMasterModeEnter() {
    digitalWrite(_pin, HIGH);
    delay(100);
    digitalWrite(_pin, LOW);
    delay(50);
    digitalWrite(_pin, HIGH);
    delay(100);
    digitalWrite(_pin, LOW);
    delay(50);
    digitalWrite(_pin, HIGH);
    delay(100);
    digitalWrite(_pin, LOW);
}

void BuzzerAdapter::playMasterModeExit() {
    digitalWrite(_pin, HIGH);
    delay(500);
    digitalWrite(_pin, LOW);
    delay(50);
    digitalWrite(_pin, HIGH);
    delay(500);
    digitalWrite(_pin, LOW);
}

void BuzzerAdapter::playCardRemoved() {
    digitalWrite(_pin, HIGH);
    delay(100);
    digitalWrite(_pin, LOW);
    delay(50);
    digitalWrite(_pin, HIGH);
    delay(100);
    digitalWrite(_pin, LOW);
    delay(50);
    digitalWrite(_pin, HIGH);
    delay(500);
    digitalWrite(_pin, LOW);
}

void BuzzerAdapter::playCardDefined() {
    digitalWrite(_pin, HIGH);
    delay(100);
    digitalWrite(_pin, LOW);
}

void BuzzerAdapter::playBoot() {
    digitalWrite(_pin, HIGH);
    delay(150);
    digitalWrite(_pin, LOW);
    delay(50);
    digitalWrite(_pin, HIGH);
    delay(150);
    digitalWrite(_pin, LOW);
}

} // namespace Adapters
