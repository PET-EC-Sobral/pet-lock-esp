#include "FingerprintAdapter.h"
#include "SystemConfig.h"
#include <Arduino.h>

namespace Adapters {

FingerprintAdapter::FingerprintAdapter(HardwareSerial *serial, uint32_t password)
    : _serial(serial), _finger(serial, password), _password(password) {}

bool FingerprintAdapter::init() {
    // Configura a UART2 para os pinos definidos no SystemConfig
    _serial->begin(57600, SERIAL_8N1, ZW_RX2, ZW_TX2);
    delay(100);
    _finger.begin(57600);
    
    if (!_finger.verifyPassword()) {
        Serial.println("[ZW111] Nao foi possivel conectar ao sensor.");
        return false;
    }
    Serial.println("[ZW111] Sensor verificado com sucesso!");
    return true;
}

int16_t FingerprintAdapter::scan() {
    flushRx();
    uint8_t p = _finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
        return 0; // Sem dedo
    }
    if (p != FINGERPRINT_OK) {
        return -2; // Erro de comunicação/imagem
    }

    flushRx();
    p = _finger.image2Tz();
    if (p != FINGERPRINT_OK) {
        return -2;
    }

    flushRx();
    p = _finger.fingerSearch();
    if (p == FINGERPRINT_OK) {
        return _finger.fingerID; // Retorna o ID correspondente
    } else if (p == FINGERPRINT_NOTFOUND) {
        return -1; // Dedo não cadastrado
    }
    
    return -2;
}

bool FingerprintAdapter::startEnroll(uint8_t slotId) {
    // Para iniciar o cadastro, garante que o dedo foi removido primeiro
    waitFingerReleased();
    return true;
}

bool FingerprintAdapter::captureStep(uint8_t step) {
    int p = -1;
    unsigned long start = millis();
    Serial.printf("[Fingerprint] captureStep(%d) iniciado. Aguardando dedo...\n", step);
    
    // Aguarda o dedo por até 10 segundos
    while (p != FINGERPRINT_OK) {
        flushRx();
        p = _finger.getImage();
        if (p != FINGERPRINT_NOFINGER && p != -1) {
            Serial.printf("[Fingerprint] getImage() retornou: 0x%02X\n", p);
        }
        if (p == FINGERPRINT_OK) {
            Serial.println("[Fingerprint] Dedo detectado!");
        }
        if (millis() - start > 10000) {
            Serial.printf("[Fingerprint] Timeout na captura do passo %d. Ultimo p: 0x%02X\n", step, p);
            return false; // Timeout
        }
        delay(100);
    }

    flushRx();
    p = _finger.image2Tz(step);
    Serial.printf("[Fingerprint] image2Tz(%d) retornou: 0x%02X\n", step, p);
    if (p != FINGERPRINT_OK) {
        return false;
    }
    
    // Se for o passo 1, aguarda a liberação do dedo para o passo 2
    if (step == 1) {
        Serial.println("[Fingerprint] Aguardando liberar o dedo...");
        waitFingerReleased();
        Serial.println("[Fingerprint] Dedo liberado.");
    }
    
    return true;
}

bool FingerprintAdapter::saveModel(uint8_t slotId) {
    flushRx();
    int p = _finger.createModel();
    if (p != FINGERPRINT_OK) {
        return false;
    }

    flushRx();
    p = _finger.storeModel(slotId);
    return (p == FINGERPRINT_OK);
}

bool FingerprintAdapter::deleteModel(uint8_t slotId) {
    flushRx();
    return (_finger.deleteModel(slotId) == FINGERPRINT_OK);
}

bool FingerprintAdapter::clearDatabase() {
    flushRx();
    return (_finger.emptyDatabase() == FINGERPRINT_OK);
}

void FingerprintAdapter::setLed(Domain::FingerprintLedMode mode, Domain::FingerprintColor color, uint8_t cycles) {
    uint8_t m = static_cast<uint8_t>(mode);
    uint8_t c = 4; // Default Red
    
    switch (color) {
        case Domain::FingerprintColor::RED:
            c = 4; // Vermelho físico
            break;
        case Domain::FingerprintColor::GREEN:
            c = 2; // Verde físico
            break;
        case Domain::FingerprintColor::BLUE:
            c = 1; // Azul físico
            break;
        case Domain::FingerprintColor::CYAN:
            c = 3; // Ciano físico
            break;
        case Domain::FingerprintColor::YELLOW:
            c = 6; // Amarelo físico
            break;
        case Domain::FingerprintColor::WHITE:
            c = 7; // Branco físico
            break;
    }

    sendLedCommand(m, c, c, cycles);
}

uint8_t FingerprintAdapter::getCount() {
    flushRx();
    _finger.getTemplateCount();
    return _finger.templateCount;
}

int8_t FingerprintAdapter::getFirstFreeSlot() {
    for (uint8_t i = 1; i <= 40; i++) {
        flushRx();
        if (_finger.loadModel(i) != FINGERPRINT_OK) {
            return i; // Slot livre encontrado!
        }
    }
    return -1; // Sem slots livres
}

void FingerprintAdapter::waitFingerReleased() {
    int p = -1;
    Serial.println("[Fingerprint] waitFingerReleased() iniciada...");
    while (p != FINGERPRINT_NOFINGER) {
        flushRx();
        p = _finger.getImage();
        if (p != FINGERPRINT_NOFINGER) {
            Serial.printf("[Fingerprint] waitFingerReleased: getImage() = 0x%02X\n", p);
        }
        delay(100);
    }
    Serial.println("[Fingerprint] waitFingerReleased() concluida.");
}

void FingerprintAdapter::sendLedCommand(uint8_t mode, uint8_t startColor, uint8_t endColor, uint8_t cycles) {
    // Pacote de controle do LED do ZW111
    uint8_t packet[] = {
        0xEF, 0x01,             // Cabeçalho
        0xFF, 0xFF, 0xFF, 0xFF, // Endereço Padrão
        0x01,                   // ID do Pacote (Comando)
        0x00, 0x07,             // Tamanho
        0x3C,                   // Instrução: Control LED
        mode,
        startColor,
        endColor,
        cycles
    };

    // Soma do Checksum: ID (0x01) + Length (0x00 + 0x07) + Data (0x3C + mode + start + end + cycles)
    uint16_t sum = 0x01 + 0x00 + 0x07 + 0x3C + mode + startColor + endColor + cycles;

    flushRx();
    _serial->write(packet, sizeof(packet));
    _serial->write(static_cast<uint8_t>(sum >> 8));   // Checksum High byte
    _serial->write(static_cast<uint8_t>(sum & 0xFF)); // Checksum Low byte
    
    delay(50); // Tempo para processamento do leitor
    while (_serial->available()) {
        _serial->read(); // Esvazia o buffer de retorno do leitor
    }
}

void FingerprintAdapter::flushRx() {
    while (_serial->available()) {
        _serial->read();
    }
}

} // namespace Adapters
