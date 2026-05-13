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

src/display.cpp
  FastLED setup, matrix coordinate mapping, text/icon drawing, page rendering,
  brightness handling, and test pattern.

src/web_api.cpp
  HTTP Basic Auth, `/api/*` routes, JSON serialization, reset/restart actions,
  and static LittleFS serving.

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

data/app.js
  Browser-side app bootstrap, form serialization, API calls, status refresh,
  and browser-local UI state such as the one-time admin password reminder
  dismissal.

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
5. Connect to Wi-Fi, or start the `PixelClock-Setup` access point.
6. Start mDNS and the web server.
7. Resolve the configured city, sync NTP, and fetch weather once.

`loop()` stays non-blocking most of the time:

- renders the display about every 200 ms,
- switches display pages when automatic paging is enabled,
- processes deferred city/time/weather work requested by the web UI,
- retries weather and NTP on their configured intervals.

## Configuration Contract

`AppConfig` in `src/app_state.h` is the shared configuration model. When adding a
setting, update these places together:

- defaults in `AppConfig`,
- load/save keys in `src/config.cpp`,
- JSON output and POST parsing in `src/web_api.cpp`,
- form field list in `data/app.js`,
- markup in `data/index.html`,
- user-facing text/translations in `data/i18n.js`,
- README or troubleshooting notes if the setting affects setup.

Passwords and API keys intentionally use "leave empty to keep current" semantics
in the web UI. Do not echo saved secrets back to the browser.

The config JSON may expose safe metadata about secrets, such as whether an API
key exists or whether the admin password is still the factory default. It must
not expose the saved secret values themselves.

## Web API

All API routes are registered in `setupServer()` in `src/web_api.cpp`:

```text
GET  /api/config          current configuration for the form
POST /api/config          save configuration
GET  /api/status          live status for the header/status panel
GET  /api/networks        Wi-Fi scan results
POST /api/restart         restart the ESP32
POST /api/reset/settings  reset settings but keep Wi-Fi and admin login
POST /api/reset/factory   clear all persisted settings
POST /api/update/firmware upload a new firmware binary to the inactive OTA slot
POST /api/update/web      upload a new LittleFS image to the web UI partition
POST /api/weather/refresh queue a weather refresh
POST /api/display/test    show a temporary test pattern
```

Every API route requires HTTP Basic Auth. Static UI files are also served with
the same authentication.

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

`GET /api/config` includes `adminPasswordIsDefault`. The browser uses this to
show the first-run admin password reminder when the default login is still
active. The dismissal is stored only in browser `localStorage`; changing the
actual password remains a normal `POST /api/config` save.

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
- OpenWeatherMap needs a user-provided API key.
- DWD weather uses the Bright Sky JSON API for DWD open weather data and does
  not need an API key.
- City lookup uses Open-Meteo geocoding and stores latitude, longitude,
  location label, and a POSIX-style timezone string.
- NTP uses `configTime()` with the configured POSIX timezone.

The firmware pins HTTPS requests to root certificates embedded in
`src/weather.cpp`. If a provider changes its certificate chain, weather or
geocoding can fail until the root certificate is updated.

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
