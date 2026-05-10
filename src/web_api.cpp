#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <math.h>

#include "app_state.h"

// HTTP API used by the LittleFS web UI. Keep routes and response fields aligned
// with data/app.js.
void sendConfigJson(AsyncWebServerRequest *request) {
  if (!requireAdminAuth(request)) return;
  JsonDocument doc;
  doc["ssid"] = config.ssid;
  doc["hasPassword"] = !config.password.isEmpty();
  doc["adminUsername"] = config.adminUsername;
  doc["defaultAdminUsername"] = DEFAULT_ADMIN_USERNAME;
  doc["defaultAdminPassword"] = DEFAULT_ADMIN_PASSWORD;
  doc["adminPasswordSet"] = hasAdminPassword();
  doc["adminPasswordIsDefault"] = config.adminPassword == DEFAULT_ADMIN_PASSWORD;
  doc["minAdminPasswordLength"] = MIN_ADMIN_PASSWORD_LENGTH;
  doc["hostname"] = config.hostname;
  doc["url"] = "http://" + config.hostname + ".local";
  doc["cityName"] = config.cityName;
  doc["locationLabel"] = config.locationLabel;
  doc["timezone"] = config.timezone;
  doc["latitude"] = config.latitude;
  doc["longitude"] = config.longitude;
  doc["weatherProvider"] = config.weatherProvider;
  doc["weatherIntervalHalfHours"] = config.weatherIntervalHalfHours;
  doc["hasOpenWeatherApiKey"] = !config.openWeatherApiKey.isEmpty();
  doc["width"] = config.width;
  doc["height"] = config.height;
  doc["dataPin"] = config.dataPin;
  doc["brightness"] = config.brightness;
  doc["fullBrightnessUnlocked"] = config.fullBrightnessUnlocked;
  doc["wiringMode"] = config.wiringMode;
  doc["origin"] = config.origin;
  doc["displayMode"] = config.displayMode;
  doc["temperatureUnit"] = config.temperatureUnit;
  doc["hourFormat"] = config.hourFormat;
  doc["colorOrder"] = config.colorRgb ? "RGB" : "GRB";
  doc["pageSeconds"] = config.pageSeconds;
  doc["autoPage"] = config.autoPage;
  doc["selectedPage"] = config.selectedPage;
  doc["nightBrightness"] = config.nightBrightness;
  doc["nightStart"] = config.nightStart;
  doc["nightEnd"] = config.nightEnd;
  doc["colorWeekday"] = colorToHex(config.colorWeekday);
  doc["colorText"] = colorToHex(config.colorText);
  doc["colorPoint"] = colorToHex(config.colorPoint);
  doc["colorColon"] = colorToHex(config.colorColon);
  doc["colorGradientMode"] = config.colorGradientMode;
  String body;
  serializeJson(doc, body);
  request->send(200, "application/json", body);
}

String paramValue(AsyncWebServerRequest *request, const char *name, const String &fallback = "") {
  return request->hasParam(name, true) ? request->getParam(name, true)->value() : fallback;
}

bool hasAdminPassword() {
  return config.adminPassword.length() >= MIN_ADMIN_PASSWORD_LENGTH;
}

void sendJsonError(AsyncWebServerRequest *request, int code, const String &message) {
  JsonDocument doc;
  doc["ok"] = false;
  doc["error"] = message;
  String body;
  serializeJson(doc, body);
  request->send(code, "application/json", body);
}

bool requireAdminAuth(AsyncWebServerRequest *request) {
  if (!hasAdminPassword()) {
    sendJsonError(request, 403, "Admin-Passwort muss zuerst in den Einstellungen gesetzt werden.");
    return false;
  }
  if (request->authenticate(config.adminUsername.c_str(), config.adminPassword.c_str())) {
    return true;
  }
  request->requestAuthentication("Pixel Clock", false);
  return false;
}

void handleConfigPost(AsyncWebServerRequest *request) {
  if (!requireAdminAuth(request)) return;
  const String newAdminUsername = sanitizeHostname(paramValue(request, "adminUsername", config.adminUsername));
  const String newAdminPassword = paramValue(request, "adminPassword", "");
  if (newAdminPassword.length() > 0 && newAdminPassword.length() < MIN_ADMIN_PASSWORD_LENGTH) {
    sendJsonError(request, 400, "Das Admin-Passwort muss mindestens 8 Zeichen lang sein.");
    return;
  }

  const uint8_t oldWidth = config.width;
  const uint8_t oldHeight = config.height;
  const uint8_t oldDataPin = config.dataPin;
  const bool oldColorRgb = config.colorRgb;
  const String oldSsid = config.ssid;
  const String oldPassword = config.password;
  const String oldAdminUsername = config.adminUsername;
  const String oldAdminPassword = config.adminPassword;
  const String oldHostname = config.hostname;
  const String oldCityName = config.cityName;
  const String oldTimezone = config.timezone;
  const float oldLatitude = config.latitude;
  const float oldLongitude = config.longitude;
  const uint8_t oldWeatherProvider = config.weatherProvider;
  const String oldOpenWeatherApiKey = config.openWeatherApiKey;

  config.ssid = paramValue(request, "ssid", config.ssid);
  const String newPassword = paramValue(request, "password", "");
  if (newPassword.length() > 0) config.password = newPassword;
  config.adminUsername = newAdminUsername;
  if (newAdminPassword.length() > 0) config.adminPassword = newAdminPassword;
  config.hostname = sanitizeHostname(paramValue(request, "hostname", config.hostname));
  config.cityName = paramValue(request, "cityName", config.cityName);
  config.cityName.trim();
  config.weatherProvider = constrain(paramValue(request, "weatherProvider", String(config.weatherProvider)).toInt(), 0, 1);
  config.weatherIntervalHalfHours = constrain(paramValue(request, "weatherIntervalHalfHours", String(config.weatherIntervalHalfHours)).toInt(), 1, 48);
  const String newOpenWeatherApiKey = paramValue(request, "openWeatherApiKey", "");
  if (newOpenWeatherApiKey.length() > 0) config.openWeatherApiKey = newOpenWeatherApiKey;
  config.timezone = paramValue(request, "timezone", config.timezone);
  config.latitude = paramValue(request, "latitude", String(config.latitude, 5)).toFloat();
  config.longitude = paramValue(request, "longitude", String(config.longitude, 5)).toFloat();
  config.width = constrain(paramValue(request, "width", String(config.width)).toInt(), 8, 64);
  config.height = constrain(paramValue(request, "height", String(config.height)).toInt(), 8, 16);
  config.dataPin = paramValue(request, "dataPin", String(config.dataPin)).toInt();
  config.fullBrightnessUnlocked = paramValue(request, "fullBrightnessUnlocked", "0") == "1";
  const uint8_t maxBrightness = config.fullBrightnessUnlocked ? 255 : SAFE_BRIGHTNESS_MAX;
  config.brightness = constrain(paramValue(request, "brightness", String(config.brightness)).toInt(), 0, maxBrightness);
  config.wiringMode = constrain(paramValue(request, "wiringMode", String(config.wiringMode)).toInt(), 0, 3);
  config.origin = constrain(paramValue(request, "origin", String(config.origin)).toInt(), 0, 3);
  config.displayMode = constrain(paramValue(request, "displayMode", String(config.displayMode)).toInt(), 0, 2);
  config.temperatureUnit = constrain(paramValue(request, "temperatureUnit", String(config.temperatureUnit)).toInt(), 0, 1);
  config.hourFormat = paramValue(request, "hourFormat", String(config.hourFormat)).toInt() == 12 ? 12 : 24;
  config.colorRgb = paramValue(request, "colorOrder", "GRB") == "RGB";
  config.pageSeconds = constrain(paramValue(request, "pageSeconds", String(config.pageSeconds)).toInt(), 3, 60);
  config.autoPage = paramValue(request, "autoPage", "0") == "1";
  config.selectedPage = constrain(paramValue(request, "selectedPage", String(config.selectedPage)).toInt(), 0, 2);
  config.nightBrightness = constrain(paramValue(request, "nightBrightness", String(config.nightBrightness)).toInt(), 0, maxBrightness);
  config.nightStart = constrain(paramValue(request, "nightStart", String(config.nightStart)).toInt(), 0, 23);
  config.nightEnd = constrain(paramValue(request, "nightEnd", String(config.nightEnd)).toInt(), 0, 23);
  config.colorWeekday = parseColor(paramValue(request, "colorWeekday", colorToHex(config.colorWeekday)), config.colorWeekday);
  config.colorText = parseColor(paramValue(request, "colorText", colorToHex(config.colorText)), config.colorText);
  config.colorPoint = parseColor(paramValue(request, "colorPoint", colorToHex(config.colorPoint)), config.colorPoint);
  config.colorColon = parseColor(paramValue(request, "colorColon", colorToHex(config.colorColon)), config.colorColon);
  config.colorGradientMode = constrain(paramValue(request, "colorGradientMode", String(config.colorGradientMode)).toInt(), 0, 2);
  saveConfig();
  const bool cityResolveQueued = oldCityName != config.cityName;
  const bool weatherSourceChanged =
    oldCityName != config.cityName ||
    oldWeatherProvider != config.weatherProvider ||
    oldOpenWeatherApiKey != config.openWeatherApiKey ||
    fabs(oldLatitude - config.latitude) > 0.0001f ||
    fabs(oldLongitude - config.longitude) > 0.0001f;
  if (WiFi.status() == WL_CONNECTED) {
    pendingCityResolve = pendingCityResolve || cityResolveQueued;
    pendingTimeSync = pendingTimeSync || oldTimezone != config.timezone;
  }
  if (weatherSourceChanged && WiFi.status() == WL_CONNECTED) {
    weather.lastFetch = 0;
    pendingWeatherFetch = true;
  }
  ledCount = min<uint16_t>(MAX_LEDS, config.width * config.height);
  FastLED.setBrightness(config.brightness);
  lastPageSwitch = millis();
  currentPage = config.selectedPage;
  renderDisplay();
  const bool restartRequired =
    oldWidth != config.width ||
    oldHeight != config.height ||
    oldDataPin != config.dataPin ||
    oldColorRgb != config.colorRgb ||
    oldSsid != config.ssid ||
    oldPassword != config.password ||
    oldHostname != config.hostname;

  JsonDocument doc;
  doc["ok"] = true;
  doc["restartRequired"] = restartRequired;
  doc["cityResolutionPending"] = cityResolveQueued && WiFi.status() == WL_CONNECTED;
  doc["weatherRefreshPending"] = weatherSourceChanged && WiFi.status() == WL_CONNECTED;
  doc["adminPasswordSet"] = hasAdminPassword();
  doc["authChanged"] = oldAdminUsername != config.adminUsername || oldAdminPassword != config.adminPassword;
  doc["hostname"] = config.hostname;
  doc["url"] = "http://" + config.hostname + ".local";
  doc["locationLabel"] = config.locationLabel;
  String body;
  serializeJson(doc, body);
  request->send(200, "application/json", body);
}

void sendStatusJson(AsyncWebServerRequest *request) {
  if (!requireAdminAuth(request)) return;
  JsonDocument doc;
  doc["wifiConnected"] = WiFi.status() == WL_CONNECTED;
  doc["setupMode"] = setupMode;
  doc["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  doc["hostname"] = config.hostname;
  doc["url"] = "http://" + config.hostname + ".local";
  doc["cityName"] = config.cityName;
  doc["locationLabel"] = config.locationLabel;
  doc["weatherProvider"] = config.weatherProvider == 1 ? "OpenWeatherMap" : "Open-Meteo";
  doc["rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  doc["lastWeatherMs"] = weather.lastFetch;
  doc["lastWeatherAttemptMs"] = weather.lastAttempt;
  doc["weatherError"] = weather.lastError;
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
  String body;
  serializeJson(doc, body);
  request->send(200, "application/json", body);
}

void handleNetworks(AsyncWebServerRequest *request) {
  if (!requireAdminAuth(request)) return;
  JsonDocument doc;
  JsonArray arr = doc["networks"].to<JsonArray>();
  const int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    JsonObject item = arr.add<JsonObject>();
    item["ssid"] = WiFi.SSID(i);
    item["rssi"] = WiFi.RSSI(i);
    item["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
  }
  WiFi.scanDelete();
  String body;
  serializeJson(doc, body);
  request->send(200, "application/json", body);
}

void restartSoon(AsyncWebServerRequest *request) {
  if (!requireAdminAuth(request)) return;
  request->send(200, "application/json", "{\"ok\":true}");
  delay(300);
  ESP.restart();
}

void handleSettingsReset(AsyncWebServerRequest *request) {
  if (!requireAdminAuth(request)) return;
  const String keepSsid = config.ssid;
  const String keepPassword = config.password;
  const String keepAdminUsername = config.adminUsername;
  const String keepAdminPassword = config.adminPassword;
  prefs.begin("pixel-clock", false);
  prefs.clear();
  prefs.end();
  config = AppConfig();
  config.ssid = keepSsid;
  config.password = keepPassword;
  config.adminUsername = keepAdminUsername;
  config.adminPassword = keepAdminPassword;
  saveConfig();
  request->send(200, "application/json", "{\"ok\":true}");
  delay(300);
  ESP.restart();
}

void handleFactoryReset(AsyncWebServerRequest *request) {
  if (!requireAdminAuth(request)) return;
  prefs.begin("pixel-clock", false);
  prefs.clear();
  prefs.end();
  request->send(200, "application/json", "{\"ok\":true}");
  delay(300);
  ESP.restart();
}

void setupServer() {
  server.on("/api/config", HTTP_GET, sendConfigJson);
  server.on("/api/config", HTTP_POST, handleConfigPost);
  server.on("/api/status", HTTP_GET, sendStatusJson);
  server.on("/api/networks", HTTP_GET, handleNetworks);
  server.on("/api/restart", HTTP_POST, restartSoon);
  server.on("/api/reset/settings", HTTP_POST, handleSettingsReset);
  server.on("/api/reset/factory", HTTP_POST, handleFactoryReset);
  server.on("/api/weather/refresh", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!requireAdminAuth(request)) return;
    pendingWeatherFetch = true;
    request->send(200, "application/json", "{\"ok\":true}");
  });
  server.on("/api/display/test", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!requireAdminAuth(request)) return;
    displayTest = true;
    displayTestUntil = millis() + 10000;
    request->send(200, "application/json", "{\"ok\":true}");
  });
  server.serveStatic("/", LittleFS, "/")
    .setDefaultFile("index.html")
    .setAuthentication(config.adminUsername.c_str(), config.adminPassword.c_str(), AsyncAuthType::AUTH_BASIC);
  server.onNotFound([](AsyncWebServerRequest *request) {
    if (!requireAdminAuth(request)) return;
    request->send(404, "text/plain", "Not found");
  });
  server.begin();
}
