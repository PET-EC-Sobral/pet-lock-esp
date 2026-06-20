#pragma once

// ==========================================
// CONFIGURAÇÕES DE REDE & SUPABASE
// ==========================================
#include "Secrets.h"

// Nomes das tabelas no Supabase (de acordo com database_model.md)
#define DB_TABLE_ADMINS "administrators"
#define DB_TABLE_USERS "lock_users"
#define DB_TABLE_FINGERPRINTS "fingerprint_credentials"
#define DB_TABLE_RFID_CARDS "rfid_credentials"
#define DB_TABLE_ACCESS_LOGS "access_logs"

// ==========================================
// CONFIGURAÇÕES DE HARDWARE & PINAGEM
// ==========================================

// Display IPS ST7789 (Conectado ao VSPI compartilhado)
#define TFT_CS 15   // Chip Select
#define TFT_DC 14   // Data/Command Selection
#define TFT_RST 4   // Reset Pin
#define TFT_MOSI 23 // Shared SPI MOSI
#define TFT_SCLK 18 // Shared SPI SCK
#define TFT_BLK 27  // Backlight control (PWM ou digital)

// RFID RC522 (Conectado ao VSPI compartilhado)
#define RC522_CS 5    // SDA / SS (Chip Select)
#define RC522_RST 22  // Reset Pin
#define RC522_MOSI 23 // Shared SPI MOSI
#define RC522_MISO 19 // SPI MISO
#define RC522_SCLK 18 // Shared SPI SCK

// Leitor de Digital Capacitivo ZW111 (UART2)
#define ZW_RX2 16       // Conecta no TX do sensor ZW111
#define ZW_TX2 17       // Conecta no RX do sensor ZW111
#define ZW_TOUCH_OUT -1 // Desabilitado para usar polling UART

// Atuadores & Sensores Físicos
#define BUZZER_PIN 33
#define LOCK_PIN 25       // Pino acionador do Relé da Fechadura (Ativo Alto)
#define WIPEBUTTON_PIN 26 // Botão físico para wipe manual (PULLUP interno recomendado)

// ==========================================
// CONFIGURAÇÕES DO SISTEMA & TEMPOS
// ==========================================
#define LOCK_UNLOCK_DURATION_MS 3000 // Tempo que a fechadura fica destravada
#define SYNC_INTERVAL_SEC 300        // Intervalo padrão de sincronização com o banco (5 minutos)
#define WIFI_TIMEOUT_MS 15000        // Timeout para conexão Wi-Fi
#define MAX_OFFLINE_LOGS 50          // Limite máximo de logs armazenados offline

// ==========================================
// CONFIGURAÇÕES DO BLUETOOTH BLE
// ==========================================
#define BLE_DEVICE_NAME "PET Lock"
#define BLE_SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // App -> Fechadura (Write)
#define BLE_CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // Fechadura -> App (Notify)
