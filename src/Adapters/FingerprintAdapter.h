#pragma once
#include <Adafruit_Fingerprint.h>
#include "../Core/Interfaces/IFingerprint.h"

namespace Adapters {

class FingerprintAdapter : public Domain::IFingerprint {
private:
    HardwareSerial *_serial;
    Adafruit_Fingerprint _finger;
    uint32_t _password;

    void sendLedCommand(uint8_t mode, uint8_t startColor, uint8_t endColor, uint8_t cycles);
    void flushRx();

public:
    FingerprintAdapter(HardwareSerial *serial, uint32_t password = 0);
    ~FingerprintAdapter() override = default;

    bool init() override;
    int16_t scan() override;
    bool startEnroll(uint8_t slotId) override;
    bool captureStep(uint8_t step) override;
    bool saveModel(uint8_t slotId) override;
    bool deleteModel(uint8_t slotId) override;
    bool clearDatabase() override;
    void setLed(Domain::FingerprintLedMode mode, Domain::FingerprintColor color, uint8_t cycles) override;
    uint8_t getCount() override;
    int8_t getFirstFreeSlot() override;
    void waitFingerReleased() override;
};

} // namespace Adapters
