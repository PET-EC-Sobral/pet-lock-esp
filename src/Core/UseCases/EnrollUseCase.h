#pragma once
#include "../Interfaces/IDisplay.h"
#include "../Interfaces/IFingerprint.h"
#include "../Interfaces/IRfid.h"
#include "../Interfaces/IStorage.h"
#include "../Interfaces/INetwork.h"
#include "../Interfaces/IBluetooth.h"
#include "../Interfaces/ISound.h"

namespace Domain {

class EnrollUseCase {
private:
    IDisplay &_display;
    IFingerprint &_fingerprint;
    IRfid &_rfid;
    IStorage &_storage;
    INetwork &_network;
    IBluetooth &_bluetooth;
    ISound &_sound;

    uint8_t getFirstFreeFingerprintSlot();

public:
    EnrollUseCase(
        IDisplay &display,
        IFingerprint &fingerprint,
        IRfid &rfid,
        IStorage &storage,
        INetwork &network,
        IBluetooth &bluetooth,
        ISound &sound
    );

    bool enrollFingerprint(uint8_t userId, uint8_t fingerId, const String& userName);
    bool enrollRfid(uint8_t userId, const String& userName);
};

} // namespace Domain
