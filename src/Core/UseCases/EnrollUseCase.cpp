#include "EnrollUseCase.h"
#include <Arduino.h>

namespace Domain {

EnrollUseCase::EnrollUseCase(
    IDisplay &display,
    IFingerprint &fingerprint,
    IRfid &rfid,
    IStorage &storage,
    INetwork &network,
    IBluetooth &bluetooth,
    ISound &sound
) : _display(display), _fingerprint(fingerprint), _rfid(rfid),
    _storage(storage), _network(network), _bluetooth(bluetooth),
    _sound(sound) {}

bool EnrollUseCase::enrollFingerprint(uint8_t userId, uint8_t fingerId, const String& userName) {
    Serial.printf("[Enroll] Iniciando cadastro de digital para o usuario %s (ID %d)\n", userName.c_str(), userId);
    
    int8_t slotId = _fingerprint.getFirstFreeSlot();
    if (slotId == -1) {
        _display.showEnrollFailed("Sem espaco no sensor");
        _bluetooth.sendEnrollStatus("failed", "no_free_slots");
        _sound.playDenied();
        return false;
    }

    _fingerprint.startEnroll(slotId);
    _fingerprint.setLed(FingerprintLedMode::ON, FingerprintColor::CYAN, 0); // Led aceso em Ciano
    
    // Passo 1
    _display.showEnrollFingerInstructions(1, fingerId, userName);
    _bluetooth.sendEnrollStatus("place_finger_1");
    
    if (!_fingerprint.captureStep(1)) {
        _display.showEnrollFailed("Captura 1 falhou");
        _bluetooth.sendEnrollStatus("failed", "capture_1_failed");
        _sound.playDenied();
        return false;
    }
    
    _sound.playCardDefined(); // Beep curto de confirmacao do passo 1
    
    // Passo 2
    _display.showEnrollFingerInstructions(2, fingerId, userName);
    _bluetooth.sendEnrollStatus("place_finger_2");
    
    if (!_fingerprint.captureStep(2)) {
        _display.showEnrollFailed("Captura 2 falhou");
        _bluetooth.sendEnrollStatus("failed", "capture_2_failed");
        _sound.playDenied();
        return false;
    }
    
    _sound.playCardDefined();
    
    // Salva o Modelo no Banco de Dados Físico do Sensor
    if (!_fingerprint.saveModel(slotId)) {
        _display.showEnrollFailed("Falha ao salvar");
        _bluetooth.sendEnrollStatus("failed", "save_failed");
        _sound.playDenied();
        return false;
    }

    // Sucesso! Grava localmente o mapeamento e o nome do usuário
    _storage.saveFingerprintMapping(slotId, fingerId, userId);
    _storage.saveUser(userId, userName);
    
    // Tenta atualizar no Supabase (se falhar, o SyncUseCase corrigirá depois)
    bool pushed = false;
    if (_network.isConnected()) {
        pushed = _network.pushFingerprintMapping(slotId, fingerId, userId);
    }
    
    _fingerprint.setLed(FingerprintLedMode::GRADUALLY_CLOSE, FingerprintColor::GREEN, 1);
    _sound.playAllowed();
    _display.showEnrollSuccess();
    
    _bluetooth.sendEnrollStatus("success", "fingerprint_index:" + String(slotId));
    
    Serial.printf("[Enroll] Digital cadastrada no slot %d. Sincronizado Supabase: %s\n", slotId, pushed ? "Sim" : "Nao (Pendente)");
    return true;
}

bool EnrollUseCase::enrollRfid(uint8_t userId, const String& userName) {
    Serial.printf("[Enroll] Iniciando cadastro de RFID para o usuario %s (ID %d)\n", userName.c_str(), userId);
    
    _display.showEnrollRfidInstructions(userName);
    _bluetooth.sendEnrollStatus("wait_card");
    _fingerprint.setLed(FingerprintLedMode::ON, FingerprintColor::CYAN, 0); // Led indicador azul/ciano aceso
    
    uint8_t uid[4];
    bool cardDetected = false;
    unsigned long start = millis();
    
    // Polling do leitor RFID por 15 segundos
    while (millis() - start < 15000) {
        if (_rfid.readCard(uid)) {
            cardDetected = true;
            break;
        }
        delay(100);
    }
    
    if (!cardDetected) {
        _display.showEnrollFailed("Tempo limite esgotado");
        _bluetooth.sendEnrollStatus("failed", "timeout");
        _sound.playDenied();
        return false;
    }

    uint32_t rfidCode = (static_cast<uint32_t>(uid[0]) << 24) |
                        (static_cast<uint32_t>(uid[1]) << 16) |
                        (static_cast<uint32_t>(uid[2]) << 8)  |
                        uid[3];
                        
    // Verifica duplicidade local
    if (_storage.getRfidUser(rfidCode) != 0) {
        _display.showEnrollFailed("Cartao ja cadastrado");
        _bluetooth.sendEnrollStatus("failed", "already_exists");
        _sound.playDenied();
        return false;
    }
    
    // Sucesso! Grava localmente
    _storage.saveRfidMapping(rfidCode, userId);
    _storage.saveUser(userId, userName);
    
    // Tenta atualizar no Supabase
    bool pushed = false;
    if (_network.isConnected()) {
        pushed = _network.pushRfidMapping(rfidCode, userId);
    }
    
    _fingerprint.setLed(FingerprintLedMode::GRADUALLY_CLOSE, FingerprintColor::GREEN, 1);
    _sound.playAllowed();
    _display.showEnrollSuccess();
    
    _bluetooth.sendEnrollStatus("success", "rfid_code:" + String(rfidCode));
    
    Serial.printf("[Enroll] Cartao %u cadastrado. Sincronizado Supabase: %s\n", rfidCode, pushed ? "Sim" : "Nao");
    return true;
}

} // namespace Domain
