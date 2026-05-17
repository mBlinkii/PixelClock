#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <FastLED.h>
#include <Preferences.h>
#include <WiFiClientSecure.h>
#include <time.h>

constexpr uint16_t MAX_LEDS = 512;
constexpr uint8_t DEFAULT_WIDTH = 32;
constexpr uint8_t DEFAULT_HEIGHT = 8;
constexpr uint8_t DEFAULT_WIRING_MODE = 3;
constexpr uint8_t DEFAULT_WEATHER_INTERVAL_HALF_HOURS = 4;
constexpr uint32_t WEATHER_INTERVAL_STEP_MS = 30UL * 60UL * 1000UL;
constexpr uint8_t SAFE_BRIGHTNESS_MAX = 102;
constexpr uint32_t WEATHER_RETRY_MS = 5UL * 60UL * 1000UL;
constexpr uint32_t NTP_INTERVAL_MS = 24UL * 60UL * 60UL * 1000UL;
constexpr uint32_t NTP_RETRY_MS = 5UL * 60UL * 1000UL;
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;
constexpr uint32_t HTTP_TIMEOUT_MS = 8000;
constexpr uint32_t BOOT_GREETING_MS = 9000;
constexpr uint8_t WEATHER_PROVIDER_OPEN_METEO = 0;
constexpr uint8_t WEATHER_PROVIDER_OPEN_WEATHER_MAP = 1;
constexpr uint8_t WEATHER_PROVIDER_DWD = 2;
constexpr int LEFT_SLOT_X = 1;
constexpr int VALUE_SLOT_X = 13;
constexpr int VALUE_SLOT_W = 18;
constexpr int RIGHT_ICON_X = 21;
constexpr int LEFT_VALUE_X = 1;
constexpr int LEFT_VALUE_W = 18;
constexpr const char *DEFAULT_ADMIN_USERNAME = "admin";
constexpr const char *DEFAULT_ADMIN_PASSWORD = "pixelclock";
constexpr const char *DEFAULT_LANGUAGE = "de";
#define FIRMWARE_VERSION_TEXT "0.1.7"
constexpr const char *FIRMWARE_VERSION = FIRMWARE_VERSION_TEXT;
extern const char FIRMWARE_VERSION_BINARY_MARKER[];
constexpr uint8_t AUTH_CONFIG_VERSION = 1;
constexpr uint8_t MIN_ADMIN_PASSWORD_LENGTH = 8;

struct AppConfig {
  String ssid;
  String password;
  String adminUsername = DEFAULT_ADMIN_USERNAME;
  String adminPassword = DEFAULT_ADMIN_PASSWORD;
  String language = DEFAULT_LANGUAGE;
  String hostname = "pixelclock";
  String cityName = "Berlin";
  String locationLabel = "Berlin, Deutschland";
  String timezone = "CET-1CEST,M3.5.0,M10.5.0/3";
  float latitude = 52.52;
  float longitude = 13.41;
  uint8_t weatherProvider = WEATHER_PROVIDER_OPEN_METEO;
  uint8_t weatherIntervalHalfHours = DEFAULT_WEATHER_INTERVAL_HALF_HOURS;
  String openWeatherApiKey;
  uint8_t width = DEFAULT_WIDTH;
  uint8_t height = DEFAULT_HEIGHT;
  uint8_t dataPin = 18;
  uint8_t brightness = 64;
  bool fullBrightnessUnlocked = false;
  uint8_t wiringMode = DEFAULT_WIRING_MODE;
  uint8_t origin = 0;
  uint8_t displayMode = 0;
  uint8_t temperatureUnit = 0;
  bool weatherIconEnabled = true;
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

extern Preferences prefs;
extern AsyncWebServer server;
extern AppConfig config;
extern WeatherState weather;
extern CRGB leds[MAX_LEDS];
extern uint16_t ledCount;
extern uint8_t currentPage;
extern uint32_t lastPageSwitch;
extern uint32_t lastRender;
extern uint32_t lastNtpSync;
extern uint32_t lastNtpAttempt;
extern bool displayTest;
extern uint32_t displayTestUntil;
extern bool setupMode;
extern uint32_t bootStarted;
extern bool forceTextGradient;
extern bool authConfigMigrationNeeded;
extern bool pendingCityResolve;
extern bool pendingWeatherFetch;
extern bool pendingTimeSync;
extern bool pendingRestart;
extern uint32_t restartAt;

void loadConfig();
void saveConfig();
void keepFirmwareVersionBinaryMarker();
CRGB packedColor(uint32_t value);
String colorToHex(uint32_t value);
uint32_t parseColor(String value, uint32_t fallback);
String sanitizeHostname(String input);
String normalizeLanguage(String value);

bool connectWifi();
void startSetupAp();
void startMdns();
void syncTime();
void seedTimeFromBuild();
bool timeIsReasonable();

void setupFastLed();
void renderDisplay();
bool isNightTime();
float displayTemperature(float celsius);
String temperatureUnitText();
void drawBitmapIcon(int x, int y, const uint32_t *icon);

void fetchWeather();
bool resolveCity();
String urlEncode(const String &input);
String timezoneFromIana(const String &iana);
int normalizeOpenWeatherCode(int code);
int normalizeBrightSkyIcon(const char *icon);
void configureWeatherClient(WiFiClientSecure &client, uint8_t weatherProvider);

bool hasAdminPassword();
bool requireAdminAuth(AsyncWebServerRequest *request);
void sendJsonError(AsyncWebServerRequest *request, int code, const String &message);
void scheduleRestart(uint32_t delayMs = 800);
void setupServer();
