#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <NimBLEDevice.h>
#include <queue>
#include "../Core/Interfaces/IBluetooth.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace Adapters {

class BleAdapter : public Domain::IBluetooth, public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks {
private:
    NimBLEServer *_pServer;
    NimBLECharacteristic *_pTxCharacteristic;
    bool _deviceConnected;
    bool _oldDeviceConnected;
    
    std::queue<String> _cmdQueue;
    SemaphoreHandle_t _queueMutex;
    String _rxBuffer;

public:
    BleAdapter();
    ~BleAdapter() override;

    void init() override;
    bool isConnected() override;
    bool hasNewCommand() override;
    String getNextCommand() override;
    void sendEnrollStatus(const String& status, const String& details = "") override;
    void sendSystemStatus(bool wifiConnected, uint8_t templatesCount) override;

    // Callbacks do Servidor
    void onConnect(NimBLEServer *pServer) override;
    void onDisconnect(NimBLEServer *pServer) override;

    // Callbacks da Caracteristica RX
    void onWrite(NimBLECharacteristic *pCharacteristic) override;

    // Manutenção de conexão no Loop
    void checkConnection();
};

} // namespace Adapters
