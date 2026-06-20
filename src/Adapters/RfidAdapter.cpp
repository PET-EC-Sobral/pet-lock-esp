#include "RfidAdapter.h"
#include <Arduino.h>

namespace Adapters {

RfidAdapter::RfidAdapter(int csPin, int rstPin, SemaphoreHandle_t spiMutex)
    : _mfrc(csPin, rstPin), _spiMutex(spiMutex) {}

void RfidAdapter::init() {
    if (_spiMutex != NULL) xSemaphoreTake(_spiMutex, portMAX_DELAY);
    _mfrc.PCD_Init();
    if (_spiMutex != NULL) xSemaphoreGive(_spiMutex);
}

bool RfidAdapter::readCard(uint8_t *uidOut) {
    bool found = false;
    
    if (_spiMutex != NULL) {
        // Usa timeout pequeno (15ms) para não prender a Task de Autenticação se a tela estiver atualizando
        if (xSemaphoreTake(_spiMutex, pdMS_TO_TICKS(15)) != pdTRUE) {
            return false;
        }
    }
    
    // Realiza a leitura física
    if (_mfrc.PICC_IsNewCardPresent() && _mfrc.PICC_ReadCardSerial()) {
        // Preenche com o UID de 4 bytes do cartão Mifare Classic
        for (uint8_t i = 0; i < 4; i++) {
            uidOut[i] = _mfrc.uid.uidByte[i];
        }
        _mfrc.PICC_HaltA();
        _mfrc.PCD_StopCrypto1();
        found = true;
    }
    
    if (_spiMutex != NULL) xSemaphoreGive(_spiMutex);
    
    return found;
}

} // namespace Adapters
