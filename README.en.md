# Pixel Clock

[Deutsch](README.md) | English

ESP32-based pixel clock for WS2812B/NeoPixel matrices. The clock shows time, date, and weather on an LED matrix and is configured through a protected web interface.

## Features

- time, date, and weather on a 32x8 LED matrix
- configurable matrix size up to 64x16, maximum 512 LEDs
- row or column wiring, straight or serpentine
- configurable start corner, data pin, and color order
- day and night brightness in percent
- automatic page rotation or fixed page
- Open-Meteo without an API key or OpenWeatherMap with your own API key
- city-based location lookup with automatic time zone for many regions
- bilingual web interface, German/English, automatic browser language selection
- integrated help/wiki directly inside the web interface
- HTTP login before the web interface is shown

## Hardware

Tested target platform:

- ESP32 Dev Module, 4 MB flash
- WS2812B/NeoPixel matrix, default 32x8
- default data pin: GPIO 5
- separate, sufficiently powerful 5 V power supply for the LEDs

Important: Do not power larger LED matrices from the ESP32 5 V pin. Connect the LED power supply ground to ESP32 GND.

## Project Structure

```text
src/main.cpp                  PlatformIO firmware
src/weather_icons.h           Weather icons for the matrix
data/                         LittleFS web interface
platformio.ini                PlatformIO configuration
partitions.csv                Flash layout
docs/                         Additional material
```

## Default Login

The browser asks for a login before the web interface is shown.

```text
User: admin
Password: pixelclock
```

Change these credentials after the first setup under `Wi-Fi > Admin user` and `Admin password`.

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
5. Under `Wi-Fi`, set network, password, and browser address.
6. Under `Location, weather and access`, set city and weather provider.
7. Under `Display hardware`, set matrix size, data pin, start corner, and wiring.
8. Under `Brightness`, set brightness and night hours.
9. Press `Save`.
10. If the interface reports that a restart is required, press `Restart`.

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
- `Wi-Fi`: network, hostname, and login credentials.
- `Location, weather and access`: city, weather provider, API key, and time zone.
- `Display hardware`: matrix size, data pin, color order, and LED mapping.
- `Display and pages`: layout, time format, temperature format, and page rotation.
- `Colors`: colors for weekday, text, dots, and colon.
- `Brightness`: day and night brightness in percent plus night period.
- `Help & Wiki`: short setup guide and troubleshooting directly in the interface.

The language is selected automatically from your browser or system language. You can manually switch between German and English in the header. The selection is saved in the browser.

## Weather

The default provider is Open-Meteo. It does not require an API key.

OpenWeatherMap can be used optionally:

1. Create an OpenWeatherMap account.
2. Create an API key.
3. Select `OpenWeatherMap` in the web interface.
4. Enter the API key.
5. Save.

Weather data is fetched on startup and then about every 2 hours. You can trigger an update manually with `Refresh weather`.

## Flash Layout

The project uses a custom partition table:

```text
factory  0x180000  Firmware
littlefs 0x270000  Web interface and assets
```

The web interface is not embedded in the firmware binary. After changing anything in `data/`, always run `uploadfs`.

## Security

- The web interface is protected with HTTP Basic Auth.
- The setup access point uses the password `pixelclock`.
- Change the admin user and admin password after the first setup.
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
- Check night brightness and night period.
- Check the data pin, default is GPIO 5.
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

See [CONTRIBUTING.md](CONTRIBUTING.md) for pull request guidance.

## Credits

This project was designed, implemented, and documented together with Codex, an AI coding assistant from OpenAI.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Security

See [SECURITY.md](SECURITY.md) for vulnerability reporting guidance.
