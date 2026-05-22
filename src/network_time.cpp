#include <ESPmDNS.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <sys/time.h>

#include "app_state.h"

// Connectivity and clock setup.
static String routerHostname() {
  uint8_t mac[6] = {};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char hostname[22] = {};
  snprintf(hostname, sizeof(hostname), "pixelclock-%02X%02X%02X", mac[3], mac[4], mac[5]);
  return String(hostname);
}

static void applyRouterHostname() {
  const String hostname = routerHostname();
  WiFi.setHostname(hostname.c_str());
}

static void applyWifiCountry() {
  const String country = normalizeWifiCountry(config.wifiCountry);
  char currentCountry[4] = {};
  if (esp_wifi_get_country_code(currentCountry) == ESP_OK &&
      currentCountry[0] == country[0] &&
      currentCountry[1] == country[1]) {
    return;
  }

  const esp_err_t err = esp_wifi_set_country_code(country.c_str(), true);
  if (err != ESP_OK) {
    Serial.printf("WiFi country %s failed: %d\n", country.c_str(), err);
  }
}

bool connectWifi() {
  if (config.ssid.isEmpty()) return false;
  applyRouterHostname();
  WiFi.mode(WIFI_STA);
  applyWifiCountry();
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.begin(config.ssid.c_str(), config.password.c_str());
  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
}

void startSetupAp() {
  setupMode = true;
  applyRouterHostname();
  WiFi.mode(WIFI_AP_STA);
  applyWifiCountry();
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
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
