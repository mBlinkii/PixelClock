# Performance and validation

Firmware 0.1.19 / web UI 0.1.17.

The display task yields instead of busy polling. TLS requests run in one worker
at idle priority so CPU-heavy library operations share time with the watchdog's
idle task. Requests have connect, handshake and body timeouts. All providers use
filtered JSON; MET parses only the first timeseries entry instead of allocating
the complete forecast. This adds a 12 KB task stack and a 1.5 KB previous-frame
buffer in exchange for responsive rendering during network requests.

## Restart diagnosis in 0.1.17

An intermittent return to the greeting was reported after 0.1.16. The greeting
is only selected during the first nine seconds after boot (apart from the
49.7-day `millis()` wrap); there is no normal periodic greeting or restart.

Code inspection found a concrete watchdog risk in the new weather worker:
ArduinoJson reads `Stream` via `readBytes()`, and Arduino ESP32 2.0.17 implements
`Stream::timedRead()` as a busy loop. The configured read timeout is eight
seconds, while this SDK watches CPU0's idle task with a five-second panic timeout.
The unpinned priority-1 worker could starve that task during a stalled response.
This fits the symptom; the specific device's reset cause has not been captured.
See Espressif's [task watchdog documentation](https://docs.espressif.com/projects/esp-idf/en/v4.4.5/esp32/api-reference/system/wdts.html).

Weather and geocoding now use `CooperativeReader` instead of `Stream::readBytes`
and `Stream::find`. It buffers 256 bytes, sleeps one RTOS tick when waiting and
yields regularly during continuous input. An eight-second idle deadline and
30-second total body deadline also bound trickle responses (underlying socket
calls retain their own timeouts). A failed response follows the existing backoff
and preserves the last valid weather. `WiFiClientSecure::setTimeout()` now
receives seconds, matching this Arduino core's API. The watchdog remains enabled.

Status now includes `resetReason`, `uptimeMs`, `minFreeHeap` and
`networkStackFreeBytes` (minimum unused worker stack, in ESP-IDF bytes). The new
web UI displays these without extra polling; older web images remain compatible.
The serial boot log prints the firmware version and reset reason at 115200 baud.
This uses ESP-IDF's [reset reason API](https://docs.espressif.com/projects/esp-idf/en/v4.4.5/esp32/api-reference/system/system.html#reset-reason),
not a new periodic flash write.

If the greeting returns, record Status's last boot reason and memory values
before removing power or requesting another restart. `Task-Watchdog` indicates
task starvation; `Unterspannung (Brownout)` indicates a detected voltage dip;
`Software-Absturz (Panic)` needs the serial panic/backtrace for further diagnosis.
A full power interruption can appear as `Einschalten / Stromversorgung`.
Installing the fix itself will normally show `Software-Neustart`.

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
fixtures for all five providers, null/missing values, Bright Sky `wind`/null
icons, failed retries and 32-bit timer rollover. They also cover Basic header
parsing and the login throttle. Slow, incomplete, disconnected and trickling network fixtures
verify scheduler pauses and both response deadlines, including timer rollover.
Node tests exercise production request coalescing, visibility
and retry scheduling, secret handling, asynchronous scan completion, LED order
parity with `xy()`, live-frame mapping, settings export/import and the first-setup
rule for the assistant. These
checks also run in the PlatformIO CI workflow.

For browser QA, run `node tests/mock-server.mjs` and open
`http://127.0.0.1:8765`. Any synthetic login works. Add `--first-run` (and for a
second instance `--port=8766`) to simulate a fresh clock in setup-AP mode with no
Wi-Fi and the default login, which opens the setup assistant. The mock renders a
clock frame for the live view and simulates preview, factory reset and, for the
password `test-throttle`, the login lock. This server binds only to loopback and
never controls hardware. Stop it after testing.

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

## Validation of 0.1.17 / web 0.1.13

- Both PlatformIO builds passed: 56,088 bytes static RAM; 1,222,021 bytes
  flash (81.1% of the OTA slot). The firmware image is 1,228,592 bytes and the
  LittleFS image is 1,114,112 bytes.
- Native C++ regression assertions and all seven Node tests passed, including
  stalled/trickling response deadlines, scheduler pauses and restart diagnostics.
- The status panel and German/English boot diagnostics were checked visually
  against synthetic API data. A fresh mock server port avoided the browser
  adapter's stale responses on the previously used port.
- Gzip companions round-trip to their source files: 107,850 bytes uncompressed /
  28,479 bytes compressed. Both image version markers were verified and SHA-256
  hashes are recorded in `dist/restart-fix-v0.1.17-sha256.txt`.
- Initial build checks did not flash a device. Subsequent on-device recovery and
  OTA validation are recorded below. An overnight run is still needed to assess
  the intermittent restart report.

## Firmware 0.1.18-0.1.19 / web UI 0.1.15-0.1.16

- Firmware: 1,249,389 bytes flash (82.9% of the OTA slot, +27 KB against 0.1.17)
  and 56,724 bytes static RAM with the pinned libraries (FastLED 3.10.3,
  ArduinoJson 7.4.3, AsyncTCP 3.5.0, ESPAsyncWebServer 3.12.1, espressif32
  7.0.0). FastLED 3.10.5 no longer compiled the frame comparison (ambiguous
  `fl::memcmp`) and, once fixed, produced 1,642,989 bytes, which exceeds the slot. The web assets are 212,956 bytes uncompressed /
  54,672 bytes gzip in seven files; the LittleFS image stays 1,114,112 bytes.
- The setup AP runs a DNS server only while it is active; `loop()` calls it
  every iteration. Station retries run every 60 s and pause while a device is
  connected to the AP, which also lets `/api/networks` scan reliably.
- The live view polls `/api/display/frame` (about 1.7 KB for 32x8) every second
  on the display page and every 3 s on the overview, only in a visible tab, and
  pauses after 5 minutes without input. Status polling is unchanged.
- PBKDF2 with 1000 iterations runs only for a new Authorization header or a
  password change; other requests compare a SHA-256 digest of the header.
- Bright Sky moved to Let's Encrypt's 2026 ECDSA chain (leaf <- YE1 <- Root YE
  <- ISRG Root X2 <- ISRG Root X1). OpenSSL verifies it with the embedded X1 alone,
  but with the added Root YE the ESP32 stops after two P-384 signatures instead of
  three plus one RSA-4096 signature. The embedded roots were checked against the
  live chains of all five providers and against the official PEMs from
  letsencrypt.org (identical public keys).
- Open-Meteo model selection: all 13 allow-listed models were queried live and
  returned temperature, weather code and daily min/max in about 580 bytes, the
  same size as best_match. BOM ACCESS-G returned no data and is not offered.
- The Bright Sky OpenAPI schema (2.2.9) allows `icon` values `wind` and `null`,
  which were decoded as "unknown" before; `condition` and `cloud_cover` now fill in.
- Not yet verified on hardware: captive-portal detection on Android/iOS/Windows,
  closing the AP after the router returns, the BOOT button reset and NVS erase,
  the hash migration from a 0.1.17 password, preview revert and the TLS time for
  the new chains. The native C++ tests compile (`-Wall -Wextra`) but were only
  run in CI, because no host compiler was available locally.

## Web UI 0.1.14 (setup assistant and redesign)

- Only existing endpoints are used; firmware 0.1.17 needs no change. Older
  firmware still falls back as before (provider list, power saving, scan mode).
- Static assets grow to 188,976 bytes uncompressed / 48,315 bytes gzip across
  seven files (one new request for `setup.js`). The LittleFS image builds at the
  unchanged 1,114,112 bytes; gzip companions round-trip to their sources.
- The overview clock advances locally between the existing 15-second status
  polls and only while the overview tab is visible; no extra requests are made.
- Eleven Node tests pass, including LED order parity with `xy()` for all origins
  and wirings, hostname/user normalization and keeping the session after a
  login change.
- Mock-server QA covered the complete first-run flow, German/English, dark and
  light mode, 375 px phone width without horizontal scrolling, validation of
  missing Wi-Fi password, API key, password mismatch and more than 512 LEDs.
- Not yet verified on hardware: the assistant against a real setup AP, the
  handoff to the home network and `.local` detection on Android/iOS/Windows.

## On-device recovery and OTA validation (13 September 2026)

The affected clock was reachable, running firmware 0.1.16, but authenticated
requests for `index.html`, `updates.js` and `index.html.gz` returned 404. Without
authentication, missing-file requests reached the API-style 401 fallback. This
confirmed unavailable web assets; it did not establish why they were missing.

The existing web update endpoint accepted the verified LittleFS 0.1.13 image.
After restart, HTML, CSS and all three JavaScript files returned HTTP 200 and
matched the local source byte for byte. Browser login, configuration and live
DWD weather display worked against the physical clock.

Firmware 0.1.17 was then installed through its separate firmware update endpoint.
The status API confirmed version 0.1.17 and `Software-Neustart`; the saved
configuration digest was unchanged and all web assets still matched. The first
weather fetch succeeded without an error. At approximately 31 seconds uptime,
the API reported 186,924 bytes free heap, 129,392 bytes minimum free heap and
7,672 bytes minimum unused weather-worker stack. These are a single boot's
diagnostics, not a long-duration stability or power-consumption measurement.

## Validation baseline (0.1.16 / web 0.1.12)

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
