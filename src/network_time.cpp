#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <sys/time.h>
#include <esp_sntp.h>
#include <atomic>

#include "app_state.h"

// Connectivity and clock setup.
static DNSServer dnsServer;
static uint32_t lastSetupApCheck = 0;
static uint32_t lastStationRetry = 0;
static uint32_t stationConnectedSince = 0;
static bool stationRetryPaused = false;
constexpr uint32_t SETUP_AP_CHECK_MS = 1000;
constexpr uint32_t SETUP_AP_CLOSE_DELAY_MS = 5000;
constexpr uint32_t STATION_RETRY_MS = 60000;

String deviceSuffix() {
  static char suffix[7] = {};
  if (!suffix[0]) {
    uint8_t mac[6] = {};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    formatDeviceSuffix(mac, suffix);
  }
  return String(suffix);
}

String routerHostname() {
  return "pixelclock-" + deviceSuffix();
}

String setupApSsid() {
  return SETUP_AP_SSID_PREFIX + deviceSuffix();
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
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  applyWifiPowerSave();
  applyWifiCountry();
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.begin(config.ssid.c_str(), config.password.c_str());
  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < WIFI_CONNECT_TIMEOUT_MS) {
    renderDisplay();
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
}

// The setup access point answers every DNS name with its own address, so
// phones and laptops open the web UI through their captive-portal check.
void startSetupAp() {
  setupMode = true;
  applyRouterHostname();
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  applyWifiCountry();
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.softAP(setupApSsid().c_str(), config.setupApPassword.c_str());
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", WiFi.softAPIP());
  lastStationRetry = millis();
  stationRetryPaused = false;
  Serial.printf("Setup AP %s on %s\n", setupApSsid().c_str(), WiFi.softAPIP().toString().c_str());
}

static void closeSetupAp() {
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.setAutoReconnect(true);
  applyWifiPowerSave();
  MDNS.end();
  startMdns();
  StateLock lock;
  setupMode = false;
  pendingTimeSync = true;
  pendingWeatherFetch = true;
  pendingCityResolve = config.resolvedCityName != config.cityName;
  lastRender = 0;
  Serial.printf("Wi-Fi connected as %s, setup AP closed\n", WiFi.localIP().toString().c_str());
}

// Runs from loop() while the setup AP is up. Station retries pause while a
// device uses the AP: ESP-IDF cannot scan while the station is connecting and
// channel hopping would disturb the configuring phone. Once the saved Wi-Fi
// works and nobody is connected to the AP, the AP closes without a restart.
void serviceSetupAp() {
  if (!setupMode) return;
  dnsServer.processNextRequest();
  const uint32_t now = millis();
  if (now - lastSetupApCheck < SETUP_AP_CHECK_MS) return;
  lastSetupApCheck = now;
  String ssid;
  String password;
  {
    StateLock lock;
    ssid = config.ssid;
    password = config.password;
  }
  if (ssid.isEmpty()) return;
  const uint8_t stations = WiFi.softAPgetStationNum();
  if (WiFi.status() == WL_CONNECTED) {
    if (stations) {
      stationConnectedSince = 0;
      return;
    }
    if (!stationConnectedSince) stationConnectedSince = now ? now : 1;
    if (now - stationConnectedSince >= SETUP_AP_CLOSE_DELAY_MS) closeSetupAp();
    return;
  }
  stationConnectedSince = 0;
  if (stations) {
    if (!stationRetryPaused) {
      WiFi.setAutoReconnect(false);
      WiFi.disconnect(false, false);
      stationRetryPaused = true;
    }
    return;
  }
  if (stationRetryPaused || now - lastStationRetry >= STATION_RETRY_MS) {
    stationRetryPaused = false;
    lastStationRetry = now;
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid.c_str(), password.c_str());
  }
}

void applyWifiPowerSave() {
  WiFi.setSleep(config.wifiPowerSave);
}

void startMdns() {
  if (MDNS.begin(config.hostname.c_str())) {
    MDNS.addService("http", "tcp", 80);
  }
}

static std::atomic<uint32_t> ntpSyncedAt{0};

void syncTime() {
  StateLock lock;
  if (WiFi.status() != WL_CONNECTED) return;
  lastNtpAttempt = millis();
  sntp_set_time_sync_notification_cb([](struct timeval *) {
    ntpSyncedAt.store(millis());
  });
  sntp_set_sync_interval(NTP_INTERVAL_MS);
  configTzTime(config.timezone.c_str(), "pool.ntp.org", "time.nist.gov");
}

uint32_t lastConfirmedNtpSync() {
  return ntpSyncedAt.load();
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
  setenv("TZ", config.timezone.c_str(), 1);
  tzset();
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
