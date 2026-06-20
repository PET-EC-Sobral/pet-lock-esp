#pragma once
#include <Arduino.h>

namespace Domain {

class IDisplay {
public:
    virtual ~IDisplay() = default;
    virtual void init() = 0;
    virtual void showSplash() = 0;
    virtual void showWaitingAccess() = 0;
    virtual void showVerifying() = 0;
    virtual void showAllowed(const String& userName) = 0;
    virtual void showDenied() = 0;
    virtual void showEnrollFingerInstructions(uint8_t step, uint8_t fingerId, const String& userName) = 0;
    virtual void showEnrollRfidInstructions(const String& userName) = 0;
    virtual void showEnrollSuccess() = 0;
    virtual void showEnrollFailed(const String& reason) = 0;
    virtual void showMessage(const String& line1, const String& line2) = 0;

    // Status indicators updated dynamically
    virtual void setWifiStatus(bool connected) = 0;
    virtual void setBleStatus(bool connected) = 0;
    virtual void setSyncStatus(bool pending) = 0;
};

} // namespace Domain
