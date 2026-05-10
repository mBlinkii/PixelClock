# Contributing

Thanks for improving Pixel Clock.

## Development Workflow

Use PlatformIO for the main development path:

```powershell
pio run
pio run --target upload
pio run --target uploadfs
```

If `pio` is not in your PATH on Windows:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
```

## New Contributor Orientation

Start with [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) before editing. The
firmware is split by responsibility so you can start in the module that matches
your change:

- `src/main.cpp`: boot flow and main loop,
- `src/app_state.h`: shared model and cross-module declarations,
- `src/config.cpp`: configuration persistence and simple helpers,
- `src/network_time.cpp`: Wi-Fi, setup AP, mDNS, and NTP,
- `src/weather.cpp`: weather, geocoding, and provider certificates,
- `src/display.cpp`: FastLED, coordinate mapping, drawing, and pages,
- `src/web_api.cpp`: HTTP API, auth, reset/restart, and static UI serving.

When adding a new setting, update the firmware model, Preferences load/save,
API JSON, POST parsing, web form, translations, and documentation together.
Secrets such as Wi-Fi passwords and API keys should remain write-only from the
browser UI.

## Before Opening a Pull Request

- Run `pio run`.
- If web files changed, test with `pio run --target uploadfs`.
- If firmware changed, test on hardware and watch `pio device monitor` when possible.
- Check that both German and English README files stay aligned.
- Do not commit local build folders, generated binaries, Wi-Fi credentials, API keys, or editor cache files.
