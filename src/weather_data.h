#pragma once
#include <stdint.h>
#include <math.h>

constexpr uint8_t WEATHER_PROVIDER_OPEN_METEO = 0;
constexpr uint8_t WEATHER_PROVIDER_OPEN_WEATHER_MAP = 1;
constexpr uint8_t WEATHER_PROVIDER_DWD = 2;
constexpr uint8_t WEATHER_PROVIDER_MET_NORWAY = 3;
constexpr uint8_t WEATHER_PROVIDER_WEATHER_API = 4;
constexpr uint8_t WEATHER_PROVIDER_MAX = WEATHER_PROVIDER_WEATHER_API;

struct WeatherReading {
  float temperature = NAN;
  float temperatureMax = NAN;
  float temperatureMin = NAN;
  int weatherCode = -1;
  bool isDay = true;
};
