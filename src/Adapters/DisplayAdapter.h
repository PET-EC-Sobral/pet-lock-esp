#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "../Core/Interfaces/IDisplay.h"

namespace Adapters {

// Cores Modernas e Premium (RGB565 convertidas de darkColors)
#define PET_COLOR_BG      0x0022 // background: #020617
#define PET_COLOR_TEXT    0xFFDF // foreground: #f8fafc
#define PET_COLOR_MUTED   0x9517 // mutedForeground: #94a3b8
#define PET_COLOR_GREEN   0x3EAD // primary: #3ed56f
#define PET_COLOR_CYAN    0x3EAD // ring/primary: #3ed56f (usado para bordas)
#define PET_COLOR_RED     0xFB8E // destructive: #f87171
#define PET_COLOR_YELLOW  0xED81 // warning: #eab308

class DisplayAdapter : public Domain::IDisplay {
public:
    enum class Screen {
        SPLASH,
        WAITING_ACCESS,
        VERIFYING,
        ALLOWED,
        DENIED,
        ENROLL_FINGER,
        ENROLL_RFID,
        ENROLL_SUCCESS,
        ENROLL_FAILED,
        MESSAGE
    };

private:
    Adafruit_ST7789 *_tft;
    int _blkPin;
    SemaphoreHandle_t _spiMutex;
    bool _isWifiConnected;
    bool _isBleConnected;
    bool _isSyncPending;
    String _lastUserAllowed;
    unsigned long _lastUpdate;
    Screen _currentScreen;

    void drawHeader(const String& title);
    void drawStatusBar();
    void drawWifiIcon(int x, int y, uint16_t color);
    void drawBluetoothIcon(int x, int y, uint16_t color);
    void refreshStatusBar();

public:
    DisplayAdapter(Adafruit_ST7789 *tft, int blkPin, SemaphoreHandle_t spiMutex);
    ~DisplayAdapter() override = default;

    void init() override;
    void showSplash() override;
    void showWaitingAccess() override;
    void showVerifying() override;
    void showAllowed(const String& userName) override;
    void showDenied() override;
    
    // Cadastro
    void showEnrollFingerInstructions(uint8_t step, uint8_t fingerId, const String& userName) override;
    void showEnrollRfidInstructions(const String& userName) override;
    void showEnrollSuccess() override;
    void showEnrollFailed(const String& reason) override;
    
    // Genérico
    void showMessage(const String& line1, const String& line2) override;

    // Status dinâmicos
    void setWifiStatus(bool connected) override;
    void setBleStatus(bool connected) override;
    void setSyncStatus(bool pending) override;
};

} // namespace Adapters
