#include "LockAdapter.h"
#include <Arduino.h>

namespace Adapters {

LockAdapter::LockAdapter(int pin) : _pin(pin), _isUnlocked(false) {}

void LockAdapter::init() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, HIGH); // Bloqueado (Ativo Baixo para liberar)
    _isUnlocked = false;
}

void LockAdapter::unlock() {
    digitalWrite(_pin, LOW); // Destrava
    _isUnlocked = true;
}

void LockAdapter::lock() {
    digitalWrite(_pin, HIGH); // Trava
    _isUnlocked = false;
}

bool LockAdapter::isUnlocked() {
    return _isUnlocked;
}

} // namespace Adapters
