#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <FastLED.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <sys/time.h>
#include <time.h>

#include "weather_icons.h"

constexpr uint16_t MAX_LEDS = 512;
constexpr uint8_t DEFAULT_WIDTH = 32;
constexpr uint8_t DEFAULT_HEIGHT = 8;
constexpr uint32_t WEATHER_INTERVAL_MS = 2UL * 60UL * 60UL * 1000UL;
constexpr uint32_t WEATHER_RETRY_MS = 5UL * 60UL * 1000UL;
constexpr uint32_t NTP_INTERVAL_MS = 24UL * 60UL * 60UL * 1000UL;
constexpr uint32_t NTP_RETRY_MS = 5UL * 60UL * 1000UL;
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;
constexpr uint32_t HTTP_TIMEOUT_MS = 8000;
constexpr uint32_t BOOT_GREETING_MS = 9000;
constexpr int LEFT_SLOT_X = 1;
constexpr int VALUE_SLOT_X = 13;
constexpr int VALUE_SLOT_W = 18;
constexpr int RIGHT_ICON_X = 21;
constexpr int LEFT_VALUE_X = 1;
constexpr int LEFT_VALUE_W = 18;
constexpr const char *DEFAULT_ADMIN_USERNAME = "admin";
constexpr const char *DEFAULT_ADMIN_PASSWORD = "pixelclock";
constexpr uint8_t AUTH_CONFIG_VERSION = 1;
constexpr uint8_t MIN_ADMIN_PASSWORD_LENGTH = 8;

static const char ISRG_ROOT_X1[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

static const char SECTIGO_PUBLIC_SERVER_AUTH_ROOT_R46[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFijCCA3KgAwIBAgIQdY39i658BwD6qSWn4cetFDANBgkqhkiG9w0BAQwFADBf
MQswCQYDVQQGEwJHQjEYMBYGA1UEChMPU2VjdGlnbyBMaW1pdGVkMTYwNAYDVQQD
Ey1TZWN0aWdvIFB1YmxpYyBTZXJ2ZXIgQXV0aGVudGljYXRpb24gUm9vdCBSNDYw
HhcNMjEwMzIyMDAwMDAwWhcNNDYwMzIxMjM1OTU5WjBfMQswCQYDVQQGEwJHQjEY
MBYGA1UEChMPU2VjdGlnbyBMaW1pdGVkMTYwNAYDVQQDEy1TZWN0aWdvIFB1Ymxp
YyBTZXJ2ZXIgQXV0aGVudGljYXRpb24gUm9vdCBSNDYwggIiMA0GCSqGSIb3DQEB
AQUAA4ICDwAwggIKAoICAQCTvtU2UnXYASOgHEdCSe5jtrch/cSV1UgrJnwUUxDa
ef0rty2k1Cz66jLdScK5vQ9IPXtamFSvnl0xdE8H/FAh3aTPaE8bEmNtJZlMKpnz
SDBh+oF8HqcIStw+KxwfGExxqjWMrfhu6DtK2eWUAtaJhBOqbchPM8xQljeSM9xf
iOefVNlI8JhD1mb9nxc4Q8UBUQvX4yMPFF1bFOdLvt30yNoDN9HWOaEhUTCDsG3X
ME6WW5HwcCSrv0WBZEMNvSE6Lzzpng3LILVCJ8zab5vuZDCQOc2TZYEhMbUjUDM3
IuM47fgxMMxF/mL50V0yeUKH32rMVhlATc6qu/m1dkmU8Sf4kaWD5QazYw6A3OAS
VYCmO2a0OYctyPDQ0RTp5A1NDvZdV3LFOxxHVp3i1fuBYYzMTYCQNFu31xR13NgE
SJ/AwSiItOkcyqex8Va3e0lMWeUgFaiEAin6OJRpmkkGj80feRQXEgyDet4fsZfu
+Zd4KKTIRJLpfSYFplhym3kT2BFfrsU4YjRosoYwjviQYZ4ybPUHNs2iTG7sijbt
8uaZFURww3y8nDnAtOFr94MlI1fZEoDlSfB1D++N6xybVCi0ITz8fAr/73trdf+L
HaAZBav6+CuBQug4urv7qv094PPK306Xlynt8xhW6aWWrL3DkJiy4Pmi1KZHQ3xt
zwIDAQABo0IwQDAdBgNVHQ4EFgQUVnNYZJX5khqwEioEYnmhQBWIIUkwDgYDVR0P
AQH/BAQDAgGGMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAC9c
mTz8Bl6MlC5w6tIyMY208FHVvArzZJ8HXtXBc2hkeqK5Duj5XYUtqDdFqij0lgVQ
YKlJfp/imTYpE0RHap1VIDzYm/EDMrraQKFz6oOht0SmDpkBm+S8f74TlH7Kph52
gDY9hAaLMyZlbcp+nv4fjFg4exqDsQ+8FxG75gbMY/qB8oFM2gsQa6H61SilzwZA
Fv97fRheORKkU55+MkIQpiGRqRxOF3yEvJ+M0ejf5lG5Nkc/kLnHvALcWxxPDkjB
JYOcCj+esQMzEhonrPcibCTRAUH4WAP+JWgiH5paPHxsnnVI84HxZmduTILA7rpX
DhjvLpr3Etiga+kFpaHpaPi8TD8SHkXoUsCjvxInebnMMTzD9joiFgOgyY9mpFui
TdaBJQbpdqQACj7LzTWb4OE4y2BThihCQRxEV+ioratF4yUQvNs+ZUH7G6aXD+u5
dHn5HrwdVw1Hr8Mvn4dGp+smWg9WY7ViYG4A++MnESLn/pmPNPW56MORcr3Ywx65
LvKRRFHQV80MNNVIIb/bE/FmJUNS0nAiNs2fxBx1IK1jcmMGDw4nztJqDby1ORrp
0XZ60Vzk50lJLVU3aPAaOpg+VBeHVOmmJ1CJeyAvP/+/oYtKR5j/K3tJPsMpRmAY
QqszKbrAKbkTidOIijlBO8n9pu0f9GBj39ItVQGL
-----END CERTIFICATE-----
)EOF";

struct AppConfig {
  String ssid;
  String password;
  String adminUsername = DEFAULT_ADMIN_USERNAME;
  String adminPassword = DEFAULT_ADMIN_PASSWORD;
  String hostname = "pixelclock";
  String cityName = "Berlin";
  String locationLabel = "Berlin, Deutschland";
  String timezone = "CET-1CEST,M3.5.0,M10.5.0/3";
  float latitude = 52.52;
  float longitude = 13.41;
  uint8_t weatherProvider = 0;
  String openWeatherApiKey;
  uint8_t width = DEFAULT_WIDTH;
  uint8_t height = DEFAULT_HEIGHT;
  uint8_t dataPin = 5;
  uint8_t brightness = 64;
  uint8_t wiringMode = 1;
  uint8_t origin = 0;
  uint8_t displayMode = 0;
  uint8_t temperatureUnit = 0;
  uint8_t hourFormat = 24;
  bool colorRgb = false;
  uint8_t pageSeconds = 8;
  bool autoPage = true;
  uint8_t selectedPage = 0;
  uint8_t nightBrightness = 16;
  uint8_t nightStart = 22;
  uint8_t nightEnd = 7;
  uint32_t colorWeekday = 0x50B4FF;
  uint32_t colorText = 0xFFF5BE;
  uint32_t colorPoint = 0x50B4FF;
  uint32_t colorColon = 0xF8D66D;
  uint8_t colorGradientMode = 0;
};

struct WeatherState {
  float temperature = NAN;
  float temperatureMax = NAN;
  float temperatureMin = NAN;
  int weatherCode = -1;
  bool isDay = true;
  uint32_t lastFetch = 0;
  uint32_t lastAttempt = 0;
  String lastError;
};

Preferences prefs;
AsyncWebServer server(80);
AppConfig config;
WeatherState weather;

CRGB leds[MAX_LEDS];
uint16_t ledCount = DEFAULT_WIDTH * DEFAULT_HEIGHT;
uint8_t currentPage = 0;
uint32_t lastPageSwitch = 0;
uint32_t lastRender = 0;
uint32_t lastNtpSync = 0;
uint32_t lastNtpAttempt = 0;
bool displayTest = false;
uint32_t displayTestUntil = 0;
bool setupMode = false;
uint32_t bootStarted = 0;
bool forceTextGradient = false;
bool authConfigMigrationNeeded = false;
bool pendingCityResolve = false;
bool pendingWeatherFetch = false;
bool pendingTimeSync = false;

void saveConfig();
void renderDisplay();
void fetchWeather();
bool resolveCity();
void syncTime();
void seedTimeFromBuild();
bool timeIsReasonable();
bool hasAdminPassword();
bool requireAdminAuth(AsyncWebServerRequest *request);
void sendJsonError(AsyncWebServerRequest *request, int code, const String &message);
void configureWeatherClient(WiFiClientSecure &client, bool openWeatherMap);
void restartSoon(AsyncWebServerRequest *request);
void handleSettingsReset(AsyncWebServerRequest *request);
void handleFactoryReset(AsyncWebServerRequest *request);
String sanitizeHostname(String input);
String urlEncode(const String &input);
String timezoneFromIana(const String &iana);
int normalizeOpenWeatherCode(int code);
CRGB packedColor(uint32_t value);
String colorToHex(uint32_t value);
uint32_t parseColor(String value, uint32_t fallback);
bool isNightTime();
void drawBitmapIcon(int x, int y, const uint32_t *icon);

uint16_t xy(uint8_t x, uint8_t y) {
  if (x >= config.width || y >= config.height) return 0;
  uint8_t physX = x;
  uint8_t physY = y;
  if (config.origin == 1 || config.origin == 3) physX = config.width - 1 - physX;
  if (config.origin == 2 || config.origin == 3) physY = config.height - 1 - physY;

  if (config.wiringMode == 2 || config.wiringMode == 3) {
    if (config.wiringMode == 3 && (physX % 2 == 1)) physY = config.height - 1 - physY;
    return physX * config.height + physY;
  }

  if (config.wiringMode == 1 && (physY % 2 == 1)) physX = config.width - 1 - physX;
  return physY * config.width + physX;
}

void px(int x, int y, CRGB color) {
  if (x < 0 || y < 0 || x >= config.width || y >= config.height) return;
  const uint16_t i = xy(x, y);
  if (i < ledCount) leds[i] = color;
}

template <uint8_t PIN>
void addFastLedsForPin() {
  if (config.colorRgb) {
    FastLED.addLeds<WS2812B, PIN, RGB>(leds, ledCount);
  } else {
    FastLED.addLeds<WS2812B, PIN, GRB>(leds, ledCount);
  }
}

void setupFastLed() {
  ledCount = min<uint16_t>(MAX_LEDS, config.width * config.height);
  switch (config.dataPin) {
    case 2: addFastLedsForPin<2>(); break;
    case 4: addFastLedsForPin<4>(); break;
    case 5: addFastLedsForPin<5>(); break;
    case 12: addFastLedsForPin<12>(); break;
    case 13: addFastLedsForPin<13>(); break;
    case 14: addFastLedsForPin<14>(); break;
    case 15: addFastLedsForPin<15>(); break;
    case 16: addFastLedsForPin<16>(); break;
    case 17: addFastLedsForPin<17>(); break;
    case 18: addFastLedsForPin<18>(); break;
    case 19: addFastLedsForPin<19>(); break;
    case 21: addFastLedsForPin<21>(); break;
    case 22: addFastLedsForPin<22>(); break;
    case 23: addFastLedsForPin<23>(); break;
    case 25: addFastLedsForPin<25>(); break;
    case 26: addFastLedsForPin<26>(); break;
    case 27: addFastLedsForPin<27>(); break;
    case 32: addFastLedsForPin<32>(); break;
    case 33: addFastLedsForPin<33>(); break;
    default:
      config.dataPin = 5;
      addFastLedsForPin<5>();
      break;
  }
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setDither(true);
  FastLED.setBrightness(config.brightness);
  FastLED.clear(true);
}

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
  config.openWeatherApiKey = prefs.getString("owmKey", "");
  config.width = prefs.getUChar("width", config.width);
  config.height = prefs.getUChar("height", config.height);
  config.dataPin = prefs.getUChar("pin", config.dataPin);
  config.brightness = prefs.getUChar("bright", config.brightness);
  config.wiringMode = prefs.getUChar("wiring", 255);
  if (config.wiringMode == 255) config.wiringMode = prefs.getBool("serp", true) ? 1 : 0;
  config.origin = prefs.getUChar("origin", config.origin);
  config.displayMode = prefs.getUChar("dispMode", config.displayMode);
  config.temperatureUnit = prefs.getUChar("tempUnit", config.temperatureUnit);
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
  config.brightness = constrain(config.brightness, 0, 255);
  config.nightBrightness = constrain(config.nightBrightness, 0, 255);
  config.pageSeconds = constrain(config.pageSeconds, 3, 60);
  config.weatherProvider = constrain(config.weatherProvider, 0, 1);
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
  prefs.putString("owmKey", config.openWeatherApiKey);
  prefs.putUChar("width", config.width);
  prefs.putUChar("height", config.height);
  prefs.putUChar("pin", config.dataPin);
  prefs.putUChar("bright", config.brightness);
  prefs.putUChar("wiring", config.wiringMode);
  prefs.putUChar("origin", config.origin);
  prefs.putUChar("dispMode", config.displayMode);
  prefs.putUChar("tempUnit", config.temperatureUnit);
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

String urlEncode(const String &input) {
  const char *hex = "0123456789ABCDEF";
  String out;
  for (uint16_t i = 0; i < input.length(); i++) {
    const uint8_t c = input[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.') {
      out += char(c);
    } else if (c == ' ') {
      out += '+';
    } else {
      out += '%';
      out += hex[c >> 4];
      out += hex[c & 0x0f];
    }
  }
  return out;
}

String timezoneFromIana(const String &iana) {
  if (iana == "Etc/UTC" || iana == "UTC" || iana == "GMT") return "UTC0";

  if (iana == "Europe/Berlin" || iana == "Europe/Amsterdam" || iana == "Europe/Paris" ||
      iana == "Europe/Rome" || iana == "Europe/Madrid" || iana == "Europe/Brussels" ||
      iana == "Europe/Vienna" || iana == "Europe/Zurich" || iana == "Europe/Prague" ||
      iana == "Europe/Warsaw" || iana == "Europe/Stockholm" || iana == "Europe/Oslo" ||
      iana == "Europe/Copenhagen") {
    return "CET-1CEST,M3.5.0,M10.5.0/3";
  }
  if (iana == "Europe/London") return "GMT0BST,M3.5.0/1,M10.5.0";
  if (iana == "Europe/Lisbon") return "WET0WEST,M3.5.0/1,M10.5.0";
  if (iana == "Europe/Athens" || iana == "Europe/Helsinki" || iana == "Europe/Bucharest" ||
      iana == "Europe/Sofia" || iana == "Europe/Tallinn" || iana == "Europe/Riga" ||
      iana == "Europe/Vilnius") {
    return "EET-2EEST,M3.5.0/3,M10.5.0/4";
  }
  if (iana == "Europe/Moscow") return "MSK-3";

  if (iana == "America/New_York" || iana == "America/Toronto") return "EST5EDT,M3.2.0/2,M11.1.0/2";
  if (iana == "America/Chicago") return "CST6CDT,M3.2.0/2,M11.1.0/2";
  if (iana == "America/Denver") return "MST7MDT,M3.2.0/2,M11.1.0/2";
  if (iana == "America/Los_Angeles" || iana == "America/Vancouver") return "PST8PDT,M3.2.0/2,M11.1.0/2";
  if (iana == "America/Phoenix") return "MST7";
  if (iana == "America/Anchorage") return "AKST9AKDT,M3.2.0/2,M11.1.0/2";
  if (iana == "Pacific/Honolulu") return "HST10";
  if (iana == "America/Sao_Paulo") return "BRT3";

  if (iana == "Asia/Tokyo") return "JST-9";
  if (iana == "Asia/Shanghai" || iana == "Asia/Hong_Kong") return "CST-8";
  if (iana == "Asia/Singapore") return "SGT-8";
  if (iana == "Asia/Dubai") return "GST-4";
  if (iana == "Asia/Kolkata") return "IST-5:30";

  if (iana == "Australia/Sydney" || iana == "Australia/Melbourne") return "AEST-10AEDT,M10.1.0,M4.1.0/3";
  if (iana == "Australia/Brisbane") return "AEST-10";
  if (iana == "Australia/Perth") return "AWST-8";

  return "";
}

int normalizeOpenWeatherCode(int code) {
  if (code >= 200 && code < 300) return 95;
  if (code >= 300 && code < 400) return 51;
  if (code >= 500 && code < 600) return 61;
  if (code >= 600 && code < 700) return 71;
  if (code >= 700 && code < 800) return 45;
  if (code == 800) return 0;
  if (code == 801) return 1;
  if (code == 802) return 2;
  if (code == 803 || code == 804) return 3;
  return -1;
}

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

void updateBrightnessForTime() {
  struct tm timeinfo;
  uint8_t target = config.brightness;
  if (getLocalTime(&timeinfo, 10)) {
    if (isNightTime()) target = min(config.brightness, config.nightBrightness);
  }
  FastLED.setBrightness(target);
}

bool isNightTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10)) return !weather.isDay;
  const uint8_t h = timeinfo.tm_hour;
  return config.nightStart > config.nightEnd
    ? (h >= config.nightStart || h < config.nightEnd)
    : (h >= config.nightStart && h < config.nightEnd);
}

CRGB textPixelColor(int x, int y, CRGB fallback) {
  uint8_t mode = config.colorGradientMode;
  if (forceTextGradient && mode == 0) mode = 1;
  if (mode == 0) return fallback;
  const uint8_t hue = mode == 2
    ? (uint8_t)((millis() / 55) & 0xff)
    : (uint8_t)((x * 9 + y * 4 + millis() / 90) & 0xff);
  return CHSV(hue, 220, 255);
}

const char *glyphFor(char c) {
  switch (c) {
    case '0': return "111101101101111";
    case '1': return "010110010010111";
    case '2': return "111001111100111";
    case '3': return "111001111001111";
    case '4': return "101101111001001";
    case '5': return "111100111001111";
    case '6': return "111100111101111";
    case '7': return "111001001010010";
    case '8': return "111101111101111";
    case '9': return "111101111001111";
    case ':': return "000010000010000";
    case '.': return "000000000000010";
    case '-': return "000000111000000";
    case 'A': return "010101111101101";
    case 'C': return "111100100100111";
    case 'D': return "110101101101110";
    case 'F': return "111100110100100";
    case 'H': return "101101111101101";
    case 'I': return "111010010010111";
    case 'L': return "100100100100111";
    case 'M': return "101111111101101";
    case 'N': return "101111111111101";
    case 'O': return "111101101101111";
    case 'R': return "110101110101101";
    case 'S': return "111100111001111";
    case 'T': return "111010010010010";
    case 'E': return "111100110100111";
    case 'P': return "110101110100100";
    case 'W': return "101101111111101";
    default: return nullptr;
  }
}

uint8_t drawChar3x5(int x, int y, char c, CRGB color) {
  if (c == ' ') return 2;
  const char *g = glyphFor(c);
  if (!g) return 0;
  for (uint8_t row = 0; row < 5; row++) {
    for (uint8_t col = 0; col < 3; col++) {
      if (g[row * 3 + col] == '1') px(x + col, y + row, textPixelColor(x + col, y + row, color));
    }
  }
  return 4;
}

uint8_t charAdvance3x5(char c) {
  if (c == ' ') return 2;
  if (c == ':' || c == '.') return 2;
  return glyphFor(c) ? 4 : 0;
}

void drawText3x5(int x, int y, const String &text, CRGB color) {
  int cursor = x;
  for (uint8_t i = 0; i < text.length(); i++) {
    cursor += drawChar3x5(cursor, y, text[i], color);
  }
}

void drawText3x5Styled(int x, int y, const String &text) {
  int cursor = x;
  for (uint8_t i = 0; i < text.length(); i++) {
    CRGB color = packedColor(config.colorText);
    if (text[i] == ':') color = packedColor(config.colorColon);
    else if (text[i] == '.') color = packedColor(config.colorPoint);
    drawChar3x5(cursor, y, text[i], color);
    cursor += charAdvance3x5(text[i]);
  }
}

uint8_t textWidth3x5(const String &text) {
  uint8_t width = 0;
  for (uint8_t i = 0; i < text.length(); i++) width += charAdvance3x5(text[i]);
  return width > 0 ? width - 1 : 0;
}

void drawRightAlignedStyled(int x, int y, int width, const String &text) {
  const int start = x + width - textWidth3x5(text);
  drawText3x5Styled(start, y, text);
}

void drawRightAlignedText(int x, int y, int width, const String &text, CRGB color) {
  const int start = x + width - textWidth3x5(text);
  int cursor = start;
  for (uint8_t i = 0; i < text.length(); i++) {
    drawChar3x5(cursor, y, text[i], color);
    cursor += charAdvance3x5(text[i]);
  }
}

void drawCenteredText3x5(int y, const String &text, CRGB color) {
  const int start = max(0, (int(config.width) - int(textWidth3x5(text))) / 2);
  drawText3x5(start, y, text, color);
}

void drawCenteredStyled3x5(int y, const String &text) {
  const int start = max(0, (int(config.width) - int(textWidth3x5(text))) / 2);
  drawText3x5Styled(start, y, text);
}

const char *bigGlyphFor(char c) {
  switch (c) {
    case '0': return "011101000110001100011000101110";
    case '1': return "001000110000100001000010001110";
    case '2': return "011101000100010001001000011111";
    case '3': return "111100000100110000010000111110";
    case '4': return "000100011001010111110001000010";
    case '5': return "111111000011110000010000111110";
    case '6': return "011101000011110100011000101110";
    case '7': return "111110000100010001000100001000";
    case '8': return "011101000101110100011000101110";
    case '9': return "011101000101111000010000101110";
    case '-': return "000000000000000111110000000000";
    case 'C': return "011101000110000100001000101110";
    case 'F': return "111111000011110100001000010000";
    default: return nullptr;
  }
}

uint8_t bigCharAdvance(char c) {
  return bigGlyphFor(c) ? 6 : 0;
}

uint8_t bigTextWidth(const String &text) {
  uint8_t width = 0;
  for (uint8_t i = 0; i < text.length(); i++) width += bigCharAdvance(text[i]);
  return width > 0 ? width - 1 : 0;
}

void drawBigChar(int x, int y, char c, CRGB color) {
  const char *g = bigGlyphFor(c);
  if (!g) return;
  for (uint8_t row = 0; row < 6; row++) {
    for (uint8_t col = 0; col < 5; col++) {
      if (g[row * 5 + col] == '1') px(x + col, y + row, textPixelColor(x + col, y + row, color));
    }
  }
}

void drawBigTextCentered(int y, const String &text) {
  int cursor = max(0, (int(config.width) - int(bigTextWidth(text))) / 2);
  for (uint8_t i = 0; i < text.length(); i++) {
    CRGB color = packedColor(config.colorText);
    if (text[i] == ':') color = packedColor(config.colorColon);
    drawBigChar(cursor, y, text[i], color);
    cursor += bigCharAdvance(text[i]);
  }
}

void drawBigClockCentered(const tm &timeinfo) {
  char hours[3];
  char minutes[3];
  uint8_t hour = timeinfo.tm_hour;
  if (config.hourFormat == 12) {
    hour %= 12;
    if (hour == 0) hour = 12;
  }
  snprintf(hours, sizeof(hours), "%02d", hour);
  snprintf(minutes, sizeof(minutes), "%02d", timeinfo.tm_min);
  const CRGB textColor = packedColor(config.colorText);
  const CRGB colonColor = packedColor(config.colorColon);
  drawBigChar(1, 1, hours[0], textColor);
  drawBigChar(7, 1, hours[1], textColor);
  px(15, 3, textPixelColor(15, 3, colonColor));
  px(16, 3, textPixelColor(16, 3, colonColor));
  px(15, 5, textPixelColor(15, 5, colonColor));
  px(16, 5, textPixelColor(16, 5, colonColor));
  drawBigChar(20, 1, minutes[0], textColor);
  drawBigChar(26, 1, minutes[1], textColor);
}

const char *tallNarrowGlyphFor(char c) {
  switch (c) {
    case '0': return "111101101101101111";
    case '1': return "010110010010010111";
    case '2': return "111001001111100111";
    case '3': return "111001001111001111";
    case '4': return "101101101111001001";
    case '5': return "111100100111001111";
    case '6': return "111100100111101111";
    case '7': return "111001010010010010";
    case '8': return "111101101111101111";
    case '9': return "111101101111001111";
    default: return nullptr;
  }
}

uint8_t tallNarrowAdvance(char c) {
  if (c == '.') return 2;
  return tallNarrowGlyphFor(c) ? 4 : 0;
}

uint8_t tallNarrowWidth(const String &text) {
  uint8_t width = 0;
  for (uint8_t i = 0; i < text.length(); i++) width += tallNarrowAdvance(text[i]);
  return width > 0 ? width - 1 : 0;
}

void drawTallNarrowChar(int x, int y, char c, CRGB color) {
  const char *g = tallNarrowGlyphFor(c);
  if (!g) return;
  for (uint8_t row = 0; row < 6; row++) {
    for (uint8_t col = 0; col < 3; col++) {
      if (g[row * 3 + col] == '1') px(x + col, y + row, textPixelColor(x + col, y + row, color));
    }
  }
}

void drawLargeDateCentered(int y) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10)) return;
  char buffer[9];
  snprintf(buffer, sizeof(buffer), "%02d.%02d.%02d", timeinfo.tm_mday, timeinfo.tm_mon + 1, (timeinfo.tm_year + 1900) % 100);
  const String text(buffer);
  int cursor = max(0, (int(config.width) - 28) / 2);
  for (uint8_t i = 0; i < text.length(); i++) {
    if (text[i] == '.') {
      px(cursor, y + 5, textPixelColor(cursor, y + 5, packedColor(config.colorPoint)));
    } else {
      drawTallNarrowChar(cursor, y, text[i], packedColor(config.colorText));
    }
    cursor += (i == 5) ? 3 : tallNarrowAdvance(text[i]);
  }
}

uint8_t compactDateAdvance(char c) {
  if (c == '.') return 2;
  if (c == '-') return 2;
  if (c == ' ') return 1;
  return glyphFor(c) ? 3 : 0;
}

uint8_t compactTextWidth3x5(const String &text) {
  uint8_t width = 0;
  for (uint8_t i = 0; i < text.length(); i++) width += compactDateAdvance(text[i]);
  return width > 0 ? width - 1 : 0;
}

void drawCompactTextCentered3x5(int y, const String &text, CRGB color) {
  int cursor = max(0, (int(config.width) - int(compactTextWidth3x5(text))) / 2);
  for (uint8_t i = 0; i < text.length(); i++) {
    if (text[i] == '.') {
      px(cursor, y + 4, textPixelColor(cursor, y + 4, packedColor(config.colorPoint)));
    } else if (text[i] != ' ') {
      drawChar3x5(cursor, y, text[i], color);
    }
    cursor += compactDateAdvance(text[i]);
  }
}

void drawCompactDateCentered(int y) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10)) return;
  char buffer[11];
  snprintf(buffer, sizeof(buffer), "%02d.%02d.%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
  const String text(buffer);
  int cursor = max(0, (int(config.width) - int(compactTextWidth3x5(text))) / 2);
  for (uint8_t i = 0; i < text.length(); i++) {
    if (text[i] == '.') {
      px(cursor, y + 4, textPixelColor(cursor, y + 4, packedColor(config.colorPoint)));
    } else {
      drawChar3x5(cursor, y, text[i], packedColor(config.colorText));
    }
    cursor += compactDateAdvance(text[i]);
  }
}

void drawDateValue(int x, int y) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10)) return;
  char day[3];
  char month[3];
  snprintf(day, sizeof(day), "%02d", timeinfo.tm_mday);
  snprintf(month, sizeof(month), "%02d", timeinfo.tm_mon + 1);
  drawText3x5(x, y, day, packedColor(config.colorText));
  px(x + 8, y + 4, textPixelColor(x + 8, y + 4, packedColor(config.colorPoint)));
  drawText3x5(x + 10, y, month, packedColor(config.colorText));
}

void drawFixedClock(int x, int y, const tm &timeinfo, bool showColon) {
  char hours[3];
  char minutes[3];
  uint8_t hour = timeinfo.tm_hour;
  if (config.hourFormat == 12) {
    hour %= 12;
    if (hour == 0) hour = 12;
  }
  snprintf(hours, sizeof(hours), "%02d", hour);
  snprintf(minutes, sizeof(minutes), "%02d", timeinfo.tm_min);
  drawText3x5(x, y, hours, packedColor(config.colorText));
  if (showColon) drawChar3x5(x + 8, y, ':', packedColor(config.colorColon));
  drawText3x5(x + 11, y, minutes, packedColor(config.colorText));
}

void drawIndicator(uint8_t active) {
  const uint8_t y = config.height - 1;
  int start = VALUE_SLOT_X + ((VALUE_SLOT_W - 13) / 2);
  if (config.displayMode == 1) start = LEFT_VALUE_X + ((LEFT_VALUE_W - 13) / 2);
  else if (config.displayMode == 2) start = (config.width - 13) / 2;
  for (uint8_t i = 0; i < 3; i++) {
    CRGB color = i == active ? packedColor(config.colorPoint) : CRGB(18, 18, 18);
    px(start + i * 5, y, color);
    px(start + i * 5 + 1, y, color);
    px(start + i * 5 + 2, y, color);
  }
}

void drawSun(int x, int y) {
  CRGB yellow(255, 190, 20);
  CRGB orange(255, 90, 0);
  px(x + 4, y, orange); px(x + 4, y + 7, orange);
  px(x, y + 3, orange); px(x + 8, y + 3, orange);
  px(x + 2, y + 1, orange); px(x + 6, y + 1, orange);
  px(x + 2, y + 6, orange); px(x + 6, y + 6, orange);
  for (uint8_t yy = 2; yy <= 5; yy++) {
    for (uint8_t xx = 3; xx <= 5; xx++) px(x + xx, y + yy, yellow);
  }
}

void drawCloud(int x, int y, CRGB color = CRGB(120, 130, 145)) {
  px(x + 3, y + 2, color); px(x + 4, y + 2, color);
  px(x + 2, y + 3, color); px(x + 3, y + 3, color); px(x + 4, y + 3, color); px(x + 5, y + 3, color);
  for (uint8_t xx = 1; xx <= 7; xx++) px(x + xx, y + 4, color);
  for (uint8_t xx = 2; xx <= 6; xx++) px(x + xx, y + 5, color);
}

void drawRain(int x, int y) {
  drawCloud(x, y, CRGB(105, 115, 130));
  CRGB blue(40, 120, 255);
  px(x + 2, y + 6, blue); px(x + 4, y + 7, blue); px(x + 6, y + 6, blue);
}

void drawSnow(int x, int y) {
  drawCloud(x, y, CRGB(130, 150, 170));
  CRGB ice(160, 220, 255);
  px(x + 2, y + 6, ice); px(x + 4, y + 6, ice); px(x + 6, y + 6, ice);
  px(x + 3, y + 7, ice); px(x + 5, y + 7, ice);
}

void drawThunder(int x, int y) {
  drawCloud(x, y, CRGB(90, 90, 110));
  CRGB bolt(255, 220, 20);
  px(x + 5, y + 5, bolt); px(x + 4, y + 6, bolt); px(x + 5, y + 6, bolt); px(x + 3, y + 7, bolt);
}

void drawFog(int x, int y) {
  CRGB grey(110, 120, 125);
  drawCloud(x, y, grey);
  for (uint8_t xx = 1; xx <= 8; xx += 2) px(x + xx, y + 7, grey);
}

void drawWeatherIcon(int x, int y) {
  const int c = weather.weatherCode;
  const bool night = isNightTime();
  const uint32_t *icon = icon_unknown;
  if (c == 0) icon = night ? icon_clear_night : icon_clear_day;
  else if (c == 1 || c == 2) icon = night ? icon_partly_night : icon_partly_day;
  else if (c == 3) icon = icon_cloud;
  else if (c == 45 || c == 48) icon = icon_fog;
  else if ((c >= 51 && c <= 67) || (c >= 80 && c <= 82)) icon = icon_rain;
  else if ((c >= 71 && c <= 77) || (c >= 85 && c <= 86)) icon = icon_snow;
  else if (c >= 95) icon = icon_thunder;
  drawBitmapIcon(x, y, icon);
}

void drawTemperatureIcon(int x, int y) {
  drawBitmapIcon(x, y, icon_temperature);
}

void drawBitmapIcon(int x, int y, const uint32_t *icon) {
  for (uint8_t yy = 0; yy < WEATHER_ICON_H; yy++) {
    for (uint8_t xx = 0; xx < WEATHER_ICON_W; xx++) {
      const uint32_t color = pgm_read_dword(&icon[yy * WEATHER_ICON_W + xx]);
      if (color == 0) continue;
      px(x + xx, y + yy, CRGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff));
    }
  }
}

void drawClockIcon(int x, int y, uint8_t frame) {
  CRGB body = packedColor(config.colorPoint);
  CRGB hand = packedColor(config.colorColon);
  px(x + 3, y, body); px(x + 4, y, body);
  px(x + 1, y + 1, body); px(x + 6, y + 1, body);
  px(x, y + 3, body); px(x + 7, y + 3, body);
  px(x, y + 4, body); px(x + 7, y + 4, body);
  px(x + 1, y + 6, body); px(x + 6, y + 6, body);
  px(x + 3, y + 7, body); px(x + 4, y + 7, body);
  px(x + 4, y + 4, hand);
  px(x + 4, y + 3, hand);
  if (frame % 4 == 0) px(x + 5, y + 4, hand);
  else if (frame % 4 == 1) px(x + 4, y + 5, hand);
  else if (frame % 4 == 2) px(x + 3, y + 4, hand);
  else px(x + 4, y + 2, hand);
}

void drawBootGreeting() {
  const uint8_t frame = ((millis() - bootStarted) / 350) % 4;
  forceTextGradient = true;
  drawClockIcon(0, 0, frame);
  drawText3x5(11, 1, "HELLO", packedColor(config.colorText));
  forceTextGradient = false;
}

void drawSetupPrompt() {
  drawClockIcon(0, 0, (millis() / 450) % 4);
  const bool showAp = ((millis() / 1600) % 2) == 1;
  drawText3x5(12, 1, showAp ? "AP" : "WIFI", packedColor(config.colorText));
}

String weekdayShort(const tm &t) {
  static const char *days[] = {"SO", "MO", "DI", "MI", "DO", "FR", "SA"};
  return days[t.tm_wday];
}

float displayTemperature(float celsius) {
  if (isnan(celsius)) return NAN;
  return config.temperatureUnit == 1 ? (celsius * 9.0f / 5.0f) + 32.0f : celsius;
}

String temperatureUnitText() {
  return config.temperatureUnit == 1 ? "F" : "C";
}

String currentTemperatureText() {
  if (isnan(weather.temperature)) return "--" + temperatureUnitText();
  return String((int)round(displayTemperature(weather.temperature))) + temperatureUnitText();
}

String temperatureOverviewText() {
  String text = currentTemperatureText();
  if (!isnan(weather.temperatureMin) && !isnan(weather.temperatureMax)) {
    text += " ";
    text += String((int)round(displayTemperature(weather.temperatureMin)));
    text += "-";
    text += String((int)round(displayTemperature(weather.temperatureMax)));
  }
  return text;
}

void drawClockPage() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10)) {
    drawText3x5(2, 1, "----", CRGB::Red);
    return;
  }
  if (config.displayMode == 1) {
    drawFixedClock(LEFT_VALUE_X, 1, timeinfo, true);
    drawWeatherIcon(RIGHT_ICON_X, 0);
  } else if (config.displayMode == 2) {
    drawBigClockCentered(timeinfo);
  } else {
    drawWeatherIcon(LEFT_SLOT_X, 0);
    drawFixedClock(VALUE_SLOT_X, 1, timeinfo, true);
  }
}

void drawDatePage() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10)) {
    drawText3x5(3, 1, "NO TIME", CRGB::Red);
    return;
  }
  if (config.displayMode == 1) {
    drawDateValue(LEFT_VALUE_X, 1);
    drawRightAlignedText(RIGHT_ICON_X, 1, 9, weekdayShort(timeinfo), packedColor(config.colorWeekday));
  } else if (config.displayMode == 2) {
    drawLargeDateCentered(1);
  } else {
    drawText3x5(LEFT_SLOT_X, 1, weekdayShort(timeinfo), packedColor(config.colorWeekday));
    drawDateValue(VALUE_SLOT_X, 1);
  }
}

void drawWeatherPage() {
  const String text = currentTemperatureText();
  if (config.displayMode == 1) {
    drawText3x5(LEFT_VALUE_X, 1, text, packedColor(config.colorText));
    drawTemperatureIcon(RIGHT_ICON_X, 0);
  } else if (config.displayMode == 2) {
    drawBigTextCentered(1, text);
  } else {
    drawTemperatureIcon(LEFT_SLOT_X, 0);
    drawRightAlignedText(VALUE_SLOT_X, 1, VALUE_SLOT_W, text, packedColor(config.colorText));
  }
}

void drawTestPattern() {
  for (uint8_t y = 0; y < config.height; y++) {
    for (uint8_t x = 0; x < config.width; x++) {
      px(x, y, CHSV((x * 255) / max<uint8_t>(1, config.width - 1), 220, 160));
    }
  }
}

void renderDisplay() {
  updateBrightnessForTime();
  fill_solid(leds, ledCount, CRGB::Black);
  if (millis() - bootStarted < BOOT_GREETING_MS) {
    drawBootGreeting();
  } else if (setupMode && config.ssid.isEmpty()) {
    drawSetupPrompt();
  } else if (displayTest && millis() < displayTestUntil) {
    drawTestPattern();
  } else {
    displayTest = false;
    const uint8_t page = config.autoPage ? currentPage : config.selectedPage;
    if (page == 0) drawClockPage();
    else if (page == 1) drawDatePage();
    else drawWeatherPage();
    if (config.displayMode != 2) drawIndicator(page);
  }
  FastLED.show();
}

void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;
  weather.lastAttempt = millis();
  if (config.weatherProvider == 1 && config.openWeatherApiKey.isEmpty()) {
    weather.lastError = "OpenWeatherMap API-Key fehlt";
    return;
  }
  WiFiClientSecure client;
  configureWeatherClient(client, config.weatherProvider == 1);
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  String url;
  if (config.weatherProvider == 1) {
    url = "https://api.openweathermap.org/data/2.5/weather?lat=" + String(config.latitude, 5) +
      "&lon=" + String(config.longitude, 5) +
      "&appid=" + config.openWeatherApiKey +
      "&units=metric&lang=de";
  } else {
    url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(config.latitude, 5) +
      "&longitude=" + String(config.longitude, 5) +
      "&current=temperature_2m,weather_code,is_day&daily=temperature_2m_max,temperature_2m_min&timezone=auto&forecast_days=1";
  }
  if (!http.begin(client, url)) {
    weather.lastError = "HTTP begin failed";
    return;
  }
  http.useHTTP10(true);
  http.addHeader("Accept-Encoding", "identity");
  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    weather.lastError = "HTTP " + String(code);
    http.end();
    return;
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) {
    weather.lastError = err.c_str();
    return;
  }
  if (config.weatherProvider == 1) {
    weather.temperature = doc["main"]["temp"] | NAN;
    weather.temperatureMax = doc["main"]["temp_max"] | weather.temperature;
    weather.temperatureMin = doc["main"]["temp_min"] | weather.temperature;
    const int openWeatherCode = doc["weather"][0]["id"] | -1;
    weather.weatherCode = normalizeOpenWeatherCode(openWeatherCode);
    const char *icon = doc["weather"][0]["icon"] | "";
    weather.isDay = strlen(icon) < 3 || icon[2] != 'n';
  } else {
    weather.temperature = doc["current"]["temperature_2m"] | NAN;
    weather.temperatureMax = doc["daily"]["temperature_2m_max"][0] | NAN;
    weather.temperatureMin = doc["daily"]["temperature_2m_min"][0] | NAN;
    weather.weatherCode = doc["current"]["weather_code"] | -1;
    weather.isDay = (doc["current"]["is_day"] | 1) == 1;
  }
  weather.lastFetch = millis();
  weather.lastError = "";
}

bool resolveCity() {
  if (WiFi.status() != WL_CONNECTED || config.cityName.length() < 2) return false;

  WiFiClientSecure client;
  configureWeatherClient(client, false);
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  const String url = "https://geocoding-api.open-meteo.com/v1/search?name=" + urlEncode(config.cityName) +
    "&count=1&language=de&format=json";
  if (!http.begin(client, url)) {
    weather.lastError = "Geocoding begin failed";
    return false;
  }
  http.useHTTP10(true);
  http.addHeader("Accept-Encoding", "identity");
  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    weather.lastError = "Geocoding HTTP " + String(code);
    http.end();
    return false;
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) {
    weather.lastError = String("Geocoding JSON ") + err.c_str();
    return false;
  }
  JsonArray results = doc["results"].as<JsonArray>();
  if (results.isNull() || results.size() == 0) {
    weather.lastError = "Stadt nicht gefunden";
    return false;
  }
  JsonObject first = results[0];
  config.latitude = first["latitude"] | config.latitude;
  config.longitude = first["longitude"] | config.longitude;
  const char *city = first["name"] | config.cityName.c_str();
  const char *country = first["country"] | "";
  const char *ianaTimezone = first["timezone"] | "";
  config.locationLabel = String(city);
  if (strlen(country) > 0) {
    config.locationLabel += ", ";
    config.locationLabel += country;
  }
  const String resolvedTimezone = timezoneFromIana(String(ianaTimezone));
  if (!resolvedTimezone.isEmpty()) {
    config.timezone = resolvedTimezone;
  }
  saveConfig();
  return true;
}

void sendConfigJson(AsyncWebServerRequest *request) {
  if (!requireAdminAuth(request)) return;
  JsonDocument doc;
  doc["ssid"] = config.ssid;
  doc["hasPassword"] = !config.password.isEmpty();
  doc["adminUsername"] = config.adminUsername;
  doc["defaultAdminUsername"] = DEFAULT_ADMIN_USERNAME;
  doc["defaultAdminPassword"] = DEFAULT_ADMIN_PASSWORD;
  doc["adminPasswordSet"] = hasAdminPassword();
  doc["minAdminPasswordLength"] = MIN_ADMIN_PASSWORD_LENGTH;
  doc["hostname"] = config.hostname;
  doc["url"] = "http://" + config.hostname + ".local";
  doc["cityName"] = config.cityName;
  doc["locationLabel"] = config.locationLabel;
  doc["timezone"] = config.timezone;
  doc["latitude"] = config.latitude;
  doc["longitude"] = config.longitude;
  doc["weatherProvider"] = config.weatherProvider;
  doc["hasOpenWeatherApiKey"] = !config.openWeatherApiKey.isEmpty();
  doc["width"] = config.width;
  doc["height"] = config.height;
  doc["dataPin"] = config.dataPin;
  doc["brightness"] = config.brightness;
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

void configureWeatherClient(WiFiClientSecure &client, bool openWeatherMap) {
  client.setCACert(openWeatherMap ? SECTIGO_PUBLIC_SERVER_AUTH_ROOT_R46 : ISRG_ROOT_X1);
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
  const String newOpenWeatherApiKey = paramValue(request, "openWeatherApiKey", "");
  if (newOpenWeatherApiKey.length() > 0) config.openWeatherApiKey = newOpenWeatherApiKey;
  config.timezone = paramValue(request, "timezone", config.timezone);
  config.latitude = paramValue(request, "latitude", String(config.latitude, 5)).toFloat();
  config.longitude = paramValue(request, "longitude", String(config.longitude, 5)).toFloat();
  config.width = constrain(paramValue(request, "width", String(config.width)).toInt(), 8, 64);
  config.height = constrain(paramValue(request, "height", String(config.height)).toInt(), 8, 16);
  config.dataPin = paramValue(request, "dataPin", String(config.dataPin)).toInt();
  config.brightness = constrain(paramValue(request, "brightness", String(config.brightness)).toInt(), 0, 255);
  config.wiringMode = constrain(paramValue(request, "wiringMode", String(config.wiringMode)).toInt(), 0, 3);
  config.origin = constrain(paramValue(request, "origin", String(config.origin)).toInt(), 0, 3);
  config.displayMode = constrain(paramValue(request, "displayMode", String(config.displayMode)).toInt(), 0, 2);
  config.temperatureUnit = constrain(paramValue(request, "temperatureUnit", String(config.temperatureUnit)).toInt(), 0, 1);
  config.hourFormat = paramValue(request, "hourFormat", String(config.hourFormat)).toInt() == 12 ? 12 : 24;
  config.colorRgb = paramValue(request, "colorOrder", "GRB") == "RGB";
  config.pageSeconds = constrain(paramValue(request, "pageSeconds", String(config.pageSeconds)).toInt(), 3, 60);
  config.autoPage = paramValue(request, "autoPage", "0") == "1";
  config.selectedPage = constrain(paramValue(request, "selectedPage", String(config.selectedPage)).toInt(), 0, 2);
  config.nightBrightness = constrain(paramValue(request, "nightBrightness", String(config.nightBrightness)).toInt(), 0, 255);
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
    : (now - weather.lastFetch >= WEATHER_INTERVAL_MS);
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
