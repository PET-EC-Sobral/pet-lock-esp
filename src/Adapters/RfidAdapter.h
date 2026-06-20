#pragma once
#include <SPI.h>
#include <MFRC522.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "../Core/Interfaces/IRfid.h"

namespace Adapters {

class RfidAdapter : public Domain::IRfid {
private:
    MFRC522 _mfrc;
    SemaphoreHandle_t _spiMutex;

public:
    RfidAdapter(int csPin, int rstPin, SemaphoreHandle_t spiMutex);
    ~RfidAdapter() override = default;

    void init() override;
    bool readCard(uint8_t *uidOut) override;
};

} // namespace Adapters
