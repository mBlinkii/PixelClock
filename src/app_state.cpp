#include "app_state.h"

Preferences prefs;
AsyncWebServer server(80);
AppConfig config;
WeatherState weather;
const char FIRMWARE_VERSION_BINARY_MARKER[] __attribute__((used)) =
  "PIXEL_CLOCK_FIRMWARE_VERSION=" FIRMWARE_VERSION_TEXT;

void keepFirmwareVersionBinaryMarker() {
  if (millis() == 0xFFFFFFFFUL) {
    Serial.print(FIRMWARE_VERSION_BINARY_MARKER);
  }
}

CRGB leds[MAX_LEDS];
uint16_t ledCount = DEFAULT_WIDTH * DEFAULT_HEIGHT;
uint8_t currentPage = 0;
uint32_t lastPageSwitch = 0;
uint32_t lastRender = 0;
uint32_t lastNtpSync = 0;
uint32_t lastNtpAttempt = 0;
bool displayTest = false;
uint32_t displayTestUntil = 0;
bool setupMode = false;
uint32_t bootStarted = 0;
bool forceTextGradient = false;
bool authConfigMigrationNeeded = false;
bool pendingCityResolve = false;
bool pendingWeatherFetch = false;
bool pendingTimeSync = false;
bool pendingRestart = false;
uint32_t restartAt = 0;
