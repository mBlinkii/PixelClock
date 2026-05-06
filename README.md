# Pixel Clock

Deutsch | [English](README.en.md)

ESP32-basierte Pixeluhr fuer WS2812B/NeoPixel-Matrizen. Die Uhr zeigt Zeit, Datum und Wetter auf einer LED-Matrix an und wird ueber eine geschuetzte Weboberflaeche eingerichtet.

## Funktionen

- Uhrzeit, Datum und Wetter auf einer 32x8-LED-Matrix
- konfigurierbare Matrixgroesse bis 64x16, maximal 512 LEDs
- Reihen- oder Spaltenverkabelung, gerade oder Serpentine
- Start-Ecke, Datenpin und Farbreihenfolge einstellbar
- Helligkeit und Nacht-Helligkeit in Prozent
- automatische Seitenrotation oder feste Seite
- Open-Meteo ohne API-Key oder OpenWeatherMap mit eigenem API-Key
- Standortsuche per Stadtname mit automatischer Zeitzone fuer viele Regionen
- zweisprachige Weboberflaeche, Deutsch/Englisch, automatische Browser-Sprachauswahl
- integrierte Hilfe/Wiki direkt in der Weboberflaeche
- HTTP-Login vor der Weboberflaeche

## Hardware

Getestete Zielplattform:

- ESP32 Dev Module, 4 MB Flash
- WS2812B/NeoPixel-Matrix, Standard 32x8
- Standard-Datenpin: GPIO 5
- separate, ausreichend starke 5-V-Stromversorgung fuer die LEDs

Wichtig: Versorge groessere LED-Matrizen nicht ueber den 5-V-Pin des ESP32. Verbinde die Masse der LED-Stromversorgung mit GND des ESP32.

## Projektstruktur

```text
src/main.cpp                  PlatformIO-Firmware
src/weather_icons.h           Wetter-Icons fuer die Matrix
data/                         LittleFS-Weboberflaeche
platformio.ini                PlatformIO-Konfiguration
partitions.csv                Flash-Layout
docs/                         Zusatzmaterial
```

## Standardzugang

Beim Zugriff auf die Weboberflaeche erscheint ein Browser-Login.

```text
Benutzer: admin
Passwort: pixelclock
```

Aendere diese Daten nach der ersten Einrichtung unter `WLAN > Admin-Benutzer` und `Admin-Passwort`.

Wenn keine WLAN-Verbindung moeglich ist, startet die Uhr einen Setup-Access-Point:

```text
WLAN: PixelClock-Setup
Passwort: pixelclock
Web UI: http://192.168.4.1
```

## Installation mit PlatformIO

1. PlatformIO installieren, zum Beispiel ueber VS Code oder die PlatformIO Core CLI.
2. Projektordner oeffnen.
3. Firmware bauen:

```powershell
pio run
```

4. ESP32 per USB verbinden und Firmware flashen:

```powershell
pio run --target upload
```

5. Weboberflaeche nach LittleFS hochladen:

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
3. `http://192.168.4.1` oeffnen.
4. Mit `admin` / `pixelclock` anmelden.
5. Unter `WLAN` Netzwerk, Passwort und Browser-Adresse setzen.
6. Unter `Ort, Wetter und Zugriff` Stadt und Wetteranbieter setzen.
7. Unter `Display-Hardware` Matrixgroesse, Datenpin, Start-Ecke und Verkabelung setzen.
8. Unter `Helligkeit` Helligkeit und Nachtzeiten einstellen.
9. `Speichern` druecken.
10. Wenn die Oberflaeche einen Neustart meldet, `Neustart` ausfuehren.

Nach erfolgreicher WLAN-Verbindung ist die Oberflaeche normalerweise erreichbar unter:

```text
http://pixelclock.local
```

Wenn du die Browser-Adresse geaendert hast, verwende entsprechend:

```text
http://<dein-hostname>.local
```

## Bedienung der Weboberflaeche

- `Status`: zeigt Wetter, Ort und Adresse.
- `WLAN`: Netzwerk, Hostname und Login-Daten.
- `Ort, Wetter und Zugriff`: Stadt, Wetteranbieter, API-Key und Zeitzone.
- `Display-Hardware`: Matrixgroesse, Datenpin, Farbreihenfolge und LED-Mapping.
- `Anzeige und Seiten`: Layout, Zeitformat, Temperaturformat und Seitenrotation.
- `Farben`: Farben fuer Wochentag, Text, Punkte und Doppelpunkt.
- `Helligkeit`: Tages- und Nacht-Helligkeit in Prozent sowie Nachtzeitraum.
- `Hilfe & Wiki`: kurze Einrichtungshilfe und Problembehandlung direkt im Interface.

Die Sprache wird automatisch anhand der Browser-/Systemsprache gewaehlt. Oben im Header kannst du manuell zwischen Deutsch und Englisch wechseln. Die Auswahl wird im Browser gespeichert.

## Wetter

Standard ist Open-Meteo. Dafuer ist kein API-Key noetig.

Optional kann OpenWeatherMap genutzt werden:

1. OpenWeatherMap-Konto erstellen.
2. API-Key erzeugen.
3. In der Weboberflaeche `OpenWeatherMap` auswaehlen.
4. API-Key eintragen.
5. Speichern.

Wetterdaten werden beim Start und danach etwa alle 2 Stunden aktualisiert. Manuell kannst du `Wetter aktualisieren` druecken.

## Flash-Layout

Das Projekt nutzt eine eigene Partitionstabelle:

```text
factory  0x180000  Firmware
littlefs 0x270000  Weboberflaeche und Assets
```

Die Weboberflaeche liegt nicht im Firmware-Binary. Nach Aenderungen an `data/` muss immer auch `uploadfs` ausgefuehrt werden.

## Sicherheit

- Die Weboberflaeche ist per HTTP Basic Auth geschuetzt.
- Der Setup-AP nutzt das Passwort `pixelclock`.
- Aendere nach der ersten Einrichtung den Admin-Benutzer und das Admin-Passwort.
- HTTP Basic Auth ist in einem normalen Heimnetz praktisch, aber nicht verschluesselt. Nutze die Uhr nicht ungeschuetzt in oeffentlichen oder fremden Netzwerken.

## Problembehandlung

### Weboberflaeche ist nicht erreichbar

- Pruefe, ob der ESP32 im WLAN verbunden ist.
- Oeffne die IP-Adresse aus dem Router statt `pixelclock.local`.
- Falls kein WLAN gespeichert ist, mit `PixelClock-Setup` verbinden und `http://192.168.4.1` oeffnen.
- Wenn mDNS nicht funktioniert, ist `*.local` im Netzwerk eventuell nicht aufloesbar.

### Login funktioniert nicht

- Standard ist `admin` / `pixelclock`.
- Wenn du Login-Daten geaendert hast, Browser-Login im Browser loeschen oder ein privates Fenster testen.
- Bei komplett verlorenen Daten hilft ein Werksreset ueber die Weboberflaeche, solange du noch eingeloggt bist.
- Ohne Zugriff musst du die NVS-Daten loeschen oder die Firmware mit einem Reset-Hilfsweg neu flashen.

### Wetter wird nicht angezeigt

- Pruefe WLAN-Verbindung und Internetzugriff.
- Pruefe unter `Status`, ob ein Fehler angezeigt wird.
- Bei OpenWeatherMap pruefen, ob der API-Key gueltig und aktiv ist.
- Bei falscher Stadt einen eindeutigeren Namen eingeben.
- Nach Aenderungen `Speichern` und danach `Wetter aktualisieren` druecken.

### Uhrzeit stimmt nicht

- WLAN und Internetzugriff pruefen.
- Zeitzone pruefen. Fuer Deutschland ist der Standard:

```text
CET-1CEST,M3.5.0,M10.5.0/3
```

- Nach Standortwechsel speichern und kurz warten, bis NTP synchronisiert.

### LEDs bleiben dunkel

- Helligkeit pruefen. 0% schaltet die Anzeige aus.
- Nacht-Helligkeit und Nachtzeitraum pruefen.
- Datenpin pruefen, Standard ist GPIO 5.
- GND zwischen ESP32 und LED-Netzteil verbinden.
- Matrix mit `Testmuster` pruefen.

### Farben sind falsch

- `Farbreihenfolge` zwischen `GRB` und `RGB` wechseln.
- Danach speichern und neu starten.

### Matrix ist gespiegelt oder durcheinander

- `Start-Ecke` pruefen.
- `LED-Verkabelung` zwischen Zeile/Spalte und Gerade/Serpentine umstellen.
- Mit `Testmuster` die Richtung pruefen.

### Aenderungen an der Weboberflaeche erscheinen nicht

- Nach Datei-Aenderungen in `data/` ausfuehren:

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

Siehe auch [CONTRIBUTING.md](CONTRIBUTING.md) fuer Hinweise zu Pull Requests.

## Credits

Dieses Projekt wurde gemeinsam mit Codex, einem KI-Coding-Assistenten von OpenAI, entworfen, implementiert und dokumentiert.

## Lizenz

Dieses Projekt steht unter der MIT-Lizenz. Details stehen in [LICENSE](LICENSE).

## Sicherheit

Hinweise zum Melden von Sicherheitsproblemen stehen in [SECURITY.md](SECURITY.md).
