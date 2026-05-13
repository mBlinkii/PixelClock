# Pixel Clock

[Deutsch](README.de.md) | English

ESP32-based pixel clock for WS2812B/NeoPixel matrices. The clock shows time, date, and weather on an LED matrix and is configured through a protected web interface.

Current firmware version: `0.1.3`
Current LittleFS web interface version: `0.1.7`

## Features

- time, date, and weather on a 32x8 LED matrix
- configurable matrix size up to 64x16, maximum 512 LEDs
- row or column wiring, straight or serpentine
- configurable start corner, data pin, and color order
- day and night brightness in percent, limited to 40% by default
- automatic page rotation or fixed page
- Open-Meteo or DWD without an API key, or OpenWeatherMap with your own API key
- city-based location lookup with automatic time zone for many regions
- bilingual web interface, German/English, automatic browser language selection
- integrated help/wiki directly inside the web interface
- HTTP login before the web interface is shown
- reminder to change the default admin password

## Hardware

Tested target platform:

- ESP32 Dev Module, 4 MB flash
- WS2812B/NeoPixel matrix, default 32x8
- default data pin: GPIO 18
- separate, sufficiently powerful 5 V power supply for the LEDs

Important: Do not power larger LED matrices from the ESP32 5 V pin. Connect the LED power supply ground to ESP32 GND.

## Project Structure

```text
src/main.cpp                  Firmware entry point, setup(), and loop()
src/app_state.h/.cpp          Shared types, constants, firmware version, and runtime state
src/config.cpp                Persisted settings and helpers
src/network_time.cpp          Wi-Fi, setup AP, mDNS, and NTP
src/weather.cpp               Weather, geocoding, time zones, and HTTPS certificates
src/display.cpp               LED mapping, text, icons, and matrix rendering
src/web_api.cpp               HTTP API, auth, restart/reset, and LittleFS serving
src/web_updates.cpp           Firmware and LittleFS OTA upload handlers
src/weather_icons.h           Weather icons for the matrix
data/                         LittleFS web interface
data/index.html               HTML for the configuration interface
data/i18n.js                  Web UI translations and language selection
data/updates.js               Web UI update upload and version checks
data/app.js                   Web UI logic, API calls, forms, and status refresh
data/app.css                  Styling for the web interface
platformio.ini                PlatformIO configuration
partitions.csv                Flash layout
docs/                         Additional material
```

New contributors can start with the compact technical overview in
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Default Login

The browser asks for a login before the web interface is shown.

```text
User: admin
Password: pixelclock
```

Change these credentials after the first setup under `Admin access`. While the default password is still active, the web interface reminds you when it opens.

If no Wi-Fi connection is possible, the clock starts a setup access point:

```text
Wi-Fi: PixelClock-Setup
Password: pixelclock
Web UI: http://192.168.4.1
```

## Installation with PlatformIO

1. Install PlatformIO, for example through VS Code or the PlatformIO Core CLI.
2. Open the project folder.
3. Build the firmware:

```powershell
pio run
```

4. Connect the ESP32 through USB and flash the firmware:

```powershell
pio run --target upload
```

5. Upload the web interface to LittleFS:

```powershell
pio run --target uploadfs
```

6. Restart the ESP32.

If `pio` is not in your PATH on Windows, PlatformIO may be located here:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
```

## First Setup

1. Start the ESP32.
2. If the clock does not know a Wi-Fi network yet, connect to `PixelClock-Setup`.
3. Open `http://192.168.4.1`.
4. Log in with `admin` / `pixelclock`.
5. Under `Wi-Fi access`, set network, Wi-Fi password, and browser address.
6. Under `Location, weather and access`, set city and weather provider.
7. Under `Display hardware`, set matrix size, data pin, start corner, and wiring.
8. Under `Brightness`, set brightness and night hours.
9. Under `Admin access`, change the default admin password.
10. Press `Save`.
11. If the interface reports that a restart is required, press `Restart`.

After a successful Wi-Fi connection, the interface is usually reachable at:

```text
http://pixelclock.local
```

If you changed the browser address, use:

```text
http://<your-hostname>.local
```

## Using the Web Interface

- `Status`: shows weather, location, and address.
- `Wi-Fi access`: network, Wi-Fi password, browser address, and Wi-Fi scan.
- `Admin access`: admin user and admin password for the browser login.
- `Location, weather and access`: city, weather provider, API key, and time zone.
- `Display hardware`: matrix size, data pin, color order, and LED mapping.
- `Display and pages`: layout, time format, temperature format, and page rotation.
- `Colors`: colors for weekday, text, dots, and colon.
- `Brightness`: day and night brightness in percent, safety unlock, and night period.
- `Help & Wiki`: short setup guide and troubleshooting directly in the interface.

The language is selected automatically from your browser or system language. You can manually switch between German and English in the header. The selection is saved in the browser.

## Weather

The default provider is Open-Meteo. It does not require an API key.

The `Deutscher Wetterdienst (DWD)` provider uses the Bright Sky JSON API for
DWD open weather data and also does not require an API key.

OpenWeatherMap can be used optionally:

1. Create an OpenWeatherMap account.
2. Create an API key.
3. Select `OpenWeatherMap` in the web interface.
4. Enter the API key.
5. Save.

Weather data is fetched on startup and then on the configured interval. The default is 2 hours; you can change it in the web UI in 0.5-hour steps. You can trigger an update manually with `Refresh weather`.

## Flash Layout

The project uses a custom partition table:

```text
otadata  0x002000  OTA selector
app0     0x170000  Firmware slot 1
app1     0x170000  Firmware slot 2
littlefs 0x110000  Web interface and assets
```

The two app slots enable firmware updates through the web interface. After changing `partitions.csv`, flash the ESP32 once over USB with `pio run --target upload` and `pio run --target uploadfs`. After that, upload new firmware and web-interface binaries in the web UI's `Firmware update` section.

The web interface is not embedded in the firmware binary. After changing anything in `data/`, build a new LittleFS image. You can then update it either over USB with `uploadfs` or through the web interface.

## Security

- The web interface is protected with HTTP Basic Auth.
- The setup access point uses the password `pixelclock`.
- Change the admin user and admin password after the first setup under `Admin access`.
- If the default admin password is still active, the web interface shows a reminder. You can jump directly to the password field or hide the reminder for that browser.
- HTTP Basic Auth is practical in a normal home network, but it is not encrypted. Do not expose the clock in public or untrusted networks.

## Troubleshooting

### Web interface is not reachable

- Check whether the ESP32 is connected to Wi-Fi.
- Open the IP address from your router instead of `pixelclock.local`.
- If no Wi-Fi is saved, connect to `PixelClock-Setup` and open `http://192.168.4.1`.
- If mDNS does not work, `*.local` may not resolve in your network.

### Login does not work

- The default is `admin` / `pixelclock`.
- If you changed credentials, clear the browser login cache or try a private window.
- After changing admin credentials, restart the clock if the interface reports that a restart is required.
- If the admin reminder still appears after changing the password, hard reload the browser and check that `Save` succeeded.
- If you still have access, use factory reset from the web interface.
- Without access, erase NVS data or reflash the firmware with a reset helper.

### Weather is not shown

- Check Wi-Fi connection and internet access.
- Check `Status` for an error message.
- If using OpenWeatherMap, make sure the API key is valid and active.
- If the city is ambiguous, enter a more specific name.
- After changes, press `Save` and then `Refresh weather`.

### Time is wrong

- Check Wi-Fi and internet access.
- Check the time zone. For Germany the default is:

```text
CET-1CEST,M3.5.0,M10.5.0/3
```

- After changing location, save and wait briefly for NTP sync.

### LEDs stay dark

- Check brightness. 0% turns the display off.
- Without the unlock switch, brightness is limited to 40%.
- Check night brightness and night period.
- Check the data pin, default is GPIO 18.
- Connect GND between ESP32 and LED power supply.
- Test the matrix with `Test pattern`.

### Colors are wrong

- Switch `Color order` between `GRB` and `RGB`.
- Save and restart afterward.

### Matrix is mirrored or scrambled

- Check `Start corner`.
- Change `LED wiring` between rows/columns and straight/serpentine.
- Use `Test pattern` to verify direction.

### Web interface changes do not appear

- After changing files in `data/`, run:

```powershell
pio run --target uploadfs
```

- Clear the browser cache or hard reload the page.

## Development

Typical PlatformIO workflow:

```powershell
pio run
pio run --target upload
pio run --target uploadfs
pio device monitor
```

Good entry points:

- `docs/ARCHITECTURE.md`: technical overview and change checklists.
- `src/main.cpp`: boot flow and main loop.
- `src/web_api.cpp`: API routes and form save logic.
- `src/display.cpp`: matrix rendering and LED coordinates.
- `src/weather.cpp`: weather and location logic.
- `data/i18n.js`: browser translations and language selection.
- `data/updates.js`: firmware/LittleFS upload flow and version checks.
- `data/app.js`: browser logic, form sync, and status refresh.
- `data/index.html`: web interface structure.

When adding or changing a setting, the firmware configuration, API JSON, form
field, translations, and documentation usually need to change together.

The firmware version shown in the web interface is set through
`FIRMWARE_VERSION` in `src/app_state.h` and exposed through `/api/status`.
Every firmware change should bump that version and keep the README files
aligned, even when the change is not part of a formal release yet.

The LittleFS web interface version is set in `data/updates.js` through
`littleFsVersionMarker`. Every change under `data/` should bump that version and
keep the README files aligned. The web interface shows both the installed
LittleFS version and, when possible, the version found in a selected LittleFS
update image before upload.

Firmware and LittleFS can be updated separately, so the web interface must stay
compatible with the firmware API that is already installed. Do not remove or
rename update routes such as `/api/update/firmware` and `/api/update/web`
without keeping the old route as an alias. For new features, expose capability
flags through `/api/status` and keep a browser fallback for older firmware.

See [CONTRIBUTING.md](CONTRIBUTING.md) for pull request guidance.

## Credits

This project was designed, implemented, and documented together with Codex, an AI coding assistant from OpenAI.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Security

See [SECURITY.md](SECURITY.md) for vulnerability reporting guidance.
