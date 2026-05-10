#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "app_state.h"

void setup() {
  Serial.begin(115200);
  bootStarted = millis();
  loadConfig();
  seedTimeFromBuild();
  if (authConfigMigrationNeeded) saveConfig();
  setupFastLed();

  if (!LittleFS.begin(true, "/littlefs", 10, "littlefs")) {
    Serial.println("LittleFS mount failed");
  }

  if (!connectWifi()) {
    startSetupAp();
  }
  startMdns();
  setupServer();

  resolveCity();
  syncTime();
  fetchWeather();
  lastPageSwitch = millis();
}

void loop() {
  const uint32_t now = millis();
  if (now - lastRender >= 200) {
    lastRender = now;
    renderDisplay();
  }
  if (config.autoPage && !displayTest && now - lastPageSwitch >= config.pageSeconds * 1000UL) {
    lastPageSwitch = now;
    currentPage = (currentPage + 1) % 3;
  }
  if (WiFi.status() == WL_CONNECTED && pendingCityResolve) {
    pendingCityResolve = false;
    if (resolveCity()) {
      pendingTimeSync = true;
      pendingWeatherFetch = true;
    }
  }
  if (WiFi.status() == WL_CONNECTED && pendingTimeSync) {
    pendingTimeSync = false;
    syncTime();
  }
  if (WiFi.status() == WL_CONNECTED && pendingWeatherFetch) {
    pendingWeatherFetch = false;
    fetchWeather();
  }
  const bool weatherDue = weather.lastFetch == 0
    ? (weather.lastAttempt == 0 || now - weather.lastAttempt >= WEATHER_RETRY_MS)
    : (now - weather.lastFetch >= config.weatherIntervalHalfHours * WEATHER_INTERVAL_STEP_MS);
  if (WiFi.status() == WL_CONNECTED && weatherDue) {
    fetchWeather();
  }
  const bool ntpDue = lastNtpSync == 0
    ? (lastNtpAttempt == 0 || now - lastNtpAttempt >= NTP_RETRY_MS)
    : (now - lastNtpSync >= NTP_INTERVAL_MS);
  if (WiFi.status() == WL_CONNECTED && ntpDue) {
    syncTime();
  }
}
