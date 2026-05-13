#include "app_state.h"

// Configuration is stored in ESP32 Preferences. Keep AppConfig defaults,
// loadConfig(), saveConfig(), sendConfigJson(), handleConfigPost(), and
// data/app.js in sync when adding or changing a setting.
void loadConfig() {
  prefs.begin("pixel-clock", true);
  const uint8_t authConfigVersion = prefs.getUChar("authVer", 0);
  config.ssid = prefs.getString("ssid", "");
  config.password = prefs.getString("pass", "");
  if (authConfigVersion == AUTH_CONFIG_VERSION) {
    config.adminUsername = sanitizeHostname(prefs.getString("adminUser", config.adminUsername));
    config.adminPassword = prefs.getString("admin", config.adminPassword);
  } else {
    config.adminUsername = DEFAULT_ADMIN_USERNAME;
    config.adminPassword = DEFAULT_ADMIN_PASSWORD;
    authConfigMigrationNeeded = true;
  }
  config.hostname = prefs.getString("host", config.hostname);
  config.cityName = prefs.getString("city", config.cityName);
  config.locationLabel = prefs.getString("locLabel", config.locationLabel);
  config.timezone = prefs.getString("tz", config.timezone);
  config.latitude = prefs.getFloat("lat", config.latitude);
  config.longitude = prefs.getFloat("lon", config.longitude);
  config.weatherProvider = prefs.getUChar("wProv", config.weatherProvider);
  config.weatherIntervalHalfHours = prefs.getUChar("wIntHalf", config.weatherIntervalHalfHours);
  config.openWeatherApiKey = prefs.getString("owmKey", "");
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
  if (config.colorGradientMode == 255) config.colorGradientMode = prefs.getBool("colGrad", false) ? 1 : 0;
  prefs.end();

  config.width = constrain(config.width, 8, 64);
  config.height = constrain(config.height, 8, 16);
  const uint8_t maxBrightness = config.fullBrightnessUnlocked ? 255 : SAFE_BRIGHTNESS_MAX;
  config.brightness = constrain(config.brightness, 0, maxBrightness);
  config.nightBrightness = constrain(config.nightBrightness, 0, maxBrightness);
  config.pageSeconds = constrain(config.pageSeconds, 3, 60);
  config.weatherProvider = constrain(config.weatherProvider, 0, WEATHER_PROVIDER_DWD);
  config.weatherIntervalHalfHours = constrain(config.weatherIntervalHalfHours, 1, 48);
  config.wiringMode = constrain(config.wiringMode, 0, 3);
  config.origin = constrain(config.origin, 0, 3);
  config.displayMode = constrain(config.displayMode, 0, 2);
  config.temperatureUnit = constrain(config.temperatureUnit, 0, 1);
  config.colorGradientMode = constrain(config.colorGradientMode, 0, 2);
  if (config.hourFormat != 12) config.hourFormat = 24;
  config.selectedPage = constrain(config.selectedPage, 0, 2);
  config.hostname = sanitizeHostname(config.hostname);
  config.adminUsername = sanitizeHostname(config.adminUsername);
  if (config.adminUsername.isEmpty()) config.adminUsername = DEFAULT_ADMIN_USERNAME;
  if (config.adminPassword.length() < MIN_ADMIN_PASSWORD_LENGTH) config.adminPassword = DEFAULT_ADMIN_PASSWORD;
}

void saveConfig() {
  prefs.begin("pixel-clock", false);
  prefs.putString("ssid", config.ssid);
  prefs.putString("pass", config.password);
  prefs.putUChar("authVer", AUTH_CONFIG_VERSION);
  prefs.putString("adminUser", config.adminUsername);
  prefs.putString("admin", config.adminPassword);
  prefs.putString("host", config.hostname);
  prefs.putString("city", config.cityName);
  prefs.putString("locLabel", config.locationLabel);
  prefs.putString("tz", config.timezone);
  prefs.putFloat("lat", config.latitude);
  prefs.putFloat("lon", config.longitude);
  prefs.putUChar("wProv", config.weatherProvider);
  prefs.putUChar("wIntHalf", config.weatherIntervalHalfHours);
  prefs.putString("owmKey", config.openWeatherApiKey);
  prefs.putUChar("width", config.width);
  prefs.putUChar("height", config.height);
  prefs.putUChar("pin", config.dataPin);
  prefs.putUChar("bright", config.brightness);
  prefs.putBool("fullBright", config.fullBrightnessUnlocked);
  prefs.putUChar("wiring", config.wiringMode);
  prefs.putUChar("origin", config.origin);
  prefs.putUChar("dispMode", config.displayMode);
  prefs.putUChar("tempUnit", config.temperatureUnit);
  prefs.putBool("tempIcon", config.weatherIconEnabled);
  prefs.putUChar("hourFmt", config.hourFormat);
  prefs.putBool("rgb", config.colorRgb);
  prefs.putUChar("pageSec", config.pageSeconds);
  prefs.putBool("autoPage", config.autoPage);
  prefs.putUChar("selPage", config.selectedPage);
  prefs.putUChar("nightB", config.nightBrightness);
  prefs.putUChar("nightS", config.nightStart);
  prefs.putUChar("nightE", config.nightEnd);
  prefs.putULong("colWeek", config.colorWeekday);
  prefs.putULong("colText", config.colorText);
  prefs.putULong("colPoint", config.colorPoint);
  prefs.putULong("colColon", config.colorColon);
  prefs.putUChar("colGradM", config.colorGradientMode);
  prefs.putBool("colGrad", config.colorGradientMode != 0);
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
