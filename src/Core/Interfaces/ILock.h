#pragma once

namespace Domain {

class ILock {
public:
    virtual ~ILock() = default;
    virtual void init() = 0;
    virtual void unlock() = 0;
    virtual void lock() = 0;
    virtual bool isUnlocked() = 0;
};

} // namespace Domain
