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

## Before Opening a Pull Request

- Run `pio run`.
- If web files changed, test with `pio run --target uploadfs`.
- Check that both German and English README files stay aligned.
- Do not commit local build folders, generated binaries, Wi-Fi credentials, API keys, or editor cache files.
