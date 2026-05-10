#include <ESPmDNS.h>
#include <WiFi.h>
#include <sys/time.h>

#include "app_state.h"

// Connectivity and clock setup.
bool connectWifi() {
  if (config.ssid.isEmpty()) return false;
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(config.hostname.c_str());
  WiFi.begin(config.ssid.c_str(), config.password.c_str());
  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
}

void startSetupAp() {
  setupMode = true;
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(config.hostname.c_str());
  WiFi.softAP("PixelClock-Setup", "pixelclock");
}

void startMdns() {
  if (MDNS.begin(config.hostname.c_str())) {
    MDNS.addService("http", "tcp", 80);
  }
}

void syncTime() {
  if (WiFi.status() != WL_CONNECTED) return;
  lastNtpAttempt = millis();
  configTzTime(config.timezone.c_str(), "pool.ntp.org", "time.nist.gov");
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 5000)) {
    lastNtpSync = millis();
  }
}

bool timeIsReasonable() {
  time_t now = time(nullptr);
  return now > 1704067200L;
}

uint8_t buildMonth(const char *month) {
  static const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  const char *found = strstr(months, month);
  return found ? ((found - months) / 3) + 1 : 1;
}

void seedTimeFromBuild() {
  if (timeIsReasonable()) return;

  char monthText[4] = {};
  int day = 1;
  int year = 2026;
  int hour = 0;
  int minute = 0;
  int second = 0;
  sscanf(__DATE__, "%3s %d %d", monthText, &day, &year);
  sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

  struct tm buildTime = {};
  buildTime.tm_year = year - 1900;
  buildTime.tm_mon = buildMonth(monthText) - 1;
  buildTime.tm_mday = day;
  buildTime.tm_hour = hour;
  buildTime.tm_min = minute;
  buildTime.tm_sec = second;

  const time_t epoch = mktime(&buildTime);
  if (epoch > 1704067200L) {
    timeval tv = {epoch, 0};
    settimeofday(&tv, nullptr);
  }
}
