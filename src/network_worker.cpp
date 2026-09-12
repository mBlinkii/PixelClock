#include <WiFi.h>
#include "app_state.h"
#include "runtime_policy.h"

static void networkWorker(void *) {
  uint32_t lastCityAttempt = 0;
  uint32_t cityRevision = 0;
  for (;;) {
    bool cityDue = false;
    bool timeDue = false;
    bool weatherDue = false;
    {
      StateLock lock;
      const uint32_t now = millis();
      if (cityRevision != weatherRevision) {
        lastCityAttempt = 0;
        cityRevision = weatherRevision;
      }
      lastNtpSync = lastConfirmedNtpSync();
      if (!pendingRestart && WiFi.status() == WL_CONNECTED) {
        cityDue = pendingCityResolve &&
          (lastCityAttempt == 0 || now - lastCityAttempt >= WEATHER_RETRY_MS);
        if (cityDue) lastCityAttempt = now;
        timeDue = pendingTimeSync;
        pendingTimeSync = false;
        const bool cacheExpired = !weather.cacheUntil || deadlineReached(now, weather.cacheUntil);
        weatherDue = !pendingCityResolve && cacheExpired && (pendingWeatherFetch || weatherFetchDue(now,
          weather.lastFetch, weather.lastAttempt,
          config.weatherIntervalHalfHours * WEATHER_INTERVAL_STEP_MS,
          weather.retryAfterMs, !weather.lastError.isEmpty()));
        if (weatherDue) pendingWeatherFetch = false;
      }
    }
    if (timeDue) syncTime();
    if (cityDue && resolveCity()) {
      StateLock lock;
      lastCityAttempt = 0;
      weatherDue = false;
    }
    if (weatherDue) fetchWeather();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void startNetworkWorker() {
  networkWorkerReady = xTaskCreate(networkWorker, "weather", 12288, nullptr, 1, nullptr) == pdPASS;
  if (!networkWorkerReady) {
    weather.lastError = "Netzwerktask konnte nicht gestartet werden";
    Serial.println(weather.lastError);
  }
}
