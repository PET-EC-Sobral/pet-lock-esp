#pragma once
#include "../Interfaces/IDisplay.h"
#include "../Interfaces/IFingerprint.h"
#include "../Interfaces/IRfid.h"
#include "../Interfaces/ILock.h"
#include "../Interfaces/ISound.h"
#include "../Interfaces/IStorage.h"
#include "../Interfaces/INetwork.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

namespace Domain {

class AccessControlUseCase {
private:
    IDisplay &_display;
    IFingerprint &_fingerprint;
    IRfid &_rfid;
    ILock &_lock;
    ISound &_sound;
    IStorage &_storage;
    INetwork &_network;
    
    TimerHandle_t _lockTimer;

    String getFormattedTime();

public:
    AccessControlUseCase(
        IDisplay &display,
        IFingerprint &fingerprint,
        IRfid &rfid,
        ILock &lock,
        ISound &sound,
        IStorage &storage,
        INetwork &network,
        TimerHandle_t lockTimer
    );

    void processRfidAccess(uint32_t rfidCode);
    void processFingerprintAccess(uint8_t index);
    void processFingerprintAccessDenied();
    void handleAccessDenied(AccessMethod method);
    
    // Callback para fechar a fechadura acionada pelo Timer
    static void lockTimerCallback(TimerHandle_t xTimer);
};

} // namespace Domain
