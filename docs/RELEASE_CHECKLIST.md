# Release Checklist

Use this before publishing a release or pushing a public branch.

## Code

- Run `pio run`.
- Flash firmware with `pio run --target upload` when firmware changed.
- Flash LittleFS with `pio run --target uploadfs` when files in `data/` changed.

## Documentation

- Keep `README.md` and `README.de.md` aligned.
- Update troubleshooting notes when behavior changes.

## Security

- Do not commit real Wi-Fi credentials.
- Do not commit real OpenWeatherMap API keys.
- Confirm default login and setup AP warnings are documented.
- Confirm the admin password reminder appears only while the default admin password is active.

## Git

- Check `git status --short`.
- Check ignored files with `git status --ignored --short` if unsure.
- Avoid committing `.pio/`, generated firmware binaries, local VS Code C++ config, or upload logs.
