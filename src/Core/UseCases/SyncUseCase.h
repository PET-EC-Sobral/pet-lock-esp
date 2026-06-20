#pragma once
#include "../Interfaces/IStorage.h"
#include "../Interfaces/INetwork.h"

namespace Domain {

class SyncUseCase {
private:
    IStorage &_storage;
    INetwork &_network;

public:
    SyncUseCase(IStorage &storage, INetwork &network);
    ~SyncUseCase() = default;

    bool syncOfflineLogs();
    bool syncCloudMappings();
    bool runFullSync();
};

} // namespace Domain
