# Pixel Clock Plan

## Phase 1: Grundsystem

- PlatformIO-Projekt
- WS2812B-Matrix mit FastLED
- LittleFS-Partition fuer Weboberflaeche
- Setup-AP bei fehlender WLAN-Konfiguration
- Web UI fuer WLAN, Standort, Zeit und Display

## Phase 2: Daten

- NTP-Sync beim Start
- NTP-Refresh einmal pro Tag
- Open-Meteo-Wetter alle 2 Stunden
- Wettercode auf 8 Pixel hohe Icons mappen

## Phase 3: Anzeige

- Seite 1: Uhrzeit mit Wettersymbol links
- Seite 2: Wochentag und Datum
- Seite 3: Temperatur und Wettericon
- Automatischer Seitenwechsel
- Seitenanzeige mit 2px breiten Markierungen unten

## Phase 4: Ausbau

- Captive-DNS fuer Setup-Portal
- OTA-Update optional, falls die Partitionen angepasst werden
- Weitere Display-Mappings fuer Sondermatrizen
- Sonnenaufgang/Sonnenuntergang oder Auto-Dimming nach Umgebungszeit
- Mehr Wetterdetails wie Regenwahrscheinlichkeit oder Luftfeuchte
