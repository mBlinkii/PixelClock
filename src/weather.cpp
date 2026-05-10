#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "app_state.h"

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

void configureWeatherClient(WiFiClientSecure &client, bool openWeatherMap) {
  client.setCACert(openWeatherMap ? SECTIGO_PUBLIC_SERVER_AUTH_ROOT_R46 : ISRG_ROOT_X1);
}

// Weather and geocoding requests run synchronously but are triggered only from
// setup(), explicit UI actions, or scheduled loop() intervals.
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
