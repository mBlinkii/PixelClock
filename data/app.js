const pins = [2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33];
const safeBrightnessPercent = 40;
const defaultWifiCountry = "DE";
const wifiCountries = [
  ["01", "Weltweit sicherer Modus", "World safe mode"],
  ["AT", "Österreich", "Austria"],
  ["AU", "Australien", "Australia"],
  ["BE", "Belgien", "Belgium"],
  ["BG", "Bulgarien", "Bulgaria"],
  ["BR", "Brasilien", "Brazil"],
  ["CA", "Kanada", "Canada"],
  ["CH", "Schweiz", "Switzerland"],
  ["CN", "China", "China"],
  ["CY", "Zypern", "Cyprus"],
  ["CZ", "Tschechien", "Czechia"],
  ["DE", "Deutschland", "Germany"],
  ["DK", "Dänemark", "Denmark"],
  ["EE", "Estland", "Estonia"],
  ["ES", "Spanien", "Spain"],
  ["FI", "Finnland", "Finland"],
  ["FR", "Frankreich", "France"],
  ["GB", "Vereinigtes Königreich", "United Kingdom"],
  ["GR", "Griechenland", "Greece"],
  ["HK", "Hong Kong", "Hong Kong"],
  ["HR", "Kroatien", "Croatia"],
  ["HU", "Ungarn", "Hungary"],
  ["IE", "Irland", "Ireland"],
  ["IN", "Indien", "India"],
  ["IS", "Island", "Iceland"],
  ["IT", "Italien", "Italy"],
  ["JP", "Japan", "Japan"],
  ["KR", "Südkorea", "South Korea"],
  ["LI", "Liechtenstein", "Liechtenstein"],
  ["LT", "Litauen", "Lithuania"],
  ["LU", "Luxembourg", "Luxembourg"],
  ["LV", "Lettland", "Latvia"],
  ["MT", "Malta", "Malta"],
  ["MX", "Mexico", "Mexico"],
  ["NL", "Niederlande", "Netherlands"],
  ["NO", "Norwegen", "Norway"],
  ["NZ", "Neuseeland", "New Zealand"],
  ["PL", "Polen", "Poland"],
  ["PT", "Portugal", "Portugal"],
  ["RO", "Rumänien", "Romania"],
  ["SE", "Schweden", "Sweden"],
  ["SI", "Slowenien", "Slovenia"],
  ["SK", "Slowakei", "Slovakia"],
  ["TW", "Taiwan", "Taiwan"],
  ["US", "Vereinigte Staaten", "United States"]
];
const fields = [
  "ssid", "wifiCountry", "wifiPowerSave", "hostname", "cityName", "timezone", "weatherProvider", "weatherIntervalHalfHours", "width", "height", "dataPin",
  "brightness", "fullBrightnessUnlocked", "wiringMode", "origin", "displayMode", "colorOrder",
  "temperatureUnit", "weatherIconEnabled", "hourFormat", "colorWeekday", "colorText", "colorPoint", "colorColon", "timePageSeconds", "pageSeconds",
  "colorGradientMode", "autoPage", "selectedPage", "nightBrightness", "nightStart", "nightEnd"
];

const $ = (id) => document.getElementById(id);
const adminReminderStorageKey = "pixelClockAdminReminderDismissed";
const panelCollapsedStoragePrefix = "pixelClockPanelCollapsed:";
const authStorageKey = "pixelClockAuth";
let currentLanguage = storedLanguage || ((navigator.language || "").toLowerCase().startsWith("de") ? "de" : "en");
let statusRefreshTimer = 0;
let statusInFlight = null;
let statusFailures = 0;
let savedForm = "";
let savedConfig = null;
let dirty = false;
let saving = false;
let capabilities = {};
let hasProviderKeys = {};

function authHeaderValue() {
  return sessionStorage.getItem(authStorageKey) || "";
}

window.authHeaderValue = authHeaderValue;

function setAuthHeader(value) {
  if (value) sessionStorage.setItem(authStorageKey, value);
  else sessionStorage.removeItem(authStorageKey);
}

function basicAuthValue(username, password) {
  const bytes = new TextEncoder().encode(`${username}:${password}`);
  let binary = "";
  for (const byte of bytes) binary += String.fromCharCode(byte);
  return `Basic ${btoa(binary)}`;
}

function authHeaders(existingHeaders) {
  const headers = new Headers(existingHeaders || {});
  const auth = authHeaderValue();
  if (auth) headers.set("Authorization", auth);
  return headers;
}

function setAuthenticatedView(isAuthenticated) {
  $("loginView").hidden = isAuthenticated;
  $("appShell").hidden = !isAuthenticated;
}

function showLogin(text = "Bitte anmelden.") {
  setAuthHeader("");
  setAuthenticatedView(false);
  $("loginMessage").textContent = tr(text);
  $("loginPassword").value = "";
  $("loginUsername").focus();
  if (statusRefreshTimer) {
    clearTimeout(statusRefreshTimer);
    statusRefreshTimer = 0;
  }
}

async function apiFetch(url, options = {}) {
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 12000);
  try {
    const res = await fetch(url, {
      ...options,
      signal: controller.signal,
      cache: "no-store",
      credentials: "same-origin",
      headers: authHeaders(options.headers)
    });
    if (res.status === 401) {
      showLogin("Bitte anmelden.");
      throw new Error("Unauthorized");
    }
    return res;
  } finally {
    clearTimeout(timeout);
  }
}

function scheduleStatusRefresh() {
  clearTimeout(statusRefreshTimer);
  if (document.hidden || !authHeaderValue() || !$("restartOverlay").hidden) return;
  statusRefreshTimer = setTimeout(async () => {
    try { await loadStatus(); } catch (_) { /* Connection state is shown in the header. */ }
    scheduleStatusRefresh();
  }, Math.min(60000, 15000 * 2 ** Math.min(statusFailures, 2)));
}

function showRestartNotice(visible) {
  $("restartNotice").hidden = !visible;
}

function showRestartOverlay(text = "Neustart läuft...") {
  clearTimeout(statusRefreshTimer);
  dirty = false;
  $("restartOverlay").hidden = false;
  const title = $("restartOverlay").querySelector("h2");
  title.textContent = tr(text);
  setTimeout(() => location.reload(), 9000);
}

window.showRestartOverlay = showRestartOverlay;
function translateTextNodes(root) {
  const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT);
  let node = walker.nextNode();
  while (node) {
    if (node.parentElement.closest("[data-no-i18n]")) { node = walker.nextNode(); continue; }
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
  fillWifiCountries();
  fillWeatherIntervals();
  translateTextNodes($("appShell"));
  translateTextNodes($("loginView"));
  translateTextNodes($("adminReminder"));
  translateTextNodes($("restartOverlay"));
  translateAttributes();
  updateAdminPasswordPlaceholder();
  updateWifiSummary();
  updateProviderFields();
  updateDirtyState();
}

function setLanguage(language) {
  currentLanguage = language === "de" ? "de" : "en";
  localStorage.setItem("pixelClockLanguage", currentLanguage);
  applyLanguage();
  if (savedConfig) {
    savedConfig.language = currentLanguage;
    const baseline = new URLSearchParams(savedForm);
    baseline.set("language", currentLanguage);
    savedForm = baseline.toString();
    updateDirtyState();
  }
  saveLanguagePreference().catch(() => message("Aktion fehlgeschlagen."));
  loadStatus().catch(() => {});
}

async function saveLanguagePreference() {
  const data = new URLSearchParams();
  data.set("language", currentLanguage);
  await apiFetch("/api/language", { method: "POST", body: data });
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
    if (id) { panel.dataset.panelId = id; panel.id = id; }

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
    body.id = `${id}Body`;
    toggle.setAttribute("aria-controls", body.id);
    heading.addEventListener("click", (event) => {
      if (!event.target.closest("button")) toggle.click();
    });
    while (heading.nextSibling) body.append(heading.nextSibling);
    panel.append(body);
    setPanelCollapsed(panel, true);
  }
}

function applyPanelStartState(config) {
  const setupOpenPanels = new Set(["statusSection", "wifiSection", "accessSection", "weatherSection", "helpSection"]);
  const normalOpenPanels = new Set(["statusSection", "weatherSection"]);
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
  const country = $("wifiCountry")?.value || "";
  summary.textContent = country ? `${ssid} - ${country}` : ssid;
  summary.hidden = !ssid;
}

function fillWifiCountries(selectedValue) {
  const current = selectedValue || $("wifiCountry").value || defaultWifiCountry;
  $("wifiCountry").innerHTML = "";
  for (const [code, deName, enName] of wifiCountries) {
    const option = document.createElement("option");
    option.value = code;
    option.textContent = `${code} - ${currentLanguage === "de" ? deName : enName}`;
    $("wifiCountry").append(option);
  }
  $("wifiCountry").value = wifiCountries.some(([code]) => code === current) ? current : defaultWifiCountry;
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
  savedConfig = { ...config };
  showRestartNotice(Boolean(config.restartRequired));
  hasProviderKeys = { 1: Boolean(config.hasOpenWeatherApiKey), 4: Boolean(config.hasWeatherApiKey) };
  for (const option of $("weatherProvider").options) option.disabled = Number(option.value) > (config.weatherProviderMax ?? 2);
  $("wifiPowerSaveField").hidden = config.wifiPowerSave === undefined;
  $("wifiPowerSave").disabled = config.wifiPowerSave === undefined;
  if (config.timePageSeconds === undefined || config.timePageSeconds === null) {
    config.timePageSeconds = config.pageSeconds ?? 8;
  }
  if (config.wifiCountry === undefined || config.wifiCountry === null) {
    config.wifiCountry = defaultWifiCountry;
  }
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
  for (const id of ["password", "adminPassword", "openWeatherApiKey", "weatherApiKey"]) $(id).value = "";
  updateProviderFields();
  updatePageControls();
  savedForm = formBody().toString();
  updateDirtyState();
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
  const password = $("password").value;
  if (password) data.set("password", password);
  const adminUsername = $("adminUsername").value.trim();
  if (adminUsername) data.set("adminUsername", adminUsername);
  const adminPassword = $("adminPassword").value;
  if (adminPassword) data.set("adminPassword", adminPassword);
  const openWeatherApiKey = $("openWeatherApiKey").value.trim();
  if (openWeatherApiKey) data.set("openWeatherApiKey", openWeatherApiKey);
  const weatherApiKey = $("weatherApiKey").value.trim();
  if (weatherApiKey) data.set("weatherApiKey", weatherApiKey);
  return data;
}

async function loadConfig() {
  const res = await apiFetch("/api/config");
  if (!res.ok) throw new Error("Config unavailable");
  setForm(await res.json());
}

async function fetchStatus() {
  const res = await apiFetch("/api/status");
  if (!res.ok) throw new Error("Status unavailable");
  const status = await res.json();
  capabilities = status.capabilities || {};
  $("connectionState").textContent = tr(status.wifiConnected ? "Verbunden" : status.setupMode ? "Setup-AP" : "WLAN getrennt");
  $("connectionState").classList.toggle("isWarning", !status.wifiConnected);
  $("weatherAge").textContent = status.weatherBusy ? tr("Wetter wird aktualisiert.") :
    status.weatherAgeMs == null ? "" : `${tr("Wetterabruf vor")} ${Math.floor(status.weatherAgeMs / 60000)} min`;
  $("runtimeStats").hidden = status.freeHeap === undefined;
  $("systemLine").textContent = status.freeHeap === undefined ? "" :
    `${Math.round(status.freeHeap / 1024)} KB ${tr("frei")} · ${status.rssi ?? 0} dBm · ${tr(status.wifiPowerSave ? "Energiesparen an" : "Energiesparen aus")}`;
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

function loadStatus() {
  if (statusInFlight) return statusInFlight;
  statusInFlight = fetchStatus().then(() => { statusFailures = 0; }).catch((error) => {
    statusFailures++;
    $("connectionState").textContent = tr("Uhr nicht erreichbar");
    $("connectionState").classList.add("isWarning");
    throw error;
  }).finally(() => { statusInFlight = null; });
  return statusInFlight;
}

function updateProviderFields() {
  const provider = Number($("weatherProvider").value);
  $("openWeatherApiKey").closest("label").hidden = provider !== 1;
  $("weatherApiKeyField").hidden = provider !== 4;
  $("metAttribution").hidden = provider !== 3;
  const hints = {
    0: "Weltweite Vorhersage, ohne API-Key. Tageshöchst- und Tiefsttemperatur verfügbar.",
    1: "Aktuelle Messwerte mit API-Key. Min/Max beziehen sich auf aktuelle Werte in der Umgebung.",
    2: "DWD-Messwerte über Bright Sky, vor allem für Deutschland. Ohne API-Key, ohne Tages-Min/Max.",
    3: "Weltweite Vorhersage von MET Norway. Ohne API-Key, ohne Tages-Min/Max. Die Cache-Zeit des Anbieters wird eingehalten.",
    4: "Aktuelles Wetter und Tages-Min/Max. Eigenen WeatherAPI-Key hinterlegen."
  };
  $("providerHint").textContent = tr(hints[provider] || "");
  $("providerKeyState").textContent = [1, 4].includes(provider) ?
    tr(hasProviderKeys[provider] ? "API-Key gespeichert. Leer lassen zum Beibehalten." : "Noch kein API-Key gespeichert.") : "";
}

function updatePageControls() {
  const automatic = $("autoPage").checked;
  $("selectedPage").disabled = automatic;
  $("timePageSeconds").disabled = !automatic;
  $("pageSeconds").disabled = !automatic;
}

function updateDirtyState() {
  if (!savedForm) return;
  dirty = formBody().toString() !== savedForm;
  $("saveState").textContent = tr(dirty ? "Ungespeicherte Änderungen" : "Alles gespeichert");
  $("saveState").classList.toggle("isDirty", dirty);
  $("saveBtn").disabled = saving || !dirty;
  $("discardBtn").disabled = saving || !dirty;
}

function validateSettings() {
  for (const el of document.querySelectorAll("section.panel input, section.panel select")) {
    if (el.disabled || el.type === "file" || el.type === "hidden" || el.closest("label")?.hidden) continue;
    if (!el.checkValidity()) {
      setPanelCollapsed(el.closest("section.panel"), false, true);
      el.reportValidity();
      return false;
    }
  }
  return true;
}

async function saveConfig() {
  if (saving || !savedConfig || !validateSettings()) return;
  const body = formBody();
  saving = true;
  const inputs = [...document.querySelectorAll("section.panel input, section.panel select")];
  const disabled = inputs.map((el) => el.disabled);
  inputs.forEach((el) => { el.disabled = true; });
  $("saveBtn").disabled = true;
  $("discardBtn").disabled = true;
  message("Speichere...");
  try {
    const res = await apiFetch("/api/config", { method: "POST", body });
    const data = await res.json();
    if (!res.ok) { message(data.error || "Speichern fehlgeschlagen."); return; }
    if (body.has("openWeatherApiKey")) hasProviderKeys[1] = true;
    if (body.has("weatherApiKey")) hasProviderKeys[4] = true;
    for (const id of ["password", "adminPassword", "openWeatherApiKey", "weatherApiKey"]) $(id).value = "";
    savedForm = formBody().toString();
    const numeric = new Set(["latitude", "longitude"]);
    for (const field of fields) {
      const el = $(field);
      savedConfig[field] = el.type === "checkbox" ? el.checked : numeric.has(field) ? Number(el.value) : el.value;
    }
    savedConfig.adminUsername = $("adminUsername").value;
    savedConfig.adminPasswordIsDefault = body.has("adminPassword") ? false : savedConfig.adminPasswordIsDefault;
    savedConfig.hasOpenWeatherApiKey = hasProviderKeys[1];
    savedConfig.hasWeatherApiKey = hasProviderKeys[4];
    savedConfig.restartRequired = Boolean(data.restartRequired);
    updateProviderFields();
    messageText(["Gespeichert.", data.cityResolutionPending ? "Ort wird im Hintergrund aktualisiert." : "",
      data.weatherRefreshPending ? "Wetter wird aktualisiert." : "",
      data.restartRequired ? "Neustart erforderlich" : "Sofort aktiv."].filter(Boolean).map(tr).join(" "));
    showRestartNotice(Boolean(data.restartRequired));
    if (data.authChanged) showLogin("Login wurde geändert, bitte mit den neuen Daten anmelden.");
  } catch (error) {
    if (error.message !== "Unauthorized") message("Speichern fehlgeschlagen.");
  } finally {
    inputs.forEach((el, i) => { el.disabled = disabled[i]; });
    saving = false;
    updateDirtyState();
  }
}

async function scanNetworks() {
  $("networks").textContent = tr("Suche...");
  let data;
  for (let attempt = 0; attempt < 15; attempt++) {
    const res = await apiFetch("/api/networks");
    data = await res.json();
    if (!res.ok) throw new Error(data.error || "Aktion fehlgeschlagen.");
    if (!data.scanning) break;
    await new Promise((resolve) => setTimeout(resolve, 1000));
  }
  if (data.scanning) {
    $("networks").textContent = tr("WLAN-Suche fehlgeschlagen. Bitte erneut versuchen.");
    throw new Error("WLAN-Suche fehlgeschlagen. Bitte erneut versuchen.");
  }
  $("networks").replaceChildren();
  const names = new Set();
  for (const network of (data.networks || []).sort((a, b) => b.rssi - a.rssi)) {
    if (!network.ssid || names.has(network.ssid)) continue;
    names.add(network.ssid);
    const btn = document.createElement("button");
    btn.type = "button";
    btn.textContent = `${network.ssid} · ${network.rssi} dBm`;
    btn.addEventListener("click", () => {
      $("ssid").value = network.ssid;
      updateWifiSummary();
      updateDirtyState();
    });
    $("networks").append(btn);
  }
  if (!names.size) $("networks").textContent = tr("Keine Netzwerke gefunden.");
}

async function postAction(url, doneText, options = {}) {
  const res = await apiFetch(url, { method: "POST" });
  if (!res.ok) {
    message(res.status === 401 ? "Admin-Anmeldung erforderlich." : "Aktion fehlgeschlagen.");
    return;
  }
  message(doneText);
  if (options.restart) showRestartOverlay(doneText);
}

async function refreshWeather() {
  const res = await apiFetch("/api/weather/refresh", { method: "POST" });
  if (!res.ok) {
    const data = await res.json();
    message(data.error || "Wetter konnte nicht aktualisiert werden.");
    return;
  }
  message("Wetteraktualisierung gestartet.");
  await loadStatus();
}

async function resetSettings() {
  if (!confirm(tr("Alle Einstellungen außer WLAN zurücksetzen und neu starten?"))) return;
  await postAction("/api/reset/settings", "Einstellungen werden zurückgesetzt...", { restart: true });
}

async function factoryReset() {
  if (!confirm(tr("Werksreset ausführen? Dabei werden auch WLAN-Daten gelöscht."))) return;
  if (!confirm(tr("Wirklich alles löschen? Der ESP startet danach im Setup-Modus."))) return;
  await postAction("/api/reset/factory", "Werksreset läuft...", { restart: true });
}

async function startAuthenticatedApp() {
  setAuthenticatedView(true);
  try {
    await loadConfig();
    await loadStatus();
    if (statusRefreshTimer) clearTimeout(statusRefreshTimer);
    scheduleStatusRefresh();
  } catch (error) {
    if (error.message !== "Unauthorized") message("Konfiguration konnte nicht geladen werden.");
  }
}

async function login(event) {
  event.preventDefault();
  if ($("loginBtn").disabled) return;
  $("loginBtn").disabled = true;
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 12000);
  const username = $("loginUsername").value.trim();
  const password = $("loginPassword").value;
  const auth = basicAuthValue(username, password);
  $("loginMessage").textContent = tr("Anmeldung läuft...");
  try {
    const res = await fetch("/api/status", {
      signal: controller.signal,
      cache: "no-store",
      credentials: "same-origin",
      headers: { Authorization: auth }
    });
    if (!res.ok) {
      $("loginMessage").textContent = tr("Anmeldung fehlgeschlagen.");
      return;
    }
    setAuthHeader(auth);
    $("loginMessage").textContent = tr("Angemeldet.");
    $("loginPassword").value = "";
    await startAuthenticatedApp();
  } catch (_) {
    $("loginMessage").textContent = tr("Anmeldung fehlgeschlagen.");
  } finally {
    clearTimeout(timeout);
    $("loginBtn").disabled = false;
  }
}

function logout() {
  showLogin("Bitte anmelden.");
}

function initUi() {
  fillPins();
  fillWifiCountries();
  fillWeatherIntervals();
  initCollapsiblePanels();
  for (const id of ["weatherLine", "locationLine", "urlLine", "firmwareLine", "littleFsLine", "networks", "message", "loginMessage", "firmwareSelectedVersion", "webSelectedVersion"]) $(id).dataset.noI18n = "";
  document.querySelectorAll(".sectionNav a").forEach((link) => link.addEventListener("click", () => {
    const section = document.querySelector(link.getAttribute("href"));
    if (section) setPanelCollapsed(section, false, true);
  }));
}
initUi();
applyNormalPanelStartState();
resetUpdateInputs();
applyLanguage();
updateStaticVersionLines();
if (authHeaderValue()) {
  startAuthenticatedApp();
} else {
  showLogin();
}

$("loginForm").addEventListener("submit", login);
$("logoutBtn").addEventListener("click", logout);
$("languageSelect").addEventListener("change", (event) => setLanguage(event.target.value));
$("ssid").addEventListener("input", updateWifiSummary);
$("wifiCountry").addEventListener("change", updateWifiSummary);
$("adminReminderGo").addEventListener("click", openAdminAccess);
$("adminReminderDismiss").addEventListener("click", () => closeAdminReminder(true));
$("saveBtn").addEventListener("click", saveConfig);
bindAction("scanBtn", scanNetworks);
$("brightnessPercent").addEventListener("input", () => syncBrightnessNumberFromSlider("brightness", "brightnessPercent", "brightnessPercentValue"));
$("nightBrightnessPercent").addEventListener("input", () => syncBrightnessNumberFromSlider("nightBrightness", "nightBrightnessPercent", "nightBrightnessPercentValue"));
$("brightnessPercentValue").addEventListener("input", () => syncBrightnessSliderFromNumber("brightness", "brightnessPercent", "brightnessPercentValue"));
$("nightBrightnessPercentValue").addEventListener("input", () => syncBrightnessSliderFromNumber("nightBrightness", "nightBrightnessPercent", "nightBrightnessPercentValue"));
$("fullBrightnessUnlocked").addEventListener("change", () => updateBrightnessLimits(true));
$("hourFormat").addEventListener("change", updateNightControlsFromStored);
for (const id of ["nightStartDisplay", "nightStartPeriod", "nightEndDisplay", "nightEndPeriod"]) {
  $(id).addEventListener("change", syncNightStoredFromDisplay);
}
bindAction("testBtn", () => postAction("/api/display/test", "Testmuster gestartet."));
bindAction("weatherBtn", refreshWeather);
bindAction("restartBtn", () => postAction("/api/restart", "Neustart läuft...", { restart: true }));
bindAction("restartRequiredBtn", () => postAction("/api/restart", "Neustart läuft...", { restart: true }));
bindAction("settingsResetBtn", resetSettings);
bindAction("factoryResetBtn", factoryReset);
$("firmwareUpdateBtn").addEventListener("click", uploadFirmware);
$("firmwareFile").addEventListener("change", updateFirmwareSelectionInfo);
$("webUpdateBtn").addEventListener("click", uploadWebInterface);
$("webFile").addEventListener("change", updateWebSelectionInfo);
window.addEventListener("pageshow", resetUpdateInputs);

function bindAction(id, action) {
  $(id).addEventListener("click", async () => {
    $(id).disabled = true;
    try { await action(); } catch (error) {
      if (error.message !== "Unauthorized") message(error.message === "Failed to fetch" || error.name === "AbortError" ? "Uhr nicht erreichbar" : error.message);
    } finally { $(id).disabled = false; }
  });
}

$("weatherProvider").addEventListener("change", updateProviderFields);
$("autoPage").addEventListener("change", updatePageControls);
$("discardBtn").addEventListener("click", () => { if (savedConfig) { setForm({ ...savedConfig }); message(""); } });
for (const eventName of ["input", "change"]) {
  $("appShell").addEventListener(eventName, (event) => {
    if (event.target.matches("input:not([type=file]), select") && event.target.id !== "languageSelect") updateDirtyState();
  });
}
window.addEventListener("beforeunload", (event) => {
  if (dirty) { event.preventDefault(); event.returnValue = ""; }
});
document.addEventListener("visibilitychange", () => {
  clearTimeout(statusRefreshTimer);
  if (!document.hidden && authHeaderValue()) loadStatus().catch(() => {}).finally(scheduleStatusRefresh);
});
