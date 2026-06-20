#pragma once
#include "../Core/Interfaces/ISound.h"

namespace Adapters {

class BuzzerAdapter : public Domain::ISound {
private:
    int _pin;

public:
    BuzzerAdapter(int pin);
    ~BuzzerAdapter() override = default;

    void init() override;
    void playAllowed() override;
    void playDenied() override;
    void playMasterModeEnter() override;
    void playMasterModeExit() override;
    void playCardRemoved() override;
    void playCardDefined() override;
    void playBoot() override;
};

} // namespace Adapters
