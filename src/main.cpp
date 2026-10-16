#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "app_state.h"
#include "runtime_policy.h"

void setup() {
  Serial.begin(115200);
  Serial.printf("PixelClock %s, Reset: %s\n", FIRMWARE_VERSION, resetReasonText());
  stateMutex = xSemaphoreCreateRecursiveMutex();
  if (!stateMutex) abort();
  keepFirmwareVersionBinaryMarker();
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  bootStarted = millis();
  loadConfig();
  loadCalendarEvents();
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
  serviceSetupAp();
  serviceResetButton();
  {
    StateLock lock;
    const uint32_t now = millis();
    if (displayTest && static_cast<int32_t>(now - displayTestUntil) >= 0) displayTest = false;
    if (displayPreviewActive && deadlineReached(now, displayPreviewUntil)) endDisplayPreview(true);
    if (pendingRestart && static_cast<int32_t>(now - restartAt) >= 0) {
      if (pendingFactoryWipe) performFactoryWipe();
      ESP.restart();
    }
    if (config.autoPage && !displayTest && now - lastPageSwitch >= displayPageDurationMs(currentPage)) {
      lastPageSwitch = now;
      currentPage = nextDisplayPage(currentPage);
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
