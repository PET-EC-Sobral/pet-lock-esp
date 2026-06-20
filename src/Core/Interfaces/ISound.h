#pragma once
#include <stdint.h>

namespace Domain {

class ISound {
public:
    virtual ~ISound() = default;
    virtual void init() = 0;
    virtual void playAllowed() = 0;
    virtual void playDenied() = 0;
    virtual void playMasterModeEnter() = 0;
    virtual void playMasterModeExit() = 0;
    virtual void playCardRemoved() = 0;
    virtual void playCardDefined() = 0;
    virtual void playBoot() = 0;
};

} // namespace Domain
