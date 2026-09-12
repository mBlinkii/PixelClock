# Performance and validation

Firmware 0.1.16 / web UI 0.1.12.

The display task yields instead of busy polling. TLS requests run in one worker
and are limited by connect, handshake and stream timeouts. All providers use
filtered JSON; MET parses only the first timeseries entry instead of allocating
the complete forecast. This adds a 12 KB task stack and a 1.5 KB previous-frame
buffer in exchange for responsive rendering during network requests.

Weather failures retain the last valid reading and wait at least five minutes,
including failures after a previously successful scheduled refresh. Rate limiting
waits at least 30 minutes; Retry-After and MET cache times are respected up to one
day. Configuration revisions prevent old city/provider responses from overwriting
new settings. Geocoding only runs for a changed or unresolved city, including after
an offline change. Station outages continue to use Wi-Fi's automatic reconnect.

Static pages render at 1 Hz; animated gradients and setup screens at 5 Hz. Actual
LED transmissions happen only when the frame or brightness changes. Turning the
display off sends black once. A powered WS2812 still consumes quiescent current;
no software-only change disconnects its power supply. Modem sleep is configurable,
while CPU frequency and the device's continuous availability remain unchanged.

Status polling changes from 5 to 15 seconds (two-thirds fewer requests while the
tab is visible), backs off to 60 seconds after errors and stops in hidden tabs.
Requests have a timeout and a shared in-flight guard. Deterministic gzip files
are produced at build time; compression consumes no ESP CPU at request time.

## Reproducible checks

```sh
pio run
pio run --target buildfs
node tests/ui.test.cjs
g++ -std=c++17 -I .pio/libdeps/esp32dev/ArduinoJson/src tests/firmware-tests.cpp -o .pio/firmware-tests
.pio/firmware-tests
```

The C++ tests execute production filtering, decoding and scheduling code with
fixtures for all five providers, null/missing values, failed retries and 32-bit
timer rollover. Node tests exercise production request coalescing, visibility
and retry scheduling, secret handling and asynchronous scan completion. These
checks also run in the PlatformIO CI workflow.

For browser QA, run `node tests/mock-server.mjs` and open
`http://127.0.0.1:8765`. Any synthetic login works. This server binds only to
loopback and never controls hardware. Stop it after testing.

## Hardware checks still required

- Compare supply current with the same matrix, displayed page, brightness and
  access point, then with Wi-Fi power saving disabled/enabled and brightness 0%.
- Unplug the router during an update; verify the display stays responsive and
  automatic reconnection, refresh and error recovery work.
- Validate night transitions, every supported LED pin/wiring layout and low
  brightness without temporal dithering. Very low levels can appear different.
- Verify real WeatherAPI/OpenWeatherMap credentials, provider errors and OTA of
  both images. The keyless MET endpoint was tested with the embedded CA; keyed
  providers were checked against API documentation and decoding fixtures.
- Watch free/minimum heap over a multi-day run, during TLS and while uploading
  firmware or LittleFS. Build size is not a measurement of runtime free heap.

No current-consumption percentage is claimed without a physical measurement.

## Validation of this build

- Firmware and LittleFS builds passed with ESP32 Arduino 2.0.17, FastLED 3.10.3,
  ArduinoJson 7.4.3 and ESPAsyncWebServer 3.11.0.
- Linker report: 56,088 bytes static RAM and 1,223,973 bytes flash (81.2% of the
  1,507,328-byte OTA application slot). This excludes runtime task/JSON/TLS allocations.
- All C++ regression assertions and six Node regression tests passed.
- Desktop/mobile layout, provider controls, save/discard and language switching
  were inspected in a browser with synthetic API data. The browser adapter reused
  scan responses during that check; asynchronous scan completion was verified in
  the deterministic Node test and still needs checking on the ESP.
- Current web assets: 105,991 bytes uncompressed / 27,817 bytes gzip, a 73.8%
  transfer reduction. Every gzip companion was decompressed and compared with its source.
- Both generated images contain their expected version markers. No device was flashed.
