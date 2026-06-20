#pragma once
#include <stdint.h>

namespace Domain {

enum class FingerprintColor : uint8_t {
    RED = 4,       // Na biblioteca de teste, gradual-close RED é 4, verde é 2?
    BLUE = 1,      // Ajustaremos na implementação do Adapter com base na especificação
    GREEN = 2,     // 2 = green no teste, 4 = red no gradual-close, no flashlight green é 4, red é 2.
    YELLOW = 6,    // Vamos mapear os códigos reais no Adapter.
    CYAN = 3,
    WHITE = 7
};

enum class FingerprintLedMode : uint8_t {
    BREATHING = 1,
    FLASHING = 2,
    ON = 3,
    OFF = 4,
    GRADUALLY_CLOSE = 6
};

class IFingerprint {
public:
    virtual ~IFingerprint() = default;
    virtual bool init() = 0;
    virtual int16_t scan() = 0; // Retorna index (1-40) se correspondência encontrada, 0 se sem dedo, -1 se não cadastrada, -2 se erro
    virtual bool startEnroll(uint8_t slotId) = 0;
    virtual bool captureStep(uint8_t step) = 0; // Passo 1 e Passo 2
    virtual bool saveModel(uint8_t slotId) = 0;
    virtual bool deleteModel(uint8_t slotId) = 0;
    virtual bool clearDatabase() = 0;
    virtual void setLed(FingerprintLedMode mode, FingerprintColor color, uint8_t cycles) = 0;
    virtual uint8_t getCount() = 0;
    virtual int8_t getFirstFreeSlot() = 0;
    virtual void waitFingerReleased() = 0;
};

} // namespace Domain
