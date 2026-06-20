#pragma once
#include <stdint.h>

namespace Domain {

class IRfid {
public:
    virtual ~IRfid() = default;
    virtual void init() = 0;
    virtual bool readCard(uint8_t *uidOut) = 0; // Preenche uidOut (4 bytes) e retorna true se cartão lido
};

} // namespace Domain
