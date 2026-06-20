#include "StorageAdapter.h"
#include "SystemConfig.h"

namespace Adapters {

StorageAdapter::StorageAdapter() {}

StorageAdapter::~StorageAdapter() {
    _prefRfid.end();
    _prefFinger.end();
    _prefNames.end();
    _prefUuids.end();
    _prefLogs.end();
}

bool StorageAdapter::init() {
    bool ok1 = _prefRfid.begin("rfid_map", false);
    bool ok2 = _prefFinger.begin("finger_map", false);
    bool ok3 = _prefNames.begin("user_names", false);
    bool ok4 = _prefUuids.begin("user_uuids", false);
    bool ok5 = _prefLogs.begin("offline_logs", false);
    return ok1 && ok2 && ok3 && ok4 && ok5;
}

bool StorageAdapter::hasUser(uint8_t userId) {
    String key = String(userId);
    return _prefNames.isKey(key.c_str());
}

bool StorageAdapter::getUserName(uint8_t userId, String &nameOut) {
    String key = String(userId);
    if (_prefNames.isKey(key.c_str())) {
        nameOut = _prefNames.getString(key.c_str(), "");
        return true;
    }
    return false;
}

void StorageAdapter::saveUser(uint8_t userId, const String& name, const String& uuid) {
    String key = String(userId);
    _prefNames.putString(key.c_str(), name);
    if (uuid.length() > 0) {
        _prefUuids.putString(key.c_str(), uuid);
    }
}

void StorageAdapter::deleteUser(uint8_t userId) {
    String key = String(userId);
    if (_prefNames.isKey(key.c_str())) {
        _prefNames.remove(key.c_str());
    }
    if (_prefUuids.isKey(key.c_str())) {
        _prefUuids.remove(key.c_str());
    }
}

bool StorageAdapter::getUserUuid(uint8_t userId, String &uuidOut) {
    String key = String(userId);
    if (_prefUuids.isKey(key.c_str())) {
        uuidOut = _prefUuids.getString(key.c_str(), "");
        return true;
    }
    return false;
}

uint8_t StorageAdapter::getOrCreateUser(const String& uuid, const String& name) {
    // 1. Procurar se o UUID já está mapeado
    for (int i = 1; i < 256; i++) {
        String key = String(i);
        if (_prefUuids.isKey(key.c_str())) {
            if (_prefUuids.getString(key.c_str(), "") == uuid) {
                // Atualiza o nome local se tiver mudado
                _prefNames.putString(key.c_str(), name);
                return i;
            }
        }
    }
    
    // 2. Achar primeiro slot livre
    for (int i = 1; i < 256; i++) {
        String key = String(i);
        if (!_prefNames.isKey(key.c_str()) && !_prefUuids.isKey(key.c_str())) {
            _prefNames.putString(key.c_str(), name);
            _prefUuids.putString(key.c_str(), uuid);
            return i;
        }
    }
    return 0; // Sem espaço livre
}

void StorageAdapter::saveRfidMapping(uint32_t rfidCode, uint8_t userId) {
    String key = String(rfidCode);
    _prefRfid.putUChar(key.c_str(), userId);
}

uint8_t StorageAdapter::getRfidUser(uint32_t rfidCode) {
    String key = String(rfidCode);
    if (_prefRfid.isKey(key.c_str())) {
        return _prefRfid.getUChar(key.c_str(), 0);
    }
    return 0; // 0 significa não encontrado
}

void StorageAdapter::deleteRfidMapping(uint32_t rfidCode) {
    String key = String(rfidCode);
    if (_prefRfid.isKey(key.c_str())) {
        _prefRfid.remove(key.c_str());
    }
}

void StorageAdapter::saveFingerprintMapping(uint8_t index, uint8_t fingerId, uint8_t userId) {
    String key = String(index);
    uint16_t val = (static_cast<uint16_t>(fingerId) << 8) | userId;
    _prefFinger.putUShort(key.c_str(), val);
}

uint8_t StorageAdapter::getFingerprintUser(uint8_t index, uint8_t &fingerIdOut) {
    String key = String(index);
    if (_prefFinger.isKey(key.c_str())) {
        uint16_t val = _prefFinger.getUShort(key.c_str(), 0);
        fingerIdOut = (val >> 8) & 0xFF;
        return val & 0xFF;
    }
    fingerIdOut = 0;
    return 0;
}

void StorageAdapter::deleteFingerprintMapping(uint8_t index) {
    String key = String(index);
    if (_prefFinger.isKey(key.c_str())) {
        _prefFinger.remove(key.c_str());
    }
}

bool StorageAdapter::saveOfflineLog(const Domain::AccessLog& log) {
    uint8_t count = _prefLogs.getUChar("count", 0);
    
    // Se estourar o limite, removemos o log mais antigo (índice 0) para liberar espaço
    if (count >= MAX_OFFLINE_LOGS) {
        removeOfflineLog(0);
        count = _prefLogs.getUChar("count", 0);
    }
    
    String val = String(log.getUserId()) + "," + 
                 String(log.getMethodVal()) + "," + 
                 String(log.isAuthorized() ? "1" : "0") + "," + 
                 log.getRfidUid() + "," + 
                 String(log.getFingerprintIndex()) + "," + 
                 log.getTimestamp();
    String key = String("log_") + count;
    
    _prefLogs.putString(key.c_str(), val);
    _prefLogs.putUChar("count", count + 1);
    
    return true;
}

uint8_t StorageAdapter::getOfflineLogsCount() {
    return _prefLogs.getUChar("count", 0);
}

bool StorageAdapter::getOfflineLog(uint8_t logIndex, Domain::AccessLog& logOut) {
    uint8_t count = _prefLogs.getUChar("count", 0);
    if (logIndex >= count) {
        return false;
    }
    
    String key = String("log_") + logIndex;
    String val = _prefLogs.getString(key.c_str(), "");
    if (val.length() == 0) {
        return false;
    }
    
    // Parse da string: "userId,method,authorized,rfidUid,fingerprintIndex,timestamp"
    int comma1 = val.indexOf(',');
    int comma2 = val.indexOf(',', comma1 + 1);
    int comma3 = val.indexOf(',', comma2 + 1);
    int comma4 = val.indexOf(',', comma3 + 1);
    int comma5 = val.indexOf(',', comma4 + 1);
    
    if (comma1 == -1 || comma2 == -1) {
        // Fallback para formato antigo de 3 campos
        if (comma1 != -1) {
            uint8_t userId = val.substring(0, comma1).toInt();
            int nextComma = val.indexOf(',', comma1 + 1);
            if (nextComma == -1) return false;
            uint8_t methodVal = val.substring(comma1 + 1, nextComma).toInt();
            String timestamp = val.substring(nextComma + 1);
            Domain::AccessMethod method = (methodVal == 1) ? 
                Domain::AccessMethod::FINGERPRINT : Domain::AccessMethod::RFID;
            logOut = Domain::AccessLog(timestamp, userId, method, true, "", -1);
            return true;
        }
        return false;
    }
    
    uint8_t userId = val.substring(0, comma1).toInt();
    uint8_t methodVal = val.substring(comma1 + 1, comma2).toInt();
    bool authorized = val.substring(comma2 + 1, comma3) == "1";
    String rfidUid = val.substring(comma3 + 1, comma4);
    int fingerprintIndex = val.substring(comma4 + 1, comma5).toInt();
    String timestamp = val.substring(comma5 + 1);
    
    Domain::AccessMethod method = (methodVal == 1) ? 
        Domain::AccessMethod::FINGERPRINT : Domain::AccessMethod::RFID;
        
    logOut = Domain::AccessLog(timestamp, userId, method, authorized, rfidUid, fingerprintIndex);
    return true;
}

void StorageAdapter::removeOfflineLog(uint8_t logIndex) {
    uint8_t count = _prefLogs.getUChar("count", 0);
    if (logIndex >= count) {
        return;
    }
    
    // Desloca todos os registros posteriores uma posição para trás
    for (uint8_t i = logIndex + 1; i < count; i++) {
        String keyNext = String("log_") + i;
        String keyPrev = String("log_") + (i - 1);
        String val = _prefLogs.getString(keyNext.c_str(), "");
        _prefLogs.putString(keyPrev.c_str(), val);
    }
    
    // Remove o último registro redundante
    String keyLast = String("log_") + (count - 1);
    _prefLogs.remove(keyLast.c_str());
    _prefLogs.putUChar("count", count - 1);
}

void StorageAdapter::clearOfflineLogs() {
    uint8_t count = _prefLogs.getUChar("count", 0);
    for (uint8_t i = 0; i < count; i++) {
        String key = String("log_") + i;
        _prefLogs.remove(key.c_str());
    }
    _prefLogs.putUChar("count", 0);
}

void StorageAdapter::clearAllData() {
    _prefRfid.clear();
    _prefFinger.clear();
    _prefNames.clear();
    _prefUuids.clear();
    _prefLogs.clear();
}

} // namespace Adapters
