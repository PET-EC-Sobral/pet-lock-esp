#pragma once
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "../Core/Interfaces/INetwork.h"

namespace Adapters {

class SupabaseAdapter : public Domain::INetwork {
private:
    String _url;
    String _key;
    Domain::IStorage& _storage;

    bool makePostRequest(const String& path, const String& jsonPayload);
    String makeGetRequest(const String& path);

public:
    SupabaseAdapter(const String& supabaseUrl, const String& supabaseKey, Domain::IStorage& storage);
    ~SupabaseAdapter() override = default;

    void init() override;
    bool isConnected() override;
    bool syncTime() override;
    bool postAccessLog(const Domain::AccessLog& log) override;
    bool fetchDatabaseMappings(Domain::IStorage& storage) override;
    bool pushFingerprintMapping(uint8_t index, uint8_t fingerId, uint8_t userId) override;
    bool pushRfidMapping(uint32_t rfidCode, uint8_t userId) override;
};

} // namespace Adapters
