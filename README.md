# Pixel Clock

[Deutsch](README.de.md) | English

ESP32-based pixel clock for WS2812B/NeoPixel matrices. The clock shows time, date, and weather on an LED matrix and is configured through a protected web interface.

Current firmware version: `0.1.20`
Current LittleFS web interface version: `0.1.17`

### New in this version

- **Easy first setup:** After joining the setup Wi-Fi the setup page opens by itself. The
  assistant only asks for Wi-Fi, location and password; the LED matrix can be adjusted in
  the advanced setup when needed.
- **Hand the clock on:** A factory reset in the web interface or with the BOOT button erases all data.
- **Modern web interface** with tabs, live view of the matrix, instant preview, light/dark mode
  and settings export/import.
- **Weather:** Open-Meteo with a selectable weather model (e.g. DWD ICON); DWD / Bright Sky
  reliably shows a weather symbol again.
- **Security:** The admin password is stored only as a hash, logins lock after failed attempts.

Important: Going back to firmware 0.1.17 or older resets the admin login to
`admin` / `pixelclock`. Older notes are under [Release notes](#release-notes).

## Quick Start

1. **Flash once over USB:** Upload firmware and web interface with PlatformIO, see
   [Install the software](#2-install-the-software-once-over-usb).
2. **Connect to the clock:** On your phone choose the Wi-Fi `PixelClock-Setup-XXXXXX`
   (password `pixelclock`). The setup page opens automatically.
3. **Follow the assistant:** Log in with `admin` / `pixelclock` and follow the steps.
   Afterwards the clock runs in your Wi-Fi at `http://pixelclock.local`.

The detailed guide is under [First Setup](#first-setup).

## Features

- time, date, and weather on a 32x8 LED matrix
- configurable matrix size up to 64x16, maximum 512 LEDs
- row or column wiring, straight or serpentine
- configurable start corner, data pin, and color order
- day and night brightness in percent, limited to 40% by default
- automatic page rotation or fixed page
- Open-Meteo, DWD or MET Norway without an API key; OpenWeatherMap and WeatherAPI with your own API key
- city-based location lookup with automatic time zone for many regions
- bilingual web interface, German/English, with matching weekdays on the display
- guided setup assistant on the first start: Wi-Fi scan, LED matrix with wiring diagram and test pattern, location and admin password
- setup Wi-Fi with a per-device name; phones and laptops open the setup page automatically (captive portal)
- modern, phone-friendly web interface with tabs, save bar and automatic light/dark mode
- live view of the matrix in the browser and instant preview of display changes
- settings export and import
- full factory reset for handing the clock on, also with the BOOT button when the login is unknown
- admin password stored as a salted hash; failed logins are throttled
- integrated help/wiki directly inside the web interface
- Login page before settings are loaded
- reminder on the overview while the default admin password is active

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
src/admin_auth.h/.cpp         Admin password hashing, Basic Auth checks and login throttling
src/maintenance.cpp           BOOT button and full factory reset
src/weather_icons.h           Weather icons for the matrix
data/                         LittleFS web interface
data/index.html               HTML for the configuration interface
data/i18n.js                  Web UI translations and language selection
data/updates.js               Web UI update upload and version checks
data/setup.js                 Guided setup assistant
data/app.js                   Web UI logic, API calls, forms, and status refresh
data/app.css                  Styling for the web interface
platformio.ini                PlatformIO configuration
partitions.csv                Flash layout
docs/                         Additional material
```

New contributors can start with the compact technical overview in
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Default Login

The web interface shows its own login page before settings are loaded.

```text
User: admin
Password: pixelclock
```

The setup assistant asks for a new admin password; you can also change it later under `Wi-Fi & access` > `Admin access`. While the default password is still active, the web interface reminds you when it opens.

If no Wi-Fi connection is possible, the clock starts a setup access point:

```text
Wi-Fi: PixelClock-Setup-XXXXXX
Password: pixelclock (can be changed under Wi-Fi & access)
Web UI: http://192.168.4.1 (usually opens automatically)
```

`XXXXXX` are the last three bytes of the ESP32 MAC address; the matrix shows
them in setup mode as well. If a saved Wi-Fi is only temporarily unavailable, for
example after a power cut, the clock keeps retrying every minute and closes the
setup Wi-Fi by itself once it is connected and no device uses the setup Wi-Fi.

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

### Windows helper scripts

The project root contains small PowerShell launchers:

```powershell
.\build-pixel-clock.cmd
.\flash-pixel-clock.cmd
.\reset-pixel-clock.cmd
```

`reset-pixel-clock.cmd` erases the complete ESP32 flash with `pio run --target erase`.
This removes firmware, LittleFS web UI, Wi-Fi, settings, admin credentials, OTA slots,
and all persisted data. Flash firmware and the web UI again afterwards:

```powershell
.\flash-pixel-clock.cmd
```

## First Setup

This guide takes you from the wired clock to a running display. Allow about 10 minutes.

### 1. What you need

- the clock: an ESP32 with the LED matrix and its own 5 V power supply (see [Hardware](#hardware)),
- a PC with a USB cable for the first upload,
- a phone, tablet or laptop with Wi-Fi,
- the name and password of your Wi-Fi. The clock supports **2.4 GHz networks** only.

### 2. Install the software (once, over USB)

1. Install [PlatformIO](https://platformio.org/), most easily as a VS Code extension.
2. Download this project and open the folder in VS Code.
3. Connect the ESP32 over USB.
4. Upload firmware and web interface, either on Windows with the helper script

   ```powershell
   .\flash-pixel-clock.cmd
   ```

   or in a terminal in the project folder with

   ```powershell
   pio run --target upload
   pio run --target uploadfs
   ```

The clock then starts. The matrix briefly shows `HELLO` and then alternates between `WIFI`,
`AP` and a six-character id such as `A1B2C3`. This means the clock is waiting to be set up.
Later updates do not need USB, see [Install updates](#5-install-updates).

### 3. Set up the clock

1. **Join the setup Wi-Fi.** In the Wi-Fi settings of your phone or laptop choose
   `PixelClock-Setup-XXXXXX`, where `XXXXXX` is the id on the matrix. The password is `pixelclock`.
2. **Open the setup page.** It usually opens automatically ("Sign in to network"). If not,
   enter `http://192.168.4.1` in the browser. A "No internet" notice is normal here.
3. **Log in** with user `admin` and password `pixelclock`.
4. **Follow the assistant.** It opens by itself and has five steps:
   - **Language:** German or English. The weekdays on the clock follow this choice.
   - **Wi-Fi:** Tap your network in the list and enter the Wi-Fi password. If it is missing,
     tap `Search` or type the name yourself.
   - **Location and weather:** Enter your city and choose a weather service without an API
     key; Open-Meteo is recommended. Also choose °C or °F and the 12 or 24-hour format.
   - **Secure access:** Set your own admin password with at least 8 characters. This is
     strongly recommended; if needed, choose `Change later`.
   - **Summary:** Check everything and tap `Save and finish`.

   A ready-built or preconfigured clock needs no hardware settings. If you built the clock
   yourself or the display looks mirrored, scrambled or dark, switch on **Advanced setup:
   configure the LED matrix** in the summary. An extra step follows: the defaults fit most
   32×8 matrices. `Apply and show test pattern` shows a rainbow starting with red on the
   left. Mirrored? Change the start corner. Fragmented? Change the wiring. The diagram shows
   how the signal runs through the LEDs. The same settings are on the `Hardware` tab later.
5. **Back to your own Wi-Fi.** The clock restarts and joins your Wi-Fi; the setup Wi-Fi
   disappears. Reconnect your phone to your normal Wi-Fi. The assistant reports
   `Clock found!` as soon as the clock is reachable and offers a link.

If the clock does not connect, the Wi-Fi password was usually wrong or the network is a
5 GHz network. After about 20 seconds the clock opens the setup Wi-Fi again; repeat from step 1.

### 4. After the setup

- In your Wi-Fi the web interface is at `http://pixelclock.local`, or `http://<your-name>.local`
  if you changed the browser address.
- If `.local` does not work (some Android devices), look up the clock's IP address in your
  router, where it is listed as `pixelclock-XXXXXX`.
- Log in with `admin` and your new password.
- Every setting can be changed on its own tab at any time. You can start the assistant again
  under `System`; it only opens by itself on a new or reset clock.
- Tip: Export your settings to a file under `System` > `Back up settings`.

### 5. Install updates

1. Download `pixel-clock-firmware-vX.Y.Z.bin` and `pixel-clock-littlefs-vX.Y.Z.bin` from the
   [release page](https://github.com/mBlinkii/PixelClock/releases/latest).
2. In the web interface open `System` > `Firmware update`.
3. Select the firmware file first and press `Update firmware`. The clock restarts.
4. Then select the web interface file and press `Update web interface`.

Do not disconnect power during an update. Settings are kept.

## Using the Web Interface

- `Overview`: time on the clock, current weather, live view of the matrix, connection, address, and brightness. While Wi-Fi or an own admin password is missing, a setup card with a checklist is shown; it can be hidden.
- `Display`: live preview, layout, time and temperature format, page rotation, colors, day and night brightness, safety unlock, and Wi-Fi power saving. Changes appear on the clock immediately and revert after 2 minutes unless saved.
- `Weather`: city, weather provider, interval, API keys, and an optional manual time zone.
- `Hardware`: matrix size, data pin, color order, start corner, and wiring with a live wiring diagram and test pattern.
- `Wi-Fi & access`: network with scan, Wi-Fi password, Wi-Fi region, browser address, admin user, admin password, and the setup Wi-Fi password.
- `System`: firmware and web interface versions, diagnostics, setup assistant, settings export/import, updates, help & wiki, and reset.

A save bar appears as soon as something is unsaved, and tabs with unsaved changes
are marked. After changing the admin login you stay logged in with the new
credentials. The interface follows the light or dark mode of your device.

The language can be switched between German and English in the header. The selection is saved in the browser and on the clock, and the weekday labels on the display follow it.

The Wi-Fi region uses the ESP-IDF country codes supported by the ESP32 package:
`01`, `AT`, `AU`, `BE`, `BG`, `BR`, `CA`, `CH`, `CN`, `CY`, `CZ`, `DE`, `DK`,
`EE`, `ES`, `FI`, `FR`, `GB`, `GR`, `HK`, `HR`, `HU`, `IE`, `IN`, `IS`, `IT`,
`JP`, `KR`, `LI`, `LT`, `LU`, `LV`, `MT`, `MX`, `NL`, `NO`, `NZ`, `PL`, `PT`,
`RO`, `SE`, `SI`, `SK`, `TW`, and `US`. The default is `DE`; use `01` for
world safe mode. Changing the Wi-Fi region requires a restart.

## Performance and power

- Weather and geocoding run in a background task so the display keeps updating.
- Unchanged LED frames are not retransmitted. At 0% the matrix receives black once.
- Static pages are checked once per second; animations retain their 200 ms cadence.
- Wi-Fi power saving defaults to on and can be disabled under `Display`.
- Cached geocoding and changed-value-only NVS writes avoid unnecessary requests and flash writes.
- Compressed web assets, tab navigation, save/discard controls and status polling only in visible tabs.

Actual power savings depend on the matrix, brightness and access point and require measurement on hardware. At 0% the LEDs remain electrically powered. Deep sleep is not used so the clock and web interface stay available.

## Weather

| Provider | API key | Data |
| --- | --- | --- |
| Open-Meteo | No | Worldwide forecast, daily min/max; optional model of a national weather service |
| DWD / Bright Sky | No | Station observations, mainly Germany; no daily min/max; symbol falls back to precipitation and cloud cover |
| MET Norway | No | Worldwide forecast; no daily min/max |
| OpenWeatherMap | Yes | Current weather; min/max of current surrounding observations |
| WeatherAPI | Yes | Current weather and daily min/max |

Enter your own WeatherAPI key under `Location and weather`. Saved keys are never returned to the browser; blank fields preserve them.

MET Norway data is provided by the [Norwegian Meteorological Institute](https://www.met.no/) under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/); temperatures are rounded and symbols mapped. Firmware respects `Expires` and uses `If-Modified-Since`. [API terms](https://docs.api.met.no/doc/TermsOfService), [WeatherAPI documentation](https://www.weatherapi.com/docs/).

The default provider is Open-Meteo. It does not require an API key. Under
`Weather` > `Weather model` you can keep the automatic model choice or pick the
forecast model of a weather service: DWD ICON (Germany), ECMWF IFS, MeteoSwiss,
GeoSphere Austria, Météo-France, KNMI, DMI, UK Met Office, MET Nordic, ItaliaMeteo
ARPAE, NOAA GFS, Environment Canada GEM or JMA. National models are usually most
accurate in their region. Unlike DWD / Bright Sky (station observations), DWD ICON
via Open-Meteo is a forecast with daily min/max.

The `Deutscher Wetterdienst (DWD)` provider uses the Bright Sky JSON API for
DWD open weather data and also does not require an API key. Bright Sky derives
its icon as a best effort: it can be `wind` or missing. The firmware then uses
the reported precipitation and cloud cover instead of showing "no weather data".
Connection problems are shown with their cause, for example `TLS: ...` for
certificate errors or `Verbindung fehlgeschlagen (...)` for network errors.

OpenWeatherMap can be used optionally:

1. Create an OpenWeatherMap account.
2. Create an API key.
3. Select `OpenWeatherMap` in the web interface.
4. Enter the API key.
5. Save.

Weather data is fetched on startup and then on the configured interval. The default is 2 hours; you can change it in the web UI in 0.5-hour steps. You can trigger an update manually with `Refresh weather`. Failures wait at least 5 minutes before retrying; HTTP 429 waits at least 30 minutes. Valid readings are retained on errors. Manual requests also respect cache and retry delays.

## Flash Layout

The project uses a custom partition table:

```text
otadata  0x002000  OTA selector
app0     0x170000  Firmware slot 1
app1     0x170000  Firmware slot 2
littlefs 0x110000  Web interface and assets
```

The two app slots enable firmware updates through the web interface. After changing `partitions.csv`, flash the ESP32 once over USB with `pio run --target upload` and `pio run --target uploadfs`. After that, upload new firmware and web-interface binaries under `System` > `Firmware update`.

The web interface is not embedded in the firmware binary. After changing anything in `data/`, build a new LittleFS image. You can then update it either over USB with `uploadfs` or through the web interface.

## Security

- The web interface is protected with HTTP Basic Auth.
- The admin password is stored as a salted PBKDF2-HMAC-SHA256 hash, never in plain text.
- After five failed logins the clock blocks further attempts from that device for 30 seconds, doubling up to 5 minutes.
- The setup access point uses the password `pixelclock` until you set your own under `Wi-Fi & access` > `Setup Wi-Fi`.
- Change the admin user and admin password after the first setup under `Admin access`.
- While the default admin password is still active, the overview shows a setup card with a `Change password` button. It can be hidden for that browser.
- HTTP Basic Auth is practical in a normal home network, but it is not encrypted. Do not expose the clock in public or untrusted networks.

## Factory Reset and Handing the Clock On

Under `System` > `Reset`, `Factory reset` erases the complete settings storage
(NVS): Wi-Fi credentials and region, admin login, setup Wi-Fi password, location,
API keys and all display settings. Firmware and web interface stay installed. The
clock restarts in setup mode like a new device, so it can be handed on.

Without access to the web interface, hold the ESP32 `BOOT` button (GPIO 0) for
10 seconds while the clock is running. After 3 seconds the matrix counts down
`RESET 7` to `RESET 1`, then erases everything and restarts. Releasing the button
earlier cancels.

Export your settings under `System` > `Back up settings` first if you want to
restore them later. Passwords and API keys are not included in the file.

## Troubleshooting

### Web interface is not reachable

- Check whether the ESP32 is connected to Wi-Fi.
- Open the IP address from your router instead of `pixelclock.local`.
- If no Wi-Fi is saved, connect to `PixelClock-Setup-XXXXXX` and open `http://192.168.4.1`.
- If mDNS does not work, `*.local` may not resolve in your network.

### Login does not work

- The default is `admin` / `pixelclock`.
- If you changed credentials, log in again with the new user and password.
- After five wrong passwords, wait until the lock shown on the login page has expired.
- If the setup card still lists the admin password after changing it, hard reload the browser and check that `Save` succeeded.
- If you still have access, use the factory reset under `System`.
- Without access, hold the ESP32 `BOOT` button for 10 seconds while the clock is running (see [Factory reset](#factory-reset-and-handing-the-clock-on)).

### Weather is not shown

- Check Wi-Fi connection and internet access.
- Check the weather card under `Overview` for an error message.
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

### Completely erase the ESP32

- Connect the ESP32 through USB.
- Optionally list ports:

```powershell
.\reset-pixel-clock.cmd -ListPorts
```

- Erase the full flash:

```powershell
.\reset-pixel-clock.cmd -Port COM5
```

- Skip the confirmation prompt, for example in automation:

```powershell
.\reset-pixel-clock.cmd -Port COM5 -Force
```

- Flash firmware and the web UI again afterwards:

```powershell
.\flash-pixel-clock.cmd -Port COM5
```

## Release Notes

Firmware 0.1.20 shows the time page twice as long as date and weather by default
(16 instead of 8 seconds). Existing clocks keep their value; change it under
`Display` > `Time duration (s)`.

Firmware 0.1.19 fixes DWD weather: Bright Sky may report the icon `wind` or none
at all, which showed as "no weather data"; precipitation and cloud cover now
provide the symbol. The Let's Encrypt root certificates now cover the 2026
certificate chains, and connection errors name their cause. Open-Meteo can use
the forecast model of a specific weather service (e.g. DWD ICON). Web UI 0.1.17
reduces the assistant to Wi-Fi, location and password; the LED matrix is an optional
step of the advanced setup. Web UI 0.1.16 opens
the setup assistant only while no Wi-Fi is saved on the clock and replaces the
admin password popup with the setup card on the overview.

Firmware 0.1.18 and web UI 0.1.15 make the first setup and handing the clock on
easier: the setup Wi-Fi gets a per-device name and opens the web UI by itself
(captive portal), the test pattern works before Wi-Fi is configured, the clock
leaves setup mode on its own once the saved Wi-Fi is back, and a factory reset
(web UI or 10 s on the BOOT button) erases all data. The admin password is stored
as a salted hash and failed logins are throttled. The web UI adds a live view of
the matrix, instant preview of display changes and settings export/import.

Downgrading to firmware 0.1.17 or older resets the admin login to
`admin` / `pixelclock`, because older firmware cannot read the hashed password.

Version 0.1.17 fixes a watchdog risk when a weather response stalls. Network
reads now yield and have a total response deadline. The web UI shows uptime,
the last boot reason and memory reserves under `System`. If the greeting returns,
check the boot reason there before disconnecting power; see
[restart diagnosis](docs/PERFORMANCE.md#restart-diagnosis-in-0117).

Web UI 0.1.14 adds a guided setup assistant for the first start and a redesigned
tab-based interface with automatic light/dark mode. It only uses the existing
firmware API and does not require a firmware update.

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
- `data/setup.js`: guided setup assistant on top of the regular form fields.
- `data/app.js`: browser logic, tabs, form sync, and status refresh.
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

Tests and implementation details: [Performance and validation](docs/PERFORMANCE.md). New providers require firmware 0.1.16; the web UI disables them on older firmware. Update both firmware and LittleFS to receive all improvements.
