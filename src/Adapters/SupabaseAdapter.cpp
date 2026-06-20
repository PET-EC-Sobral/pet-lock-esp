#include "SupabaseAdapter.h"
#include "SystemConfig.h"
#include <ArduinoJson.h>
#include <WiFi.h>

namespace Adapters {

SupabaseAdapter::SupabaseAdapter(const String& supabaseUrl, const String& supabaseKey, Domain::IStorage& storage)
    : _url(supabaseUrl), _key(supabaseKey), _storage(storage) {}

void SupabaseAdapter::init() {
    // Não é necessário inicializar o cliente global
}

bool SupabaseAdapter::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool SupabaseAdapter::syncTime() {
    if (!isConnected()) return false;
    
    Serial.println("[Supabase] Sincronizando horario via NTP (GMT-3)...");
    configTime(-10800, 0, "pool.ntp.org", "time.google.com");
    
    // Aguarda sincronização por até 5 segundos
    time_t now = time(nullptr);
    int retries = 0;
    while (now < 24 * 3600 * 365 && retries < 50) {
        delay(100);
        now = time(nullptr);
        retries++;
    }
    
    if (now > 24 * 3600 * 365) {
        struct tm timeinfo;
        getLocalTime(&timeinfo);
        Serial.printf("[Supabase] Horario sincronizado: %04d-%02d-%02d %02d:%02d:%02d\n",
                      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        return true;
    }
    
    Serial.println("[Supabase] Falha ao sincronizar horario.");
    return false;
}

bool SupabaseAdapter::postAccessLog(const Domain::AccessLog& log) {
    StaticJsonDocument<512> doc;
    doc["timestamp"] = log.getTimestamp();
    
    if (log.getUserId() != 0) {
        String uuid;
        if (_storage.getUserUuid(log.getUserId(), uuid)) {
            doc["user_id"] = uuid;
        } else {
            doc["user_id"] = nullptr;
        }
    } else {
        doc["user_id"] = nullptr;
    }
    
    if (log.getMethod() == Domain::AccessMethod::FINGERPRINT) {
        doc["method"] = "digital";
        if (log.getFingerprintIndex() >= 0) {
            doc["fingerprint_index"] = log.getFingerprintIndex();
        } else {
            doc["fingerprint_index"] = nullptr;
        }
        doc["rfid_uid"] = nullptr;
    } else {
        doc["method"] = "rfid";
        doc["fingerprint_index"] = nullptr;
        if (log.getRfidUid().length() > 0) {
            doc["rfid_uid"] = log.getRfidUid();
        } else {
            doc["rfid_uid"] = nullptr;
        }
    }
    
    doc["result"] = log.isAuthorized() ? "authorized" : "denied";

    String payload;
    serializeJson(doc, payload);

    return makePostRequest("/rest/v1/" DB_TABLE_ACCESS_LOGS, payload);
}

static uint32_t parseHex(const String& hexStr) {
    return strtoul(hexStr.c_str(), nullptr, 16);
}

bool SupabaseAdapter::fetchDatabaseMappings(Domain::IStorage& storage) {
    // 1. Puxa todos os Usuários ativos
    String usersJson = makeGetRequest("/rest/v1/" DB_TABLE_USERS "?select=id,name&status=eq.active");
    if (usersJson.length() == 0) return false;

    // 2. Puxa todos os Mapeamentos de Cartão RFID ativos
    String rfidsJson = makeGetRequest("/rest/v1/" DB_TABLE_RFID_CARDS "?select=rfid_uid,user_id&status=eq.active");
    if (rfidsJson.length() == 0) return false;

    // 3. Puxa todos os Mapeamentos de Digitais ativas
    String fingersJson = makeGetRequest("/rest/v1/" DB_TABLE_FINGERPRINTS "?select=fingerprint_index,user_id&status=eq.active");
    if (fingersJson.length() == 0) return false;

    // Limpa tabelas locais de mapeamento para reinstanciar (evita lixo de remoções remotas)
    storage.clearAllData();

    // Parse Usuários
    DynamicJsonDocument docUsers(8192);
    DeserializationError err1 = deserializeJson(docUsers, usersJson);
    if (!err1) {
        JsonArray arr = docUsers.as<JsonArray>();
        uint8_t localId = 1;
        for (JsonObject obj : arr) {
            String uuid = obj["id"].as<String>();
            String name = obj["name"].as<String>();
            storage.saveUser(localId, name, uuid);
            localId++;
            if (localId == 0) break; // Overflow protection
        }
    }

    // Parse RFID
    DynamicJsonDocument docRfids(4096);
    DeserializationError err2 = deserializeJson(docRfids, rfidsJson);
    if (!err2) {
        JsonArray arr = docRfids.as<JsonArray>();
        for (JsonObject obj : arr) {
            String rfidUidStr = obj["rfid_uid"].as<String>();
            String userUuid = obj["user_id"].as<String>();
            
            uint8_t localId = 0;
            for (int i = 1; i < 256; i++) {
                String tempUuid;
                if (storage.getUserUuid(i, tempUuid) && tempUuid == userUuid) {
                    localId = i;
                    break;
                }
            }
            
            if (localId != 0) {
                uint32_t rfidCode = parseHex(rfidUidStr);
                storage.saveRfidMapping(rfidCode, localId);
            }
        }
    }

    // Parse Digitais
    DynamicJsonDocument docFingers(4096);
    DeserializationError err3 = deserializeJson(docFingers, fingersJson);
    if (!err3) {
        JsonArray arr = docFingers.as<JsonArray>();
        for (JsonObject obj : arr) {
            uint8_t index = obj["fingerprint_index"];
            String userUuid = obj["user_id"].as<String>();
            
            uint8_t localId = 0;
            for (int i = 1; i < 256; i++) {
                String tempUuid;
                if (storage.getUserUuid(i, tempUuid) && tempUuid == userUuid) {
                    localId = i;
                    break;
                }
            }
            
            if (localId != 0) {
                storage.saveFingerprintMapping(index, index, localId);
            }
        }
    }

    Serial.println("[Supabase] Mapeamentos sincronizados e cacheados localmente!");
    return true;
}

static String getFingerName(uint8_t fingerId) {
    switch(fingerId) {
        case 1: return "Polegar Esquerdo";
        case 2: return "Indicador Esquerdo";
        case 3: return "Medio Esquerdo";
        case 4: return "Anelar Esquerdo";
        case 5: return "Minimo Esquerdo";
        case 6: return "Polegar Direito";
        case 7: return "Indicador Direito";
        case 8: return "Medio Direito";
        case 9: return "Anelar Direito";
        case 10: return "Minimo Direito";
        default: return "Dedo " + String(fingerId);
    }
}

bool SupabaseAdapter::pushFingerprintMapping(uint8_t index, uint8_t fingerId, uint8_t userId) {
    String uuid;
    if (!_storage.getUserUuid(userId, uuid)) {
        Serial.printf("[Supabase] Erro ao obter UUID para o usuario local ID %d\n", userId);
        return false;
    }

    StaticJsonDocument<256> doc;
    doc["fingerprint_index"] = index;
    doc["fingerprint_name"] = getFingerName(fingerId);
    doc["user_id"] = uuid;
    doc["status"] = "active";
    doc["sync_status"] = "synced";

    String payload;
    serializeJson(doc, payload);

    return makePostRequest("/rest/v1/" DB_TABLE_FINGERPRINTS, payload);
}

bool SupabaseAdapter::pushRfidMapping(uint32_t rfidCode, uint8_t userId) {
    String uuid;
    if (!_storage.getUserUuid(userId, uuid)) {
        Serial.printf("[Supabase] Erro ao obter UUID para o usuario local ID %d\n", userId);
        return false;
    }

    char hexBuf[9];
    sprintf(hexBuf, "%08X", rfidCode);
    String rfidUidStr(hexBuf);

    StaticJsonDocument<256> doc;
    doc["rfid_uid"] = rfidUidStr;
    doc["user_id"] = uuid;
    doc["status"] = "active";
    doc["sync_status"] = "synced";

    String payload;
    serializeJson(doc, payload);

    return makePostRequest("/rest/v1/" DB_TABLE_RFID_CARDS, payload);
}

bool SupabaseAdapter::makePostRequest(const String& path, const String& jsonPayload) {
    if (!isConnected()) return false;

    uint32_t freeHeapBefore = ESP.getFreeHeap();
    Serial.printf("[Supabase] POST %s | Heap Livre antes: %d bytes\n", path.c_str(), freeHeapBefore);

    WiFiClientSecure wifiClient;
    wifiClient.setInsecure();
    
    HTTPClient http;
    String targetUrl = _url + path;
    
    if (!http.begin(wifiClient, targetUrl)) {
        Serial.println("[Supabase] Erro ao iniciar HTTPClient no POST");
        return false;
    }
    
    // Headers padrões do Supabase PostgREST
    http.addHeader("apikey", _key);
    http.addHeader("Authorization", "Bearer " + _key);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Prefer", "return=minimal"); // Apenas o status da inserção é necessário
    http.addHeader("Connection", "close");      // Solicita fechamento da conexão pós-resposta
    
    int code = http.POST(jsonPayload);
    http.end();
    wifiClient.stop();

    uint32_t freeHeapAfter = ESP.getFreeHeap();
    Serial.printf("[Supabase] POST concluído. Status: %d | Heap Livre depois: %d bytes\n", code, freeHeapAfter);

    if (code >= 200 && code < 300) {
        return true;
    }
    
    Serial.printf("[Supabase] Erro POST %s: Status %d\n", path.c_str(), code);
    return false;
}

String SupabaseAdapter::makeGetRequest(const String& path) {
    if (!isConnected()) return "";

    uint32_t freeHeapBefore = ESP.getFreeHeap();
    Serial.printf("[Supabase] GET %s | Heap Livre antes: %d bytes\n", path.c_str(), freeHeapBefore);

    WiFiClientSecure wifiClient;
    wifiClient.setInsecure();
    
    HTTPClient http;
    String targetUrl = _url + path;
    
    if (!http.begin(wifiClient, targetUrl)) {
        Serial.println("[Supabase] Erro ao iniciar HTTPClient no GET");
        return "";
    }
    
    http.addHeader("apikey", _key);
    http.addHeader("Authorization", "Bearer " + _key);
    http.addHeader("Connection", "close");      // Solicita fechamento da conexão pós-resposta
    
    int code = http.GET();
    String response = "";
    if (code == 200) {
        response = http.getString();
    } else {
        Serial.printf("[Supabase] Erro GET %s: Status %d\n", path.c_str(), code);
    }
    http.end();
    wifiClient.stop();

    uint32_t freeHeapAfter = ESP.getFreeHeap();
    Serial.printf("[Supabase] GET concluído. Status: %d | Heap Livre depois: %d bytes\n", code, freeHeapAfter);
    
    return response;
}

} // namespace Adapters
