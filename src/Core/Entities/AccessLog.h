#pragma once
#include <Arduino.h>

namespace Domain {

enum class AccessMethod : uint8_t {
    RFID = 0,
    FINGERPRINT = 1
};

class AccessLog {
private:
    String _timestamp;
    uint8_t _userId;
    AccessMethod _method;
    bool _authorized;
    String _rfidUid;
    int _fingerprintIndex;

public:
    AccessLog() 
        : _timestamp(""), _userId(0), _method(AccessMethod::RFID), 
          _authorized(false), _rfidUid(""), _fingerprintIndex(-1) {}

    AccessLog(const String& timestamp, uint8_t userId, AccessMethod method, 
              bool authorized, const String& rfidUid = "", int fingerprintIndex = -1)
        : _timestamp(timestamp), _userId(userId), _method(method), 
          _authorized(authorized), _rfidUid(rfidUid), _fingerprintIndex(fingerprintIndex) {}

    const String& getTimestamp() const { return _timestamp; }
    uint8_t getUserId() const { return _userId; }
    AccessMethod getMethod() const { return _method; }
    uint8_t getMethodVal() const { return static_cast<uint8_t>(_method); }
    bool isAuthorized() const { return _authorized; }
    const String& getRfidUid() const { return _rfidUid; }
    int getFingerprintIndex() const { return _fingerprintIndex; }
};

} // namespace Domain
