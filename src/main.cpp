#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_ST7789.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "SystemConfig.h"
#include "Core/Interfaces/IDisplay.h"
#include "Core/Interfaces/IFingerprint.h"
#include "Core/Interfaces/IRfid.h"
#include "Core/Interfaces/ILock.h"
#include "Core/Interfaces/ISound.h"
#include "Core/Interfaces/IStorage.h"
#include "Core/Interfaces/INetwork.h"
#include "Core/Interfaces/IBluetooth.h"

#include "Core/UseCases/AccessControlUseCase.h"
#include "Core/UseCases/EnrollUseCase.h"
#include "Core/UseCases/SyncUseCase.h"

#include "Adapters/DisplayAdapter.h"
#include "Adapters/FingerprintAdapter.h"
#include "Adapters/RfidAdapter.h"
#include "Adapters/LockAdapter.h"
#include "Adapters/BuzzerAdapter.h"
#include "Adapters/StorageAdapter.h"
#include "Adapters/SupabaseAdapter.h"
#include "Adapters/BleAdapter.h"

// ==========================================
// FREERTOS HANDLES & MUTEXES
// ==========================================
SemaphoreHandle_t xSPI_Mutex = NULL;
SemaphoreHandle_t xLocalDB_Mutex = NULL;

TaskHandle_t xAuthTask = NULL;
TaskHandle_t xBleTask = NULL;
TaskHandle_t xSyncTask = NULL;
TaskHandle_t xSystemTask = NULL;

TimerHandle_t xLockTimer = NULL;
volatile bool isEnrolling = false;

// ==========================================
// DRIVERS & ADAPTERS
// ==========================================
HardwareSerial fingerprintSerial(2);

// Instâncias globais utilizando o barramento unificado SPI
Adafruit_ST7789 displayDriver = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);

Adapters::DisplayAdapter *displayAdapter = nullptr;
Adapters::FingerprintAdapter *fingerprintAdapter = nullptr;
Adapters::RfidAdapter *rfidAdapter = nullptr;
Adapters::LockAdapter *lockAdapter = nullptr;
Adapters::BuzzerAdapter *buzzerAdapter = nullptr;
Adapters::StorageAdapter *storageAdapter = nullptr;
Adapters::SupabaseAdapter *supabaseAdapter = nullptr;
Adapters::BleAdapter *bleAdapter = nullptr;

// ==========================================
// USE CASES
// ==========================================
Domain::AccessControlUseCase *accessControlUseCase = nullptr;
Domain::EnrollUseCase *enrollUseCase = nullptr;
Domain::SyncUseCase *syncUseCase = nullptr;

// ==========================================
// FREERTOS TASK DEFINITIONS
// ==========================================

// Task 1: Autenticação de Cartões e Digitais (Core 1, Prioridade 3)
void vAuthTaskCode(void *pvParameters) {
    uint8_t uid[4];
    Serial.println("[AuthTask] Iniciada.");
    
    for (;;) {
        if (isEnrolling) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // 1. Tenta ler um cartão RFID
        bool cardRead = false;
        if (rfidAdapter != nullptr) {
            cardRead = rfidAdapter->readCard(uid);
        }
        
        if (cardRead) {
            uint32_t rfidCode = (static_cast<uint32_t>(uid[0]) << 24) |
                                (static_cast<uint32_t>(uid[1]) << 16) |
                                (static_cast<uint32_t>(uid[2]) << 8)  |
                                uid[3];
                                
            if (xSemaphoreTake(xLocalDB_Mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                accessControlUseCase->processRfidAccess(rfidCode);
                xSemaphoreGive(xLocalDB_Mutex);
            }
        } else {
            // 2. Se não leu cartão, tenta escanear digital (ZW111)
            int16_t scanResult = 0;
            if (fingerprintAdapter != nullptr) {
                scanResult = fingerprintAdapter->scan();
            }
            
            if (scanResult > 0) {
                // Digital Encontrada no slot scanResult
                if (xSemaphoreTake(xLocalDB_Mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    accessControlUseCase->processFingerprintAccess(scanResult);
                    xSemaphoreGive(xLocalDB_Mutex);
                }
            } else if (scanResult == -1) {
                // Digital lida mas não cadastrada
                if (xSemaphoreTake(xLocalDB_Mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    accessControlUseCase->processFingerprintAccessDenied();
                    xSemaphoreGive(xLocalDB_Mutex);
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // Cede tempo para outras tasks rodarem no Core 1
    }
}

// Task 2: Gerenciamento BLE & Comandos do App (Core 0, Prioridade 2)
void vBleTaskCode(void *pvParameters) {
    Serial.println("[BleTask] Iniciada.");
    bool wasConnected = false;
    
    for (;;) {
        if (bleAdapter != nullptr) {
            bleAdapter->checkConnection();
            bool isConnected = bleAdapter->isConnected();
            if (displayAdapter != nullptr) {
                displayAdapter->setBleStatus(isConnected);
            }
            
            if (isConnected && !wasConnected) {
                // Nova conexao detectada. Aguarda 1 segundo para o cliente subscrever no canal de notificacoes
                vTaskDelay(pdMS_TO_TICKS(1000));
                
                bool wifiConnected = (WiFi.status() == WL_CONNECTED);
                uint8_t count = 0;
                if (fingerprintAdapter != nullptr) {
                    count = fingerprintAdapter->getCount();
                }
                
                bleAdapter->sendSystemStatus(wifiConnected, count);
                wasConnected = true;
            } else if (!isConnected) {
                wasConnected = false;
            }
            
            if (bleAdapter->hasNewCommand()) {
                String cmdJson = bleAdapter->getNextCommand();
                
                // Processa o comando JSON vindo do aplicativo
                StaticJsonDocument<256> doc;
                DeserializationError err = deserializeJson(doc, cmdJson);
                
                if (!err) {
                    const char *cmd = doc["cmd"];
                    if (cmd && strcmp(cmd, "enter_enroll") == 0) {
                        String userUuid = doc["user_id"].as<String>();
                        const char *type = doc["type"];
                        const char *userName = doc["user_name"] | "Estudante";
                        
                        // Mapeia o UUID recebido para um ID local de 1 byte
                        uint8_t userId = storageAdapter->getOrCreateUser(userUuid, userName);
                        if (userId == 0) {
                            Serial.println("[BLE] Erro: Sem espaco para novos usuarios localmente.");
                            bleAdapter->sendEnrollStatus("failed", "no_local_slots");
                            continue;
                        }
                        
                        Serial.printf("[BLE] Solicitando cadastro de %s para usuario local ID: %d (%s)\n", type, userId, userName);
                        
                        // Sinaliza início do modo de cadastro
                        isEnrolling = true;
                        
                        // Aguarda a AuthTask concluir seu ciclo e liberar os semáforos/periféricos
                        delay(200);
                        
                        // Esvazia buffer serial do leitor para evitar leituras de lixo residual
                        while (fingerprintSerial.available()) {
                            fingerprintSerial.read();
                        }
                        
                        if (strcmp(type, "finger") == 0) {
                            uint8_t fingerId = doc["finger_id"];
                            if (xSemaphoreTake(xLocalDB_Mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
                                enrollUseCase->enrollFingerprint(userId, fingerId, userName);
                                xSemaphoreGive(xLocalDB_Mutex);
                            }
                        } else if (strcmp(type, "rfid") == 0) {
                            if (xSemaphoreTake(xLocalDB_Mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
                                enrollUseCase->enrollRfid(userId, userName);
                                xSemaphoreGive(xLocalDB_Mutex);
                            }
                        }
                        
                        // Restaura a leitura de acessos normais
                        isEnrolling = false;
                        
                        // Volta para a tela de aguardando acesso
                        displayAdapter->showWaitingAccess();
                    }
                    else if (cmd && strcmp(cmd, "remote_open") == 0) {
                        Serial.println("[BLE] Abertura remota solicitada.");
                        lockAdapter->unlock();
                        xTimerStart(xLockTimer, 0);
                        buzzerAdapter->playAllowed();
                        displayAdapter->showAllowed("App Remoto");
                        delay(2000);
                        displayAdapter->showWaitingAccess();
                    }
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(50)); // Resposta ágil do BLE polling
    }
}

// Task 3: Gerenciamento de Rede, NTP & Sincronização Supabase (Core 0, Prioridade 1)
void vSyncTaskCode(void *pvParameters) {
    Serial.println("[SyncTask] Iniciada.");
    
    // Configura o Wi-Fi em modo Station explicitamente
    WiFi.mode(WIFI_STA);
    
    // Registra listener de diagnóstico para obter códigos de erro/motivo de desconexão
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
        Serial.printf("[WiFi Event] Desconectado! Codigo do motivo: %d\n", info.wifi_sta_disconnected.reason);
    }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    unsigned long lastWifiCheck = millis(); // Inicializa com millis() para contar a partir de agora
    unsigned long lastSyncTime = 0;
    bool initialSyncDone = false;
    bool lastConnectionStatus = false;
    
    for (;;) {
        bool connected = (WiFi.status() == WL_CONNECTED);
        
        // Log de status de conexão caso mude
        if (connected != lastConnectionStatus) {
            if (connected) {
                Serial.printf("[WiFi] Conectado! IP: %s\n", WiFi.localIP().toString().c_str());
            } else {
                Serial.println("[WiFi] Conexao perdida.");
            }
            lastConnectionStatus = connected;
        }

        if (displayAdapter != nullptr) {
            displayAdapter->setWifiStatus(connected);
            if (storageAdapter != nullptr) {
                displayAdapter->setSyncStatus(storageAdapter->getOfflineLogsCount() > 0);
            }
        }
        
        // Reconecta se não estiver conectado e se passaram 20 segundos da última tentativa/desconexão
        if (!connected) {
            if (millis() - lastWifiCheck > 20000) {
                Serial.println("[WiFi] Tentando reconectar ao Wi-Fi...");
                WiFi.disconnect();
                WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
                lastWifiCheck = millis();
            }
        } else {
            // Mantém o temporizador atualizado enquanto estiver conectado
            lastWifiCheck = millis();
        }
        
        if (connected) {
            if (!initialSyncDone) {
                // Sincroniza horário e faz a primeira carga de mapeamento do banco de dados
                supabaseAdapter->syncTime();
                
                if (xSemaphoreTake(xLocalDB_Mutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
                    syncUseCase->runFullSync();
                    xSemaphoreGive(xLocalDB_Mutex);
                }
                
                initialSyncDone = true;
                lastSyncTime = millis();
            } else if (millis() - lastSyncTime > SYNC_INTERVAL_SEC * 1000) {
                // Sincronização periódica ordinária
                if (xSemaphoreTake(xLocalDB_Mutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
                    syncUseCase->runFullSync();
                    xSemaphoreGive(xLocalDB_Mutex);
                }
                lastSyncTime = millis();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Roda a cada 1 segundo para atualizar o display
    }
}

// Task 4: Monitor de Botão Físico (Wipe Manual) (Core 1, Prioridade 1)
void vSystemTaskCode(void *pvParameters) {
    pinMode(WIPEBUTTON_PIN, INPUT_PULLUP);
    
    for (;;) {
        // Botão físico ativo em nível ALTO (baseado no código original)
        if (digitalRead(WIPEBUTTON_PIN) == HIGH) {
            unsigned long pressedStart = millis();
            bool held = true;
            
            // Aguarda 10 segundos mantendo pressionado
            while (digitalRead(WIPEBUTTON_PIN) == HIGH) {
                if (millis() - pressedStart > 10000) {
                    held = true;
                    break;
                }
                held = false;
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            
            if (held) {
                Serial.println("[System] Botao de Wipe segurado por 10s. Limpando dados do sistema...");
                vTaskSuspend(xAuthTask); // Para leituras físicas
                
                buzzerAdapter->playMasterModeEnter();
                displayAdapter->showMessage("Limpando Memoria", "Aguarde...");
                
                if (xSemaphoreTake(xLocalDB_Mutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
                    storageAdapter->clearAllData();
                    fingerprintAdapter->clearDatabase();
                    xSemaphoreGive(xLocalDB_Mutex);
                }
                
                buzzerAdapter->playCardRemoved();
                displayAdapter->showMessage("Memoria Limpa", "Reiniciando!");
                
                delay(2000);
                ESP.restart(); // Reinicia o processador para limpar estados voláteis
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ==========================================
// ARDUINO SETUP & LOOP
// ==========================================

void setup() {
    Serial.begin(115200);
    Serial.println("==========================================");
    Serial.println("          PET LOCK - FIRMWARE INICIANDO   ");
    Serial.println("==========================================");

    // 1. Cria os Mutexes de compartilhamento de barramento e banco de dados local
    xSPI_Mutex = xSemaphoreCreateMutex();
    xLocalDB_Mutex = xSemaphoreCreateMutex();

    // 2. Inicializa o barramento SPI unificado (Compartilhado ST7789 e MFRC522)
    // TFT_SCLK: 18, RC522_MISO: 19, TFT_MOSI: 23. CS são controlados individualmente nos Adapters.
    SPI.begin(TFT_SCLK, RC522_MISO, TFT_MOSI, -1);

    // 3. Inicializa os Adapters de hardware básico
    buzzerAdapter = new Adapters::BuzzerAdapter(BUZZER_PIN);
    buzzerAdapter->init();
    buzzerAdapter->playBoot();

    lockAdapter = new Adapters::LockAdapter(LOCK_PIN);
    lockAdapter->init();

    storageAdapter = new Adapters::StorageAdapter();
    storageAdapter->init();

    // Cria o Software Timer do FreeRTOS para fechamento automático do trinco acionador
    xLockTimer = xTimerCreate(
        "LockTimer",
        pdMS_TO_TICKS(LOCK_UNLOCK_DURATION_MS),
        pdFALSE,                         // One-shot timer (dispara apenas uma vez por destravamento)
        (void *)lockAdapter,             // ID passará o ponteiro da fechadura
        Domain::AccessControlUseCase::lockTimerCallback
    );

    displayAdapter = new Adapters::DisplayAdapter(&displayDriver, TFT_BLK, xSPI_Mutex);
    displayAdapter->init();
    displayAdapter->showSplash();

    fingerprintAdapter = new Adapters::FingerprintAdapter(&fingerprintSerial, 0x00000000);
    fingerprintAdapter->init();

    rfidAdapter = new Adapters::RfidAdapter(RC522_CS, RC522_RST, xSPI_Mutex);
    rfidAdapter->init();

    supabaseAdapter = new Adapters::SupabaseAdapter(SUPABASE_URL, SUPABASE_ANON_KEY, *storageAdapter);
    supabaseAdapter->init();

    bleAdapter = new Adapters::BleAdapter();
    bleAdapter->init();

    // 4. Instancia os Use Cases
    accessControlUseCase = new Domain::AccessControlUseCase(
        *displayAdapter, *fingerprintAdapter, *rfidAdapter, 
        *lockAdapter, *buzzerAdapter, *storageAdapter, 
        *supabaseAdapter, xLockTimer
    );

    enrollUseCase = new Domain::EnrollUseCase(
        *displayAdapter, *fingerprintAdapter, *rfidAdapter,
        *storageAdapter, *supabaseAdapter, *bleAdapter,
        *buzzerAdapter
    );

    syncUseCase = new Domain::SyncUseCase(*storageAdapter, *supabaseAdapter);

    // Exibe a tela aguardando cartões ou digitais
    if (displayAdapter != nullptr) {
        displayAdapter->setWifiStatus(WiFi.status() == WL_CONNECTED);
        if (storageAdapter != nullptr) {
            displayAdapter->setSyncStatus(storageAdapter->getOfflineLogsCount() > 0);
        }
        displayAdapter->showWaitingAccess();
    }

    // 5. Instancia as Tasks do FreeRTOS
    // Core 1 (Lógica, UI e sensores locais)
    xTaskCreatePinnedToCore(
        vAuthTaskCode,
        "AuthTask",
        8192, // Stack Size em bytes (aumentado para 8KB devido às operações HTTPS/SSL)
        NULL,
        3,    // Prioridade Alta (Garante leitura rápida de digitais e RFID)
        &xAuthTask,
        1     // Core 1
    );

    /*
    xTaskCreatePinnedToCore(
        vSystemTaskCode,
        "SystemTask",
        2048,
        NULL,
        1,    // Prioridade Baixa
        &xSystemTask,
        1     // Core 1
    );
    */

    // Core 0 (Processamento de pilhas de rede WiFi, HTTPS/Supabase e BLE)
    xTaskCreatePinnedToCore(
        vBleTaskCode,
        "BleTask",
        8192, // Stack Size em bytes (aumentado para 8KB devido às operações HTTPS/SSL)
        NULL,
        2,    // Prioridade Média
        &xBleTask,
        0     // Core 0
    );

    xTaskCreatePinnedToCore(
        vSyncTaskCode,
        "SyncTask",
        8192, // WiFi e HTTPS exigem mais espaço de stack
        NULL,
        1,    // Prioridade Baixa (Executará em background)
        &xSyncTask,
        0     // Core 0
    );

    Serial.println("[System] FreeRTOS Task Scheduler iniciado!");
}

void loop() {
    // No FreeRTOS, a tarefa padrão do loop Arduino pode ser deletada para liberar ciclos de CPU
    vTaskDelete(NULL);
}
