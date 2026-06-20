#pragma once
#include <Preferences.h>
#include "../Core/Interfaces/IStorage.h"

namespace Adapters {

class StorageAdapter : public Domain::IStorage {
private:
    Preferences _prefRfid;
    Preferences _prefFinger;
    Preferences _prefNames;
    Preferences _prefUuids;
    Preferences _prefLogs;

public:
    StorageAdapter();
    ~StorageAdapter() override;

    bool init() override;

    // Gestão de Usuários
    bool hasUser(uint8_t userId) override;
    bool getUserName(uint8_t userId, String &nameOut) override;
    void saveUser(uint8_t userId, const String& name, const String& uuid = "") override;
    void deleteUser(uint8_t userId) override;
    bool getUserUuid(uint8_t userId, String &uuidOut) override;
    uint8_t getOrCreateUser(const String& uuid, const String& name) override;

    // Gestão RFID
    void saveRfidMapping(uint32_t rfidCode, uint8_t userId) override;
    uint8_t getRfidUser(uint32_t rfidCode) override;
    void deleteRfidMapping(uint32_t rfidCode) override;

    // Gestão de Digital
    void saveFingerprintMapping(uint8_t index, uint8_t fingerId, uint8_t userId) override;
    uint8_t getFingerprintUser(uint8_t index, uint8_t &fingerIdOut) override;
    void deleteFingerprintMapping(uint8_t index) override;

    // Logs Offline
    bool saveOfflineLog(const Domain::AccessLog& log) override;
    uint8_t getOfflineLogsCount() override;
    bool getOfflineLog(uint8_t logIndex, Domain::AccessLog& logOut) override;
    void removeOfflineLog(uint8_t logIndex) override;
    void clearOfflineLogs() override;

    // Limpeza Geral
    void clearAllData() override;
};

} // namespace Adapters
