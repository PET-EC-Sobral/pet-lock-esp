#include "AccessControlUseCase.h"
#include <time.h>

namespace Domain {

AccessControlUseCase::AccessControlUseCase(
    IDisplay &display,
    IFingerprint &fingerprint,
    IRfid &rfid,
    ILock &lock,
    ISound &sound,
    IStorage &storage,
    INetwork &network,
    TimerHandle_t lockTimer
) : _display(display), _fingerprint(fingerprint), _rfid(rfid),
    _lock(lock), _sound(sound), _storage(storage), _network(network),
    _lockTimer(lockTimer) {
    // Configura o ponteiro da fechadura como ID do timer para que o callback saiba o que trancar
    vTimerSetTimerID(_lockTimer, &_lock);
}

void AccessControlUseCase::processRfidAccess(uint32_t rfidCode) {
    Serial.printf("[Auth] Cartao lido: %u\n", rfidCode);
    _display.showVerifying();
    
    char hexBuf[9];
    sprintf(hexBuf, "%08X", rfidCode);
    String rfidUidStr(hexBuf);
    
    uint8_t userId = _storage.getRfidUser(rfidCode);
    if (userId != 0) {
        // Acesso Permitido!
        _lock.unlock();
        xTimerStart(_lockTimer, 0); // Dispara timer para travar após LOCK_UNLOCK_DURATION_MS
        
        _sound.playAllowed();
        
        String userName = "";
        _storage.getUserName(userId, userName);
        _display.showAllowed(userName);
        
        // Log de acesso autorizado
        String timestamp = getFormattedTime();
        AccessLog log(timestamp, userId, AccessMethod::RFID, true, rfidUidStr, -1);
        
        // Envia para Supabase ou salva offline se falhar/desconectado
        bool posted = false;
        if (_network.isConnected()) {
            posted = _network.postAccessLog(log);
        }
        
        if (!posted) {
            _storage.saveOfflineLog(log);
            Serial.println("[Auth] Log de acesso salvo offline.");
        } else {
            Serial.println("[Auth] Log de acesso enviado online.");
        }
        
        delay(2000);
        _display.showWaitingAccess();
    } else {
        // Log de acesso negado
        String timestamp = getFormattedTime();
        AccessLog log(timestamp, 0, AccessMethod::RFID, false, rfidUidStr, -1);
        
        bool posted = false;
        if (_network.isConnected()) {
            posted = _network.postAccessLog(log);
        }
        
        if (!posted) {
            _storage.saveOfflineLog(log);
            Serial.println("[Auth] Log de acesso negado salvo offline.");
        } else {
            Serial.println("[Auth] Log de acesso negado enviado online.");
        }
        
        handleAccessDenied(AccessMethod::RFID);
    }
}

void AccessControlUseCase::processFingerprintAccess(uint8_t index) {
    Serial.printf("[Auth] Digital detectada no slot: %d\n", index);
    _display.showVerifying();
    
    uint8_t fingerId = 0;
    uint8_t userId = _storage.getFingerprintUser(index, fingerId);
    
    if (userId != 0) {
        // Acesso Permitido!
        _lock.unlock();
        xTimerStart(_lockTimer, 0);
        
        _sound.playAllowed();
        
        // Led do leitor pisca em Verde
        //mod1fi3d_fingerprint.setLed(FingerprintLedMode::FLASHING, FingerprintColor::GREEN, 2); //mod1fi3d
        
        String userName = "";
        _storage.getUserName(userId, userName);
        _display.showAllowed(userName);
        
        // Log de acesso autorizado
        String timestamp = getFormattedTime();
        AccessLog log(timestamp, userId, AccessMethod::FINGERPRINT, true, "", index);
        
        bool posted = false;
        if (_network.isConnected()) {
            posted = _network.postAccessLog(log);
        }
        
        if (!posted) {
            _storage.saveOfflineLog(log);
            Serial.println("[Auth] Log de acesso salvo offline.");
        } else {
            Serial.println("[Auth] Log de acesso enviado online.");
        }
        
        _fingerprint.waitFingerReleased();
        delay(2000);
        _display.showWaitingAccess();
    } else {
        // Digital cadastrada fisicamente, mas sem mapeamento local
        String timestamp = getFormattedTime();
        AccessLog log(timestamp, 0, AccessMethod::FINGERPRINT, false, "", index);
        
        bool posted = false;
        if (_network.isConnected()) {
            posted = _network.postAccessLog(log);
        }
        
        if (!posted) {
            _storage.saveOfflineLog(log);
            Serial.println("[Auth] Log de acesso negado salvo offline.");
        } else {
            Serial.println("[Auth] Log de acesso negado enviado online.");
        }
        
        handleAccessDenied(AccessMethod::FINGERPRINT);
    }
}

void AccessControlUseCase::processFingerprintAccessDenied() {
    _display.showVerifying();
    
    // Log de acesso negado (sem index pois a digital não foi reconhecida)
    String timestamp = getFormattedTime();
    AccessLog log(timestamp, 0, AccessMethod::FINGERPRINT, false, "", -1);
    
    bool posted = false;
    if (_network.isConnected()) {
        posted = _network.postAccessLog(log);
    }
    
    if (!posted) {
        _storage.saveOfflineLog(log);
        Serial.println("[Auth] Log de acesso negado salvo offline.");
    } else {
        Serial.println("[Auth] Log de acesso negado enviado online.");
    }
    
    handleAccessDenied(AccessMethod::FINGERPRINT);
}

void AccessControlUseCase::handleAccessDenied(AccessMethod method) {
    _sound.playDenied();
    
    if (method == AccessMethod::FINGERPRINT) {
        //mod1fi3d_fingerprint.setLed(FingerprintLedMode::FLASHING, FingerprintColor::RED, 2);
    }
    
    _display.showDenied();
    
    if (method == AccessMethod::FINGERPRINT) {
        _fingerprint.waitFingerReleased();
    }
    
    delay(2000);
    _display.showWaitingAccess();
}

String AccessControlUseCase::getFormattedTime() {
    time_t now = time(nullptr);
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo); // Usar UTC para o banco de dados
    
    char buf[30];
    // Formato ISO 8601 UTC: YYYY-MM-DDTHH:MM:SSZ
    sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02dZ",
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday,
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec);
            
    return String(buf);
}

void AccessControlUseCase::lockTimerCallback(TimerHandle_t xTimer) {
    ILock *lock = static_cast<ILock*>(pvTimerGetTimerID(xTimer));
    if (lock != nullptr) {
        lock->lock();
    }
}

} // namespace Domain
