#pragma once
#include "../Core/Interfaces/ILock.h"

namespace Adapters {

class LockAdapter : public Domain::ILock {
private:
    int _pin;
    bool _isUnlocked;

public:
    LockAdapter(int pin);
    ~LockAdapter() override = default;

    void init() override;
    void unlock() override;
    void lock() override;
    bool isUnlocked() override;
};

} // namespace Adapters
