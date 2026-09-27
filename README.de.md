# Pixel Clock

Deutsch | [English](README.md)

ESP32-basierte Pixeluhr für WS2812B/NeoPixel-Matrizen. Die Uhr zeigt Zeit, Datum und Wetter auf einer LED-Matrix an und wird über eine geschützte Weboberfläche eingerichtet.

Aktuelle Firmware-Version: `0.1.19`
Aktuelle LittleFS-Weboberflächen-Version: `0.1.16`

### Neu in dieser Version

- **Einfache Ersteinrichtung:** Nach dem Verbinden mit dem Setup-WLAN öffnet sich die
  Einrichtungsseite von selbst, ein Assistent führt durch alle Schritte.
- **Uhr weitergeben:** Werksreset in der Weboberfläche oder per BOOT-Taste löscht alle Daten.
- **Moderne Weboberfläche** mit Tabs, Live-Ansicht der Matrix, Sofort-Vorschau,
  Hell-/Dunkelmodus und Export/Import der Einstellungen.
- **Wetter:** Open-Meteo mit wählbarem Wettermodell (z. B. DWD ICON); DWD / Bright Sky
  zeigt wieder zuverlässig ein Wettersymbol.
- **Sicherheit:** Admin-Passwort nur noch als Hash gespeichert, Login-Sperre nach Fehlversuchen.

Wichtig: Ein Wechsel zurück auf Firmware 0.1.17 oder älter setzt den Admin-Login auf
`admin` / `pixelclock` zurück. Ältere Hinweise stehen unter [Versionshinweise](#versionshinweise).

## Schnellstart

1. **Einmalig per USB aufspielen:** Firmware und Weboberfläche mit PlatformIO auf den ESP32
   laden, siehe [Software aufspielen](#2-software-aufspielen-einmalig-per-usb).
2. **Mit der Uhr verbinden:** Am Handy das WLAN `PixelClock-Setup-XXXXXX` wählen
   (Passwort `pixelclock`). Die Einrichtungsseite öffnet sich automatisch.
3. **Assistent durchklicken:** Mit `admin` / `pixelclock` anmelden und den Schritten folgen.
   Danach läuft die Uhr in deinem WLAN unter `http://pixelclock.local`.

Die ausführliche Anleitung steht unter [Erste Einrichtung](#erste-einrichtung).

## Funktionen

- Uhrzeit, Datum und Wetter auf einer 32x8-LED-Matrix
- konfigurierbare Matrixgröße bis 64x16, maximal 512 LEDs
- Reihen- oder Spaltenverkabelung, gerade oder Serpentine
- Start-Ecke, Datenpin und Farbreihenfolge einstellbar
- Helligkeit und Nacht-Helligkeit in Prozent, standardmäßig auf 40% begrenzt
- automatische Seitenrotation oder feste Seite
- Open-Meteo, DWD oder MET Norway ohne API-Key; OpenWeatherMap und WeatherAPI mit eigenem API-Key
- Standortsuche per Stadtname mit automatischer Zeitzone für viele Regionen
- zweisprachige Weboberfläche, Deutsch/Englisch, mit passenden Wochentagen auf dem Display
- Einrichtungsassistent beim ersten Start: WLAN-Suche, LED-Matrix mit Verkabelungsdiagramm und Testmuster, Standort und Admin-Passwort
- Setup-WLAN mit Namen pro Gerät; Handys und Laptops öffnen die Einrichtungsseite automatisch (Captive Portal)
- moderne, handytaugliche Weboberfläche mit Tabs, Speicherleiste und automatischem Hell-/Dunkelmodus
- Live-Ansicht der Matrix im Browser und Sofort-Vorschau von Anzeige-Änderungen
- Einstellungen exportieren und importieren
- vollständiger Werksreset zum Weitergeben, auch per BOOT-Taste, wenn der Login unbekannt ist
- Admin-Passwort als gesalzener Hash gespeichert; fehlgeschlagene Logins werden gebremst
- integrierte Hilfe/Wiki direkt in der Weboberfläche
- Login-Seite vor dem Laden der Einstellungen
- Hinweis auf der Übersicht, solange das Standard-Admin-Passwort aktiv ist

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
src/app_state.h/.cpp          gemeinsame Typen, Konstanten, Firmware-Version und Laufzeitstatus
src/config.cpp                gespeicherte Einstellungen und Hilfsfunktionen
src/network_time.cpp          WLAN, Setup-AP, mDNS und NTP
src/weather.cpp               Wetter, Geocoding, Zeitzonen und HTTPS-Zertifikate
src/display.cpp               LED-Mapping, Text, Icons und Matrix-Rendering
src/web_api.cpp               HTTP-API, Auth, Neustart/Reset und LittleFS-Serving
src/web_updates.cpp           Firmware- und LittleFS-OTA-Upload-Handler
src/admin_auth.h/.cpp         Admin-Passwort-Hash, Basic-Auth-Prüfung und Login-Bremse
src/maintenance.cpp           BOOT-Taste und vollständiger Werksreset
src/weather_icons.h           Wetter-Icons für die Matrix
data/                         LittleFS-Weboberfläche
data/index.html               HTML der Konfigurationsoberfläche
data/i18n.js                  Web-UI-Uebersetzungen und Sprachauswahl
data/updates.js               Web-UI-Updates und Versionspruefungen
data/setup.js                 Einrichtungsassistent
data/app.js                   Web-UI-Logik, API-Aufrufe, Formulare und Status
data/app.css                  Styling der Weboberfläche
platformio.ini                PlatformIO-Konfiguration
partitions.csv                Flash-Layout
docs/                         Zusatzmaterial
```

Für neue Entwickler gibt es eine kompakte technische Übersicht in
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Standardzugang

Beim Zugriff auf die Weboberfläche erscheint eine eigene Login-Seite.

```text
Benutzer: admin
Passwort: pixelclock
```

Der Einrichtungsassistent fragt nach einem neuen Admin-Passwort; später kannst du es unter `WLAN & Zugang` > `Admin-Zugriff` ändern. Solange das Standardpasswort aktiv ist, erinnert dich die Weboberfläche beim Öffnen daran.

Wenn keine WLAN-Verbindung möglich ist, startet die Uhr einen Setup-Access-Point:

```text
WLAN: PixelClock-Setup-XXXXXX
Passwort: pixelclock (änderbar unter WLAN & Zugang)
Web UI: http://192.168.4.1 (öffnet sich meist automatisch)
```

`XXXXXX` sind die letzten drei Bytes der ESP32-MAC-Adresse; die Matrix zeigt sie
im Setup-Modus ebenfalls an. Ist ein gespeichertes WLAN nur vorübergehend nicht
erreichbar, etwa nach einem Stromausfall, versucht die Uhr es jede Minute erneut
und schließt das Setup-WLAN selbst, sobald sie verbunden ist und kein Gerät mehr
am Setup-WLAN hängt.

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

### Windows-Hilfsskripte

Im Projektordner liegen kleine Starter fuer PowerShell:

```powershell
.\build-pixel-clock.cmd
.\flash-pixel-clock.cmd
.\reset-pixel-clock.cmd
```

`reset-pixel-clock.cmd` loescht den kompletten ESP32-Flash mit `pio run --target erase`.
Dabei gehen Firmware, LittleFS-Weboberflaeche, WLAN, Einstellungen, Admin-Zugangsdaten,
OTA-Slots und alle gespeicherten Daten verloren. Danach Firmware und Weboberflaeche wieder flashen:

```powershell
.\flash-pixel-clock.cmd
```

## Erste Einrichtung

Diese Anleitung führt von der fertig verkabelten Uhr bis zur laufenden Anzeige.
Rechne mit etwa 10 Minuten.

### 1. Das brauchst du

- die Uhr: ESP32 mit angeschlossener LED-Matrix und eigenem 5-V-Netzteil (siehe [Hardware](#hardware)),
- einen PC mit USB-Kabel für das erste Aufspielen,
- ein Handy, Tablet oder Notebook mit WLAN,
- den Namen und das Passwort deines WLANs. Die Uhr unterstützt nur **2,4-GHz-Netze**.

### 2. Software aufspielen (einmalig per USB)

1. [PlatformIO](https://platformio.org/) installieren, am einfachsten als Erweiterung für VS Code.
2. Dieses Projekt herunterladen und den Ordner in VS Code öffnen.
3. ESP32 per USB anschließen.
4. Firmware und Weboberfläche aufspielen, entweder unter Windows mit dem Hilfsskript

   ```powershell
   .\flash-pixel-clock.cmd
   ```

   oder in einem Terminal im Projektordner mit

   ```powershell
   pio run --target upload
   pio run --target uploadfs
   ```

Danach startet die Uhr. Die Matrix zeigt kurz `HELLO` und dann abwechselnd `WIFI`, `AP`
und eine sechsstellige Kennung wie `A1B2C3`. Das bedeutet: Die Uhr wartet auf die Einrichtung.
Spätere Updates brauchen kein USB mehr, siehe [Updates einspielen](#5-updates-einspielen).

### 3. Uhr einrichten

1. **Mit dem Setup-WLAN verbinden.** Am Handy oder Notebook in den WLAN-Einstellungen
   `PixelClock-Setup-XXXXXX` wählen. `XXXXXX` ist die Kennung von der Matrix.
   Das Passwort lautet `pixelclock`.
2. **Einrichtungsseite öffnen.** Meist öffnet sich die Seite automatisch („Im Netzwerk
   anmelden“). Falls nicht, im Browser `http://192.168.4.1` eingeben. Die Meldung
   „Kein Internet“ ist hier normal.
3. **Anmelden** mit Benutzer `admin` und Passwort `pixelclock`.
4. **Dem Assistenten folgen.** Er öffnet sich von selbst und hat sechs Schritte:
   - **Sprache:** Deutsch oder Englisch. Auch die Wochentage auf der Uhr folgen dieser Wahl.
   - **WLAN:** Dein Netz aus der Liste antippen und das WLAN-Passwort eingeben.
     Steht dein Netz nicht in der Liste, auf `Suchen` tippen oder den Namen selbst eintragen.
   - **LED-Matrix:** Die Vorgaben passen für die meisten 32×8-Matrizen. Mit
     `Übernehmen und Testmuster zeigen` erscheint ein Regenbogen, links beginnend mit Rot.
     Ist er gespiegelt, die Start-Ecke ändern; ist er zerstückelt, die Verkabelung ändern.
     Das Diagramm zeigt, wie das Signal durch die LEDs läuft.
   - **Standort und Wetter:** Deine Stadt eintragen. Wetterdienst ohne API-Key wählen,
     empfohlen ist Open-Meteo. Dazu Temperatur in °C oder °F und 12- oder 24-Stunden-Format.
   - **Zugang absichern:** Ein eigenes Admin-Passwort mit mindestens 8 Zeichen festlegen.
     Das empfehlen wir dringend; notfalls geht es später über `Später ändern`.
   - **Zusammenfassung:** Alles prüfen und `Speichern und abschließen` tippen.
5. **Zurück ins eigene WLAN.** Die Uhr startet neu und verbindet sich mit deinem WLAN. Das
   Setup-WLAN verschwindet. Verbinde dein Handy wieder mit deinem normalen WLAN. Der
   Assistent meldet `Uhr gefunden!`, sobald die Uhr erreichbar ist, und bietet einen Link an.

Verbindet sich die Uhr nicht, war meist das WLAN-Passwort falsch oder das Netz ist ein
5-GHz-Netz. Nach etwa 20 Sekunden öffnet die Uhr wieder das Setup-WLAN; dann ab Schritt 1
wiederholen.

### 4. Nach der Einrichtung

- Die Weboberfläche erreichst du im eigenen WLAN unter `http://pixelclock.local`. Hast du die
  Browser-Adresse geändert, unter `http://<dein-name>.local`.
- Klappt `.local` nicht (manche Android-Geräte), die IP-Adresse der Uhr im Router nachsehen.
  Dort heißt sie `pixelclock-XXXXXX`.
- Anmelden mit `admin` und deinem neuen Passwort.
- Alle Einstellungen lassen sich jederzeit einzeln in den Tabs ändern. Den Assistenten
  kannst du unter `System` erneut starten; von selbst erscheint er nur bei einer neuen oder
  zurückgesetzten Uhr.
- Tipp: Unter `System` > `Einstellungen sichern` die Einstellungen als Datei exportieren.

### 5. Updates einspielen

1. Auf der [Release-Seite](https://github.com/mBlinkii/PixelClock/releases/latest) die beiden
   Dateien `pixel-clock-firmware-vX.Y.Z.bin` und `pixel-clock-littlefs-vX.Y.Z.bin` herunterladen.
2. In der Weboberfläche `System` > `Firmware-Update` öffnen.
3. Zuerst die Firmware-Datei auswählen und `Firmware aktualisieren` drücken. Die Uhr startet neu.
4. Danach die Weboberflächen-Datei auswählen und `Weboberfläche aktualisieren` drücken.

Während eines Updates die Stromversorgung nicht trennen. Einstellungen bleiben erhalten.

## Bedienung der Weboberfläche

- `Übersicht`: Uhrzeit der Uhr, aktuelles Wetter, Live-Ansicht der Matrix, Verbindung, Adresse und Helligkeit. Solange WLAN oder ein eigenes Admin-Passwort fehlen, erscheint eine ausblendbare Einrichtungskarte mit Checkliste.
- `Anzeige`: Live-Vorschau, Layout, Zeit- und Temperaturformat, Seitenwechsel, Farben, Tages- und Nacht-Helligkeit, Sicherheits-Freischalter und WLAN-Energiesparen. Änderungen erscheinen sofort auf der Uhr und werden ohne Speichern nach 2 Minuten zurückgesetzt.
- `Wetter`: Stadt, Wetteranbieter, Intervall, API-Keys und optional eine manuelle Zeitzone.
- `Hardware`: Matrixgröße, Datenpin, Farbreihenfolge, Start-Ecke und Verkabelung mit Live-Verkabelungsdiagramm und Testmuster.
- `WLAN & Zugang`: Netzwerk mit Suche, WLAN-Passwort, WLAN-Region, Browser-Adresse, Admin-Benutzer, Admin-Passwort und Setup-WLAN-Passwort.
- `System`: Firmware- und Weboberflächen-Version, Diagnose, Einrichtungsassistent, Einstellungen exportieren/importieren, Updates, Hilfe & Wiki und Zurücksetzen.

Sobald etwas ungespeichert ist, erscheint eine Speicherleiste; Tabs mit
ungespeicherten Änderungen sind markiert. Nach einer Änderung des Admin-Logins
bleibst du mit den neuen Daten angemeldet. Die Oberfläche folgt dem Hell- oder
Dunkelmodus deines Geräts.

Oben im Header kannst du zwischen Deutsch und Englisch wechseln. Die Auswahl wird im Browser und auf der Uhr gespeichert; die Wochentage auf dem Display folgen dieser Sprache.

Die WLAN-Region nutzt die vom ESP32-Paket unterstützten ESP-IDF-Ländercodes:
`01`, `AT`, `AU`, `BE`, `BG`, `BR`, `CA`, `CH`, `CN`, `CY`, `CZ`, `DE`, `DK`,
`EE`, `ES`, `FI`, `FR`, `GB`, `GR`, `HK`, `HR`, `HU`, `IE`, `IN`, `IS`, `IT`,
`JP`, `KR`, `LI`, `LT`, `LU`, `LV`, `MT`, `MX`, `NL`, `NO`, `NZ`, `PL`, `PT`,
`RO`, `SE`, `SI`, `SK`, `TW` und `US`. Standard ist `DE`; `01` ist der
weltweite sichere Modus. Eine geänderte WLAN-Region wird nach einem Neustart
aktiv.

## Performance und Energie

- Wetter und Standortsuche laufen in einem Hintergrundtask; die Anzeige bleibt bedienbar.
- Unveränderte LED-Bilder werden nicht erneut übertragen. Bei 0 % wird einmal schwarz gesendet.
- Statische Seiten werden einmal pro Sekunde geprüft, Animationen weiterhin alle 200 ms.
- WLAN-Energiesparen ist standardmäßig aktiv und unter `Anzeige` abschaltbar.
- Geocoding-Ergebnisse werden gespeichert; unveränderte Einstellungen lösen keine erneuten NVS-Schreibvorgänge aus.
- Komprimierte Webdateien, Tab-Navigation, Speichern/Verwerfen und Statusabfragen nur im aktiven Browser-Tab.

Die tatsächliche Stromersparnis hängt von Matrix, Helligkeit und Access Point ab und muss am Gerät gemessen werden. Bei 0 % bleiben die LEDs elektrisch versorgt; Deep Sleep wird nicht verwendet, damit Uhr und Weboberfläche verfügbar bleiben.

## Wetter

| Anbieter | API-Key | Daten |
| --- | --- | --- |
| Open-Meteo | Nein | Weltweite Vorhersage, Tages-Min/Max; optional Modell eines nationalen Wetterdienstes |
| DWD / Bright Sky | Nein | Stationsdaten, hauptsächlich Deutschland; kein Tages-Min/Max; Symbol notfalls aus Niederschlag und Bewölkung |
| MET Norway | Nein | Weltweite Vorhersage; kein Tages-Min/Max |
| OpenWeatherMap | Ja | Aktuelles Wetter; Min/Max der aktuellen Umgebung |
| WeatherAPI | Ja | Aktuelles Wetter und Tages-Min/Max |

Für WeatherAPI einen eigenen Schlüssel unter `Ort und Wetter` hinterlegen. Gespeicherte Schlüssel werden nicht an den Browser zurückgegeben; leere Felder behalten sie bei.

MET-Norway-Daten stammen vom [Norwegischen Meteorologischen Institut](https://www.met.no/) unter [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/); Temperaturen werden gerundet und Symbole zugeordnet. Die Firmware beachtet `Expires` und nutzt `If-Modified-Since`. [API-Nutzungsregeln](https://docs.api.met.no/doc/TermsOfService), [WeatherAPI-Dokumentation](https://www.weatherapi.com/docs/).

Standard ist Open-Meteo. Dafür ist kein API-Key nötig. Unter `Wetter` >
`Wettermodell` bleibt entweder die automatische Modellwahl aktiv, oder du wählst
das Vorhersagemodell eines Wetterdienstes: DWD ICON (Deutschland), ECMWF IFS,
MeteoSwiss, GeoSphere Austria, Météo-France, KNMI, DMI, UK Met Office, MET Nordic,
ItaliaMeteo ARPAE, NOAA GFS, Environment Canada GEM oder JMA. Landesmodelle sind
in ihrer Region meist am genauesten. Anders als DWD / Bright Sky (Stationsmesswerte)
ist DWD ICON über Open-Meteo eine Vorhersage mit Tages-Min/Max.

Der Anbieter `Deutscher Wetterdienst (DWD)` nutzt die Bright-Sky-JSON-API
für offene DWD-Wetterdaten und braucht ebenfalls keinen API-Key. Bright Sky
leitet sein Icon nach bestem Wissen ab: Es kann `wind` sein oder fehlen. Die
Firmware nutzt dann den gemeldeten Niederschlag und die Bewölkung, statt „Noch
keine Wetterdaten“ anzuzeigen. Verbindungsprobleme erscheinen mit Ursache, etwa
`TLS: ...` bei Zertifikatsfehlern oder `Verbindung fehlgeschlagen (...)` bei
Netzwerkfehlern.

Optional kann OpenWeatherMap genutzt werden:

1. OpenWeatherMap-Konto erstellen.
2. API-Key erzeugen.
3. In der Weboberfläche `OpenWeatherMap` auswählen.
4. API-Key eintragen.
5. Speichern.

Wetterdaten werden beim Start und danach im eingestellten Intervall aktualisiert. Standard sind 2 Stunden; in der Weboberfläche kannst du das Intervall in 0,5-Stunden-Schritten ändern. Manuell kannst du `Wetter aktualisieren` drücken. Fehler werden frühestens nach 5 Minuten erneut versucht; HTTP 429 wartet mindestens 30 Minuten. Gültige Werte bleiben bei Fehlern erhalten. Manuelle Abfragen berücksichtigen Cache- und Wartezeiten ebenfalls.

## Flash-Layout

Das Projekt nutzt eine eigene Partitionstabelle:

```text
otadata  0x002000  OTA-Auswahl
app0     0x170000  Firmware-Slot 1
app1     0x170000  Firmware-Slot 2
littlefs 0x110000  Weboberfläche und Assets
```

Die zwei App-Slots ermöglichen Firmware-Updates über die Weboberfläche. Nach einer Änderung an `partitions.csv` muss der ESP32 einmal per USB mit `pio run --target upload` und `pio run --target uploadfs` neu geflasht werden. Danach können neue Firmware- und Weboberflächen-Binaries unter `System` > `Firmware-Update` hochgeladen werden.

Die Weboberfläche liegt nicht im Firmware-Binary. Nach Änderungen an `data/` muss ein neues LittleFS-Image gebaut werden. Du kannst es danach entweder per USB mit `uploadfs` oder über die Weboberfläche aktualisieren.

## Sicherheit

- Die Weboberfläche ist per HTTP Basic Auth geschützt.
- Das Admin-Passwort wird als gesalzener PBKDF2-HMAC-SHA256-Hash gespeichert, nie im Klartext.
- Nach fünf fehlgeschlagenen Logins sperrt die Uhr weitere Versuche dieses Geräts für 30 Sekunden, verdoppelt bis maximal 5 Minuten.
- Der Setup-AP nutzt das Passwort `pixelclock`, bis du unter `WLAN & Zugang` > `Setup-WLAN` ein eigenes festlegst.
- Ändere nach der ersten Einrichtung den Admin-Benutzer und das Admin-Passwort unter `Admin-Zugriff`.
- Solange das Standard-Admin-Passwort aktiv ist, zeigt die Übersicht eine Einrichtungskarte mit `Passwort ändern`. Sie lässt sich für diesen Browser ausblenden.
- HTTP Basic Auth ist in einem normalen Heimnetz praktisch, aber nicht verschlüsselt. Nutze die Uhr nicht ungeschützt in öffentlichen oder fremden Netzwerken.

## Werksreset und Uhr weitergeben

Unter `System` > `Zurücksetzen` löscht `Werksreset` den kompletten
Einstellungsspeicher (NVS): WLAN-Zugangsdaten und Region, Admin-Login,
Setup-WLAN-Passwort, Standort, API-Keys und alle Anzeige-Einstellungen. Firmware
und Weboberfläche bleiben installiert. Die Uhr startet danach im Setup-Modus wie
ein neues Gerät und kann weitergegeben werden.

Ohne Zugang zur Weboberfläche die `BOOT`-Taste des ESP32 (GPIO 0) bei laufender
Uhr 10 Sekunden gedrückt halten. Nach 3 Sekunden zählt die Matrix `RESET 7` bis
`RESET 1` herunter, dann wird alles gelöscht und die Uhr startet neu. Früheres
Loslassen bricht ab.

Wer die Einstellungen später wiederherstellen möchte, exportiert sie vorher unter
`System` > `Einstellungen sichern`. Passwörter und API-Keys sind in der Datei
nicht enthalten.

## Problembehandlung

### Weboberfläche ist nicht erreichbar

- Prüfe, ob der ESP32 im WLAN verbunden ist.
- Öffne die IP-Adresse aus dem Router statt `pixelclock.local`.
- Falls kein WLAN gespeichert ist, mit `PixelClock-Setup-XXXXXX` verbinden und `http://192.168.4.1` öffnen.
- Wenn mDNS nicht funktioniert, ist `*.local` im Netzwerk eventuell nicht auflösbar.

### Login funktioniert nicht

- Standard ist `admin` / `pixelclock`.
- Wenn du Login-Daten geändert hast, melde dich mit Benutzer und Passwort neu an.
- Nach fünf falschen Passwörtern warten, bis die auf der Login-Seite angezeigte Sperre abgelaufen ist.
- Wenn die Einrichtungskarte trotz geändertem Passwort noch das Admin-Passwort auflistet, Browser-Cache hart neu laden und prüfen, ob `Speichern` erfolgreich war.
- Solange du noch eingeloggt bist, hilft der Werksreset unter `System`.
- Ohne Zugriff die `BOOT`-Taste am ESP32 bei laufender Uhr 10 Sekunden gedrückt halten (siehe [Werksreset](#werksreset-und-uhr-weitergeben)).

### Wetter wird nicht angezeigt

- Prüfe WLAN-Verbindung und Internetzugriff.
- Prüfe in der Wetterkarte unter `Übersicht`, ob ein Fehler angezeigt wird.
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

### ESP32 komplett loeschen

- ESP32 per USB verbinden.
- Optional Ports anzeigen:

```powershell
.\reset-pixel-clock.cmd -ListPorts
```

- Flash komplett loeschen:

```powershell
.\reset-pixel-clock.cmd -Port COM5
```

- Ohne Rueckfrage, zum Beispiel fuer Automatisierung:

```powershell
.\reset-pixel-clock.cmd -Port COM5 -Force
```

- Danach Firmware und Weboberflaeche neu flashen:

```powershell
.\flash-pixel-clock.cmd -Port COM5
```

## Versionshinweise

Firmware 0.1.19 behebt DWD-Wetter: Bright Sky meldet als Icon teils `wind` oder
gar keines, was als „Noch keine Wetterdaten“ erschien; jetzt liefern Niederschlag
und Bewölkung das Symbol. Die Let's-Encrypt-Wurzelzertifikate decken die neuen
Zertifikatsketten von 2026 ab, und Verbindungsfehler nennen ihre Ursache.
Open-Meteo kann das Vorhersagemodell eines bestimmten Wetterdienstes nutzen
(z. B. DWD ICON).
Weboberfläche 0.1.16 öffnet den Einrichtungsassistenten nur, solange auf der Uhr
kein WLAN gespeichert ist, und ersetzt das Admin-Passwort-Popup durch die
Einrichtungskarte auf der Übersicht.

Firmware 0.1.18 und Weboberfläche 0.1.15 erleichtern die erste Einrichtung und
das Weitergeben der Uhr: Das Setup-WLAN hat einen Namen pro Gerät und öffnet die
Weboberfläche von selbst (Captive Portal), das Testmuster funktioniert schon vor
der WLAN-Einrichtung, die Uhr verlässt den Setup-Modus selbstständig, sobald das
gespeicherte WLAN wieder da ist, und ein Werksreset (Weboberfläche oder 10 s
BOOT-Taste) löscht alle Daten. Das Admin-Passwort wird als gesalzener Hash
gespeichert, fehlgeschlagene Logins werden gebremst. Die Weboberfläche zeigt die
Matrix live, übernimmt Anzeige-Änderungen sofort als Vorschau und kann
Einstellungen exportieren und importieren.

Ein Wechsel zurück auf Firmware 0.1.17 oder älter setzt den Admin-Login auf
`admin` / `pixelclock` zurück, weil ältere Firmware das gehashte Passwort nicht
lesen kann.

Version 0.1.17 behebt ein Watchdog-Risiko bei stockenden Wetterantworten.
Netzwerk-Lesevorgänge geben jetzt regelmäßig Rechenzeit frei und haben eine
Gesamtfrist. Die Weboberfläche zeigt unter `System` die Laufzeit, den letzten
Startgrund und Speicherreserven. Erscheint die Begrüßung erneut, den Startgrund
vor dem Trennen der Stromversorgung ablesen; siehe
[Neustartdiagnose](docs/PERFORMANCE.md#restart-diagnosis-in-0117).

Weboberfläche 0.1.14 bringt einen Einrichtungsassistenten für den ersten Start
und eine neu gestaltete Oberfläche mit Bereichs-Tabs sowie automatischem Hell-/
Dunkelmodus. Sie nutzt nur die vorhandene Firmware-API und braucht kein
Firmware-Update.

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
- `data/i18n.js`: Browser-Uebersetzungen und Sprachauswahl.
- `data/updates.js`: Firmware-/LittleFS-Upload und Versionspruefungen.
- `data/setup.js`: Einrichtungsassistent auf Basis der normalen Formularfelder.
- `data/app.js`: Browser-Logik, Tabs, Formular-Sync und Status-Refresh.
- `data/index.html`: Struktur der Weboberfläche.

Wenn du eine Einstellung erweiterst, müssen meist Firmware-Konfiguration,
API-JSON, Formularfeld, Übersetzungen und Doku gemeinsam angepasst werden.

Die in der Weboberfläche angezeigte Firmware-Version wird über
`FIRMWARE_VERSION` in `src/app_state.h` gesetzt und über `/api/status`
ausgeliefert. Bei jeder Firmware-Änderung diese Version erhöhen und die
README-Dateien mitziehen, auch wenn daraus noch kein formales Release entsteht.

Die LittleFS-Weboberflächen-Version wird in `data/updates.js` über
`littleFsVersionMarker` gesetzt. Bei jeder Änderung unter `data/` diese Version
erhöhen und die README-Dateien mitziehen. Die Weboberfläche zeigt sowohl die
installierte LittleFS-Version als auch, wenn möglich, die Version aus einem
ausgewählten LittleFS-Update-Image vor dem Upload an.

Firmware und LittleFS können getrennt aktualisiert werden. Die Weboberfläche
muss deshalb mit der bereits installierten Firmware-API kompatibel bleiben.
Update-Routen wie `/api/update/firmware` und `/api/update/web` nicht entfernen
oder umbenennen, ohne die alte Route als Alias beizubehalten. Für neue
Funktionen besser Fähigkeiten über `/api/status` melden und im Browser einen
Fallback für ältere Firmware anbieten.

Siehe auch [CONTRIBUTING.md](CONTRIBUTING.md) für Hinweise zu Pull Requests.

## Credits

Dieses Projekt wurde gemeinsam mit Codex, einem KI-Coding-Assistenten von OpenAI, entworfen, implementiert und dokumentiert.

## Lizenz

Dieses Projekt steht unter der MIT-Lizenz. Details stehen in [LICENSE](LICENSE).

## Sicherheit

Hinweise zum Melden von Sicherheitsproblemen stehen in [SECURITY.md](SECURITY.md).

Tests und technische Details: [Performance und Validierung](docs/PERFORMANCE.md). Neue Anbieter benötigen Firmware 0.1.16; die Weboberfläche deaktiviert sie bei älterer Firmware. Für alle Änderungen Firmware und LittleFS aktualisieren.
