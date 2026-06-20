#pragma once
#include <Arduino.h>

namespace Domain {

class IBluetooth {
public:
    virtual ~IBluetooth() = default;
    virtual void init() = 0;
    virtual bool isConnected() = 0;
    virtual bool hasNewCommand() = 0;
    virtual String getNextCommand() = 0;
    virtual void sendEnrollStatus(const String& status, const String& details = "") = 0;
    virtual void sendSystemStatus(bool wifiConnected, uint8_t templatesCount) = 0;
};

} // namespace Domain
