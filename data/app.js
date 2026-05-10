const pins = [2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33];
const safeBrightnessPercent = 40;
const fields = [
  "ssid", "hostname", "cityName", "timezone", "weatherProvider", "weatherIntervalHalfHours", "width", "height", "dataPin",
  "brightness", "fullBrightnessUnlocked", "wiringMode", "origin", "displayMode", "colorOrder",
  "temperatureUnit", "hourFormat", "colorWeekday", "colorText", "colorPoint", "colorColon", "pageSeconds",
  "colorGradientMode", "autoPage", "selectedPage", "nightBrightness", "nightStart", "nightEnd"
];

const $ = (id) => document.getElementById(id);
const storedLanguage = localStorage.getItem("pixelClockLanguage");
const adminReminderStorageKey = "pixelClockAdminReminderDismissed";
let currentLanguage = storedLanguage || ((navigator.language || "").toLowerCase().startsWith("de") ? "de" : "en");

const translations = {
  en: {
    "Lade Status...": "Loading status...",
    "Neustart": "Restart",
    "Sprache": "Language",
    "Deutsch": "German",
    "Status": "Status",
    "Wetter": "Weather",
    "Ort": "Location",
    "Adresse": "Address",
    "Noch keine Wetterdaten": "No weather data yet",
    "Noch kein Ort geladen": "No location loaded",
    "WLAN": "Wi-Fi",
    "WLAN-Zugang": "Wi-Fi access",
    "Netzwerk": "Network",
    "Browser-Adresse": "Browser address",
    "Scannen": "Scan",
    "Passwort": "Password",
    "Admin-Zugriff": "Admin access",
    "Admin-Benutzer": "Admin user",
    "Admin-Passwort": "Admin password",
    "Ort, Wetter und Zugriff": "Location, weather and access",
    "Stadt": "City",
    "Wetteranbieter": "Weather provider",
    "Wetter-Intervall": "Weather interval",
    "OpenWeatherMap API-Key": "OpenWeatherMap API key",
    "Zeitzone": "Time zone",
    "Display-Hardware": "Display hardware",
    "Breite": "Width",
    "Höhe": "Height",
    "Daten-Pin": "Data pin",
    "Farbreihenfolge": "Color order",
    "Start-Ecke": "Start corner",
    "Oben links": "Top left",
    "Oben rechts": "Top right",
    "Unten links": "Bottom left",
    "Unten rechts": "Bottom right",
    "LED-Verkabelung": "LED wiring",
    "Zeilenweise gerade": "Rows, straight",
    "Zeilenweise Serpentine": "Rows, serpentine",
    "Spaltenweise gerade": "Columns, straight",
    "Spaltenweise Serpentine": "Columns, serpentine",
    "Testmuster": "Test pattern",
    "Anzeige und Seiten": "Display and pages",
    "Anzeige-Modus": "Display mode",
    "Icons links, Text rechts": "Icons left, text right",
    "Text links, Icons rechts": "Text left, icons right",
    "Ohne Icons, zentriert": "No icons, centered",
    "Uhrzeitformat": "Time format",
    "24 Stunden": "24 hours",
    "12 Stunden": "12 hours",
    "Temperatur": "Temperature",
    "Seitenwechsel": "Page interval",
    "Feste Seite": "Fixed page",
    "Uhrzeit": "Time",
    "Datum": "Date",
    "Seiten automatisch wechseln": "Switch pages automatically",
    "Farben": "Colors",
    "Wochentag": "Weekday",
    "Text/Uhrzeit": "Text/time",
    "Punkt/Markierung": "Dot/marker",
    "Doppelpunkt": "Colon",
    "Farbverlaufs-Modus": "Color gradient mode",
    "Standard": "Default",
    "Pixel-Farbverlauf": "Pixel gradient",
    "Sanfter Farbwechsel": "Smooth color cycle",
    "Helligkeit": "Brightness",
    "Helligkeit (%)": "Brightness (%)",
    "Nacht-Helligkeit (%)": "Night brightness (%)",
    "Volle Helligkeit freischalten": "Unlock full brightness",
    "Warnung: Volle Helligkeit kann den ESP32 oder USB-Port überlasten, wenn die Matrix direkt darüber versorgt wird.": "Warning: Full brightness can overload the ESP32 or USB port when the matrix is powered directly from it.",
    "Nacht ab": "Night starts",
    "Nacht bis": "Night ends",
    "Hilfe & Wiki": "Help & Wiki",
    "Schnellstart": "Quick start",
    "Mit PixelClock-Setup verbinden, wenn noch kein WLAN eingerichtet ist.": "Connect to PixelClock-Setup if Wi-Fi is not configured yet.",
    "http://192.168.4.1 öffnen und mit admin / pixelclock anmelden.": "Open http://192.168.4.1 and log in with admin / pixelclock.",
    "WLAN, Stadt, Matrixgröße und Datenpin setzen.": "Set Wi-Fi, city, matrix size, and data pin.",
    "Speichern drücken und bei Bedarf neu starten.": "Press Save and restart if needed.",
    "Login und Sprache": "Login and language",
    "Der Standardzugang ist admin / pixelclock. Ändere Benutzer und Passwort nach der Einrichtung unter Admin-Zugriff. Die Sprache wird automatisch gewählt und kann oben umgestellt werden.": "The default login is admin / pixelclock. Change user and password after setup under Admin access. The language is selected automatically and can be changed at the top.",
    "WLAN und Adresse": "Wi-Fi and address",
    "Die Browser-Adresse ist der Hostname für die .local-Adresse. Mit pixelclock erreichst du die Uhr normalerweise unter http://pixelclock.local. Wenn .local nicht klappt, nutze die IP-Adresse aus dem Router.": "The browser address is the hostname for the .local address. With pixelclock you usually reach the clock at http://pixelclock.local. If .local does not work, use the IP address from your router.",
    "Display einrichten": "Set up the display",
    "Starte mit Breite 32, Höhe 8, GPIO 18 und spaltenweiser Serpentine. Wenn die Anzeige gespiegelt ist, ändere Start-Ecke oder LED-Verkabelung. Das Testmuster hilft beim Prüfen der Richtung.": "Start with width 32, height 8, GPIO 18, and column serpentine wiring. If the display is mirrored, change start corner or LED wiring. The test pattern helps verify direction.",
    "Wetter einrichten": "Set up weather",
    "Open-Meteo funktioniert ohne API-Key. Für OpenWeatherMap brauchst du einen eigenen API-Key. Das Wetter-Intervall kannst du in 0,5-Stunden-Schritten einstellen.": "Open-Meteo works without an API key. For OpenWeatherMap you need your own API key. You can set the weather interval in 0.5-hour steps.",
    "Helligkeit und Nachtmodus": "Brightness and night mode",
    "0% schaltet die Matrix aus. Ohne Freischalter ist die maximale Helligkeit auf 40% begrenzt. Die Nacht-Helligkeit gilt im eingestellten Zeitraum.": "0% turns the matrix off. Without the unlock switch, maximum brightness is limited to 40%. Night brightness applies during the configured period.",
    "Häufige Probleme": "Common problems",
    "Keine Weboberfläche: IP-Adresse aus dem Router testen oder Setup-AP verwenden.": "No web interface: try the IP address from the router or use the setup AP.",
    "Keine Wetterdaten: WLAN, Internet, Stadt und API-Key prüfen.": "No weather data: check Wi-Fi, internet, city, and API key.",
    "Falsche Farben: Farbreihenfolge zwischen GRB und RGB wechseln.": "Wrong colors: switch color order between GRB and RGB.",
    "Durcheinanderes Bild: Start-Ecke und LED-Verkabelung anpassen.": "Scrambled image: adjust start corner and LED wiring.",
    "Speichern": "Save",
    "Einstellungen resetten": "Reset settings",
    "Werksreset": "Factory reset",
    "Wetter aktualisieren": "Refresh weather",
    "Admin-Passwort ändern": "Change admin password",
    "Der Standardzugang ist noch aktiv. Ändere das Admin-Passwort, damit niemand im Netzwerk die Uhr einfach umstellen kann.": "The default login is still active. Change the admin password so nobody on the network can easily change the clock.",
    "Jetzt ändern": "Change now",
    "Nicht mehr anzeigen": "Do not show again",
    "Leer lassen zum Beibehalten": "Leave empty to keep current",
    "z. B. Berlin": "e.g. Berlin",
    "Helligkeit in Prozent": "Brightness in percent",
    "Nacht-Helligkeit in Prozent": "Night brightness in percent",
    "Nacht ab AM/PM": "Night starts AM/PM",
    "Nacht bis AM/PM": "Night ends AM/PM",
    "Setup-AP": "Setup AP",
    "keine Uhrzeit": "no time",
    "Fehler": "Error",
    "Speichere...": "Saving...",
    "Admin-Anmeldung erforderlich.": "Admin login required.",
    "Speichern fehlgeschlagen.": "Save failed.",
    "Gespeichert.": "Saved.",
    "Ort wird im Hintergrund aktualisiert.": "Location is updating in the background.",
    "Wetter wird aktualisiert.": "Weather is updating.",
    "Login wurde geändert, bitte mit den neuen Daten neu laden.": "Login changed, please reload with the new credentials.",
    "Neustart für Pin, Größe, Farbe, WLAN, Adresse oder Login nötig.": "Restart required for pin, size, color, Wi-Fi, address, or login.",
    "Sofort aktiv.": "Active immediately.",
    "Suche...": "Searching...",
    "Aktion fehlgeschlagen.": "Action failed.",
    "Wetter konnte nicht aktualisiert werden.": "Weather could not be refreshed.",
    "Wetteraktualisierung gestartet.": "Weather refresh started.",
    "Konfiguration konnte nicht geladen werden.": "Configuration could not be loaded.",
    "Testmuster gestartet.": "Test pattern started.",
    "Neustart läuft...": "Restarting...",
    "Einstellungen werden zurückgesetzt...": "Resetting settings...",
    "Werksreset läuft...": "Factory reset running...",
    "Alle Einstellungen außer WLAN zurücksetzen und neu starten?": "Reset all settings except Wi-Fi and restart?",
    "Werksreset ausführen? Dabei werden auch WLAN-Daten gelöscht.": "Run factory reset? This also deletes Wi-Fi credentials.",
    "Wirklich alles löschen? Der ESP startet danach im Setup-Modus.": "Really delete everything? The ESP will restart in setup mode.",
    "Klarer Himmel": "Clear sky",
    "Überwiegend klar": "Mostly clear",
    "Teilweise bewölkt": "Partly cloudy",
    "Bewölkt": "Cloudy",
    "Nebel": "Fog",
    "Nieselregen": "Drizzle",
    "Regen": "Rain",
    "Schnee": "Snow",
    "Regenschauer": "Rain showers",
    "Schneeschauer": "Snow showers",
    "Gewitter": "Thunderstorm",
    "Wettercode": "Weather code",
    "OpenWeatherMap API-Key fehlt": "OpenWeatherMap API key missing",
    "Stadt nicht gefunden": "City not found",
    "HTTP begin failed": "HTTP begin failed",
    "Geocoding begin failed": "Geocoding begin failed"
  }
};

function tr(text) {
  return currentLanguage === "de" ? text : (translations[currentLanguage]?.[text] || text);
}

function translateTextNodes(root) {
  const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT);
  let node = walker.nextNode();
  while (node) {
    const source = node._i18nSource || node.nodeValue.trim();
    if (source) {
      node._i18nSource = source;
      const leading = node.nodeValue.match(/^\s*/)[0];
      const trailing = node.nodeValue.match(/\s*$/)[0];
      node.nodeValue = leading + tr(source) + trailing;
    }
    node = walker.nextNode();
  }
}

function translateAttributes() {
  for (const el of document.querySelectorAll("[placeholder], [aria-label]")) {
    if (el.placeholder !== undefined) {
      const source = el.dataset.i18nPlaceholder || el.getAttribute("placeholder");
      if (source) {
        el.dataset.i18nPlaceholder = source;
        el.setAttribute("placeholder", tr(source));
      }
    }
    const aria = el.getAttribute("aria-label");
    if (aria !== null) {
      const source = el.dataset.i18nAria || aria;
      el.dataset.i18nAria = source;
      el.setAttribute("aria-label", tr(source));
    }
  }
}

function updateAdminPasswordPlaceholder() {
  $("adminPassword").placeholder = tr("Leer lassen zum Beibehalten");
}

function applyLanguage() {
  document.documentElement.lang = currentLanguage;
  $("languageSelect").value = currentLanguage;
  fillWeatherIntervals();
  translateTextNodes(document.querySelector("main"));
  translateTextNodes($("adminReminder"));
  translateAttributes();
  updateAdminPasswordPlaceholder();
}

function setLanguage(language) {
  currentLanguage = language === "de" ? "de" : "en";
  localStorage.setItem("pixelClockLanguage", currentLanguage);
  applyLanguage();
  loadStatus().catch(() => {});
}

function message(text) {
  $("message").textContent = tr(text);
}

function showAdminReminder(config) {
  if (!config.adminPasswordIsDefault || localStorage.getItem(adminReminderStorageKey) === "1") return;
  $("adminReminder").hidden = false;
  $("adminReminderGo").focus();
}

function closeAdminReminder(rememberDismissal) {
  $("adminReminder").hidden = true;
  if (rememberDismissal) localStorage.setItem(adminReminderStorageKey, "1");
}

function openAdminAccess() {
  closeAdminReminder(false);
  document.querySelector(".accessSection")?.scrollIntoView({ behavior: "smooth", block: "start" });
  $("adminPassword").focus();
}

function fillPins() {
  $("dataPin").innerHTML = pins.map((pin) => `<option value="${pin}">${pin}</option>`).join("");
}

function fillWeatherIntervals(selectedValue) {
  const current = selectedValue || $("weatherIntervalHalfHours").value || "4";
  $("weatherIntervalHalfHours").innerHTML = "";
  for (let halfHours = 1; halfHours <= 48; halfHours++) {
    const option = document.createElement("option");
    const hours = halfHours / 2;
    option.value = String(halfHours);
    option.textContent = `${currentLanguage === "de" ? String(hours).replace(".", ",") : hours} h`;
    $("weatherIntervalHalfHours").append(option);
  }
  $("weatherIntervalHalfHours").value = current;
}

function updateRangeValues() {
  updateBrightnessLimits(false);
  setBrightnessPercent("brightness", "brightnessPercent", "brightnessPercentValue", byteToPercent($("brightness").value));
  setBrightnessPercent("nightBrightness", "nightBrightnessPercent", "nightBrightnessPercentValue", byteToPercent($("nightBrightness").value));
}

function brightnessMaxPercent() {
  return $("fullBrightnessUnlocked").checked ? 100 : safeBrightnessPercent;
}

function updateBrightnessLimits(clampValues = true) {
  const max = brightnessMaxPercent();
  for (const id of ["brightnessPercent", "brightnessPercentValue", "nightBrightnessPercent", "nightBrightnessPercentValue"]) {
    $(id).max = String(max);
  }
  $("brightnessWarning").hidden = !$("fullBrightnessUnlocked").checked;
  if (clampValues) {
    setBrightnessPercent("brightness", "brightnessPercent", "brightnessPercentValue", $("brightnessPercentValue").value);
    setBrightnessPercent("nightBrightness", "nightBrightnessPercent", "nightBrightnessPercentValue", $("nightBrightnessPercentValue").value);
  }
}

function clampPercent(value) {
  const parsed = Number.parseInt(value, 10);
  if (Number.isNaN(parsed)) return 0;
  return Math.min(brightnessMaxPercent(), Math.max(0, parsed));
}

function byteToPercent(value) {
  const parsed = Number.parseInt(value, 10);
  if (Number.isNaN(parsed)) return 0;
  return Math.round(Math.min(255, Math.max(0, parsed)) * 100 / 255);
}

function percentToByte(value) {
  return Math.round(clampPercent(value) * 255 / 100);
}

function setBrightnessPercent(hiddenId, sliderId, numberId, percent) {
  const value = clampPercent(percent);
  $(sliderId).value = value;
  $(numberId).value = value;
  $(hiddenId).value = percentToByte(value);
}

function syncBrightnessNumberFromSlider(hiddenId, sliderId, numberId) {
  setBrightnessPercent(hiddenId, sliderId, numberId, $(sliderId).value);
}

function syncBrightnessSliderFromNumber(hiddenId, sliderId, numberId) {
  setBrightnessPercent(hiddenId, sliderId, numberId, $(numberId).value);
}

function clampHour(value) {
  const hour = Number.parseInt(value, 10);
  if (Number.isNaN(hour)) return 0;
  return Math.min(23, Math.max(0, hour));
}

function hour24ToDisplay(hour) {
  hour = clampHour(hour);
  if ($("hourFormat").value === "12") {
    const period = hour >= 12 ? "PM" : "AM";
    let display = hour % 12;
    if (display === 0) display = 12;
    return { display, period };
  }
  return { display: hour, period: "AM" };
}

function displayToHour24(display, period) {
  let hour = Number.parseInt(display, 10);
  if (Number.isNaN(hour)) hour = 0;
  if ($("hourFormat").value === "12") {
    hour = Math.min(12, Math.max(1, hour));
    if (period === "PM" && hour !== 12) hour += 12;
    if (period === "AM" && hour === 12) hour = 0;
    return hour;
  }
  return Math.min(23, Math.max(0, hour));
}

function setNightDisplay(prefix) {
  const value = hour24ToDisplay($(prefix).value);
  $(`${prefix}Display`).value = value.display;
  $(`${prefix}Period`).value = value.period;
}

function updateNightControlsFromStored() {
  const is12h = $("hourFormat").value === "12";
  for (const prefix of ["nightStart", "nightEnd"]) {
    const display = $(`${prefix}Display`);
    const period = $(`${prefix}Period`);
    const row = display.closest(".timeRow");
    display.min = is12h ? "1" : "0";
    display.max = is12h ? "12" : "23";
    period.disabled = !is12h;
    row.classList.toggle("compact", !is12h);
    setNightDisplay(prefix);
  }
}

function syncNightStoredFromDisplay() {
  for (const prefix of ["nightStart", "nightEnd"]) {
    $(prefix).value = displayToHour24($(`${prefix}Display`).value, $(`${prefix}Period`).value);
  }
}

function weatherDescription(code) {
  if (code === null || code === undefined || code < 0) return tr("Noch keine Wetterdaten");
  if (code === 0) return tr("Klarer Himmel");
  if (code === 1) return tr("Überwiegend klar");
  if (code === 2) return tr("Teilweise bewölkt");
  if (code === 3) return tr("Bewölkt");
  if (code === 45 || code === 48) return tr("Nebel");
  if (code >= 51 && code <= 57) return tr("Nieselregen");
  if (code >= 61 && code <= 67) return tr("Regen");
  if (code >= 71 && code <= 77) return tr("Schnee");
  if (code >= 80 && code <= 82) return tr("Regenschauer");
  if (code >= 85 && code <= 86) return tr("Schneeschauer");
  if (code >= 95) return tr("Gewitter");
  return `${tr("Wettercode")} ${code}`;
}

function setForm(config) {
  for (const field of fields) {
    const el = $(field);
    if (!el) continue;
    if (el.type === "checkbox") el.checked = Boolean(config[field]);
    else el.value = config[field] ?? "";
  }
  updateBrightnessLimits(false);
  $("locationLine").textContent = `${config.locationLabel || config.cityName || "-"} - ${config.url || ""}`;
  $("urlLine").textContent = config.url || "-";
  $("adminUsername").value = config.adminUsername || config.defaultAdminUsername || "admin";
  updateAdminPasswordPlaceholder();
  updateRangeValues();
  updateNightControlsFromStored();
  showAdminReminder(config);
}

function formBody() {
  updateBrightnessLimits(true);
  syncNightStoredFromDisplay();
  const data = new URLSearchParams();
  for (const field of fields) {
    const el = $(field);
    if (!el) continue;
    data.set(field, el.type === "checkbox" ? (el.checked ? "1" : "0") : el.value);
  }
  const password = $("password").value.trim();
  if (password) data.set("password", password);
  const adminUsername = $("adminUsername").value.trim();
  if (adminUsername) data.set("adminUsername", adminUsername);
  const adminPassword = $("adminPassword").value.trim();
  if (adminPassword) data.set("adminPassword", adminPassword);
  const openWeatherApiKey = $("openWeatherApiKey").value.trim();
  if (openWeatherApiKey) data.set("openWeatherApiKey", openWeatherApiKey);
  return data;
}

async function loadConfig() {
  const res = await fetch("/api/config", { credentials: "same-origin" });
  setForm(await res.json());
}

async function loadStatus() {
  const res = await fetch("/api/status", { credentials: "same-origin" });
  const status = await res.json();
  const mode = status.setupMode ? tr("Setup-AP") : tr("WLAN");
  const unit = status.temperatureUnit || "C";
  const temp = status.temperature === null || status.temperature === undefined
    ? ""
    : ` - ${Math.round(status.temperature)} ${unit}`;
  $("statusLine").textContent = `${mode} - ${status.ip || "-"} - ${status.url || ""} - ${status.localTime || tr("keine Uhrzeit")}${temp}`;
  const weatherTemp = status.temperature === null || status.temperature === undefined
    ? ""
    : `, ${Math.round(status.temperature)} ${unit}`;
  const weatherRange = status.temperatureMin === null || status.temperatureMin === undefined ||
    status.temperatureMax === null || status.temperatureMax === undefined
    ? ""
    : ` (${Math.round(status.temperatureMin)}-${Math.round(status.temperatureMax)} ${unit})`;
  const provider = status.weatherProvider ? ` - ${status.weatherProvider}` : "";
  const weatherError = status.weatherError ? ` - ${tr("Fehler")}: ${tr(status.weatherError)}` : "";
  $("weatherLine").textContent = `${weatherDescription(status.weatherCode)}${weatherTemp}${weatherRange}${provider}${weatherError}`;
  $("urlLine").textContent = status.url || "-";
  if (status.locationLabel || status.cityName) {
    $("locationLine").textContent = `${status.locationLabel || status.cityName} - ${status.url || ""}`;
  }
}

async function saveConfig() {
  message("Speichere...");
  const res = await fetch("/api/config", { method: "POST", body: formBody(), credentials: "same-origin" });
  let data = {};
  try {
    data = await res.json();
  } catch (_) {
    data = {};
  }
  if (!res.ok) {
    message(data.error || (res.status === 401 ? "Admin-Anmeldung erforderlich." : "Speichern fehlgeschlagen."));
    return;
  }
  $("adminPassword").value = "";
  const statusParts = [
    "Gespeichert.",
    data.cityResolutionPending ? "Ort wird im Hintergrund aktualisiert." : "",
    data.weatherRefreshPending ? "Wetter wird aktualisiert." : "",
    data.authChanged ? "Login wurde geändert, bitte mit den neuen Daten neu laden." : "",
    data.restartRequired ? "Neustart für Pin, Größe, Farbe, WLAN, Adresse oder Login nötig." : "Sofort aktiv."
  ];
  $("message").textContent = statusParts.filter(Boolean).map(tr).join(" ");
}

async function scanNetworks() {
  $("networks").textContent = tr("Suche...");
  const res = await fetch("/api/networks", { credentials: "same-origin" });
  const data = await res.json();
  $("networks").innerHTML = "";
  for (const network of data.networks || []) {
    const btn = document.createElement("button");
    btn.type = "button";
    btn.textContent = `${network.ssid} ${network.rssi} dBm`;
    btn.addEventListener("click", () => {
      $("ssid").value = network.ssid;
    });
    $("networks").append(btn);
  }
}

async function postAction(url, doneText) {
  const res = await fetch(url, { method: "POST", credentials: "same-origin" });
  if (!res.ok) {
    message(res.status === 401 ? "Admin-Anmeldung erforderlich." : "Aktion fehlgeschlagen.");
    return;
  }
  message(doneText);
}

async function refreshWeather() {
  const res = await fetch("/api/weather/refresh", { method: "POST", credentials: "same-origin" });
  if (!res.ok) {
    message(res.status === 401 ? "Admin-Anmeldung erforderlich." : "Wetter konnte nicht aktualisiert werden.");
    return;
  }
  message("Wetteraktualisierung gestartet.");
  await loadStatus();
}

async function resetSettings() {
  if (!confirm(tr("Alle Einstellungen außer WLAN zurücksetzen und neu starten?"))) return;
  await postAction("/api/reset/settings", "Einstellungen werden zurückgesetzt...");
  setTimeout(() => location.reload(), 4000);
}

async function factoryReset() {
  if (!confirm(tr("Werksreset ausführen? Dabei werden auch WLAN-Daten gelöscht."))) return;
  if (!confirm(tr("Wirklich alles löschen? Der ESP startet danach im Setup-Modus."))) return;
  await postAction("/api/reset/factory", "Werksreset läuft...");
  setTimeout(() => location.reload(), 4000);
}

fillPins();
fillWeatherIntervals();
applyLanguage();
loadConfig().catch(() => message("Konfiguration konnte nicht geladen werden."));
loadStatus().catch(() => {});
setInterval(loadStatus, 5000);

$("languageSelect").addEventListener("change", (event) => setLanguage(event.target.value));
$("adminReminderGo").addEventListener("click", openAdminAccess);
$("adminReminderDismiss").addEventListener("click", () => closeAdminReminder(true));
$("saveBtn").addEventListener("click", saveConfig);
$("scanBtn").addEventListener("click", scanNetworks);
$("brightnessPercent").addEventListener("input", () => syncBrightnessNumberFromSlider("brightness", "brightnessPercent", "brightnessPercentValue"));
$("nightBrightnessPercent").addEventListener("input", () => syncBrightnessNumberFromSlider("nightBrightness", "nightBrightnessPercent", "nightBrightnessPercentValue"));
$("brightnessPercentValue").addEventListener("input", () => syncBrightnessSliderFromNumber("brightness", "brightnessPercent", "brightnessPercentValue"));
$("nightBrightnessPercentValue").addEventListener("input", () => syncBrightnessSliderFromNumber("nightBrightness", "nightBrightnessPercent", "nightBrightnessPercentValue"));
$("fullBrightnessUnlocked").addEventListener("change", () => updateBrightnessLimits(true));
$("hourFormat").addEventListener("change", updateNightControlsFromStored);
for (const id of ["nightStartDisplay", "nightStartPeriod", "nightEndDisplay", "nightEndPeriod"]) {
  $(id).addEventListener("change", syncNightStoredFromDisplay);
}
$("testBtn").addEventListener("click", () => postAction("/api/display/test", "Testmuster gestartet."));
$("weatherBtn").addEventListener("click", refreshWeather);
$("restartBtn").addEventListener("click", () => postAction("/api/restart", "Neustart läuft..."));
$("settingsResetBtn").addEventListener("click", resetSettings);
$("factoryResetBtn").addEventListener("click", factoryReset);
