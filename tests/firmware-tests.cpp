#include <cassert>
#include <iostream>
#include "../src/runtime_policy.h"
#include "../src/weather_decode.h"

static WeatherReading parse(const char *json, uint8_t provider) {
  JsonDocument doc, filter;
  filterWeather(filter, provider);
  assert(!deserializeJson(doc, json, DeserializationOption::Filter(filter)));
  WeatherReading result;
  decodeWeather(doc, provider, result);
  return result;
}

int main() {
  const uint32_t interval = 7200000, retry = 300000;
  assert(weatherFetchDue(1000, 0, 0, interval, retry, false));
  assert(!weatherFetchDue(9000, 5000, 5000, interval, retry, false));
  assert(weatherFetchDue(interval + 5000, 5000, 5000, interval, retry, false));
  // Regression: an expired good reading must not cause an unbounded failure loop.
  assert(!weatherFetchDue(interval + 6000, 5000, interval + 5000, interval, retry, true));
  assert(weatherFetchDue(interval + 5000 + retry, 5000, interval + 5000, interval, retry, true));
  assert(!weatherFetchDue(50000, 0, 1000, interval, retry, true));
  assert(!weatherFetchDue(5000, 10, UINT32_MAX - 1000, interval, retry, true));
  assert(weatherFetchDue(400000, 10, UINT32_MAX - 1000, interval, retry, true));
  assert(deadlineReached(20, UINT32_MAX - 10));
  assert(!deadlineReached(UINT32_MAX - 10, 20));

  auto meteo = parse(R"({"current":{"temperature_2m":-3.5,"weather_code":71,"is_day":0},"daily":{"temperature_2m_min":[-7],"temperature_2m_max":[2]},"unused":[1,2,3]})", 0);
  assert(meteo.temperature == -3.5 && meteo.temperatureMin == -7 && meteo.temperatureMax == 2 && !meteo.isDay && meteo.weatherCode == 71);
  auto owm = parse(R"({"main":{"temp":21,"temp_min":19,"temp_max":23},"weather":[{"id":802,"icon":"03n"}]})", 1);
  assert(owm.temperature == 21 && owm.temperatureMin == 19 && owm.weatherCode == 2 && !owm.isDay);
  auto dwd = parse(R"({"weather":{"temperature":0,"icon":"sleet"}})", 2);
  assert(dwd.temperature == 0 && dwd.weatherCode == 67 && std::isnan(dwd.temperatureMin));
  auto met = parse(R"({"time":"2026-09-11T10:00:00Z","data":{"instant":{"details":{"air_temperature":12.5}},"next_1_hours":{"summary":{"symbol_code":"lightrainshowers_night"}}}})", 3);
  assert(met.temperature == 12.5 && met.weatherCode == 80 && !met.isDay && std::isnan(met.temperatureMax));
  auto fallback = parse(R"({"data":{"instant":{"details":{"air_temperature":6}},"next_6_hours":{"summary":{"symbol_code":"snow"}}}})", 3);
  assert(fallback.temperature == 6 && fallback.weatherCode == 71);
  auto wa = parse(R"({"current":{"temp_c":18.2,"is_day":1,"condition":{"code":1276}},"forecast":{"forecastday":[{"day":{"mintemp_c":8,"maxtemp_c":22},"hour":[{"temp_c":999}]}]}})", 4);
  assert(wa.temperature > 18 && wa.temperatureMin == 8 && wa.temperatureMax == 22 && wa.weatherCode == 95 && wa.isDay);
  for (uint8_t provider = 0; provider <= WEATHER_PROVIDER_MAX; ++provider) {
    assert(std::isnan(parse("{}", provider).temperature));
  }
  assert(std::isnan(parse(R"({"current":{"temp_c":null}})", 4).temperature));
  assert(normalizeMetSymbol("unknown") == -1);
  assert(normalizeMetSymbol(nullptr) == -1);
  assert(normalizeMetSymbol("heavysleetandthunder") == 95);
  assert(normalizeMetSymbol("partlycloudy_polartwilight") == 2);
  assert(normalizeWeatherApiCode(9999) == -1);
  assert(normalizeWeatherApiCode(1000) == 0);
  assert(normalizeWeatherApiCode(1168) == 57);
  assert(normalizeWeatherApiCode(1258) == 85);
  std::cout << "Firmware regression tests passed (scheduling, rollover, five providers, missing/null data, condition mapping).\n";
}
