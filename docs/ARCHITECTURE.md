# Architecture

This document is the fast map for new contributors. It explains where the main
pieces live and which files usually need to change together.

## Runtime Shape

Pixel Clock is an ESP32 Arduino/PlatformIO project with two deployable parts:

- Firmware in `src/`, built and uploaded with `pio run --target upload`.
- LittleFS web UI in `data/`, uploaded separately with `pio run --target uploadfs`.

The firmware serves the web UI from LittleFS and exposes JSON endpoints under
`/api/*`. The browser UI reads and writes those endpoints; it is not bundled
into the firmware binary.

The partition table uses two OTA app slots. Once that layout and the LittleFS
image have been flashed over USB, later firmware and LittleFS image binaries
can be uploaded through the web UI.

## Main Files

```text
src/main.cpp
  Firmware entry point. Keeps boot orchestration and the main loop easy to scan.

src/app_state.h
  Shared constants, firmware version, structs, globals, and cross-module
  function declarations.

src/app_state.cpp
  Definitions for shared runtime state such as config, weather, server, LEDs,
  timers, and deferred work flags.

src/config.cpp
  Preferences load/save, color conversion, and hostname sanitizing.

src/network_time.cpp
  Wi-Fi connection, setup access point, mDNS, NTP sync, and build-time fallback.

src/weather.cpp
  HTTPS root certificates, weather provider requests, city geocoding, and
  timezone mapping.

src/cooperative_reader.h
  Buffered JSON input with scheduler pauses, idle and total body deadlines.
  Shared by weather and geocoding; tested with simulated slow/disconnected clients.

src/diagnostics.cpp
  ESP reset reason labels for the serial boot log and authenticated status API.

src/admin_auth.h
  Portable helpers without Arduino types: base64 and Basic header parsing,
  constant-time comparison, the per-client login throttle and the device suffix
  formatter. Covered by the native tests in tests/firmware-tests.cpp.

src/admin_auth.cpp
  Salted PBKDF2-HMAC-SHA256 admin password hashing, `requireAdminAuth()` for API
  routes (401/429 responses) and `isAdminAuthorized()` for upload chunks.

src/maintenance.cpp
  BOOT button countdown and the full factory reset that erases the NVS partition.

src/display.cpp
  FastLED setup, matrix coordinate mapping, text/icon drawing, page rendering,
  brightness handling, and test pattern.

src/web_api.cpp
  Captive-portal redirect, `/api/*` routes, JSON serialization, display preview
  and frame mirror, reset/restart actions, and static LittleFS serving.

src/web_updates.cpp
  Firmware and LittleFS OTA upload handlers used by the update routes.

src/web_updates.h
  Route callback declarations for the OTA handlers.

src/weather_icons.h
  Packed 8 px high bitmap weather icons used by the matrix renderer.

data/index.html
  Static markup for the configuration interface.

data/app.css
  Styling for the configuration interface.

data/i18n.js
  Browser-side language selection and translation strings.

data/updates.js
  Firmware/LittleFS version scanning and browser-side binary upload flow.

data/setup.js
  Guided setup assistant. Its controls mirror regular form fields through
  `data-bind="<field id>"`, so it saves through the same formBody()/saveConfig()
  path and needs no firmware endpoint of its own.

data/app.js
  Browser-side app bootstrap, hash-based page tabs, form serialization, API
  calls, status refresh, and browser-local UI state such as the admin reminder
  and setup assistant dismissal.

platformio.ini
  Board, framework, filesystem, partition table, and library dependencies.

partitions.csv
  Flash layout. Keep enough room for both firmware and LittleFS assets.
```

## Firmware Flow

`setup()` performs the boot sequence:

1. Load persisted settings from ESP32 Preferences.
2. Seed the clock from build time until NTP is available.
3. Configure FastLED for the selected matrix pin and color order.
4. Mount LittleFS.
5. Connect to Wi-Fi, or start the `PixelClock-Setup-XXXXXX` access point with its captive-portal DNS.
6. Start mDNS and the web server.
7. Start the network worker, resolve the city only when its cached name differs, start asynchronous SNTP, and queue weather.

`loop()` services the setup access point (DNS, station retries, closing the AP)
and the BOOT button, ends an expired display preview, checks static display pages
every second and animations every 200 ms, switches pages, then yields for 20 ms.
A pending factory reset erases NVS right before `ESP.restart()`. Unchanged frames and repeated black frames
are not transmitted. Temporal LED dithering is disabled because frames are latched.

`src/network_worker.cpp` serializes weather/geocoding in one 12 KB FreeRTOS task.
It copies configuration under `StateLock`, performs HTTPS outside the lock and
publishes only if the weather revision still matches. UI handlers and rendering
use the same recursive mutex. SNTP reports successful sync through its callback
and owns its daily schedule/retries; build time is not reported as an NTP success.
See [PERFORMANCE.md](PERFORMANCE.md) for validation and known limits.

## Configuration Contract

`AppConfig` in `src/app_state.h` is the shared configuration model. When adding a
setting, update these places together:

- defaults in `AppConfig`,
- load/save keys in `src/config.cpp`,
- JSON output and POST parsing in `src/web_api.cpp`,
- form field list in `data/app.js`,
- markup in `data/index.html` (inside the matching `.page` tab),
- a `data-bind` control in `data/setup.js`/`index.html` if first-time users need it,
- user-facing text/translations in `data/i18n.js`,
- README or troubleshooting notes if the setting affects setup.

Passwords and API keys intentionally use "leave empty to keep current" semantics
in the web UI. Do not echo saved secrets back to the browser.

The config JSON may expose safe metadata about secrets, such as whether an API
key exists or whether the admin password is still the factory default. It must
not expose the saved secret values themselves.

Visual settings listed in `DisplayPreviewFields` can be changed temporarily via
`POST /api/display/preview`. While a preview runs, `saveConfig()` persists the
backed-up values, `GET /api/config` reports them, and `POST /api/config` ends the
preview with the submitted values. Add new visual settings to the struct, to
`readDisplayParams()` in `src/web_api.cpp` and to `previewFields` in `data/app.js`.

The admin password is stored as `adminSalt`/`adminHash` (auth version 2). An
empty hash means the factory default is active. Firmware 0.1.17 and older stored
the password in plain text (auth version 1); loading such data hashes it once.
Older firmware reading version 2 falls back to the default login.

## Web API

All API routes are registered in `setupServer()` in `src/web_api.cpp`:

```text
GET  /api/setup           public: firstSetup flag so a new clock needs no login
POST /api/recovery/start  public: show a 6-digit recovery code on the matrix
POST /api/recovery/finish public: code + adminPassword sets a new admin password
GET  /api/config          current configuration for the form
POST /api/config          save configuration
GET  /api/status          live status for the header/status panel
GET  /api/networks        Wi-Fi scan results
POST /api/restart         restart the ESP32
POST /api/reset/settings  reset settings but keep Wi-Fi and admin login
POST /api/reset/factory   erase the whole NVS partition and restart in setup mode
POST /api/update/firmware upload a new firmware binary to the inactive OTA slot
POST /api/update/web      upload a new LittleFS image to the web UI partition
POST /api/weather/refresh queue a weather refresh
POST /api/display/test    show a temporary test pattern
GET  /api/display/frame   last LED frame (physical order, RRGGBB hex) for the live view
POST /api/display/preview apply visual settings temporarily (reverts after 2 minutes)
POST /api/display/preview/cancel  restore the saved visual settings
```

Every API route requires HTTP Basic Auth. Static UI files serve the login shell publicly; credentials are sent by the browser when calling the protected API.
The firmware verifies the password against its PBKDF2 hash only for a new
Authorization header and remembers the SHA-256 digest of the last valid header
until the credentials change. After five failures a client gets 429 with
`retryAfterSeconds` (30 s doubling to 5 min). `/api/display/preview/cancel` must
stay registered before `/api/display/preview` because routes also match subpaths.

While the setup AP runs, a `CaptivePortalRedirect` handler registered first
redirects requests for foreign host names that arrive on the AP interface to
`http://192.168.4.1/`, so phones open the web UI after joining the AP. Station
retries pause while a device is connected to the AP (ESP-IDF cannot scan while
the station connects); once the saved Wi-Fi works and the AP is unused for five
seconds, the AP and DNS server close without a restart.

`GET /api/status` exposes `firmwareVersion`, sourced from `FIRMWARE_VERSION` in
`src/app_state.h`. Bump that constant for every firmware change and keep README
version mentions aligned.

The LittleFS web interface version lives in `littleFsVersionMarker` in
`data/updates.js`. Bump it for every change under `data/` and keep README version
mentions aligned. The browser shows this installed web UI version in the status
panel and scans selected LittleFS update images for the same marker before
uploading them.

## Web/Firmware Compatibility

Firmware and LittleFS can be updated independently. Because of that, the web UI
must stay compatible with the firmware API that is already installed on the
device.

Keep the update endpoints stable:

```text
POST /api/update/firmware
POST /api/update/web
```

Do not rename or remove those routes unless the old route remains as an alias
for at least one release. If the web UI needs a new firmware feature, add a
capability or version field to `GET /api/status` and make the browser choose a
fallback path when the field is missing. This prevents a separately uploaded web
UI from showing 404 errors on devices that have not received the matching
firmware yet.

The setup assistant opens by itself only while the clock has no saved SSID
(first start or after a factory reset); this is decided from `GET /api/config`,
not from browser storage, so a configured clock never shows it after login.
While `adminPasswordIsDefault` is set, the overview shows a non-modal setup card
with a password shortcut instead. Because the firmware checks Basic Auth against
the live configuration, the browser replaces its stored credentials after a
successful login change instead of forcing a new login.

Firmware before 0.1.18 shows its WIFI/AP prompt instead of the test pattern while
no SSID is saved; the assistant therefore asks for Wi-Fi first and saves before it
requests a test pattern (`capabilities.setupTestPattern` hides the hint on newer
firmware). After the final restart it polls the public `favicon.svg` of the new
`.local` address to tell the user when the clock is reachable in the home network.

`/api/status` reports `capabilities` (`captivePortal`, `setupApPassword`,
`setupTestPattern`, `displayFrame`, `displayPreview`, `fullFactoryReset`,
`resetButton`, `loginThrottle`, `openSetup`, `passwordRecovery`) plus `setupApSsid` and `routerHostname`; the web UI
hides the related controls when they are missing.

## Display Pipeline

The renderer in `src/display.cpp` works from low-level pixels upward:

1. `xy()` maps logical matrix coordinates to physical LED indexes.
2. `px()` writes a bounded pixel.
3. Text/icon helpers draw small glyphs and weather symbols.
4. Page functions draw clock, date, or weather views.
5. `renderDisplay()` clears the matrix, selects the active page, and calls
   `FastLED.show()`.

If a matrix looks mirrored or scrambled, inspect `xy()`, `wiringMode`, and
`origin` before changing drawing code.

## Weather And Time

- Open-Meteo is the default weather provider and does not need an API key.
  `weatherModel` (NVS `wModel`) adds `&models=<id>`; ids are allow-listed in
  `src/weather_models.h` and mirrored by `openMeteoModels` in `data/app.js`
  (a UI test compares both lists). Empty means Open-Meteo's best_match.
- OpenWeatherMap needs a user-provided API key.
- DWD weather uses the Bright Sky JSON API for DWD open weather data and does
  not need an API key. Its `icon` may be `wind` or null; `condition` and
  `cloud_cover` then provide the symbol.
- City lookup uses Open-Meteo geocoding and stores latitude, longitude,
  location label, and a POSIX-style timezone string. Input that contains a
  4–5 digit postal code (`parsePostalQuery()` in `location_query.h`, optional
  country prefix like `D-`, `AT-`, `CH `) goes to OpenStreetMap Nominatim
  (`postalcode` + `countrycodes`, default: the Wi-Fi country), because
  Open-Meteo misses many German codes or matches them abroad. A country-limited
  Open-Meteo name search for the found place then supplies the time zone.
  Nominatim uses a Let's Encrypt chain and allows 1 request/s with an
  identifying User-Agent; lookups only run after the location changes.
- MET Norway uses Locationforecast compact and WeatherAPI uses a one-day forecast.
- Provider filters and decoders live in `weather_decode.h`; normalized symbols in `weather_codes.h`.
- NTP uses asynchronous `configTzTime()` with the configured POSIX timezone.

The firmware pins HTTPS requests to root certificates embedded in
`src/weather.cpp`. If a provider changes its certificate chain, weather or
geocoding can fail until the root certificate is updated. Let's Encrypt hosts
(Open-Meteo, Bright Sky, WeatherAPI) use `LETS_ENCRYPT_ROOTS`: ISRG Root X1 and
X2 plus the 2025 roots YE and YR, so the ESP32 verifies the shorter chains of the
2026 YE1/YR1 intermediates. Connection failures report the mbedtls error text
(`TLS: ...`) instead of a bare `HTTP -1`. Check a chain with
`openssl s_client -connect host:443 -servername host -CAfile roots.pem -no-CApath -no-CAstore`.

## Before Changing Behavior

Run at least:

```powershell
pio run
```

When `data/` changed, also upload LittleFS on hardware:

```powershell
pio run --target uploadfs
```

To build the upload binaries without flashing anything, use:

```powershell
.\build-pixel-clock.cmd
```

It creates versioned copies of `firmware.bin` and `littlefs.bin` in `dist/`.

For firmware or LittleFS changes, use the web UI's update section after the OTA
partition layout is already on the device, or flash over USB and watch the
serial monitor for firmware changes:

```powershell
pio run --target upload
pio device monitor
```

## Web delivery and scanning

`scripts/prepare_web.py` generates deterministic `.gz` companions before the
PlatformIO build. ESPAsyncWebServer prefers those files and derives ETags from
their CRC. Static responses revalidate with `Cache-Control: no-cache`; API JSON
uses `no-store`. Original assets remain in LittleFS for version-marker scanning.
Do not commit generated `.gz` files.

`GET /api/networks` starts an asynchronous scan and responds with `scanning: true`
until results are ready. The UI polls only for the duration of an explicit scan
and also accepts synchronous responses from older firmware. Firmware reports
capabilities under `/api/status`; `/api/config` includes `weatherProviderMax` and
the optional `wifiPowerSave` setting so newer controls can be disabled on old firmware.
