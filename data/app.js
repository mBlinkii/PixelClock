const pins = [2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33];
const safeBrightnessPercent = 40;
const fields = [
  "ssid", "hostname", "cityName", "timezone", "weatherProvider", "weatherIntervalHalfHours", "width", "height", "dataPin",
  "brightness", "fullBrightnessUnlocked", "wiringMode", "origin", "displayMode", "colorOrder",
  "temperatureUnit", "weatherIconEnabled", "hourFormat", "colorWeekday", "colorText", "colorPoint", "colorColon", "pageSeconds",
  "colorGradientMode", "autoPage", "selectedPage", "nightBrightness", "nightStart", "nightEnd"
];

const $ = (id) => document.getElementById(id);
const adminReminderStorageKey = "pixelClockAdminReminderDismissed";
const panelCollapsedStoragePrefix = "pixelClockPanelCollapsed:";
let currentLanguage = storedLanguage || ((navigator.language || "").toLowerCase().startsWith("de") ? "de" : "en");
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
  updateWifiSummary();
}

function setLanguage(language) {
  currentLanguage = language === "de" ? "de" : "en";
  localStorage.setItem("pixelClockLanguage", currentLanguage);
  applyLanguage();
  saveLanguagePreference().catch(() => {});
  loadStatus().catch(() => {});
}

async function saveLanguagePreference() {
  const data = new URLSearchParams();
  data.set("language", currentLanguage);
  await fetch("/api/language", { method: "POST", body: data, credentials: "same-origin" });
}

function message(text) {
  $("message").textContent = tr(text);
}

function messageText(text) {
  $("message").textContent = text;
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
  const section = document.querySelector(".accessSection");
  if (section) setPanelCollapsed(section, false, true);
  section?.scrollIntoView({ behavior: "smooth", block: "start" });
  $("adminPassword").focus();
}

function panelId(panel) {
  return panel.dataset.panelId || Array.from(panel.classList).find((name) => name.endsWith("Section")) || "";
}

function storedPanelCollapsed(panel) {
  const id = panelId(panel);
  if (!id) return null;
  const value = localStorage.getItem(panelCollapsedStoragePrefix + id);
  if (value === null) return null;
  return value === "1";
}

function rememberPanelCollapsed(panel, collapsed) {
  const id = panelId(panel);
  if (id) localStorage.setItem(panelCollapsedStoragePrefix + id, collapsed ? "1" : "0");
}

function setPanelToggleLabel(button, collapsed) {
  const source = collapsed ? "Bereich ausklappen" : "Bereich einklappen";
  button.dataset.i18nAria = source;
  button.setAttribute("aria-label", tr(source));
}

function setPanelCollapsed(panel, collapsed, remember = false) {
  const button = panel.querySelector(".panelToggle");
  panel.classList.toggle("isCollapsed", collapsed);
  if (button) {
    button.setAttribute("aria-expanded", String(!collapsed));
    setPanelToggleLabel(button, collapsed);
  }
  if (remember) rememberPanelCollapsed(panel, collapsed);
}

function initCollapsiblePanels() {
  for (const panel of document.querySelectorAll("section.panel")) {
    const heading = panel.querySelector(":scope > h2");
    if (!heading || heading.querySelector(".panelToggle")) continue;
    const id = panelId(panel);
    if (id) panel.dataset.panelId = id;

    const title = document.createElement("span");
    title.className = "panelTitle";
    while (heading.firstChild) title.append(heading.firstChild);

    const summary = document.createElement("span");
    summary.className = "panelSummary";
    if (panel.classList.contains("wifiSection")) summary.classList.add("wifiSummary");

    const toggle = document.createElement("button");
    toggle.type = "button";
    toggle.className = "panelToggle";
    toggle.innerHTML = '<svg aria-hidden="true"><use href="#icon-chevron"></use></svg>';
    toggle.addEventListener("click", () => setPanelCollapsed(panel, !panel.classList.contains("isCollapsed"), true));

    heading.classList.add("panelHeader");
    heading.append(title, summary, toggle);

    const body = document.createElement("div");
    body.className = "panelBody";
    while (heading.nextSibling) body.append(heading.nextSibling);
    panel.append(body);
    setPanelCollapsed(panel, true);
  }
}

function applyPanelStartState(config) {
  const setupOpenPanels = new Set(["statusSection", "wifiSection", "accessSection", "weatherSection", "helpSection"]);
  const normalOpenPanels = new Set(["statusSection", "helpSection"]);
  const isFirstSetup = !String(config.ssid || "").trim() || config.adminPasswordIsDefault;
  const openPanels = isFirstSetup ? setupOpenPanels : normalOpenPanels;

  for (const panel of document.querySelectorAll("section.panel")) {
    const id = panelId(panel);
    const savedCollapsed = storedPanelCollapsed(panel);
    const shouldBeCollapsed = savedCollapsed ?? (id === "displaySection" || !openPanels.has(id));
    setPanelCollapsed(panel, shouldBeCollapsed);
  }
}

function applyNormalPanelStartState() {
  applyPanelStartState({ ssid: "configured", adminPasswordIsDefault: false });
}

function updateWifiSummary() {
  const summary = document.querySelector(".wifiSummary");
  if (!summary) return;
  const ssid = $("ssid")?.value.trim() || "";
  summary.textContent = ssid;
  summary.hidden = !ssid;
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
  if (config.language === "de" || config.language === "en") {
    currentLanguage = config.language;
    localStorage.setItem("pixelClockLanguage", currentLanguage);
    applyLanguage();
  }
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
  updateWifiSummary();
  applyPanelStartState(config);
  updateRangeValues();
  updateNightControlsFromStored();
  showAdminReminder(config);
}

function formBody() {
  updateBrightnessLimits(true);
  syncNightStoredFromDisplay();
  const data = new URLSearchParams();
  data.set("language", currentLanguage);
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
  currentFirmwareVersion = status.firmwareVersion || currentFirmwareVersion;
  $("firmwareLine").textContent = formatVersion(status.firmwareVersion) || "-";
  updateStaticVersionLines();
  renderFirmwareSelectionVersion(selectedFirmwareVersion);
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
      updateWifiSummary();
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
initCollapsiblePanels();
applyNormalPanelStartState();
resetUpdateInputs();
applyLanguage();
updateStaticVersionLines();
loadConfig().catch(() => message("Konfiguration konnte nicht geladen werden."));
loadStatus().catch(() => {});
setInterval(loadStatus, 5000);

$("languageSelect").addEventListener("change", (event) => setLanguage(event.target.value));
$("ssid").addEventListener("input", updateWifiSummary);
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
$("firmwareUpdateBtn").addEventListener("click", uploadFirmware);
$("firmwareFile").addEventListener("change", updateFirmwareSelectionInfo);
$("webUpdateBtn").addEventListener("click", uploadWebInterface);
$("webFile").addEventListener("change", updateWebSelectionInfo);
window.addEventListener("pageshow", resetUpdateInputs);
