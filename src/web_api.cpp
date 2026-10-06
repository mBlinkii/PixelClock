#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <math.h>

#include "app_state.h"
#include "web_updates.h"

static bool restartRequiredSinceBoot = false;

// HTTP API used by the LittleFS web UI. Keep routes and response fields aligned
// with data/app.js.
void sendConfigJson(AsyncWebServerRequest *request) {
  StateLock lock;
  if (!requireAdminAuth(request)) return;
  AppConfig c = config;
  if (displayPreviewActive) applyDisplayPreviewFields(c, displayPreviewBackup);
  JsonDocument doc;
  doc["ssid"] = c.ssid;
  doc["hasPassword"] = !c.password.isEmpty();
  doc["adminUsername"] = c.adminUsername;
  doc["defaultAdminUsername"] = DEFAULT_ADMIN_USERNAME;
  doc["adminPasswordSet"] = true;
  doc["adminPasswordIsDefault"] = adminPasswordIsDefault();
  doc["minAdminPasswordLength"] = MIN_ADMIN_PASSWORD_LENGTH;
  doc["maxAdminPasswordLength"] = MAX_ADMIN_PASSWORD_LENGTH;
  doc["setupApSsid"] = setupApSsid();
  doc["setupApPasswordIsDefault"] = c.setupApPassword == DEFAULT_SETUP_AP_PASSWORD;
  doc["minSetupApPasswordLength"] = MIN_SETUP_AP_PASSWORD_LENGTH;
  doc["maxSetupApPasswordLength"] = MAX_SETUP_AP_PASSWORD_LENGTH;
  doc["routerHostname"] = routerHostname();
  doc["language"] = c.language;
  doc["wifiCountry"] = c.wifiCountry;
  doc["hostname"] = c.hostname;
  doc["url"] = "http://" + c.hostname + ".local";
  doc["cityName"] = c.cityName;
  doc["locationLabel"] = c.locationLabel;
  doc["timezone"] = c.timezone;
  doc["latitude"] = c.latitude;
  doc["longitude"] = c.longitude;
  doc["weatherProvider"] = c.weatherProvider;
  doc["weatherIntervalHalfHours"] = c.weatherIntervalHalfHours;
  doc["weatherModel"] = c.weatherModel;
  doc["hasOpenWeatherApiKey"] = !c.openWeatherApiKey.isEmpty();
  doc["hasWeatherApiKey"] = !c.weatherApiKey.isEmpty();
  doc["weatherProviderMax"] = WEATHER_PROVIDER_MAX;
  doc["wifiPowerSave"] = c.wifiPowerSave;
  doc["width"] = c.width;
  doc["height"] = c.height;
  doc["dataPin"] = c.dataPin;
  doc["brightness"] = c.brightness;
  doc["fullBrightnessUnlocked"] = c.fullBrightnessUnlocked;
  doc["wiringMode"] = c.wiringMode;
  doc["origin"] = c.origin;
  doc["displayMode"] = c.displayMode;
  doc["temperatureUnit"] = c.temperatureUnit;
  doc["weatherIconEnabled"] = c.weatherIconEnabled;
  doc["hourFormat"] = c.hourFormat;
  doc["colorOrder"] = c.colorRgb ? "RGB" : "GRB";
  doc["pageSeconds"] = c.pageSeconds;
  doc["timePageSeconds"] = c.timePageSeconds;
  doc["autoPage"] = c.autoPage;
  doc["selectedPage"] = c.selectedPage;
  doc["nightBrightness"] = c.nightBrightness;
  doc["nightStart"] = c.nightStart;
  doc["nightEnd"] = c.nightEnd;
  doc["colorWeekday"] = colorToHex(c.colorWeekday);
  doc["colorText"] = colorToHex(c.colorText);
  doc["colorPoint"] = colorToHex(c.colorPoint);
  doc["colorColon"] = colorToHex(c.colorColon);
  doc["colorGradientMode"] = c.colorGradientMode;
  doc["restartRequired"] = restartRequiredSinceBoot;
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->addHeader("Cache-Control", "no-store");
  serializeJson(doc, *response);
  request->send(response);
}

String paramValue(AsyncWebServerRequest *request, const char *name, const String &fallback = "") {
  return request->hasParam(name, true) ? request->getParam(name, true)->value() : fallback;
}

const char *weatherProviderName() {
  switch (config.weatherProvider) {
    case WEATHER_PROVIDER_OPEN_WEATHER_MAP:
      return "OpenWeatherMap";
    case WEATHER_PROVIDER_MET_NORWAY:
      return "MET Norway";
    case WEATHER_PROVIDER_WEATHER_API:
      return "WeatherAPI";
    case WEATHER_PROVIDER_DWD:
      return "DWD (Bright Sky)";
    default:
      return "Open-Meteo";
  }
}

void sendJsonError(AsyncWebServerRequest *request, int code, const String &message) {
  JsonDocument doc;
  doc["ok"] = false;
  doc["error"] = message;
  String body;
  serializeJson(doc, body);
  request->send(code, "application/json", body);
}

// Visual settings shared by POST /api/config and POST /api/display/preview.
// Missing checkboxes mean "off", matching the full form the web UI sends.
static void readDisplayParams(AsyncWebServerRequest *request) {
  config.fullBrightnessUnlocked = paramValue(request, "fullBrightnessUnlocked", "0") == "1";
  const uint8_t maxBrightness = config.fullBrightnessUnlocked ? 255 : SAFE_BRIGHTNESS_MAX;
  config.brightness = constrain(paramValue(request, "brightness", String(config.brightness)).toInt(), 0, maxBrightness);
  config.nightBrightness = constrain(paramValue(request, "nightBrightness", String(config.nightBrightness)).toInt(), 0, maxBrightness);
  config.displayMode = constrain(paramValue(request, "displayMode", String(config.displayMode)).toInt(), 0, 2);
  config.temperatureUnit = constrain(paramValue(request, "temperatureUnit", String(config.temperatureUnit)).toInt(), 0, 1);
  config.weatherIconEnabled = paramValue(
    request,
    "weatherIconEnabled",
    config.weatherIconEnabled ? "1" : "0") == "1";
  config.hourFormat = paramValue(request, "hourFormat", String(config.hourFormat)).toInt() == 12 ? 12 : 24;
  config.pageSeconds = constrain(paramValue(request, "pageSeconds", String(config.pageSeconds)).toInt(), 3, 60);
  config.timePageSeconds = constrain(paramValue(request, "timePageSeconds", String(config.timePageSeconds)).toInt(), 3, 60);
  config.autoPage = paramValue(request, "autoPage", "0") == "1";
  config.selectedPage = constrain(paramValue(request, "selectedPage", String(config.selectedPage)).toInt(), 0, 2);
  config.colorWeekday = parseColor(paramValue(request, "colorWeekday", colorToHex(config.colorWeekday)), config.colorWeekday);
  config.colorText = parseColor(paramValue(request, "colorText", colorToHex(config.colorText)), config.colorText);
  config.colorPoint = parseColor(paramValue(request, "colorPoint", colorToHex(config.colorPoint)), config.colorPoint);
  config.colorColon = parseColor(paramValue(request, "colorColon", colorToHex(config.colorColon)), config.colorColon);
  config.colorGradientMode = constrain(paramValue(request, "colorGradientMode", String(config.colorGradientMode)).toInt(), 0, 2);
}

void scheduleRestart(uint32_t delayMs) {
  StateLock lock;
  pendingRestart = true;
  restartAt = millis() + delayMs;
}

void handleConfigPost(AsyncWebServerRequest *request) {
  StateLock lock;
  if (!requireAdminAuth(request)) return;
  const String newAdminUsername = sanitizeHostname(paramValue(request, "adminUsername", config.adminUsername));
  const String newAdminPassword = paramValue(request, "adminPassword", "");
  if (newAdminPassword.length() > 0 && newAdminPassword.length() < MIN_ADMIN_PASSWORD_LENGTH) {
    sendJsonError(request, 400, "Das Admin-Passwort muss mindestens 8 Zeichen lang sein.");
    return;
  }
  if (newAdminPassword.length() > MAX_ADMIN_PASSWORD_LENGTH) {
    sendJsonError(request, 400, "Das Admin-Passwort darf höchstens 64 Zeichen lang sein.");
    return;
  }
  const String newSetupApPassword = paramValue(request, "setupApPassword", "");
  if (newSetupApPassword.length() > 0 && (newSetupApPassword.length() < MIN_SETUP_AP_PASSWORD_LENGTH ||
                                          newSetupApPassword.length() > MAX_SETUP_AP_PASSWORD_LENGTH)) {
    sendJsonError(request, 400, "Das Setup-WLAN-Passwort muss 8 bis 63 Zeichen lang sein.");
    return;
  }

  if (request->hasParam("cityName", true) && paramValue(request, "cityName").length() < 2) {
    sendJsonError(request, 400, "Bitte eine Stadt mit mindestens 2 Zeichen eingeben.");
    return;
  }
  const uint8_t oldWidth = config.width;
  const uint8_t oldHeight = config.height;
  const uint8_t oldDataPin = config.dataPin;
  const bool oldColorRgb = config.colorRgb;
  const String oldSsid = config.ssid;
  const String oldPassword = config.password;
  const String oldWifiCountry = config.wifiCountry;
  const String oldAdminUsername = config.adminUsername;
  const String oldSetupApPassword = config.setupApPassword;
  const String oldHostname = config.hostname;
  const String oldCityName = config.cityName;
  const String oldTimezone = config.timezone;
  const float oldLatitude = config.latitude;
  const float oldLongitude = config.longitude;
  const uint8_t oldWeatherProvider = config.weatherProvider;
  const String oldWeatherModel = config.weatherModel;
  const String oldOpenWeatherApiKey = config.openWeatherApiKey;
  const String oldWeatherApiKey = config.weatherApiKey;

  // Hashing is the only step that can fail, so it runs before other changes.
  if (newAdminPassword.length() > 0 && !setAdminPassword(newAdminPassword)) {
    sendJsonError(request, 500, "Admin-Passwort konnte nicht gespeichert werden.");
    return;
  }
  config.ssid = paramValue(request, "ssid", config.ssid);
  const String newPassword = paramValue(request, "password", "");
  if (newPassword.length() > 0) config.password = newPassword;
  config.adminUsername = newAdminUsername;
  if (oldAdminUsername != config.adminUsername) invalidateAdminAuthCache();
  if (newSetupApPassword.length() > 0) config.setupApPassword = newSetupApPassword;
  config.language = normalizeLanguage(paramValue(request, "language", config.language));
  config.wifiCountry = normalizeWifiCountry(paramValue(request, "wifiCountry", config.wifiCountry));
  config.hostname = sanitizeHostname(paramValue(request, "hostname", config.hostname));
  config.cityName = paramValue(request, "cityName", config.cityName);
  config.cityName.trim();
  config.weatherProvider = constrain(paramValue(request, "weatherProvider", String(config.weatherProvider)).toInt(), 0, WEATHER_PROVIDER_MAX);
  config.weatherIntervalHalfHours = constrain(paramValue(request, "weatherIntervalHalfHours", String(config.weatherIntervalHalfHours)).toInt(), 1, 48);
  const String newWeatherModel = paramValue(request, "weatherModel", config.weatherModel);
  if (isOpenMeteoModel(newWeatherModel.c_str())) config.weatherModel = newWeatherModel;
  const String newOpenWeatherApiKey = paramValue(request, "openWeatherApiKey", "");
  if (newOpenWeatherApiKey.length() > 0) config.openWeatherApiKey = newOpenWeatherApiKey;
  const String newWeatherApiKey = paramValue(request, "weatherApiKey", "");
  if (!newWeatherApiKey.isEmpty()) config.weatherApiKey = newWeatherApiKey;
  config.wifiPowerSave = paramValue(request, "wifiPowerSave", config.wifiPowerSave ? "1" : "0") == "1";
  applyWifiPowerSave();
  config.timezone = paramValue(request, "timezone", config.timezone);
  config.latitude = paramValue(request, "latitude", String(config.latitude, 5)).toFloat();
  config.longitude = paramValue(request, "longitude", String(config.longitude, 5)).toFloat();
  config.width = constrain(paramValue(request, "width", String(config.width)).toInt(), 8, 64);
  config.height = constrain(paramValue(request, "height", String(config.height)).toInt(), 8, 16);
  config.dataPin = paramValue(request, "dataPin", String(config.dataPin)).toInt();
  config.wiringMode = constrain(paramValue(request, "wiringMode", String(config.wiringMode)).toInt(), 0, 3);
  config.origin = constrain(paramValue(request, "origin", String(config.origin)).toInt(), 0, 3);
  config.colorRgb = paramValue(request, "colorOrder", "GRB") == "RGB";
  config.nightStart = constrain(paramValue(request, "nightStart", String(config.nightStart)).toInt(), 0, 23);
  config.nightEnd = constrain(paramValue(request, "nightEnd", String(config.nightEnd)).toInt(), 0, 23);
  readDisplayParams(request);
  // The submitted values replace any running preview.
  endDisplayPreview(false);
  saveConfig();
  const bool cityResolveQueued = oldCityName != config.cityName;
  const bool weatherSourceChanged =
    oldCityName != config.cityName ||
    oldWeatherProvider != config.weatherProvider ||
    oldWeatherModel != config.weatherModel ||
    oldOpenWeatherApiKey != config.openWeatherApiKey ||
    oldWeatherApiKey != config.weatherApiKey ||
    fabs(oldLatitude - config.latitude) > 0.0001f ||
    fabs(oldLongitude - config.longitude) > 0.0001f;
  pendingCityResolve = pendingCityResolve || cityResolveQueued;
  pendingTimeSync = pendingTimeSync || oldTimezone != config.timezone;
  if (oldTimezone != config.timezone) {
    setenv("TZ", config.timezone.c_str(), 1);
    tzset();
  }
  if (weatherSourceChanged) {
    ++weatherRevision;
    weather = WeatherState();
    pendingWeatherFetch = true;
  }
  // FastLED's controller length/pin remains unchanged until the requested restart.
  lastPageSwitch = millis();
  currentPage = config.selectedPage;
  lastRender = 0;
  // Credentials are checked live, so a login change needs no restart.
  const bool authChanged = oldAdminUsername != config.adminUsername || newAdminPassword.length() > 0;
  const bool restartRequired =
    oldWidth != config.width ||
    oldHeight != config.height ||
    oldDataPin != config.dataPin ||
    oldColorRgb != config.colorRgb ||
    oldSsid != config.ssid ||
    oldPassword != config.password ||
    oldWifiCountry != config.wifiCountry ||
    oldHostname != config.hostname ||
    (setupMode && oldSetupApPassword != config.setupApPassword);

  JsonDocument doc;
  doc["ok"] = true;
  restartRequiredSinceBoot = restartRequiredSinceBoot || restartRequired;
  doc["restartRequired"] = restartRequiredSinceBoot;
  doc["cityResolutionPending"] = cityResolveQueued;
  doc["weatherRefreshPending"] = weatherSourceChanged;
  doc["adminPasswordSet"] = true;
  doc["adminPasswordIsDefault"] = adminPasswordIsDefault();
  doc["setupApPasswordIsDefault"] = config.setupApPassword == DEFAULT_SETUP_AP_PASSWORD;
  doc["authChanged"] = authChanged;
  doc["hostname"] = config.hostname;
  doc["url"] = "http://" + config.hostname + ".local";
  doc["locationLabel"] = config.locationLabel;
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->addHeader("Cache-Control", "no-store");
  serializeJson(doc, *response);
  request->send(response);
}

void handleLanguagePost(AsyncWebServerRequest *request) {
  StateLock lock;
  if (!requireAdminAuth(request)) return;
  config.language = normalizeLanguage(paramValue(request, "language", config.language));
  saveConfig();
  lastRender = 0;

  JsonDocument doc;
  doc["ok"] = true;
  doc["language"] = config.language;
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->addHeader("Cache-Control", "no-store");
  serializeJson(doc, *response);
  request->send(response);
}

void sendStatusJson(AsyncWebServerRequest *request) {
  StateLock lock;
  if (!requireAdminAuth(request)) return;
  JsonDocument doc;
  doc["wifiConnected"] = WiFi.status() == WL_CONNECTED;
  doc["setupMode"] = setupMode;
  doc["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  doc["hostname"] = config.hostname;
  doc["url"] = "http://" + config.hostname + ".local";
  doc["cityName"] = config.cityName;
  doc["locationLabel"] = config.locationLabel;
  // Lets the web UI show the found place (or why the lookup failed) next to the input.
  doc["locationPending"] = pendingCityResolve;
  if (pendingCityResolve && !weather.lastError.isEmpty()) doc["locationError"] = weather.lastError;
  doc["language"] = config.language;
  doc["firmwareVersion"] = FIRMWARE_VERSION;
  doc["weatherProvider"] = weatherProviderName();
  if (config.weatherProvider == WEATHER_PROVIDER_OPEN_METEO) doc["weatherModel"] = config.weatherModel;
  doc["rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  doc["lastWeatherMs"] = weather.lastFetch;
  doc["lastWeatherAttemptMs"] = weather.lastAttempt;
  doc["weatherError"] = weather.lastError;
  doc["weatherBusy"] = weather.busy || ((pendingWeatherFetch || pendingCityResolve) && weather.lastError.isEmpty());
  if (weather.lastFetch) doc["weatherAgeMs"] = millis() - weather.lastFetch;
  else doc["weatherAgeMs"] = nullptr;
  doc["weatherFetchDurationMs"] = weather.fetchDurationMs;
  doc["uptimeMs"] = millis();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["minFreeHeap"] = ESP.getMinFreeHeap();
  doc["resetReason"] = resetReasonText();
  doc["networkStackFreeBytes"] = networkWorkerStackFree();
  doc["wifiPowerSave"] = config.wifiPowerSave;
  doc["networkWorkerReady"] = networkWorkerReady;
  doc["setupApSsid"] = setupApSsid();
  doc["routerHostname"] = routerHostname();
  doc["displayPreviewActive"] = displayPreviewActive;
  JsonObject capabilities = doc["capabilities"].to<JsonObject>();
  capabilities["asyncWifiScan"] = true;
  capabilities["weatherProviderMax"] = WEATHER_PROVIDER_MAX;
  capabilities["wifiPowerSave"] = true;
  capabilities["captivePortal"] = true;
  capabilities["setupApPassword"] = true;
  capabilities["setupTestPattern"] = true;
  capabilities["displayFrame"] = true;
  capabilities["displayPreview"] = true;
  capabilities["fullFactoryReset"] = true;
  capabilities["resetButton"] = true;
  capabilities["loginThrottle"] = true;
  capabilities["weatherModel"] = true;
  capabilities["openSetup"] = true;
  capabilities["passwordRecovery"] = true;
  capabilities["locationResult"] = true;
  if (isnan(weather.temperature)) {
    doc["temperature"] = nullptr;
  } else {
    doc["temperature"] = displayTemperature(weather.temperature);
  }
  if (isnan(weather.temperatureMin)) {
    doc["temperatureMin"] = nullptr;
  } else {
    doc["temperatureMin"] = displayTemperature(weather.temperatureMin);
  }
  if (isnan(weather.temperatureMax)) {
    doc["temperatureMax"] = nullptr;
  } else {
    doc["temperatureMax"] = displayTemperature(weather.temperatureMax);
  }
  doc["temperatureUnit"] = temperatureUnitText();
  doc["weatherCode"] = weather.weatherCode;
  doc["lastNtpMs"] = lastNtpSync;
  doc["lastNtpAttemptMs"] = lastNtpAttempt;
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 10)) {
    char now[24];
    strftime(now, sizeof(now), "%Y-%m-%d %H:%M:%S", &timeinfo);
    doc["localTime"] = now;
  }
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->addHeader("Cache-Control", "no-store");
  serializeJson(doc, *response);
  request->send(response);
}

void handleNetworks(AsyncWebServerRequest *request) {
  StateLock lock;
  if (!requireAdminAuth(request)) return;
  JsonDocument doc;
  JsonArray arr = doc["networks"].to<JsonArray>();
  static uint32_t scanStarted = 0;
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) {
    doc["scanning"] = true;
  } else if (n == WIFI_SCAN_FAILED) {
    if (scanStarted && millis() - scanStarted < 15000) {
      sendJsonError(request, 503, "WLAN-Suche fehlgeschlagen. Bitte erneut versuchen.");
      return;
    }
    scanStarted = millis();
    n = WiFi.scanNetworks(true);
    doc["scanning"] = n == WIFI_SCAN_RUNNING;
  }
  for (int i = 0; i < min(n, 32); i++) {
    JsonObject item = arr.add<JsonObject>();
    item["ssid"] = WiFi.SSID(i);
    item["rssi"] = WiFi.RSSI(i);
    item["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
  }
  if (n >= 0) {
    WiFi.scanDelete();
    scanStarted = 0;
  }
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->addHeader("Cache-Control", "no-store");
  serializeJson(doc, *response);
  request->send(response);
}

void restartSoon(AsyncWebServerRequest *request) {
  StateLock lock;
  if (!requireAdminAuth(request)) return;
  request->send(200, "application/json", "{\"ok\":true}");
  scheduleRestart();
}

void handleSettingsReset(AsyncWebServerRequest *request) {
  StateLock lock;
  if (!requireAdminAuth(request)) return;
  const String keepSsid = config.ssid;
  const String keepPassword = config.password;
  const String keepWifiCountry = config.wifiCountry;
  const String keepAdminUsername = config.adminUsername;
  const String keepAdminSalt = config.adminPasswordSalt;
  const String keepAdminHash = config.adminPasswordHash;
  const String keepSetupApPassword = config.setupApPassword;
  endDisplayPreview(false);
  prefs.begin("pixel-clock", false);
  prefs.clear();
  prefs.end();
  config = AppConfig();
  config.ssid = keepSsid;
  config.password = keepPassword;
  config.wifiCountry = keepWifiCountry;
  config.adminUsername = keepAdminUsername;
  config.adminPasswordSalt = keepAdminSalt;
  config.adminPasswordHash = keepAdminHash;
  config.setupApPassword = keepSetupApPassword;
  saveConfig();
  request->send(200, "application/json", "{\"ok\":true}");
  scheduleRestart();
}

// Full factory reset for handing the clock on; the NVS erase itself runs in
// loop() right before the restart so no handle is open (performFactoryWipe()).
void handleFactoryReset(AsyncWebServerRequest *request) {
  StateLock lock;
  if (!requireAdminAuth(request)) return;
  endDisplayPreview(false);
  pendingFactoryWipe = true;
  lastRender = 0;
  JsonDocument doc;
  doc["ok"] = true;
  doc["fullWipe"] = true;
  doc["setupApSsid"] = setupApSsid();
  String body;
  serializeJson(doc, body);
  request->send(200, "application/json", body);
  scheduleRestart(1500);
}

// Mirrors the LEDs for the browser: pixels are the last frame sent to the
// matrix in physical LED order as RRGGBB hex, before brightness scaling.
void sendDisplayFrame(AsyncWebServerRequest *request) {
  StateLock lock;
  if (!requireAdminAuth(request)) return;
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->addHeader("Cache-Control", "no-store");
  response->printf(
    "{\"width\":%u,\"height\":%u,\"count\":%u,\"origin\":%u,\"wiring\":%u,\"brightness\":%u,\"preview\":%s,\"pixels\":\"",
    config.width, config.height, ledCount, config.origin, config.wiringMode, FastLED.getBrightness(),
    displayPreviewActive ? "true" : "false");
  char hex[7];
  for (uint16_t i = 0; i < ledCount; i++) {
    snprintf(hex, sizeof(hex), "%02x%02x%02x", displayFrame[i].r, displayFrame[i].g, displayFrame[i].b);
    response->print(hex);
  }
  response->print("\"}");
  request->send(response);
}

// Applies visual settings immediately without saving them. They revert after
// DISPLAY_PREVIEW_MS unless the full form is saved; saveConfig() keeps
// persisting the previous values meanwhile.
void handleDisplayPreview(AsyncWebServerRequest *request) {
  StateLock lock;
  if (!requireAdminAuth(request)) return;
  if (!displayPreviewActive) {
    displayPreviewBackup = captureDisplayPreviewFields(config);
    displayPreviewActive = true;
  }
  readDisplayParams(request);
  displayPreviewUntil = millis() + DISPLAY_PREVIEW_MS;
  if (!config.autoPage) currentPage = config.selectedPage;
  lastRender = 0;
  JsonDocument doc;
  doc["ok"] = true;
  doc["previewSeconds"] = DISPLAY_PREVIEW_MS / 1000;
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->addHeader("Cache-Control", "no-store");
  serializeJson(doc, *response);
  request->send(response);
}

void handleDisplayPreviewCancel(AsyncWebServerRequest *request) {
  StateLock lock;
  if (!requireAdminAuth(request)) return;
  endDisplayPreview(true);
  request->send(200, "application/json", "{\"ok\":true}");
}

// Captive portal: while the setup AP runs, requests for foreign host names
// (connectivity checks of phones and laptops) are redirected to the web UI.
static bool isIpAddressHost(const String &host) {
  if (host.isEmpty()) return false;
  for (size_t i = 0; i < host.length(); i++) {
    const char c = host[i];
    if (!((c >= '0' && c <= '9') || c == '.' || c == ':' || c == '[' || c == ']')) return false;
  }
  return true;
}

static String setupPortalUrl() {
  return "http://" + WiFi.softAPIP().toString() + "/";
}

// Only clients of the setup AP are redirected, never requests from the home
// network while the station is already connected.
static bool arrivedOnSetupAp(AsyncWebServerRequest *request) {
  AsyncClient *client = request->client();
  return setupMode && client && client->localIP() == WiFi.softAPIP();
}

class CaptivePortalRedirect : public AsyncWebHandler {
 public:
  bool canHandle(AsyncWebServerRequest *request) const override {
    StateLock lock;
    if (!arrivedOnSetupAp(request)) return false;
    String host = request->host();
    const int port = host.lastIndexOf(':');
    if (port > 0 && host.indexOf(']') < 0) host = host.substring(0, port);
    host.toLowerCase();
    return !host.isEmpty() && !isIpAddressHost(host) && host != config.hostname + ".local";
  }

  void handleRequest(AsyncWebServerRequest *request) override {
    request->redirect(setupPortalUrl());
  }
};

// Public: tells the web UI whether the clock is new (no Wi-Fi saved and the
// default admin password), so it can start the assistant without a login.
void sendSetupState(AsyncWebServerRequest *request) {
  StateLock lock;
  JsonDocument doc;
  const bool firstSetup = config.ssid.isEmpty() && adminPasswordIsDefault();
  doc["firstSetup"] = firstSetup;
  if (firstSetup) doc["adminUsername"] = config.adminUsername;
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->addHeader("Cache-Control", "no-store");
  serializeJson(doc, *response);
  request->send(response);
}

void setupServer() {
  // Must stay the first handler so it sees captive-portal checks before the file server.
  server.addHandler(new CaptivePortalRedirect());
  server.on("/api/setup", HTTP_GET, sendSetupState);
  // Public, guarded by the code on the matrix and the login throttle.
  server.on("/api/recovery/start", HTTP_POST, handleRecoveryStart);
  server.on("/api/recovery/finish", HTTP_POST, handleRecoveryFinish);
  server.on("/api/config", HTTP_GET, sendConfigJson);
  server.on("/api/config", HTTP_POST, handleConfigPost);
  server.on("/api/language", HTTP_POST, handleLanguagePost);
  server.on("/api/status", HTTP_GET, sendStatusJson);
  server.on("/api/networks", HTTP_GET, handleNetworks);
  server.on("/api/restart", HTTP_POST, restartSoon);
  server.on("/api/reset/settings", HTTP_POST, handleSettingsReset);
  server.on("/api/reset/factory", HTTP_POST, handleFactoryReset);
  server.on("/api/display/frame", HTTP_GET, sendDisplayFrame);
  server.on("/api/display/preview/cancel", HTTP_POST, handleDisplayPreviewCancel);
  server.on("/api/display/preview", HTTP_POST, handleDisplayPreview);
  server.on("/api/update/firmware", HTTP_POST, handleFirmwareUpdateDone, handleFirmwareUpdateUpload);
  server.on("/api/update/web", HTTP_POST, handleWebUpdateDone, handleWebUpdateUpload);
  server.on("/api/weather/refresh", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!requireAdminAuth(request)) return;
    StateLock lock;
    const uint32_t now = millis();
    if (weather.busy || (weather.lastAttempt && now - weather.lastAttempt < 30000) ||
        (weather.cacheUntil && static_cast<int32_t>(now - weather.cacheUntil) < 0) ||
        (!weather.lastError.isEmpty() && weather.lastAttempt && now - weather.lastAttempt < weather.retryAfterMs)) {
      sendJsonError(request, 429, "Bitte vor der nächsten Wetterabfrage kurz warten.");
      return;
    }
    pendingWeatherFetch = true;
    request->send(200, "application/json", "{\"ok\":true}");
  });
  server.on("/api/display/test", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!requireAdminAuth(request)) return;
    StateLock lock;
    displayTest = true;
    lastRender = 0;
    displayTestUntil = millis() + 10000;
    request->send(200, "application/json", "{\"ok\":true}");
  });
  server.serveStatic("/", LittleFS, "/")
    .setDefaultFile("index.html")
    .setCacheControl("no-cache");
  server.onNotFound([](AsyncWebServerRequest *request) {
    bool redirectToSetup;
    {
      StateLock lock;
      redirectToSetup = arrivedOnSetupAp(request);
    }
    if (redirectToSetup) {
      request->redirect(setupPortalUrl());
      return;
    }
    if (!requireAdminAuth(request)) return;
    request->send(404, "text/plain", "Not found");
  });
  server.begin();
}
