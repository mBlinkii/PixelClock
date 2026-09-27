#include <cassert>
#include <iostream>
#include <string>
#include <algorithm>
#include "../src/runtime_policy.h"
#include "../src/weather_decode.h"
#include "../src/cooperative_reader.h"

struct TestClock {
  static inline uint32_t time = 0, pauses = 0;
  static uint32_t now() { return time; }
  static void pause() { ++time; ++pauses; }
  static void reset(uint32_t start = 0) { time = start; pauses = 0; }
};

struct TestClient {
  std::string data;
  size_t position = 0, chunkSize = 256, reads = 0;
  uint32_t readyAt = TestClock::now(), gapMs = 0, readCostMs = 0;
  bool keepOpen = false;
  int available() {
    return deadlineReached(TestClock::now(), readyAt)
        ? static_cast<int>(std::min(chunkSize, data.size() - position)) : 0;
  }
  int read(uint8_t *output, size_t count) {
    count = std::min(count, static_cast<size_t>(available()));
    memcpy(output, data.data() + position, count);
    position += count;
    ++reads;
    TestClock::time += readCostMs;
    readyAt = TestClock::now() + gapMs;
    return static_cast<int>(count);
  }
  bool connected() { return keepOpen || position < data.size(); }
};

using TestReader = CooperativeReader<TestClient, TestClock>;

static void testNetworkReader() {
  for (uint32_t start : {0U, UINT32_MAX - 4000}) {
    TestClock::reset(start);
    TestClient stalled{"{\"current\":{\"temperature_2m\":"};
    stalled.keepOpen = true;
    TestReader reader(stalled, 8000, 30000);
    JsonDocument doc;
    const auto error = deserializeJson(doc, reader);
    assert(error == DeserializationError::IncompleteInput);
    assert(TestClock::now() - start == 8000);
    // A stalled response must sleep every tick, not spin for the 8 s timeout.
    assert(TestClock::pauses >= 8000);
    stalled.data += "12}}";
    assert(reader.read() == -1); // A timed-out response cannot resume later.
  }

  TestClock::reset();
  TestClient slow{R"({"current":{"temperature_2m":12.5,"weather_code":2,"is_day":1}})"};
  slow.chunkSize = 1;
  slow.gapMs = 150;
  TestReader slowReader(slow, 8000, 30000);
  JsonDocument doc, filter;
  filterWeather(filter, WEATHER_PROVIDER_OPEN_METEO);
  assert(!deserializeJson(doc, slowReader, DeserializationOption::Filter(filter)));
  WeatherReading reading;
  decodeWeather(doc, WEATHER_PROVIDER_OPEN_METEO, reading);
  assert(reading.temperature == 12.5 && reading.weatherCode == 2);
  assert(TestClock::pauses > 5000); // Successful, but longer than the ESP watchdog.

  TestClock::reset(UINT32_MAX - 10000);
  const uint32_t start = TestClock::now();
  TestClient trickle{std::string(4000, ' ')};
  trickle.chunkSize = 1;
  trickle.gapMs = 100;
  TestReader bounded(trickle, 8000, 30000);
  assert(!bounded.find("missing"));
  assert(TestClock::now() - start == 30000); // Total limit, despite arriving data.
  assert(TestClock::pauses == 30000);

  TestClock::reset();
  TestClient met{std::string(250, ' ') + R"(""timeseries":[{"data":{"instant":{"details":{"air_temperature":6}}}}])"};
  met.readCostMs = 20;
  TestReader metReader(met, 8000, 30000);
  assert(metReader.find("\"timeseries\"") && metReader.find("["));
  filter.clear();
  filterWeather(filter, WEATHER_PROVIDER_MET_NORWAY);
  assert(!deserializeJson(doc, metReader, DeserializationOption::Filter(filter)));
  decodeWeather(doc, WEATHER_PROVIDER_MET_NORWAY, reading);
  assert(reading.temperature == 6 && TestClock::pauses >= 2);
  assert(met.reads == 2); // Buffered TLS reads, including a split search marker.

  TestClock::reset();
  TestClient disconnected{""};
  TestReader ended(disconnected, 8000, 30000);
  char buffer[8];
  assert(ended.readBytes(buffer, sizeof(buffer)) == 0 && TestClock::pauses == 0);
  TestClient bytes{"A\xff"};
  TestReader binary(bytes, 8000, 30000);
  assert(binary.readBytes(buffer, sizeof(buffer)) == 2);
  assert(buffer[0] == 'A' && static_cast<uint8_t>(buffer[1]) == 255);
}

static WeatherReading parse(const char *json, uint8_t provider) {
  JsonDocument doc, filter;
  filterWeather(filter, provider);
  assert(!deserializeJson(doc, json, DeserializationOption::Filter(filter)));
  WeatherReading result;
  decodeWeather(doc, provider, result);
  return result;
}

int main() {
  testNetworkReader();
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
  std::cout << "Firmware regression tests passed (cooperative network reads, deadlines, scheduling, rollover, five providers, missing/null data, condition mapping).\n";
}
