#include "SyncUseCase.h"
#include <Arduino.h>

namespace Domain {

SyncUseCase::SyncUseCase(IStorage &storage, INetwork &network)
    : _storage(storage), _network(network) {}

bool SyncUseCase::syncOfflineLogs() {
    if (!_network.isConnected()) {
        Serial.println("[Sync] Sincronizacao abortada: sem conexao de internet.");
        return false;
    }

    uint8_t count = _storage.getOfflineLogsCount();
    if (count == 0) {
        Serial.println("[Sync] Nenhum log offline para sincronizar.");
        return true;
    }

    Serial.printf("[Sync] Encontrado(s) %d log(s) offline. Enviando para Supabase...\n", count);
    
    uint8_t syncedCount = 0;
    while (_storage.getOfflineLogsCount() > 0 && _network.isConnected()) {
        AccessLog log;
        if (_storage.getOfflineLog(0, log)) {
            // Tenta postar na nuvem
            if (_network.postAccessLog(log)) {
                // Remove da fila local (deslocando os demais)
                _storage.removeOfflineLog(0);
                syncedCount++;
            } else {
                Serial.println("[Sync] Erro temporario ao enviar log. Tentara mais tarde.");
                break; // Para o envio para tentar no proximo ciclo
            }
        } else {
            // Log corrompido, remove da fila
            _storage.removeOfflineLog(0);
        }
        delay(50); // Evita sobrecarga de requisicoes rapidas
    }

    Serial.printf("[Sync] Sincronizacao concluida: %d logs enviados.\n", syncedCount);
    return true;
}

bool Domain::SyncUseCase::syncCloudMappings() {
    if (!_network.isConnected()) {
        Serial.println("[Sync] Erro: Sem rede para atualizar os cadastros.");
        return false;
    }

    Serial.println("[Sync] Baixando cadastros atualizados do Supabase...");
    return _network.fetchDatabaseMappings(_storage);
}

bool Domain::SyncUseCase::runFullSync() {
    bool logsSynced = syncOfflineLogs();
    bool mappingsSynced = syncCloudMappings();
    return logsSynced && mappingsSynced;
}

} // namespace Domain
