#pragma once
#include <Arduino.h>
#include "../Entities/AccessLog.h"

namespace Domain {

class IStorage {
public:
    virtual ~IStorage() = default;
    virtual bool init() = 0;

    // Gestão de Usuários e Nomes Localmente (Cache leve)
    virtual bool hasUser(uint8_t userId) = 0;
    virtual bool getUserName(uint8_t userId, String &nameOut) = 0;
    virtual void saveUser(uint8_t userId, const String& name, const String& uuid = "") = 0;
    virtual void deleteUser(uint8_t userId) = 0;
    virtual bool getUserUuid(uint8_t userId, String &uuidOut) = 0;
    virtual uint8_t getOrCreateUser(const String& uuid, const String& name) = 0;

    // Gestão de Mapeamentos RFID (código 4-bytes -> userId)
    virtual void saveRfidMapping(uint32_t rfidCode, uint8_t userId) = 0;
    virtual uint8_t getRfidUser(uint32_t rfidCode) = 0;
    virtual void deleteRfidMapping(uint32_t rfidCode) = 0;

    // Gestão de Mapeamentos de Digital (index 1-byte -> fingerId + userId)
    virtual void saveFingerprintMapping(uint8_t index, uint8_t fingerId, uint8_t userId) = 0;
    virtual uint8_t getFingerprintUser(uint8_t index, uint8_t &fingerIdOut) = 0;
    virtual void deleteFingerprintMapping(uint8_t index) = 0;

    // Fila Offline de Registros de Acesso
    virtual bool saveOfflineLog(const AccessLog& log) = 0;
    virtual uint8_t getOfflineLogsCount() = 0;
    virtual bool getOfflineLog(uint8_t logIndex, AccessLog& logOut) = 0;
    virtual void removeOfflineLog(uint8_t logIndex) = 0;
    virtual void clearOfflineLogs() = 0;

    // Limpeza Geral
    virtual void clearAllData() = 0;
};

} // namespace Domain
