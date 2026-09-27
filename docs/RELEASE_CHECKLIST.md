# Release Checklist

Use this before publishing a release or pushing a public branch.

## Code

- Bump `FIRMWARE_VERSION` in `src/app_state.h` for every firmware change.
- Bump `littleFsVersionMarker` in `data/updates.js` for every LittleFS web UI change.
- Keep existing `/api/update/firmware` and `/api/update/web` routes working.
- For changed web/API contracts, keep old routes as aliases or add `/api/status` capability detection.
- Run `pio run`.
- Platform and library versions are pinned in `platformio.ini`. Update them
  deliberately, rebuild and keep the firmware below the 1,507,328-byte OTA slot
  (FastLED 3.10.5, for example, grows it to about 1.64 MB and no longer fits).
- Make sure the `PlatformIO` workflow is green on the commit you tag; the
  `Release` workflow only builds and does not run the tests.
- Run `.\build-pixel-clock.cmd` when you need the two versioned upload binaries in `dist/`.
- Flash firmware with `pio run --target upload` when firmware changed.
- Flash LittleFS with `pio run --target uploadfs` when files in `data/` changed.

## Documentation

- Keep `README.md` and `README.de.md` aligned.
- Keep the firmware version in the README files aligned with `FIRMWARE_VERSION`.
- Keep the LittleFS web interface version in the README files aligned with `littleFsVersionMarker`.
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
- Push a `vX.Y.Z` tag after merging the release commit. The `Release` GitHub
  Actions workflow builds firmware and LittleFS images, creates the GitHub
  release, and uploads the versioned `.bin` files.
