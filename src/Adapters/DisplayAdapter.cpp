#include "DisplayAdapter.h"
#include <Arduino.h>

namespace Adapters {

DisplayAdapter::DisplayAdapter(Adafruit_ST7789 *tft, int blkPin, SemaphoreHandle_t spiMutex)
    : _tft(tft), _blkPin(blkPin), _spiMutex(spiMutex), 
      _isWifiConnected(false), _isBleConnected(false), _isSyncPending(false), _lastUpdate(0),
      _currentScreen(Screen::SPLASH) {}

void DisplayAdapter::init() {
    if (_blkPin >= 0) {
        pinMode(_blkPin, OUTPUT);
        digitalWrite(_blkPin, HIGH); // Liga o backlight
    }

    if (_spiMutex != NULL) xSemaphoreTake(_spiMutex, portMAX_DELAY);
    _tft->init(240, 320);
    _tft->setRotation(1); // Modo paisagem (320x240)
    _tft->fillScreen(PET_COLOR_BG);
    if (_spiMutex != NULL) xSemaphoreGive(_spiMutex);
}

void DisplayAdapter::drawHeader(const String& title) {
    // Fundo do cabeçalho
    _tft->fillRect(0, 0, 320, 40, PET_COLOR_BG);
    
    // Texto do Título
    _tft->setTextColor(PET_COLOR_TEXT);
    _tft->setTextSize(2);
    _tft->setCursor(15, 12);
    _tft->print(title);
    
    // Divisor
    _tft->drawFastHLine(0, 40, 320, PET_COLOR_CYAN);
    
    drawStatusBar();
}

void DisplayAdapter::drawStatusBar() {
    // Desenha ícones/status no canto superior direito
    int xStart = 220;
    
    // Apaga a área de status
    _tft->fillRect(xStart, 5, 95, 30, PET_COLOR_BG);
    
    // Símbolo Wi-Fi
    uint16_t wifiColor = _isWifiConnected ? PET_COLOR_GREEN : PET_COLOR_RED;
    drawWifiIcon(xStart + 10, 14, wifiColor);
    
    // Símbolo Bluetooth
    uint16_t bleColor = _isBleConnected ? PET_COLOR_GREEN : PET_COLOR_MUTED;
    drawBluetoothIcon(xStart + 45, 14, bleColor);
    
    // Símbolo de Sincronização pendente
    if (_isSyncPending) {
        _tft->fillCircle(xStart + 78, 20, 2, PET_COLOR_YELLOW);
        _tft->setTextSize(1);
        _tft->setTextColor(PET_COLOR_YELLOW);
        _tft->setCursor(xStart + 83, 17);
        _tft->print("S");
    }
}
 
static const unsigned char wifi_icon_bmp[] PROGMEM = {
    0x00, 0x00, 0x07, 0xE0, 0x18, 0x18, 0x20, 0x04, 0x03, 0xC0, 0x04, 0x20,
    0x00, 0x00, 0x01, 0x80, 0x02, 0x40, 0x00, 0x00, 0x01, 0x80, 0x01, 0x80
};

void DisplayAdapter::drawWifiIcon(int x, int y, uint16_t color) {
    _tft->drawBitmap(x, y, wifi_icon_bmp, 16, 12, color);
}

void DisplayAdapter::drawBluetoothIcon(int x, int y, uint16_t color) {
    _tft->drawLine(x + 4, y, x + 4, y + 12, color);
    _tft->drawLine(x + 4, y, x + 8, y + 3, color);
    _tft->drawLine(x + 8, y + 3, x + 1, y + 9, color);
    _tft->drawLine(x + 1, y + 3, x + 8, y + 9, color);
    _tft->drawLine(x + 8, y + 9, x + 4, y + 12, color);
}

void DisplayAdapter::showSplash() {
    _currentScreen = Screen::SPLASH;
    if (_spiMutex != NULL) xSemaphoreTake(_spiMutex, portMAX_DELAY);
    
    _tft->fillScreen(PET_COLOR_BG);
    
    // Borda decorativa
    _tft->drawRect(10, 10, 300, 220, PET_COLOR_CYAN);
    _tft->drawRect(12, 12, 296, 216, PET_COLOR_BG);
    
    // Texto Central
    _tft->setTextColor(PET_COLOR_GREEN);
    _tft->setTextSize(4);
    _tft->setCursor(65, 70);
    _tft->print("PET LOCK");
    
    _tft->setTextColor(PET_COLOR_TEXT);
    _tft->setTextSize(2);
    _tft->setCursor(105, 130);
    _tft->print("UFC Sobral");
    
    _tft->setTextColor(PET_COLOR_MUTED);
    _tft->setTextSize(1);
    _tft->setCursor(80, 170);
    _tft->print("Engenharia de Computacao");
    
    if (_spiMutex != NULL) xSemaphoreGive(_spiMutex);
    
    delay(2500);
}

void DisplayAdapter::showWaitingAccess() {
    _currentScreen = Screen::WAITING_ACCESS;

    if (_spiMutex != NULL) xSemaphoreTake(_spiMutex, portMAX_DELAY);
    
    _tft->fillScreen(PET_COLOR_BG);
    drawHeader("PET LOCK");
    
    // Instruções de aproximação
    _tft->setTextColor(PET_COLOR_TEXT);
    _tft->setTextSize(2);
    _tft->setCursor(45, 90);
    _tft->print("Aproxime o cartao");
    _tft->setCursor(60, 125);
    _tft->print("ou use a digital");
    
    // Barra de Status de Pronto
    _tft->fillRect(40, 180, 240, 35, 0x01E0); // Fundo verde escuro
    _tft->drawRect(40, 180, 240, 35, PET_COLOR_GREEN);
    _tft->setTextColor(PET_COLOR_GREEN);
    _tft->setTextSize(2);
    _tft->setCursor(85, 190);
    _tft->print("SISTEMA PRONTO");
    
    if (_spiMutex != NULL) xSemaphoreGive(_spiMutex);
}

void DisplayAdapter::showVerifying() {
    _currentScreen = Screen::VERIFYING;
    if (_spiMutex != NULL) xSemaphoreTake(_spiMutex, portMAX_DELAY);
    
    _tft->fillScreen(PET_COLOR_BG);
    
    _tft->setTextColor(PET_COLOR_YELLOW);
    _tft->setTextSize(3);
    _tft->setCursor(65, 100);
    _tft->print("Verificando...");
    
    if (_spiMutex != NULL) xSemaphoreGive(_spiMutex);
}

void DisplayAdapter::showAllowed(const String& userName) {
    _currentScreen = Screen::ALLOWED;
    if (_spiMutex != NULL) xSemaphoreTake(_spiMutex, portMAX_DELAY);
    
    _tft->fillScreen(PET_COLOR_BG);
    
    // Borda verde de sucesso
    _tft->drawRect(5, 5, 310, 230, PET_COLOR_GREEN);
    _tft->drawRect(6, 6, 308, 228, PET_COLOR_GREEN);
    
    _tft->setTextColor(PET_COLOR_GREEN);
    _tft->setTextSize(3);
    _tft->setCursor(40, 50);
    _tft->print("ACESSO LIBERADO");
    
    _tft->setTextColor(PET_COLOR_TEXT);
    _tft->setTextSize(2);
    if (userName.length() > 0) {
        _tft->setCursor(20, 110);
        _tft->print("Seja bem-vindo(a),");
        
        // Centraliza e exibe o nome
        int xPos = (320 - (userName.length() * 12)) / 2;
        if (xPos < 10) xPos = 10;
        _tft->setCursor(xPos, 140);
        _tft->setTextColor(PET_COLOR_CYAN);
        _tft->print(userName);
    } else {
        _tft->setCursor(85, 120);
        _tft->print("Seja bem-vindo!");
    }
    
    _tft->setTextColor(PET_COLOR_MUTED);
    _tft->setTextSize(1);
    _tft->setCursor(100, 195);
    _tft->print("Porta destravada...");
    
    if (_spiMutex != NULL) xSemaphoreGive(_spiMutex);
}

void DisplayAdapter::showDenied() {
    _currentScreen = Screen::DENIED;
    if (_spiMutex != NULL) xSemaphoreTake(_spiMutex, portMAX_DELAY);
    
    _tft->fillScreen(PET_COLOR_BG);
    
    // Borda vermelha de erro
    _tft->drawRect(5, 5, 310, 230, PET_COLOR_RED);
    _tft->drawRect(6, 6, 308, 228, PET_COLOR_RED);
    
    _tft->setTextColor(PET_COLOR_RED);
    _tft->setTextSize(3);
    _tft->setCursor(55, 75);
    _tft->print("ACESSO NEGADO");
    
    _tft->setTextColor(PET_COLOR_TEXT);
    _tft->setTextSize(2);
    _tft->setCursor(80, 135);
    _tft->print("Nao autorizado");
    
    if (_spiMutex != NULL) xSemaphoreGive(_spiMutex);
}

void DisplayAdapter::showEnrollFingerInstructions(uint8_t step, uint8_t fingerId, const String& userName) {
    _currentScreen = Screen::ENROLL_FINGER;
    if (_spiMutex != NULL) xSemaphoreTake(_spiMutex, portMAX_DELAY);
    
    _tft->fillScreen(PET_COLOR_BG);
    drawHeader("CADASTRO DIGITAL");
    
    _tft->setTextColor(PET_COLOR_TEXT);
    _tft->setTextSize(2);
    _tft->setCursor(15, 60);
    _tft->print("User: ");
    _tft->setTextColor(PET_COLOR_CYAN);
    _tft->print(userName);
    
    _tft->setTextColor(PET_COLOR_TEXT);
    _tft->setCursor(15, 95);
    _tft->print("Dedo: ");
    _tft->setTextColor(PET_COLOR_YELLOW);
    _tft->print("ID ");
    _tft->print(fingerId);
    
    _tft->fillRect(15, 135, 290, 80, 0x0A9F); // Fundo azul escuro ciano
    _tft->drawRect(15, 135, 290, 80, PET_COLOR_CYAN);
    
    _tft->setTextSize(1);
    _tft->setTextColor(PET_COLOR_TEXT);
    _tft->setCursor(30, 150);
    if (step == 1) {
        _tft->print("Encoste o dedo no leitor ZW111");
        _tft->setCursor(30, 170);
        _tft->setTextColor(PET_COLOR_CYAN);
        _tft->print("[Passo 1 de 2: Capturando]");
    } else {
        _tft->print("Remova e coloque o dedo");
        _tft->setCursor(30, 170);
        _tft->print("novamente no leitor");
        _tft->setCursor(30, 190);
        _tft->setTextColor(PET_COLOR_CYAN);
        _tft->print("[Passo 2 de 2: Confirmando]");
    }
    
    if (_spiMutex != NULL) xSemaphoreGive(_spiMutex);
}
 
void DisplayAdapter::showEnrollRfidInstructions(const String& userName) {
    _currentScreen = Screen::ENROLL_RFID;
    if (_spiMutex != NULL) xSemaphoreTake(_spiMutex, portMAX_DELAY);
    
    _tft->fillScreen(PET_COLOR_BG);
    drawHeader("CADASTRO RFID");
    
    _tft->setTextColor(PET_COLOR_TEXT);
    _tft->setTextSize(2);
    _tft->setCursor(15, 60);
    _tft->print("User: ");
    _tft->setTextColor(PET_COLOR_CYAN);
    _tft->print(userName);
    
    _tft->fillRect(15, 110, 290, 100, 0x0A9F);
    _tft->drawRect(15, 110, 290, 100, PET_COLOR_CYAN);
    
    _tft->setTextSize(2);
    _tft->setTextColor(PET_COLOR_TEXT);
    _tft->setCursor(30, 135);
    _tft->print("Aproxime o cartao");
    _tft->setCursor(30, 165);
    _tft->print("do sensor RC522...");
    
    if (_spiMutex != NULL) xSemaphoreGive(_spiMutex);
}
 
void DisplayAdapter::showEnrollSuccess() {
    _currentScreen = Screen::ENROLL_SUCCESS;
    if (_spiMutex != NULL) xSemaphoreTake(_spiMutex, portMAX_DELAY);
    
    _tft->fillScreen(PET_COLOR_BG);
    
    _tft->drawRect(5, 5, 310, 230, PET_COLOR_GREEN);
    
    _tft->setTextColor(PET_COLOR_GREEN);
    _tft->setTextSize(3);
    _tft->setCursor(55, 80);
    _tft->print("CADASTRADO!");
    
    _tft->setTextColor(PET_COLOR_TEXT);
    _tft->setTextSize(2);
    _tft->setCursor(30, 140);
    _tft->print("Credenciais salvas");
    
    if (_spiMutex != NULL) xSemaphoreGive(_spiMutex);
    delay(2000);
}
 
void DisplayAdapter::showEnrollFailed(const String& reason) {
    _currentScreen = Screen::ENROLL_FAILED;
    if (_spiMutex != NULL) xSemaphoreTake(_spiMutex, portMAX_DELAY);
    
    _tft->fillScreen(PET_COLOR_BG);
    
    _tft->drawRect(5, 5, 310, 230, PET_COLOR_RED);
    
    _tft->setTextColor(PET_COLOR_RED);
    _tft->setTextSize(3);
    _tft->setCursor(45, 60);
    _tft->print("FALHA CADASTRO");
    
    _tft->setTextColor(PET_COLOR_TEXT);
    _tft->setTextSize(2);
    _tft->setCursor(20, 120);
    _tft->print("Motivo:");
    
    _tft->setTextColor(PET_COLOR_YELLOW);
    _tft->setCursor(20, 150);
    _tft->print(reason);
    
    if (_spiMutex != NULL) xSemaphoreGive(_spiMutex);
    delay(2500);
}
 
void DisplayAdapter::showMessage(const String& line1, const String& line2) {
    _currentScreen = Screen::MESSAGE;
    if (_spiMutex != NULL) xSemaphoreTake(_spiMutex, portMAX_DELAY);
    
    _tft->fillScreen(PET_COLOR_BG);
    _tft->setTextColor(PET_COLOR_TEXT);
    _tft->setTextSize(2);
    
    // Centraliza na vertical
    _tft->setCursor(20, 90);
    _tft->print(line1);
    
    _tft->setCursor(20, 130);
    _tft->setTextColor(PET_COLOR_CYAN);
    _tft->print(line2);
    
    if (_spiMutex != NULL) xSemaphoreGive(_spiMutex);
}

void DisplayAdapter::setWifiStatus(bool connected) {
    if (_isWifiConnected != connected) {
        _isWifiConnected = connected;
        refreshStatusBar();
    }
}

void DisplayAdapter::setBleStatus(bool connected) {
    if (_isBleConnected != connected) {
        _isBleConnected = connected;
        refreshStatusBar();
    }
}

void DisplayAdapter::setSyncStatus(bool pending) {
    if (_isSyncPending != pending) {
        _isSyncPending = pending;
        refreshStatusBar();
    }
}

void DisplayAdapter::refreshStatusBar() {
    if (_currentScreen == Screen::WAITING_ACCESS || 
        _currentScreen == Screen::ENROLL_FINGER || 
        _currentScreen == Screen::ENROLL_RFID) {
        
        if (_spiMutex != NULL) xSemaphoreTake(_spiMutex, portMAX_DELAY);
        drawStatusBar();
        if (_spiMutex != NULL) xSemaphoreGive(_spiMutex);
    }
}

} // namespace Adapters
