#include "app_state.h"

// Configuration is stored in ESP32 Preferences. Keep AppConfig defaults,
// loadConfig(), saveConfig(), sendConfigJson(), handleConfigPost(), and
// data/app.js in sync when adding or changing a setting.
String normalizeLanguage(String value) {
  value.trim();
  value.toLowerCase();
  return value == "en" ? "en" : DEFAULT_LANGUAGE;
}

String normalizeWifiCountry(String value) {
  static const char *supportedCountries[] = {
    "01", "AT", "AU", "BE", "BG", "BR", "CA", "CH", "CN", "CY", "CZ", "DE",
    "DK", "EE", "ES", "FI", "FR", "GB", "GR", "HK", "HR", "HU", "IE", "IN",
    "IS", "IT", "JP", "KR", "LI", "LT", "LU", "LV", "MT", "MX", "NL", "NO",
    "NZ", "PL", "PT", "RO", "SE", "SI", "SK", "TW", "US"
  };
  value.trim();
  value.toUpperCase();
  for (const char *country : supportedCountries) {
    if (value == country) return value;
  }
  return DEFAULT_WIFI_COUNTRY;
}

void loadConfig() {
  prefs.begin("pixel-clock", true);
  const uint8_t authConfigVersion = prefs.getUChar("authVer", 0);
  String plainAdminPassword;
  config.ssid = prefs.getString("ssid", "");
  config.password = prefs.getString("pass", "");
  if (authConfigVersion == AUTH_CONFIG_VERSION) {
    config.adminUsername = sanitizeHostname(prefs.getString("adminUser", config.adminUsername));
    config.adminPasswordSalt = prefs.getString("adminSalt", "");
    config.adminPasswordHash = prefs.getString("adminHash", "");
  } else if (authConfigVersion == 1) {
    // Firmware up to 0.1.17 stored the admin password in plain text.
    config.adminUsername = sanitizeHostname(prefs.getString("adminUser", config.adminUsername));
    plainAdminPassword = prefs.getString("admin", "");
    authConfigMigrationNeeded = true;
  } else {
    config.adminUsername = DEFAULT_ADMIN_USERNAME;
    authConfigMigrationNeeded = true;
  }
  config.setupApPassword = prefs.getString("apPass", config.setupApPassword);
  config.language = prefs.getString("lang", config.language);
  config.hostname = prefs.getString("host", config.hostname);
  config.cityName = prefs.getString("city", config.cityName);
  config.locationLabel = prefs.getString("locLabel", config.locationLabel);
  config.timezone = prefs.getString("tz", config.timezone);
  config.latitude = prefs.getFloat("lat", config.latitude);
  config.longitude = prefs.getFloat("lon", config.longitude);
  config.weatherProvider = prefs.getUChar("wProv", config.weatherProvider);
  config.weatherIntervalHalfHours = prefs.getUChar("wIntHalf", config.weatherIntervalHalfHours);
  config.weatherModel = prefs.getString("wModel", "");
  config.openWeatherApiKey = prefs.getString("owmKey", "");
  config.weatherApiKey = prefs.getString("waKey", "");
  config.resolvedCityName = prefs.getString("resolvedCity", "");
  config.wifiPowerSave = prefs.getBool("wifiPS", config.wifiPowerSave);
  config.width = prefs.getUChar("width", config.width);
  config.height = prefs.getUChar("height", config.height);
  config.dataPin = prefs.getUChar("pin", config.dataPin);
  config.brightness = prefs.getUChar("bright", config.brightness);
  config.fullBrightnessUnlocked = prefs.getBool("fullBright", config.fullBrightnessUnlocked);
  config.wiringMode = prefs.getUChar("wiring", 255);
  if (config.wiringMode == 255) {
    config.wiringMode = prefs.isKey("serp") ? (prefs.getBool("serp", true) ? 1 : 0) : DEFAULT_WIRING_MODE;
  }
  config.origin = prefs.getUChar("origin", config.origin);
  config.displayMode = prefs.getUChar("dispMode", config.displayMode);
  config.temperatureUnit = prefs.getUChar("tempUnit", config.temperatureUnit);
  config.weatherIconEnabled = prefs.getBool("tempIcon", config.weatherIconEnabled);
  config.hourFormat = prefs.getUChar("hourFmt", config.hourFormat);
  config.colorRgb = prefs.getBool("rgb", config.colorRgb);
  config.pageSeconds = prefs.getUChar("pageSec", config.pageSeconds);
  config.timePageSeconds = prefs.getUChar("timePageSec", config.timePageSeconds);
  config.autoPage = prefs.getBool("autoPage", config.autoPage);
  config.selectedPage = prefs.getUChar("selPage", config.selectedPage);
  config.nightBrightness = prefs.getUChar("nightB", config.nightBrightness);
  config.nightStart = prefs.getUChar("nightS", config.nightStart);
  config.nightEnd = prefs.getUChar("nightE", config.nightEnd);
  config.colorWeekday = prefs.getULong("colWeek", config.colorWeekday);
  config.colorText = prefs.getULong("colText", config.colorText);
  config.colorPoint = prefs.getULong("colPoint", config.colorPoint);
  config.colorColon = prefs.getULong("colColon", config.colorColon);
  config.colorGradientMode = prefs.getUChar("colGradM", 255);
  config.wifiCountry = prefs.getString("wifiCtry", config.wifiCountry);
  if (config.colorGradientMode == 255) config.colorGradientMode = prefs.getBool("colGrad", false) ? 1 : 0;
  prefs.end();

  if (authConfigVersion == 1 && !migratePlainAdminPassword(plainAdminPassword)) {
    config.adminPasswordSalt = "";
    config.adminPasswordHash = "";
  }
  plainAdminPassword = "";

  config.width = constrain(config.width, 8, 64);
  config.height = constrain(config.height, 8, 16);
  const uint8_t maxBrightness = config.fullBrightnessUnlocked ? 255 : SAFE_BRIGHTNESS_MAX;
  config.brightness = constrain(config.brightness, 0, maxBrightness);
  config.nightBrightness = constrain(config.nightBrightness, 0, maxBrightness);
  config.pageSeconds = constrain(config.pageSeconds, 3, 60);
  config.timePageSeconds = constrain(config.timePageSeconds, 3, 60);
  config.weatherProvider = constrain(config.weatherProvider, 0, WEATHER_PROVIDER_MAX);
  config.weatherIntervalHalfHours = constrain(config.weatherIntervalHalfHours, 1, 48);
  if (!isOpenMeteoModel(config.weatherModel.c_str())) config.weatherModel = "";
  config.wiringMode = constrain(config.wiringMode, 0, 3);
  config.origin = constrain(config.origin, 0, 3);
  config.displayMode = constrain(config.displayMode, 0, 2);
  config.temperatureUnit = constrain(config.temperatureUnit, 0, 1);
  config.colorGradientMode = constrain(config.colorGradientMode, 0, 2);
  if (config.hourFormat != 12) config.hourFormat = 24;
  config.language = normalizeLanguage(config.language);
  config.selectedPage = constrain(config.selectedPage, 0, 2);
  config.hostname = sanitizeHostname(config.hostname);
  config.adminUsername = sanitizeHostname(config.adminUsername);
  config.wifiCountry = normalizeWifiCountry(config.wifiCountry);
  if (config.adminUsername.isEmpty()) config.adminUsername = DEFAULT_ADMIN_USERNAME;
  if (!adminCredentialsValid()) {
    config.adminPasswordSalt = "";
    config.adminPasswordHash = "";
  }
  if (config.setupApPassword.length() < MIN_SETUP_AP_PASSWORD_LENGTH ||
      config.setupApPassword.length() > MAX_SETUP_AP_PASSWORD_LENGTH) {
    config.setupApPassword = DEFAULT_SETUP_AP_PASSWORD;
  }
}

DisplayPreviewFields captureDisplayPreviewFields(const AppConfig &source) {
  DisplayPreviewFields fields;
  fields.brightness = source.brightness;
  fields.nightBrightness = source.nightBrightness;
  fields.fullBrightnessUnlocked = source.fullBrightnessUnlocked;
  fields.displayMode = source.displayMode;
  fields.temperatureUnit = source.temperatureUnit;
  fields.weatherIconEnabled = source.weatherIconEnabled;
  fields.hourFormat = source.hourFormat;
  fields.colorWeekday = source.colorWeekday;
  fields.colorText = source.colorText;
  fields.colorPoint = source.colorPoint;
  fields.colorColon = source.colorColon;
  fields.colorGradientMode = source.colorGradientMode;
  fields.autoPage = source.autoPage;
  fields.selectedPage = source.selectedPage;
  fields.pageSeconds = source.pageSeconds;
  fields.timePageSeconds = source.timePageSeconds;
  return fields;
}

void applyDisplayPreviewFields(AppConfig &target, const DisplayPreviewFields &fields) {
  target.brightness = fields.brightness;
  target.nightBrightness = fields.nightBrightness;
  target.fullBrightnessUnlocked = fields.fullBrightnessUnlocked;
  target.displayMode = fields.displayMode;
  target.temperatureUnit = fields.temperatureUnit;
  target.weatherIconEnabled = fields.weatherIconEnabled;
  target.hourFormat = fields.hourFormat;
  target.colorWeekday = fields.colorWeekday;
  target.colorText = fields.colorText;
  target.colorPoint = fields.colorPoint;
  target.colorColon = fields.colorColon;
  target.colorGradientMode = fields.colorGradientMode;
  target.autoPage = fields.autoPage;
  target.selectedPage = fields.selectedPage;
  target.pageSeconds = fields.pageSeconds;
  target.timePageSeconds = fields.timePageSeconds;
}

// Ends a temporary display preview, optionally restoring the saved values.
void endDisplayPreview(bool restore) {
  StateLock lock;
  if (!displayPreviewActive) return;
  if (restore) applyDisplayPreviewFields(config, displayPreviewBackup);
  displayPreviewActive = false;
  lastRender = 0;
}

void saveConfig() {
  StateLock lock;
  // A running display preview is temporary; persist the values it replaced.
  AppConfig stored = config;
  if (displayPreviewActive) applyDisplayPreviewFields(stored, displayPreviewBackup);
  const AppConfig &c = stored;
  prefs.begin("pixel-clock", false);
  if (!prefs.isKey("ssid") || prefs.getString("ssid") != c.ssid) prefs.putString("ssid", c.ssid);
  if (!prefs.isKey("pass") || prefs.getString("pass") != c.password) prefs.putString("pass", c.password);
  if (!prefs.isKey("authVer") || prefs.getUChar("authVer") != AUTH_CONFIG_VERSION) prefs.putUChar("authVer", AUTH_CONFIG_VERSION);
  if (!prefs.isKey("adminUser") || prefs.getString("adminUser") != c.adminUsername) prefs.putString("adminUser", c.adminUsername);
  if (!prefs.isKey("adminSalt") || prefs.getString("adminSalt") != c.adminPasswordSalt) prefs.putString("adminSalt", c.adminPasswordSalt);
  if (!prefs.isKey("adminHash") || prefs.getString("adminHash") != c.adminPasswordHash) prefs.putString("adminHash", c.adminPasswordHash);
  if (prefs.isKey("admin")) prefs.remove("admin");
  if (!prefs.isKey("apPass") || prefs.getString("apPass") != c.setupApPassword) prefs.putString("apPass", c.setupApPassword);
  if (!prefs.isKey("lang") || prefs.getString("lang") != c.language) prefs.putString("lang", c.language);
  if (!prefs.isKey("wifiCtry") || prefs.getString("wifiCtry") != normalizeWifiCountry(c.wifiCountry)) prefs.putString("wifiCtry", normalizeWifiCountry(c.wifiCountry));
  if (!prefs.isKey("host") || prefs.getString("host") != c.hostname) prefs.putString("host", c.hostname);
  if (!prefs.isKey("city") || prefs.getString("city") != c.cityName) prefs.putString("city", c.cityName);
  if (!prefs.isKey("locLabel") || prefs.getString("locLabel") != c.locationLabel) prefs.putString("locLabel", c.locationLabel);
  if (!prefs.isKey("tz") || prefs.getString("tz") != c.timezone) prefs.putString("tz", c.timezone);
  if (!prefs.isKey("lat") || prefs.getFloat("lat") != c.latitude) prefs.putFloat("lat", c.latitude);
  if (!prefs.isKey("lon") || prefs.getFloat("lon") != c.longitude) prefs.putFloat("lon", c.longitude);
  if (!prefs.isKey("wProv") || prefs.getUChar("wProv") != c.weatherProvider) prefs.putUChar("wProv", c.weatherProvider);
  if (!prefs.isKey("wIntHalf") || prefs.getUChar("wIntHalf") != c.weatherIntervalHalfHours) prefs.putUChar("wIntHalf", c.weatherIntervalHalfHours);
  if (!prefs.isKey("wModel") || prefs.getString("wModel") != c.weatherModel) prefs.putString("wModel", c.weatherModel);
  if (!prefs.isKey("owmKey") || prefs.getString("owmKey") != c.openWeatherApiKey) prefs.putString("owmKey", c.openWeatherApiKey);
  if (!prefs.isKey("waKey") || prefs.getString("waKey") != c.weatherApiKey) prefs.putString("waKey", c.weatherApiKey);
  if (!prefs.isKey("resolvedCity") || prefs.getString("resolvedCity") != c.resolvedCityName) prefs.putString("resolvedCity", c.resolvedCityName);
  if (!prefs.isKey("wifiPS") || prefs.getBool("wifiPS") != c.wifiPowerSave) prefs.putBool("wifiPS", c.wifiPowerSave);
  if (!prefs.isKey("width") || prefs.getUChar("width") != c.width) prefs.putUChar("width", c.width);
  if (!prefs.isKey("height") || prefs.getUChar("height") != c.height) prefs.putUChar("height", c.height);
  if (!prefs.isKey("pin") || prefs.getUChar("pin") != c.dataPin) prefs.putUChar("pin", c.dataPin);
  if (!prefs.isKey("bright") || prefs.getUChar("bright") != c.brightness) prefs.putUChar("bright", c.brightness);
  if (!prefs.isKey("fullBright") || prefs.getBool("fullBright") != c.fullBrightnessUnlocked) prefs.putBool("fullBright", c.fullBrightnessUnlocked);
  if (!prefs.isKey("wiring") || prefs.getUChar("wiring") != c.wiringMode) prefs.putUChar("wiring", c.wiringMode);
  if (!prefs.isKey("origin") || prefs.getUChar("origin") != c.origin) prefs.putUChar("origin", c.origin);
  if (!prefs.isKey("dispMode") || prefs.getUChar("dispMode") != c.displayMode) prefs.putUChar("dispMode", c.displayMode);
  if (!prefs.isKey("tempUnit") || prefs.getUChar("tempUnit") != c.temperatureUnit) prefs.putUChar("tempUnit", c.temperatureUnit);
  if (!prefs.isKey("tempIcon") || prefs.getBool("tempIcon") != c.weatherIconEnabled) prefs.putBool("tempIcon", c.weatherIconEnabled);
  if (!prefs.isKey("hourFmt") || prefs.getUChar("hourFmt") != c.hourFormat) prefs.putUChar("hourFmt", c.hourFormat);
  if (!prefs.isKey("rgb") || prefs.getBool("rgb") != c.colorRgb) prefs.putBool("rgb", c.colorRgb);
  if (!prefs.isKey("pageSec") || prefs.getUChar("pageSec") != c.pageSeconds) prefs.putUChar("pageSec", c.pageSeconds);
  if (!prefs.isKey("timePageSec") || prefs.getUChar("timePageSec") != c.timePageSeconds) prefs.putUChar("timePageSec", c.timePageSeconds);
  if (!prefs.isKey("autoPage") || prefs.getBool("autoPage") != c.autoPage) prefs.putBool("autoPage", c.autoPage);
  if (!prefs.isKey("selPage") || prefs.getUChar("selPage") != c.selectedPage) prefs.putUChar("selPage", c.selectedPage);
  if (!prefs.isKey("nightB") || prefs.getUChar("nightB") != c.nightBrightness) prefs.putUChar("nightB", c.nightBrightness);
  if (!prefs.isKey("nightS") || prefs.getUChar("nightS") != c.nightStart) prefs.putUChar("nightS", c.nightStart);
  if (!prefs.isKey("nightE") || prefs.getUChar("nightE") != c.nightEnd) prefs.putUChar("nightE", c.nightEnd);
  if (!prefs.isKey("colWeek") || prefs.getULong("colWeek") != c.colorWeekday) prefs.putULong("colWeek", c.colorWeekday);
  if (!prefs.isKey("colText") || prefs.getULong("colText") != c.colorText) prefs.putULong("colText", c.colorText);
  if (!prefs.isKey("colPoint") || prefs.getULong("colPoint") != c.colorPoint) prefs.putULong("colPoint", c.colorPoint);
  if (!prefs.isKey("colColon") || prefs.getULong("colColon") != c.colorColon) prefs.putULong("colColon", c.colorColon);
  if (!prefs.isKey("colGradM") || prefs.getUChar("colGradM") != c.colorGradientMode) prefs.putUChar("colGradM", c.colorGradientMode);
  if (!prefs.isKey("colGrad") || prefs.getBool("colGrad") != (c.colorGradientMode != 0)) prefs.putBool("colGrad", c.colorGradientMode != 0);
  prefs.end();
}

CRGB packedColor(uint32_t value) {
  return CRGB((value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff);
}

String colorToHex(uint32_t value) {
  char buffer[8];
  snprintf(buffer, sizeof(buffer), "#%06lX", value & 0xffffffUL);
  return String(buffer);
}

uint32_t parseColor(String value, uint32_t fallback) {
  value.trim();
  if (value.startsWith("#")) value.remove(0, 1);
  if (value.length() != 6) return fallback;
  uint32_t result = 0;
  for (uint8_t i = 0; i < 6; i++) {
    const char c = value[i];
    result <<= 4;
    if (c >= '0' && c <= '9') result |= c - '0';
    else if (c >= 'a' && c <= 'f') result |= c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') result |= c - 'A' + 10;
    else return fallback;
  }
  return result;
}

String sanitizeHostname(String input) {
  input.trim();
  input.toLowerCase();
  String out;
  for (uint16_t i = 0; i < input.length() && out.length() < 31; i++) {
    const char c = input[i];
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') {
      out += c;
    }
  }
  while (out.startsWith("-")) out.remove(0, 1);
  while (out.endsWith("-")) out.remove(out.length() - 1);
  if (out.isEmpty()) out = "pixelclock";
  return out;
}
