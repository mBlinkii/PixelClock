#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "app_state.h"

void setup() {
  Serial.begin(115200);
  Serial.printf("PixelClock %s, Reset: %s\n", FIRMWARE_VERSION, resetReasonText());
  stateMutex = xSemaphoreCreateRecursiveMutex();
  if (!stateMutex) abort();
  keepFirmwareVersionBinaryMarker();
  bootStarted = millis();
  loadConfig();
  seedTimeFromBuild();
  if (authConfigMigrationNeeded) saveConfig();
  setupFastLed();
  renderDisplay();

  if (!LittleFS.begin(true, "/littlefs", 10, "littlefs")) {
    Serial.println("LittleFS mount failed");
  }

  if (!connectWifi()) {
    startSetupAp();
  }
  startMdns();
  pendingCityResolve = config.resolvedCityName != config.cityName;
  pendingTimeSync = true;
  pendingWeatherFetch = true;
  lastPageSwitch = millis();
  startNetworkWorker();
  setupServer();
}

void loop() {
  {
    StateLock lock;
    const uint32_t now = millis();
    if (displayTest && static_cast<int32_t>(now - displayTestUntil) >= 0) displayTest = false;
    if (pendingRestart && static_cast<int32_t>(now - restartAt) >= 0) ESP.restart();
    const uint8_t seconds = currentPage == 0 ? config.timePageSeconds : config.pageSeconds;
    if (config.autoPage && !displayTest && now - lastPageSwitch >= seconds * 1000UL) {
      lastPageSwitch = now;
      currentPage = (currentPage + 1) % 3;
      lastRender = 0;
    }
    if (!lastRender || now - lastRender >= displayRenderInterval()) {
      lastRender = now;
      renderDisplay();
    }
  }
  // Give the idle task and Wi-Fi power management time between display checks.
  delay(20);
}
