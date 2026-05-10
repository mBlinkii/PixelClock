# Pixel Clock

Deutsch | [English](README.md)

ESP32-basierte Pixeluhr für WS2812B/NeoPixel-Matrizen. Die Uhr zeigt Zeit, Datum und Wetter auf einer LED-Matrix an und wird über eine geschützte Weboberfläche eingerichtet.

## Funktionen

- Uhrzeit, Datum und Wetter auf einer 32x8-LED-Matrix
- konfigurierbare Matrixgröße bis 64x16, maximal 512 LEDs
- Reihen- oder Spaltenverkabelung, gerade oder Serpentine
- Start-Ecke, Datenpin und Farbreihenfolge einstellbar
- Helligkeit und Nacht-Helligkeit in Prozent, standardmäßig auf 40% begrenzt
- automatische Seitenrotation oder feste Seite
- Open-Meteo ohne API-Key oder OpenWeatherMap mit eigenem API-Key
- Standortsuche per Stadtname mit automatischer Zeitzone für viele Regionen
- zweisprachige Weboberfläche, Deutsch/Englisch, automatische Browser-Sprachauswahl
- integrierte Hilfe/Wiki direkt in der Weboberfläche
- HTTP-Login vor der Weboberfläche
- Erinnerung zum Ändern des Standard-Admin-Passworts

## Hardware

Getestete Zielplattform:

- ESP32 Dev Module, 4 MB Flash
- WS2812B/NeoPixel-Matrix, Standard 32x8
- Standard-Datenpin: GPIO 18
- separate, ausreichend starke 5-V-Stromversorgung für die LEDs

Wichtig: Versorge größere LED-Matrizen nicht über den 5-V-Pin des ESP32. Verbinde die Masse der LED-Stromversorgung mit GND des ESP32.

## Projektstruktur

```text
src/main.cpp                  Firmware-Einstieg, setup() und loop()
src/app_state.h/.cpp          gemeinsame Typen, Konstanten und Laufzeitstatus
src/config.cpp                gespeicherte Einstellungen und Hilfsfunktionen
src/network_time.cpp          WLAN, Setup-AP, mDNS und NTP
src/weather.cpp               Wetter, Geocoding, Zeitzonen und HTTPS-Zertifikate
src/display.cpp               LED-Mapping, Text, Icons und Matrix-Rendering
src/web_api.cpp               HTTP-API, Auth, Neustart/Reset und LittleFS-Serving
src/weather_icons.h           Wetter-Icons für die Matrix
data/                         LittleFS-Weboberfläche
data/index.html               HTML der Konfigurationsoberfläche
data/app.js                   Web-UI-Logik, API-Aufrufe und Übersetzungen
data/app.css                  Styling der Weboberfläche
platformio.ini                PlatformIO-Konfiguration
partitions.csv                Flash-Layout
docs/                         Zusatzmaterial
```

Für neue Entwickler gibt es eine kompakte technische Übersicht in
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Standardzugang

Beim Zugriff auf die Weboberfläche erscheint ein Browser-Login.

```text
Benutzer: admin
Passwort: pixelclock
```

Ändere diese Daten nach der ersten Einrichtung unter `Admin-Zugriff`. Solange das Standardpasswort aktiv ist, erinnert dich die Weboberfläche beim Öffnen daran.

Wenn keine WLAN-Verbindung möglich ist, startet die Uhr einen Setup-Access-Point:

```text
WLAN: PixelClock-Setup
Passwort: pixelclock
Web UI: http://192.168.4.1
```

## Installation mit PlatformIO

1. PlatformIO installieren, zum Beispiel über VS Code oder die PlatformIO Core CLI.
2. Projektordner öffnen.
3. Firmware bauen:

```powershell
pio run
```

4. ESP32 per USB verbinden und Firmware flashen:

```powershell
pio run --target upload
```

5. Weboberfläche nach LittleFS hochladen:

```powershell
pio run --target uploadfs
```

6. ESP32 neu starten.

Falls `pio` nicht im PATH liegt, kann PlatformIO unter Windows zum Beispiel hier liegen:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
```

## Erste Einrichtung

1. ESP32 starten.
2. Falls die Uhr noch kein WLAN kennt, mit `PixelClock-Setup` verbinden.
3. `http://192.168.4.1` öffnen.
4. Mit `admin` / `pixelclock` anmelden.
5. Unter `WLAN-Zugang` Netzwerk, WLAN-Passwort und Browser-Adresse setzen.
6. Unter `Ort, Wetter und Zugriff` Stadt und Wetteranbieter setzen.
7. Unter `Display-Hardware` Matrixgröße, Datenpin, Start-Ecke und Verkabelung setzen.
8. Unter `Helligkeit` Helligkeit und Nachtzeiten einstellen.
9. Unter `Admin-Zugriff` das Standard-Admin-Passwort ändern.
10. `Speichern` drücken.
11. Wenn die Oberfläche einen Neustart meldet, `Neustart` ausführen.

Nach erfolgreicher WLAN-Verbindung ist die Oberfläche normalerweise erreichbar unter:

```text
http://pixelclock.local
```

Wenn du die Browser-Adresse geändert hast, verwende entsprechend:

```text
http://<dein-hostname>.local
```

## Bedienung der Weboberfläche

- `Status`: zeigt Wetter, Ort und Adresse.
- `WLAN-Zugang`: Netzwerk, WLAN-Passwort, Browser-Adresse und WLAN-Scan.
- `Admin-Zugriff`: Admin-Benutzer und Admin-Passwort für den Browser-Login.
- `Ort, Wetter und Zugriff`: Stadt, Wetteranbieter, API-Key und Zeitzone.
- `Display-Hardware`: Matrixgröße, Datenpin, Farbreihenfolge und LED-Mapping.
- `Anzeige und Seiten`: Layout, Zeitformat, Temperaturformat und Seitenrotation.
- `Farben`: Farben für Wochentag, Text, Punkte und Doppelpunkt.
- `Helligkeit`: Tages- und Nacht-Helligkeit in Prozent, Sicherheits-Freischalter und Nachtzeitraum.
- `Hilfe & Wiki`: kurze Einrichtungshilfe und Problembehandlung direkt im Interface.

Die Sprache wird automatisch anhand der Browser-/Systemsprache gewählt. Oben im Header kannst du manuell zwischen Deutsch und Englisch wechseln. Die Auswahl wird im Browser gespeichert.

## Wetter

Standard ist Open-Meteo. Dafür ist kein API-Key nötig.

Optional kann OpenWeatherMap genutzt werden:

1. OpenWeatherMap-Konto erstellen.
2. API-Key erzeugen.
3. In der Weboberfläche `OpenWeatherMap` auswählen.
4. API-Key eintragen.
5. Speichern.

Wetterdaten werden beim Start und danach im eingestellten Intervall aktualisiert. Standard sind 2 Stunden; in der Weboberfläche kannst du das Intervall in 0,5-Stunden-Schritten ändern. Manuell kannst du `Wetter aktualisieren` drücken.

## Flash-Layout

Das Projekt nutzt eine eigene Partitionstabelle:

```text
factory  0x180000  Firmware
littlefs 0x270000  Weboberfläche und Assets
```

Die Weboberfläche liegt nicht im Firmware-Binary. Nach Änderungen an `data/` muss immer auch `uploadfs` ausgeführt werden.

## Sicherheit

- Die Weboberfläche ist per HTTP Basic Auth geschützt.
- Der Setup-AP nutzt das Passwort `pixelclock`.
- Ändere nach der ersten Einrichtung den Admin-Benutzer und das Admin-Passwort unter `Admin-Zugriff`.
- Bei aktivem Standard-Admin-Passwort zeigt die Weboberfläche eine Erinnerung an. Du kannst direkt zum Passwortfeld springen oder die Erinnerung für diesen Browser ausblenden.
- HTTP Basic Auth ist in einem normalen Heimnetz praktisch, aber nicht verschlüsselt. Nutze die Uhr nicht ungeschützt in öffentlichen oder fremden Netzwerken.

## Problembehandlung

### Weboberfläche ist nicht erreichbar

- Prüfe, ob der ESP32 im WLAN verbunden ist.
- Öffne die IP-Adresse aus dem Router statt `pixelclock.local`.
- Falls kein WLAN gespeichert ist, mit `PixelClock-Setup` verbinden und `http://192.168.4.1` öffnen.
- Wenn mDNS nicht funktioniert, ist `*.local` im Netzwerk eventuell nicht auflösbar.

### Login funktioniert nicht

- Standard ist `admin` / `pixelclock`.
- Wenn du Login-Daten geändert hast, Browser-Login im Browser löschen oder ein privates Fenster testen.
- Starte die Uhr nach geänderten Login-Daten neu, wenn die Oberfläche einen Neustart meldet.
- Wenn die Admin-Erinnerung trotz geändertem Passwort erneut erscheint, Browser-Cache hart neu laden und prüfen, ob `Speichern` erfolgreich war.
- Bei komplett verlorenen Daten hilft ein Werksreset über die Weboberfläche, solange du noch eingeloggt bist.
- Ohne Zugriff musst du die NVS-Daten löschen oder die Firmware mit einem Reset-Hilfsweg neu flashen.

### Wetter wird nicht angezeigt

- Prüfe WLAN-Verbindung und Internetzugriff.
- Prüfe unter `Status`, ob ein Fehler angezeigt wird.
- Bei OpenWeatherMap prüfen, ob der API-Key gültig und aktiv ist.
- Bei falscher Stadt einen eindeutigeren Namen eingeben.
- Nach Änderungen `Speichern` und danach `Wetter aktualisieren` drücken.

### Uhrzeit stimmt nicht

- WLAN und Internetzugriff prüfen.
- Zeitzone prüfen. Für Deutschland ist der Standard:

```text
CET-1CEST,M3.5.0,M10.5.0/3
```

- Nach Standortwechsel speichern und kurz warten, bis NTP synchronisiert.

### LEDs bleiben dunkel

- Helligkeit prüfen. 0% schaltet die Anzeige aus.
- Ohne Freischalter ist die Helligkeit auf 40% begrenzt.
- Nacht-Helligkeit und Nachtzeitraum prüfen.
- Datenpin prüfen, Standard ist GPIO 18.
- GND zwischen ESP32 und LED-Netzteil verbinden.
- Matrix mit `Testmuster` prüfen.

### Farben sind falsch

- `Farbreihenfolge` zwischen `GRB` und `RGB` wechseln.
- Danach speichern und neu starten.

### Matrix ist gespiegelt oder durcheinander

- `Start-Ecke` prüfen.
- `LED-Verkabelung` zwischen Zeile/Spalte und Gerade/Serpentine umstellen.
- Mit `Testmuster` die Richtung prüfen.

### Änderungen an der Weboberfläche erscheinen nicht

- Nach Datei-Änderungen in `data/` ausführen:

```powershell
pio run --target uploadfs
```

- Browser-Cache leeren oder Seite hart neu laden.

## Entwicklung

Typischer PlatformIO-Ablauf:

```powershell
pio run
pio run --target upload
pio run --target uploadfs
pio device monitor
```

Gute Einstiegspunkte:

- `docs/ARCHITECTURE.md`: technischer Überblick und Änderungs-Checklisten.
- `src/main.cpp`: Boot-Ablauf und Hauptloop.
- `src/web_api.cpp`: API-Routen und Formular-Speicherlogik.
- `src/display.cpp`: Matrix-Rendering und LED-Koordinaten.
- `src/weather.cpp`: Wetter- und Standortlogik.
- `data/app.js`: Browser-Logik, Formular-Sync, Status-Refresh und Übersetzungen.
- `data/index.html`: Struktur der Weboberfläche.

Wenn du eine Einstellung erweiterst, müssen meist Firmware-Konfiguration,
API-JSON, Formularfeld, Übersetzungen und Doku gemeinsam angepasst werden.

Siehe auch [CONTRIBUTING.md](CONTRIBUTING.md) für Hinweise zu Pull Requests.

## Credits

Dieses Projekt wurde gemeinsam mit Codex, einem KI-Coding-Assistenten von OpenAI, entworfen, implementiert und dokumentiert.

## Lizenz

Dieses Projekt steht unter der MIT-Lizenz. Details stehen in [LICENSE](LICENSE).

## Sicherheit

Hinweise zum Melden von Sicherheitsproblemen stehen in [SECURITY.md](SECURITY.md).
