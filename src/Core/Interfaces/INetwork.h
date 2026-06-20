#pragma once
#include <Arduino.h>
#include "../Entities/AccessLog.h"
#include "IStorage.h"

namespace Domain {

class INetwork {
public:
    virtual ~INetwork() = default;
    virtual void init() = 0;
    virtual bool isConnected() = 0;
    virtual bool syncTime() = 0;
    virtual bool postAccessLog(const AccessLog& log) = 0;
    virtual bool fetchDatabaseMappings(IStorage& storage) = 0;
    virtual bool pushFingerprintMapping(uint8_t index, uint8_t fingerId, uint8_t userId) = 0;
    virtual bool pushRfidMapping(uint32_t rfidCode, uint8_t userId) = 0;
};

} // namespace Domain
