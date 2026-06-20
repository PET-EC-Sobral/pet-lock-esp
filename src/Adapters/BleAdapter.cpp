#include "BleAdapter.h"
#include "../include/SystemConfig.h"
#include <Arduino.h>

namespace Adapters {

BleAdapter::BleAdapter()
    : _pServer(nullptr), _pTxCharacteristic(nullptr),
      _deviceConnected(false), _oldDeviceConnected(false), _rxBuffer("") {
    _queueMutex = xSemaphoreCreateMutex();
}

BleAdapter::~BleAdapter() {
    vSemaphoreDelete(_queueMutex);
}

void BleAdapter::init() {
    NimBLEDevice::init(BLE_DEVICE_NAME);

    // Cria o servidor BLE e define callbacks
    _pServer = NimBLEDevice::createServer();
    _pServer->setCallbacks(this);

    // Cria o Serviço UART
    NimBLEService *pService = _pServer->createService(BLE_SERVICE_UUID);

    // Característica TX (Notificações para o App)
    _pTxCharacteristic = pService->createCharacteristic(
        BLE_CHARACTERISTIC_UUID_TX,
        NIMBLE_PROPERTY::NOTIFY
    );

    // Característica RX (Recebe comandos do App)
    NimBLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        BLE_CHARACTERISTIC_UUID_RX,
        NIMBLE_PROPERTY::WRITE
    );
    pRxCharacteristic->setCallbacks(this);

    pService->start();

    // Inicia o Advertising
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setName(BLE_DEVICE_NAME);
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // Ajuda com problemas de conexão em iOS
    pAdvertising->setMinPreferred(0x12);
    pAdvertising->start();

    Serial.println("[BLE] Servidor inicializado e escutando!");
}

bool BleAdapter::isConnected() {
    return _deviceConnected;
}

bool BleAdapter::hasNewCommand() {
    bool empty = true;
    if (xSemaphoreTake(_queueMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        empty = _cmdQueue.empty();
        xSemaphoreGive(_queueMutex);
    }
    return !empty;
}

String BleAdapter::getNextCommand() {
    String cmd = "";
    if (xSemaphoreTake(_queueMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (!_cmdQueue.empty()) {
            cmd = _cmdQueue.front();
            _cmdQueue.pop();
        }
        xSemaphoreGive(_queueMutex);
    }
    return cmd;
}

void BleAdapter::sendEnrollStatus(const String& status, const String& details) {
    if (!_deviceConnected) return;

    StaticJsonDocument<256> doc;
    doc["event"] = "enroll_status";
    doc["status"] = status;
    if (details.length() > 0) {
        doc["details"] = details;
    }

    String payload;
    serializeJson(doc, payload);
    
    // Envia o payload. Como BLE possui limite de MTU por pacote (geralmente 20 bytes por padrão se não negociado),
    // enviamos em blocos caso exceda ou enviamos inteiro se o MTU permitir.
    _pTxCharacteristic->setValue(std::string(payload.c_str()));
    _pTxCharacteristic->notify();
    
    Serial.print("[BLE] Enviado Status: ");
    Serial.println(payload);
}

void BleAdapter::sendSystemStatus(bool wifiConnected, uint8_t templatesCount) {
    if (!_deviceConnected) return;

    StaticJsonDocument<256> doc;
    doc["event"] = "system_status";
    doc["wifi"] = wifiConnected ? "connected" : "disconnected";
    doc["fingerprints_count"] = templatesCount;

    String payload;
    serializeJson(doc, payload);
    
    _pTxCharacteristic->setValue(std::string(payload.c_str()));
    _pTxCharacteristic->notify();
    
    Serial.print("[BLE] Enviado Status do Sistema: ");
    Serial.println(payload);
}

void BleAdapter::onConnect(NimBLEServer *pServer) {
    _deviceConnected = true;
    Serial.println("[BLE] App Conectado!");
}

void BleAdapter::onDisconnect(NimBLEServer *pServer) {
    _deviceConnected = false;
    _rxBuffer = ""; // Limpa buffer ao desconectar
    Serial.println("[BLE] App Desconectado.");
}

void BleAdapter::onWrite(NimBLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
        String chunk = String(value.c_str());
        _rxBuffer += chunk;
        
        // Limpa lixo antes do início do JSON
        int firstBrace = _rxBuffer.indexOf('{');
        if (firstBrace > 0) {
            _rxBuffer = _rxBuffer.substring(firstBrace);
        } else if (firstBrace == -1) {
            // Se não contém '{', limpa o buffer pois é lixo residual
            _rxBuffer = "";
            return;
        }

        // Verifica se o buffer contém um JSON completo (contagem de chaves balanceadas)
        int openBraces = 0;
        int closeBraces = 0;
        bool inString = false;
        
        for (unsigned int i = 0; i < _rxBuffer.length(); i++) {
            char c = _rxBuffer[i];
            // Ignora chaves dentro de strings/aspas
            if (c == '"' && (i == 0 || _rxBuffer[i-1] != '\\')) {
                inString = !inString;
            }
            if (!inString) {
                if (c == '{') openBraces++;
                else if (c == '}') closeBraces++;
            }
        }
        
        // Se as chaves estiverem balanceadas e houver pelo menos uma aberta
        if (openBraces > 0 && openBraces == closeBraces) {
            String fullCmd = _rxBuffer;
            _rxBuffer = ""; // Limpa para a próxima mensagem
            
            Serial.print("[BLE] Comando completo recebido: ");
            Serial.println(fullCmd);
            
            if (xSemaphoreTake(_queueMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                _cmdQueue.push(fullCmd);
                xSemaphoreGive(_queueMutex);
            }
        } else {
            Serial.printf("[BLE] Fragmento acumulado (%d/%d chaves): %s\n", openBraces, closeBraces, chunk.c_str());
        }
    }
}

void BleAdapter::checkConnection() {
    if (!_deviceConnected && _oldDeviceConnected) {
        delay(500);
        NimBLEDevice::startAdvertising();
        Serial.println("[BLE] Reiniciando Advertising...");
        _oldDeviceConnected = _deviceConnected;
    }
    if (_deviceConnected && !_oldDeviceConnected) {
        _oldDeviceConnected = _deviceConnected;
    }
}

} // namespace Adapters
