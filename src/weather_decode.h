#pragma once
#include <ArduinoJson.h>
#include "weather_data.h"
#include "weather_codes.h"

inline void filterWeather(JsonDocument &filter, uint8_t provider) {
  if (provider == WEATHER_PROVIDER_OPEN_WEATHER_MAP) {
    for (const char *key : {"temp", "temp_min", "temp_max"}) filter["main"][key] = true;
    filter["weather"][0]["id"] = true;
    filter["weather"][0]["icon"] = true;
  } else if (provider == WEATHER_PROVIDER_DWD) {
    filter["weather"]["temperature"] = true;
    filter["weather"]["icon"] = true;
  } else if (provider == WEATHER_PROVIDER_WEATHER_API) {
    filter["current"]["temp_c"] = true;
    filter["current"]["is_day"] = true;
    filter["current"]["condition"]["code"] = true;
    filter["forecast"]["forecastday"][0]["day"]["mintemp_c"] = true;
    filter["forecast"]["forecastday"][0]["day"]["maxtemp_c"] = true;
  } else if (provider == WEATHER_PROVIDER_MET_NORWAY) {
    filter["data"]["instant"]["details"]["air_temperature"] = true;
    filter["data"]["next_1_hours"]["summary"]["symbol_code"] = true;
    filter["data"]["next_6_hours"]["summary"]["symbol_code"] = true;
  } else {
    for (const char *key : {"temperature_2m", "weather_code", "is_day"}) filter["current"][key] = true;
    filter["daily"]["temperature_2m_max"][0] = true;
    filter["daily"]["temperature_2m_min"][0] = true;
  }
}

inline void decodeWeather(JsonDocument &doc, uint8_t provider, WeatherReading &sample) {
  sample.temperatureMin = NAN;
  sample.temperatureMax = NAN;
  if (provider == WEATHER_PROVIDER_OPEN_WEATHER_MAP) {
    sample.temperature = doc["main"]["temp"] | NAN;
    sample.temperatureMin = doc["main"]["temp_min"] | NAN;
    sample.temperatureMax = doc["main"]["temp_max"] | NAN;
    sample.weatherCode = normalizeOpenWeatherCode(doc["weather"][0]["id"] | -1);
    const char *icon = doc["weather"][0]["icon"] | "";
    sample.isDay = strlen(icon) < 3 || icon[2] != 'n';
  } else if (provider == WEATHER_PROVIDER_DWD) {
    sample.temperature = doc["weather"]["temperature"] | NAN;
    const char *icon = doc["weather"]["icon"] | "";
    sample.weatherCode = normalizeBrightSkyIcon(icon);
    sample.isDay = strstr(icon, "night") == nullptr;
  } else if (provider == WEATHER_PROVIDER_WEATHER_API) {
    sample.temperature = doc["current"]["temp_c"] | NAN;
    sample.temperatureMin = doc["forecast"]["forecastday"][0]["day"]["mintemp_c"] | NAN;
    sample.temperatureMax = doc["forecast"]["forecastday"][0]["day"]["maxtemp_c"] | NAN;
    sample.weatherCode = normalizeWeatherApiCode(doc["current"]["condition"]["code"] | -1);
    sample.isDay = (doc["current"]["is_day"] | 1) == 1;
  } else if (provider == WEATHER_PROVIDER_MET_NORWAY) {
    sample.temperature = doc["data"]["instant"]["details"]["air_temperature"] | NAN;
    const char *symbol = doc["data"]["next_1_hours"]["summary"]["symbol_code"] |
      (doc["data"]["next_6_hours"]["summary"]["symbol_code"] | "");
    sample.weatherCode = normalizeMetSymbol(symbol);
    sample.isDay = strstr(symbol, "night") == nullptr;
  } else {
    sample.temperature = doc["current"]["temperature_2m"] | NAN;
    sample.temperatureMax = doc["daily"]["temperature_2m_max"][0] | NAN;
    sample.temperatureMin = doc["daily"]["temperature_2m_min"][0] | NAN;
    sample.weatherCode = doc["current"]["weather_code"] | -1;
    sample.isDay = (doc["current"]["is_day"] | 1) == 1;
  }
}

