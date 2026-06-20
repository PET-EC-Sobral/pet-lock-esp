#pragma once
#include <Arduino.h>

namespace Domain {

class User {
private:
    uint8_t _id;
    String _name;

public:
    User() : _id(0), _name("") {}
    User(uint8_t id, const String& name) : _id(id), _name(name) {}

    uint8_t getId() const { return _id; }
    const String& getName() const { return _name; }
};

} // namespace Domain
