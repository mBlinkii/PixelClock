const pins = [2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33];
const safeBrightnessPercent = 40;
const maxLeds = 512;
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
  "ssid", "wifiCountry", "wifiPowerSave", "hostname", "cityName", "timezone", "weatherProvider", "weatherModel", "weatherIntervalHalfHours", "width", "height", "dataPin",
  "brightness", "fullBrightnessUnlocked", "wiringMode", "origin", "displayMode", "colorOrder",
  "temperatureUnit", "weatherIconEnabled", "hourFormat", "colorWeekday", "colorText", "colorPoint", "colorColon", "timePageSeconds", "pageSeconds",
  "colorGradientMode", "autoPage", "selectedPage", "nightBrightness", "nightStart", "nightEnd"
];
const pageNames = ["overview", "display", "weather", "hardware", "network", "system"];
// Open-Meteo "models" ids, same list as src/weather_models.h; "" = best_match.
const openMeteoModels = [
  ["", "Automatisch (bestes Modell für den Ort)", "Automatic (best model for the location)"],
  ["icon_seamless", "DWD ICON – Deutschland", "DWD ICON – Germany"],
  ["ecmwf_ifs025", "ECMWF IFS – Europa/weltweit", "ECMWF IFS – Europe/worldwide"],
  ["meteoswiss_icon_seamless", "MeteoSwiss – Schweiz", "MeteoSwiss – Switzerland"],
  ["geosphere_seamless", "GeoSphere – Österreich", "GeoSphere – Austria"],
  ["meteofrance_seamless", "Météo-France – Frankreich", "Météo-France – France"],
  ["knmi_seamless", "KNMI – Niederlande", "KNMI – Netherlands"],
  ["dmi_seamless", "DMI – Dänemark", "DMI – Denmark"],
  ["ukmo_seamless", "UK Met Office – Großbritannien", "UK Met Office – United Kingdom"],
  ["metno_seamless", "MET Nordic – Skandinavien", "MET Nordic – Scandinavia"],
  ["italia_meteo_arpae_icon_2i", "ItaliaMeteo ARPAE – Italien", "ItaliaMeteo ARPAE – Italy"],
  ["gfs_seamless", "NOAA GFS – USA/weltweit", "NOAA GFS – USA/worldwide"],
  ["gem_seamless", "GEM – Kanada", "GEM – Canada"],
  ["jma_seamless", "JMA – Japan", "JMA – Japan"]
];
// Visual settings the firmware can preview without saving (/api/display/preview).
const previewFields = [
  "brightness", "nightBrightness", "fullBrightnessUnlocked", "displayMode", "temperatureUnit", "weatherIconEnabled",
  "hourFormat", "colorWeekday", "colorText", "colorPoint", "colorColon", "colorGradientMode", "autoPage", "selectedPage",
  "pageSeconds", "timePageSeconds"
];
const secretFields = ["password", "adminPassword", "openWeatherApiKey", "weatherApiKey", "setupApPassword"];
const settingsExportFormat = "pixel-clock-settings";
const liveIdleMs = 5 * 60 * 1000;

const $ = (id) => document.getElementById(id);
const setupDismissedStorageKey = "pixelClockSetupDismissed";
const setupCardHiddenStorageKey = "pixelClockSetupCardHidden";
const authStorageKey = "pixelClockAuth";
const defaultAdminPassword = "pixelclock";
let currentLanguage = storedLanguage || ((navigator.language || "").toLowerCase().startsWith("de") ? "de" : "en");
let statusRefreshTimer = 0;
let statusInFlight = null;
let statusFailures = 0;
let savedForm = "";
let savedConfig = null;
let lastStatus = null;
let dirty = false;
let saving = false;
let capabilities = {};
let hasProviderKeys = {};
let currentPage = "overview";
let deviceClock = null;
let clockTimer = 0;
let toastTimer = 0;
let languageChosenBeforeLogin = "";
let loginMessage = null;
let recoveryMessage = null;
let locationWatchUntil = 0;
let previewTimer = 0;
let previewKeepAlive = 0;
let previewActive = false;
let liveTimer = 0;
let liveInFlight = false;
let livePaused = false;
let lastFrame = null;
let lastInteraction = Date.now();

// Browser storage can be unavailable (private windows, blocked site data).
function readStorage(key) {
  try { return localStorage.getItem(key); } catch (_) { return null; }
}

function writeStorage(key, value) {
  try {
    if (value === null) localStorage.removeItem(key);
    else localStorage.setItem(key, value);
  } catch (_) { /* Preference is optional. */ }
}

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

function decodeBasicAuth(value) {
  if (!value || !value.startsWith("Basic ")) return null;
  try {
    const bytes = Uint8Array.from(atob(value.slice(6)), (char) => char.charCodeAt(0));
    const text = new TextDecoder().decode(bytes);
    const split = text.indexOf(":");
    return split < 0 ? null : { username: text.slice(0, split), password: text.slice(split + 1) };
  } catch (_) {
    return null;
  }
}

// Mirrors sanitizeHostname() in src/config.cpp, which the firmware applies to
// the hostname and the admin user name.
function sanitizeName(value, fallback = "pixelclock") {
  const clean = String(value || "").trim().toLowerCase().replace(/[^a-z0-9-]/g, "").slice(0, 31).replace(/^-+|-+$/g, "");
  return clean || fallback;
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

function setLoginMessage(text, values = {}) {
  loginMessage = { text, values };
  $("loginMessage").textContent = trFormat(text, values);
}

function showLogin(text = "Bitte anmelden.") {
  setAuthHeader("");
  setAuthenticatedView(false);
  showRecovery(false);
  $("setupWizard").hidden = true;
  document.documentElement?.classList.remove("noScroll");
  setLoginMessage(text);
  $("loginPassword").value = "";
  $("loginUsername").focus();
  clearTimeout(clockTimer);
  clockTimer = 0;
  clearTimeout(liveTimer);
  liveTimer = 0;
  clearTimeout(previewTimer);
  clearTimeout(previewKeepAlive);
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
  // Poll quickly right after a location change until the clock has found it.
  const watching = Date.now() < locationWatchUntil;
  statusRefreshTimer = setTimeout(async () => {
    try { await loadStatus(); } catch (_) { /* Connection state is shown in the header. */ }
    scheduleStatusRefresh();
  }, watching ? 2500 : Math.min(60000, 15000 * 2 ** Math.min(statusFailures, 2)));
}

// What the clock found for the entered city or postal code. Firmware before
// 0.1.22 reports no locationPending; its label is shown as found.
function locationResult(status, typed, saved) {
  if (String(typed || "").trim() !== String(saved || "").trim()) return { text: "Der Ort wird nach dem Speichern gesucht.", state: "pending" };
  if (!status) return null;
  if (status.locationPending) {
    if (!status.locationError) return { text: "Ort wird gesucht...", state: "pending" };
    if (/nicht gefunden/.test(status.locationError)) return { text: "{error}. Bitte Eingabe prüfen.", values: { error: tr(status.locationError) }, state: "error" };
    return { text: "Ortssuche fehlgeschlagen: {error}", values: { error: status.locationError }, state: "error" };
  }
  return status.locationLabel ? { text: "Gefunden: {label}", values: { label: status.locationLabel }, state: "ok" } : null;
}

function renderLocationResult() {
  const result = locationResult(lastStatus, $("cityName").value, savedConfig?.cityName);
  const el = $("locationResult");
  el.hidden = !result;
  if (!result) return;
  el.textContent = trFormat(result.text, result.values || {});
  el.classList.toggle("isPending", result.state === "pending");
  el.classList.toggle("isError", result.state === "error");
  if (result.state !== "pending" && lastStatus?.locationPending === false) locationWatchUntil = 0;
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

function trFormat(text, values = {}) {
  return tr(text).replace(/\{(\w+)\}/g, (match, key) => (key in values ? String(values[key]) : match));
}

function translateTextNodes(root) {
  const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT);
  let node = walker.nextNode();
  while (node) {
    if (node.parentElement.closest("[data-no-i18n], script, style")) { node = walker.nextNode(); continue; }
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
  for (const el of document.querySelectorAll("[placeholder], [aria-label], [title]")) {
    for (const [attribute, key] of [["placeholder", "i18nPlaceholder"], ["aria-label", "i18nAria"], ["title", "i18nTitle"]]) {
      const value = el.getAttribute(attribute);
      if (value === null) continue;
      const source = el.dataset[key] || value;
      el.dataset[key] = source;
      el.setAttribute(attribute, tr(source));
    }
  }
}

function updateAdminPasswordPlaceholder() {
  $("adminPassword").placeholder = tr("Leer lassen zum Beibehalten");
}

function applyLanguage() {
  document.documentElement.lang = currentLanguage;
  $("languageSelect").value = currentLanguage;
  $("loginLanguage").value = currentLanguage;
  fillWifiCountries();
  fillWeatherIntervals();
  fillWeatherModels();
  translateTextNodes(document.body);
  translateAttributes();
  if (loginMessage) $("loginMessage").textContent = trFormat(loginMessage.text, loginMessage.values);
  if (recoveryMessage) $("recoveryMessage").textContent = trFormat(recoveryMessage.text, recoveryMessage.values);
  updateAdminPasswordPlaceholder();
  updateProviderFields();
  updateHardwareInfo();
  updateOverviewFromConfig();
  updateSetupCard();
  updateSetupApSection();
  if (lastFrame) drawLiveMatrix(lastFrame);
  if (lastStatus) {
    renderConnection(lastStatus);
    renderWeather(lastStatus);
    updateRestartDiagnostics(lastStatus);
  }
  renderDeviceClock();
  updateDirtyState();
  if (typeof refreshSetupWizardText === "function") refreshSetupWizardText();
}

function setLanguage(language) {
  currentLanguage = language === "de" ? "de" : "en";
  writeStorage("pixelClockLanguage", currentLanguage);
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

function setLoginLanguage(language) {
  currentLanguage = language === "de" ? "de" : "en";
  languageChosenBeforeLogin = currentLanguage;
  writeStorage("pixelClockLanguage", currentLanguage);
  applyLanguage();
}

async function saveLanguagePreference() {
  const data = new URLSearchParams();
  data.set("language", currentLanguage);
  await apiFetch("/api/language", { method: "POST", body: data });
}

function showToast(text) {
  const toast = $("message");
  clearTimeout(toastTimer);
  toast.textContent = text;
  toast.classList.toggle("isVisible", Boolean(text));
  if (text) toastTimer = setTimeout(() => toast.classList.toggle("isVisible", false), Math.max(4000, text.length * 70));
}

function message(text) {
  showToast(text ? tr(text) : "");
}

function messageText(text) {
  showToast(text);
}

function openAdminAccess() {
  showPage("network");
  document.querySelector(".accessSection")?.scrollIntoView({ behavior: "smooth", block: "start" });
  $("adminPassword").focus({ preventScroll: true });
}

// Pages are plain hash routes so the browser back button and bookmarks work.
function pageFromHash() {
  const name = location.hash.slice(1);
  return pageNames.includes(name) ? name : "overview";
}

function pageOf(el) {
  return el?.closest(".page")?.dataset.page || "overview";
}

function showPage(name) {
  if (!pageNames.includes(name)) name = "overview";
  const changed = currentPage !== name;
  currentPage = name;
  for (const page of document.querySelectorAll(".page")) page.hidden = page.dataset.page !== name;
  for (const tab of document.querySelectorAll(".tabs a")) {
    const active = tab.dataset.page === name;
    tab.classList.toggle("isActive", active);
    if (active) {
      tab.setAttribute("aria-current", "page");
      tab.scrollIntoView({ block: "nearest", inline: "nearest" });
    } else {
      tab.removeAttribute("aria-current");
    }
  }
  if (location.hash.slice(1) !== name) history.replaceState(null, "", `#${name}`);
  if (changed) window.scrollTo(0, 0);
  scheduleClockTick();
  scheduleLiveMatrix();
}

function markDirtyTabs(body) {
  const saved = new URLSearchParams(savedForm);
  const pages = new Set();
  for (const [key, value] of body) {
    if (saved.get(key) === value) continue;
    const el = $(key);
    if (el?.closest(".page")) pages.add(pageOf(el));
  }
  for (const tab of document.querySelectorAll(".tabs a")) tab.classList.toggle("isDirty", pages.has(tab.dataset.page));
}

function initPasswordReveal() {
  for (const input of document.querySelectorAll("input[data-reveal]")) {
    const wrap = document.createElement("span");
    wrap.className = "revealWrap";
    input.before(wrap);
    wrap.append(input);
    const button = document.createElement("button");
    button.type = "button";
    button.className = "revealBtn";
    button.innerHTML = '<svg class="icon" aria-hidden="true"><use href="#icon-eye"></use></svg>';
    button.setAttribute("aria-label", "Passwort anzeigen");
    button.setAttribute("aria-pressed", "false");
    button.addEventListener("click", () => {
      const reveal = input.type === "password";
      input.type = reveal ? "text" : "password";
      button.classList.toggle("isOn", reveal);
      button.setAttribute("aria-pressed", String(reveal));
      button.dataset.i18nAria = reveal ? "Passwort verbergen" : "Passwort anzeigen";
      button.setAttribute("aria-label", tr(button.dataset.i18nAria));
    });
    wrap.append(button);
  }
}

function hideRevealedPasswords() {
  for (const button of document.querySelectorAll(".revealBtn.isOn")) button.click();
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

function fillWeatherModels(selectedValue) {
  const select = $("weatherModel");
  const current = selectedValue ?? select.value ?? "";
  select.innerHTML = "";
  for (const [id, deName, enName] of openMeteoModels) {
    const option = document.createElement("option");
    option.value = id;
    option.textContent = currentLanguage === "de" ? deName : enName;
    select.append(option);
  }
  select.value = openMeteoModels.some(([id]) => id === current) ? current : "";
}

function weatherModelLabel(id) {
  const model = openMeteoModels.find(([modelId]) => modelId === id);
  return model && id ? (currentLanguage === "de" ? model[1] : model[2]).split(" – ")[0] : "";
}

function fillPins() {
  $("dataPin").innerHTML = pins.map((pin) => `<option value="${pin}">GPIO ${pin}</option>`).join("");
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

function weatherIconId(code) {
  if (code === null || code === undefined || code < 0) return "wx-cloud";
  if (code <= 1) return "wx-sun";
  if (code === 2) return "wx-partly";
  if (code === 45 || code === 48) return "wx-fog";
  if ((code >= 71 && code <= 77) || code === 85 || code === 86) return "wx-snow";
  if (code >= 95) return "wx-thunder";
  if (code >= 51) return "wx-rain";
  return "wx-cloud";
}

// Same physical LED order as xy() in src/display.cpp.
function ledIndex(x, y, width, height, origin, wiring) {
  let physX = x;
  let physY = y;
  if (origin === 1 || origin === 3) physX = width - 1 - physX;
  if (origin === 2 || origin === 3) physY = height - 1 - physY;
  if (wiring === 2 || wiring === 3) {
    if (wiring === 3 && physX % 2 === 1) physY = height - 1 - physY;
    return physX * height + physY;
  }
  if (wiring === 1 && physY % 2 === 1) physX = width - 1 - physX;
  return physY * width + physX;
}

function renderWiringDiagram(svg, origin, wiring) {
  if (!svg) return;
  const cols = 8;
  const rows = 4;
  const gap = 22;
  const pad = 14;
  const order = [];
  for (let y = 0; y < rows; y++) {
    for (let x = 0; x < cols; x++) order[ledIndex(x, y, cols, rows, origin, wiring)] = [pad + x * gap, pad + y * gap];
  }
  const width = pad * 2 + (cols - 1) * gap;
  const height = pad * 2 + (rows - 1) * gap;
  const [lastX, lastY] = order[order.length - 1];
  const [prevX, prevY] = order[order.length - 2];
  const angle = Math.atan2(lastY - prevY, lastX - prevX) * 180 / Math.PI;
  const leds = order.map(([x, y], i) => {
    const kind = i === 0 ? " ledStart" : i === order.length - 1 ? " ledEnd" : "";
    return `<circle class="led${kind}" cx="${x}" cy="${y}" r="${i === 0 ? 6.5 : 4.5}"/>`;
  }).join("");
  svg.setAttribute("viewBox", `0 0 ${width} ${height}`);
  svg.innerHTML = `<polyline class="wirePath" points="${order.map(([x, y]) => `${x},${y}`).join(" ")}"/>${leds}` +
    `<path class="arrow" d="M0 -5 L9 0 L0 5 Z" transform="translate(${lastX} ${lastY}) rotate(${angle}) translate(6 0)"/>`;
}

function updateHardwareInfo() {
  const width = Number.parseInt($("width").value, 10) || 0;
  const height = Number.parseInt($("height").value, 10) || 0;
  const count = width * height;
  const tooMany = count > maxLeds;
  $("height").setCustomValidity(tooMany ? tr("Maximal 512 LEDs (Breite × Höhe).") : "");
  $("ledCountLine").textContent = `${tr("LED-Anzahl")}: ${count || "-"}${tooMany ? ` · ${tr("Maximal 512 LEDs (Breite × Höhe).")}` : ""}`;
  renderWiringDiagram($("wiringDiagram"), Number($("origin").value), Number($("wiringMode").value));
}

function setupChecklist() {
  if (!savedConfig) return [];
  return [
    { done: Boolean(String(savedConfig.ssid || "").trim()) && !lastStatus?.setupMode, text: "WLAN verbunden" },
    { done: !savedConfig.adminPasswordIsDefault, text: "Eigenes Admin-Passwort" }
  ];
}

function setupIncomplete() {
  return setupChecklist().some((item) => !item.done);
}

// Only a clock without saved Wi-Fi (first start or after a factory reset)
// opens the assistant on its own; this is decided by the device, not by the
// browser, so a configured clock never shows it after login.
function isFirstSetup() {
  return Boolean(savedConfig) && !String(savedConfig.ssid || "").trim();
}

function updateSetupCard() {
  const items = setupChecklist();
  $("setupCard").hidden = !items.some((item) => !item.done) || readStorage(setupCardHiddenStorageKey) === "1";
  $("setupCardPasswordBtn").hidden = !savedConfig?.adminPasswordIsDefault;
  const list = $("setupChecklist");
  list.replaceChildren();
  for (const item of items) {
    const li = document.createElement("li");
    li.className = item.done ? "isDone" : "isOpen";
    li.textContent = `${item.done ? "✓" : "○"} ${tr(item.text)}`;
    list.append(li);
  }
}

function setLink(link, url) {
  link.textContent = url ? url.replace(/^https?:\/\//, "") : "-";
  link.href = url || "/";
}

function updateOverviewFromConfig() {
  if (!savedConfig) return;
  $("locationLine").textContent = savedConfig.locationLabel || savedConfig.cityName || "-";
  setLink($("urlLine"), savedConfig.url);
  $("brightnessLine").textContent =
    `${byteToPercent(savedConfig.brightness)} % · ${tr("Nacht")} ${byteToPercent(savedConfig.nightBrightness)} %`;
  for (const canvas of [$("liveMatrix"), $("liveMatrixDisplay")]) {
    if (!lastFrame && savedConfig.width && savedConfig.height) canvas.style.aspectRatio = `${savedConfig.width} / ${savedConfig.height}`;
  }
  updateSetupCard();
}

function updateSetupApSection() {
  const supported = savedConfig?.setupApPasswordIsDefault !== undefined;
  $("setupApSection").hidden = !supported;
  if (!supported) return;
  $("setupApSsidLine").textContent = savedConfig.setupApSsid || "PixelClock-Setup";
  $("setupApPasswordState").textContent = tr(savedConfig.setupApPasswordIsDefault ? "Keins (offenes WLAN)" : "Eigenes Passwort");
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
    writeStorage("pixelClockLanguage", currentLanguage);
    applyLanguage();
  }
  for (const field of fields) {
    const el = $(field);
    if (!el) continue;
    if (el.type === "checkbox") el.checked = Boolean(config[field]);
    else el.value = config[field] ?? "";
  }
  updateBrightnessLimits(false);
  $("adminUsername").value = config.adminUsername || config.defaultAdminUsername || "admin";
  updateAdminPasswordPlaceholder();
  updateRangeValues();
  updateNightControlsFromStored();
  for (const id of secretFields) $(id).value = "";
  if (config.maxAdminPasswordLength) $("adminPassword").maxLength = config.maxAdminPasswordLength;
  hideRevealedPasswords();
  updateProviderFields();
  updatePageControls();
  updateHardwareInfo();
  updateOverviewFromConfig();
  updateSetupApSection();
  savedForm = formBody().toString();
  updateDirtyState();
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
  const setupApPassword = $("setupApPassword").value;
  if (setupApPassword) data.set("setupApPassword", setupApPassword);
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

function formatUptime(milliseconds) {
  if (!Number.isFinite(milliseconds) || milliseconds < 0) return "-";
  const seconds = Math.floor(milliseconds / 1000);
  const days = Math.floor(seconds / 86400);
  const clock = [Math.floor(seconds / 3600) % 24, Math.floor(seconds / 60) % 60, seconds % 60]
    .map(value => String(value).padStart(2, "0")).join(":");
  return `${days ? `${days} d ` : ""}${clock}`;
}

function updateRestartDiagnostics(status) {
  $("restartStats").hidden = !status.resetReason;
  $("restartLine").textContent = status.resetReason
    ? `${formatUptime(status.uptimeMs)} · ${tr(status.resetReason)}` : "";
  const memory = [];
  if (Number.isFinite(status.minFreeHeap)) memory.push(`${tr("Min. freier Speicher")}: ${Math.round(status.minFreeHeap / 1024)} KB`);
  if (status.networkStackFreeBytes > 0) memory.push(`${tr("Min. freier Wetter-Stack")}: ${status.networkStackFreeBytes} B`);
  $("memoryLine").textContent = memory.join(" · ");
}

// The device reports its local wall time; the browser only advances it between polls.
function setDeviceClock(status) {
  const match = /^(\d{4})-(\d{2})-(\d{2})[ T](\d{2}):(\d{2}):(\d{2})/.exec(status.localTime || "");
  deviceClock = match ? {
    base: Date.UTC(Number(match[1]), Number(match[2]) - 1, Number(match[3]), Number(match[4]), Number(match[5]), Number(match[6])),
    at: Date.now()
  } : null;
  $("timeSyncState").hidden = !deviceClock || status.lastNtpMs === undefined || status.lastNtpMs > 0;
  renderDeviceClock();
  scheduleClockTick();
}

function renderDeviceClock() {
  $("timeSyncState").textContent = tr("Zeit noch nicht synchronisiert");
  if (!deviceClock) {
    $("clockTime").textContent = "--:--";
    $("clockDate").textContent = savedConfig ? tr("keine Uhrzeit") : "";
    return;
  }
  const now = new Date(deviceClock.base + Date.now() - deviceClock.at);
  const locale = currentLanguage === "de" ? "de-DE" : "en-US";
  const hour12 = String(savedConfig?.hourFormat) === "12";
  $("clockTime").textContent = now.toLocaleTimeString(locale, { hour: hour12 ? "numeric" : "2-digit", minute: "2-digit", hour12, timeZone: "UTC" });
  $("clockDate").textContent = now.toLocaleDateString(locale, { weekday: "long", day: "numeric", month: "long", year: "numeric", timeZone: "UTC" });
}

function scheduleClockTick() {
  clearTimeout(clockTimer);
  clockTimer = 0;
  if (!deviceClock || document.hidden || currentPage !== "overview" || $("appShell").hidden) return;
  const elapsed = Date.now() - deviceClock.at;
  clockTimer = setTimeout(() => {
    renderDeviceClock();
    scheduleClockTick();
  }, 1000 - (elapsed % 1000) + 25);
}

// Converts /api/display/frame (physical LED order, RRGGBB hex) into row-major colors.
function frameToGrid(frame) {
  const width = Number(frame.width) || 0;
  const height = Number(frame.height) || 0;
  const count = Math.min(Number(frame.count) || 0, width * height);
  const pixels = String(frame.pixels || "");
  const colors = [];
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const index = ledIndex(x, y, width, height, Number(frame.origin), Number(frame.wiring));
      colors.push(index < count ? pixels.slice(index * 6, index * 6 + 6) || "000000" : "000000");
    }
  }
  return { width, height, colors };
}

function paintMatrix(canvas, grid, brightness) {
  if (!canvas || canvas.closest("[hidden]") || !grid.width || !grid.height) return;
  const cssWidth = canvas.clientWidth;
  if (!cssWidth) return;
  const ratio = window.devicePixelRatio || 1;
  const cell = cssWidth / grid.width;
  canvas.width = Math.round(cssWidth * ratio);
  canvas.height = Math.round(cell * grid.height * ratio);
  canvas.style.height = `${cell * grid.height}px`;
  const context = canvas.getContext("2d");
  context.setTransform(ratio, 0, 0, ratio, 0, 0);
  context.fillStyle = "#050607";
  context.fillRect(0, 0, cssWidth, cell * grid.height);
  grid.colors.forEach((hex, i) => {
    const lit = brightness > 0 && hex !== "000000";
    context.beginPath();
    context.fillStyle = lit ? `#${hex}` : "#16191e";
    context.arc((i % grid.width + 0.5) * cell, (Math.floor(i / grid.width) + 0.5) * cell, cell * 0.38, 0, Math.PI * 2);
    context.fill();
  });
}

function drawLiveMatrix(frame) {
  lastFrame = frame;
  const grid = frameToGrid(frame);
  paintMatrix($("liveMatrix"), grid, frame.brightness);
  paintMatrix($("liveMatrixDisplay"), grid, frame.brightness);
  for (const chip of document.querySelectorAll(".previewChip")) chip.hidden = !frame.preview;
  $("liveCard").querySelector(".liveHint").textContent = frame.brightness === 0 ? tr("Die Anzeige ist ausgeschaltet (Helligkeit 0 %).") : "";
}

function liveMatrixActive() {
  return Boolean(capabilities.displayFrame) && !document.hidden && !$("appShell").hidden &&
    $("restartOverlay").hidden && (currentPage === "overview" || currentPage === "display");
}

function setLivePaused(paused) {
  livePaused = paused;
  for (const chip of document.querySelectorAll(".pausedChip")) chip.hidden = !paused;
}

// Polls the frame only while a live view is visible: 1 s while editing the
// display page, 3 s on the overview, paused after 5 minutes without input.
function scheduleLiveMatrix(delay = 0) {
  clearTimeout(liveTimer);
  liveTimer = 0;
  if (liveInFlight || !liveMatrixActive()) return;
  if (Date.now() - lastInteraction > liveIdleMs) {
    setLivePaused(true);
    return;
  }
  setLivePaused(false);
  liveTimer = setTimeout(refreshLiveMatrix, delay);
}

async function refreshLiveMatrix() {
  liveTimer = 0;
  if (!liveMatrixActive()) return;
  liveInFlight = true;
  try {
    const res = await apiFetch("/api/display/frame");
    if (res.ok) drawLiveMatrix(await res.json());
  } catch (_) {
    /* The next poll retries. */
  } finally {
    liveInFlight = false;
  }
  scheduleLiveMatrix(currentPage === "display" ? 1000 : 3000);
}

function noteInteraction() {
  lastInteraction = Date.now();
  if (livePaused) scheduleLiveMatrix();
}

function updateCapabilityUi() {
  const live = Boolean(capabilities.displayFrame);
  $("liveCard").hidden = !live;
  $("liveCardDisplay").hidden = !live;
  $("resetButtonHint").hidden = !capabilities.resetButton;
  if (live && !liveTimer && !liveInFlight && !livePaused) scheduleLiveMatrix();
}

// Visual changes are shown on the clock right away and revert on the device
// unless saved (DISPLAY_PREVIEW_MS in the firmware).
function previewDiffersFromSaved(body) {
  const saved = new URLSearchParams(savedForm);
  return previewFields.some((key) => body.get(key) !== saved.get(key));
}

function schedulePreview() {
  if (!capabilities.displayPreview || !savedForm) return;
  clearTimeout(previewTimer);
  previewTimer = setTimeout(sendPreview, 250);
}

async function sendPreview() {
  previewTimer = 0;
  clearTimeout(previewKeepAlive);
  const body = formBody();
  if (!previewDiffersFromSaved(body)) {
    await cancelPreview();
    return;
  }
  const data = new URLSearchParams();
  for (const key of previewFields) data.set(key, body.get(key) ?? "");
  try {
    const res = await apiFetch("/api/display/preview", { method: "POST", body: data });
    previewActive = res.ok;
  } catch (_) {
    return;
  }
  if (!previewActive) return;
  scheduleLiveMatrix(150);
  previewKeepAlive = setTimeout(() => { if (!document.hidden && dirty) sendPreview(); }, 90000);
}

async function cancelPreview() {
  clearTimeout(previewTimer);
  clearTimeout(previewKeepAlive);
  if (!previewActive) return;
  previewActive = false;
  try {
    await apiFetch("/api/display/preview/cancel", { method: "POST" });
  } catch (_) {
    /* The device reverts after its timeout. */
  }
  scheduleLiveMatrix(150);
}

function renderConnection(status) {
  const connected = Boolean(status.wifiConnected);
  $("connectionState").textContent = tr(connected ? "Verbunden" : status.setupMode ? "Setup-AP" : "WLAN getrennt");
  $("connectionState").classList.toggle("isWarning", !connected);
  $("networkLine").textContent = connected
    ? `${savedConfig?.ssid || tr("WLAN")} · ${status.rssi ?? 0} dBm`
    : status.setupMode ? `${tr("Setup-AP")} · PixelClock-Setup` : tr("WLAN getrennt");
  $("ipLine").textContent = status.ip || "-";
  if (status.url) setLink($("urlLine"), status.url);
  $("runtimeStats").hidden = status.freeHeap === undefined;
  $("systemLine").textContent = status.freeHeap === undefined ? "" :
    `${Math.round(status.freeHeap / 1024)} KB ${tr("frei")} · ${status.rssi ?? 0} dBm · ${tr(status.wifiPowerSave ? "Energiesparen an" : "Energiesparen aus")}`;
}

function renderWeather(status) {
  const isNumber = (value) => typeof value === "number" && Number.isFinite(value);
  const unit = `°${status.temperatureUnit || "C"}`;
  $("weatherTemp").textContent = isNumber(status.temperature) ? `${Math.round(status.temperature)}${unit}` : "--";
  $("weatherLine").textContent = weatherDescription(status.weatherCode);
  $("weatherIcon").querySelector("use").setAttribute("href", `#${weatherIconId(status.weatherCode)}`);
  const meta = [];
  if (isNumber(status.temperatureMin) && isNumber(status.temperatureMax)) {
    meta.push(`↓ ${Math.round(status.temperatureMin)}° ↑ ${Math.round(status.temperatureMax)}°`);
  }
  if (status.weatherProvider) {
    const model = weatherModelLabel(status.weatherModel);
    meta.push(model ? `${status.weatherProvider} · ${model}` : status.weatherProvider);
  }
  $("weatherMeta").textContent = meta.join(" · ");
  $("weatherAge").textContent = status.weatherBusy ? tr("Wetter wird aktualisiert.") :
    status.weatherAgeMs == null ? "" : trFormat("Wetterabruf vor {min} min", { min: Math.floor(status.weatherAgeMs / 60000) });
  $("weatherError").hidden = !status.weatherError;
  $("weatherError").textContent = status.weatherError ? `${tr("Fehler")}: ${tr(status.weatherError)}` : "";
}

async function fetchStatus() {
  const res = await apiFetch("/api/status");
  if (!res.ok) throw new Error("Status unavailable");
  const status = await res.json();
  lastStatus = status;
  updateRestartDiagnostics(status);
  capabilities = status.capabilities || {};
  updateCapabilityUi();
  renderConnection(status);
  renderWeather(status);
  setDeviceClock(status);
  currentFirmwareVersion = status.firmwareVersion || currentFirmwareVersion;
  $("firmwareLine").textContent = formatVersion(status.firmwareVersion) || "-";
  updateStaticVersionLines();
  renderFirmwareSelectionVersion(selectedFirmwareVersion);
  if (status.locationLabel || status.cityName) $("locationLine").textContent = status.locationLabel || status.cityName;
  renderLocationResult();
  updateSetupCard();
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
  // Firmware before 0.1.19 has no model setting and ignores the field.
  $("weatherModelField").hidden = provider !== 0 || savedConfig?.weatherModel === undefined;
  const hints = {
    0: "Weltweite Vorhersage, ohne API-Key. Tageshöchst- und Tiefsttemperatur verfügbar.",
    // Shown together with the model selection.
    model: "Wählt das Vorhersagemodell eines Wetterdienstes. Landesmodelle sind in ihrer Region meist am genauesten.",
    1: "Aktuelle Messwerte mit API-Key. Min/Max beziehen sich auf aktuelle Werte in der Umgebung.",
    2: "DWD-Messwerte über Bright Sky, vor allem für Deutschland. Ohne API-Key, ohne Tages-Min/Max.",
    3: "Weltweite Vorhersage von MET Norway. Ohne API-Key, ohne Tages-Min/Max. Die Cache-Zeit des Anbieters wird eingehalten.",
    4: "Aktuelles Wetter und Tages-Min/Max. Eigenen WeatherAPI-Key hinterlegen."
  };
  $("providerHint").textContent = [hints[provider], $("weatherModelField").hidden ? "" : hints.model]
    .filter(Boolean).map(tr).join(" ");
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
  const body = formBody();
  dirty = body.toString() !== savedForm;
  $("saveState").textContent = tr(dirty ? "Ungespeicherte Änderungen" : "Alles gespeichert");
  $("saveState").classList.toggle("isDirty", dirty);
  $("saveBtn").disabled = saving || !dirty;
  $("discardBtn").disabled = saving || !dirty;
  $("saveBar").classList.toggle("isVisible", dirty || saving);
  markDirtyTabs(body);
}

function fieldLabelText(el) {
  const label = el.closest("label");
  const text = label ? Array.from(label.childNodes).find((node) => node.nodeType === Node.TEXT_NODE && node.nodeValue.trim()) : null;
  return text ? text.nodeValue.trim() : el.name || el.id;
}

function invalidFieldMessage(el) {
  return `${fieldLabelText(el)}: ${el.validationMessage}`;
}

function findInvalidField() {
  for (const el of document.querySelectorAll(".page input, .page select")) {
    if (el.disabled || el.type === "file" || el.type === "hidden" || el.closest("label")?.hidden) continue;
    if (!el.checkValidity()) return el;
  }
  return null;
}

function validateSettings() {
  const invalid = findInvalidField();
  if (!invalid) return true;
  showPage(pageOf(invalid));
  invalid.closest("details")?.setAttribute("open", "");
  invalid.reportValidity();
  return false;
}

// The firmware accepts new credentials immediately, so keep the session instead
// of forcing a new login after the admin user or password changed.
function adoptChangedLogin(body) {
  const current = decodeBasicAuth(authHeaderValue());
  if (!current) {
    showLogin("Login wurde geändert, bitte mit den neuen Daten anmelden.");
    return;
  }
  const username = body.has("adminUsername") ? sanitizeName(body.get("adminUsername"), current.username) : current.username;
  setAuthHeader(basicAuthValue(username, body.get("adminPassword") || current.password));
  $("adminUsername").value = username;
  savedConfig.adminUsername = username;
}

async function saveConfig() {
  if (saving || !savedConfig || !validateSettings()) return null;
  const body = formBody();
  saving = true;
  const inputs = [...document.querySelectorAll(".page input, .page select")];
  const disabled = inputs.map((el) => el.disabled);
  inputs.forEach((el) => { el.disabled = true; });
  $("saveBtn").disabled = true;
  $("discardBtn").disabled = true;
  message("Speichere...");
  try {
    const res = await apiFetch("/api/config", { method: "POST", body });
    const data = await res.json();
    if (!res.ok) { message(data.error || "Speichern fehlgeschlagen."); return null; }
    if (body.has("openWeatherApiKey")) hasProviderKeys[1] = true;
    if (body.has("weatherApiKey")) hasProviderKeys[4] = true;
    if (data.authChanged) adoptChangedLogin(body);
    for (const id of secretFields) $(id).value = "";
    hideRevealedPasswords();
    const numeric = new Set(["latitude", "longitude"]);
    for (const field of fields) {
      const el = $(field);
      savedConfig[field] = el.type === "checkbox" ? el.checked : numeric.has(field) ? Number(el.value) : el.value;
    }
    previewActive = false;
    clearTimeout(previewTimer);
    clearTimeout(previewKeepAlive);
    if (data.hostname) {
      $("hostname").value = data.hostname;
      savedConfig.hostname = data.hostname;
    }
    if (data.setupApPasswordIsDefault !== undefined) savedConfig.setupApPasswordIsDefault = data.setupApPasswordIsDefault;
    if (data.url) savedConfig.url = data.url;
    if (data.locationLabel) savedConfig.locationLabel = data.locationLabel;
    if (body.has("password")) savedConfig.hasPassword = true;
    savedForm = formBody().toString();
    savedConfig.adminUsername = $("adminUsername").value;
    savedConfig.adminPasswordIsDefault = data.adminPasswordIsDefault ??
      (body.has("adminPassword") ? false : savedConfig.adminPasswordIsDefault);
    savedConfig.hasOpenWeatherApiKey = hasProviderKeys[1];
    savedConfig.hasWeatherApiKey = hasProviderKeys[4];
    savedConfig.restartRequired = Boolean(data.restartRequired);
    updateProviderFields();
    updateOverviewFromConfig();
    updateSetupApSection();
    renderDeviceClock();
    scheduleLiveMatrix(200);
    if (data.cityResolutionPending) {
      if (lastStatus) lastStatus = { ...lastStatus, locationPending: true, locationError: "" };
      locationWatchUntil = Date.now() + 60000;
      scheduleStatusRefresh();
    }
    renderLocationResult();
    messageText(["Gespeichert.", data.cityResolutionPending ? "Ort wird im Hintergrund aktualisiert." : "",
      data.weatherRefreshPending ? "Wetter wird aktualisiert." : "",
      data.restartRequired ? "Neustart erforderlich" : "Sofort aktiv."].filter(Boolean).map(tr).join(" "));
    showRestartNotice(Boolean(data.restartRequired));
    return data;
  } catch (error) {
    if (error.message !== "Unauthorized") message("Speichern fehlgeschlagen.");
    return null;
  } finally {
    inputs.forEach((el, i) => { el.disabled = disabled[i]; });
    saving = false;
    updateDirtyState();
  }
}

async function fetchNetworks() {
  let data;
  for (let attempt = 0; attempt < 15; attempt++) {
    const res = await apiFetch("/api/networks");
    data = await res.json();
    if (!res.ok) throw new Error(data.error || "Aktion fehlgeschlagen.");
    if (!data.scanning) break;
    await new Promise((resolve) => setTimeout(resolve, 1000));
  }
  if (data.scanning) throw new Error("WLAN-Suche fehlgeschlagen. Bitte erneut versuchen.");
  const names = new Set();
  return (data.networks || []).sort((a, b) => b.rssi - a.rssi).filter((network) => {
    if (!network.ssid || names.has(network.ssid)) return false;
    names.add(network.ssid);
    return true;
  });
}

function signalLevel(rssi) {
  return rssi >= -55 ? 4 : rssi >= -67 ? 3 : rssi >= -75 ? 2 : 1;
}

// SSIDs are untrusted text: only ever assign them through textContent.
// After a pick the list shrinks to the chosen network, so on a phone the
// password field stays visible above the on-screen keyboard.
function setNetworkListCollapsed(container, more, collapsed, others) {
  container.classList.toggle("isCollapsed", collapsed);
  more.hidden = !collapsed || !others;
  more.textContent = trFormat("Andere Netzwerke anzeigen ({count})", { count: others });
}

function renderNetworks(container, networks, onPick, selectedSsid, collapsed = false) {
  container.replaceChildren();
  if (!networks.length) {
    container.textContent = tr("Keine Netzwerke gefunden.");
    return;
  }
  const more = document.createElement("button");
  more.type = "button";
  more.className = "networkMore linkButton";
  const othersThan = (ssid) => networks.filter((network) => network.ssid !== ssid).length;
  for (const network of networks) {
    const button = document.createElement("button");
    button.type = "button";
    button.className = network.ssid === selectedSsid ? "networkItem isSelected" : "networkItem";
    const name = document.createElement("span");
    name.className = "networkName";
    name.textContent = network.ssid;
    const meta = document.createElement("span");
    meta.className = `networkMeta signal${signalLevel(network.rssi)}`;
    meta.textContent = `${network.secure === false ? `${tr("offen")} · ` : ""}${network.rssi} dBm`;
    button.append(name, meta);
    button.addEventListener("click", () => {
      for (const item of container.children) item.classList.toggle("isSelected", item === button);
      setNetworkListCollapsed(container, more, true, networks.length - 1);
      onPick(network);
    });
    container.append(button);
  }
  more.addEventListener("click", () => setNetworkListCollapsed(container, more, false, 0));
  container.append(more);
  const selected = networks.some((network) => network.ssid === selectedSsid);
  setNetworkListCollapsed(container, more, collapsed && selected, othersThan(selectedSsid));
}

async function scanNetworks() {
  $("networks").textContent = tr("Suche...");
  let networks;
  try {
    networks = await fetchNetworks();
  } catch (error) {
    $("networks").textContent = tr(error.message === "Unauthorized" ? "" : error.message);
    throw error;
  }
  renderNetworks($("networks"), networks, (network) => {
    $("ssid").value = network.ssid;
    updateDirtyState();
    $("password").focus();
  }, $("ssid").value);
}

async function postAction(url, doneText, options = {}) {
  const res = await apiFetch(url, { method: "POST" });
  if (!res.ok) {
    message(res.status === 401 ? "Admin-Anmeldung erforderlich." : "Aktion fehlgeschlagen.");
    return false;
  }
  message(doneText);
  if (options.restart) showRestartOverlay(doneText);
  return true;
}

async function runTestPattern() {
  const res = await apiFetch("/api/display/test", { method: "POST" });
  if (!res.ok) {
    message("Aktion fehlgeschlagen.");
    return false;
  }
  messageText([tr("Testmuster gestartet."), dirty ? tr("Ungespeicherte Änderungen sind darin noch nicht enthalten.") : ""].filter(Boolean).join(" "));
  return true;
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

function openFactoryReset() {
  $("factoryResetConfirm").checked = false;
  $("factoryResetGo").disabled = true;
  $("factoryResetModal").hidden = false;
  $("factoryResetCancel").focus();
}

function closeFactoryReset() {
  $("factoryResetModal").hidden = true;
}

// The clock leaves this network after a factory reset, so the page shows the
// next steps instead of reloading.
function showFinalOverlay(title, text) {
  clearTimeout(statusRefreshTimer);
  clearTimeout(liveTimer);
  dirty = false;
  $("restartOverlay").hidden = false;
  $("restartOverlay").querySelector(".spinner").hidden = true;
  $("restartOverlay").querySelector("h2").textContent = tr(title);
  $("restartOverlayText").textContent = text;
}

async function factoryReset() {
  const res = await apiFetch("/api/reset/factory", { method: "POST" });
  if (!res.ok) {
    message("Aktion fehlgeschlagen.");
    return;
  }
  const data = await res.json().catch(() => ({}));
  const ssid = data.setupApSsid || lastStatus?.setupApSsid || "PixelClock-Setup";
  closeFactoryReset();
  writeStorage(setupDismissedStorageKey, null);
  writeStorage(setupCardHiddenStorageKey, null);
  setAuthHeader("");
  showFinalOverlay("Werksreset läuft...", trFormat(
    "Die Uhr löscht alle Daten und startet im Setup-Modus. Zum Einrichten mit dem WLAN „{ssid}“ verbinden; ein Passwort ist nicht nötig.",
    { ssid }));
}

// Exports saved, non-secret settings; secrets never leave the device.
function buildSettingsExport() {
  const saved = new URLSearchParams(savedForm);
  const settings = {};
  for (const field of fields) {
    if (!secretFields.includes(field) && saved.has(field)) settings[field] = saved.get(field);
  }
  settings.language = currentLanguage;
  return {
    format: settingsExportFormat,
    version: 1,
    exportedAt: new Date().toISOString(),
    firmwareVersion: typeof currentFirmwareVersion === "string" ? currentFirmwareVersion : "",
    webVersion: typeof littleFsVersion === "string" ? littleFsVersion : "",
    settings
  };
}

function exportSettings() {
  const blob = new Blob([JSON.stringify(buildSettingsExport(), null, 2)], { type: "application/json" });
  const link = document.createElement("a");
  link.href = URL.createObjectURL(blob);
  link.download = `pixel-clock-${sanitizeName(savedConfig?.hostname)}-${new Date().toISOString().slice(0, 10)}.json`;
  document.body.append(link);
  link.click();
  link.remove();
  setTimeout(() => URL.revokeObjectURL(link.href), 1000);
  message("Einstellungen exportiert.");
}

// Accepts only known form fields with primitive values from an export file.
function parseSettingsImport(text) {
  let data;
  try {
    data = JSON.parse(text);
  } catch (_) {
    data = null;
  }
  if (!data || data.format !== settingsExportFormat || typeof data.settings !== "object" || data.settings === null) {
    throw new Error("Die Datei ist keine gültige Pixel-Clock-Sicherung.");
  }
  const values = {};
  for (const field of fields) {
    const value = data.settings[field];
    if (["string", "number", "boolean"].includes(typeof value)) values[field] = String(value);
  }
  const language = data.settings.language === "de" || data.settings.language === "en" ? data.settings.language : "";
  return { values, language };
}

function applySettingsImport({ values, language }) {
  let applied = 0;
  for (const [field, raw] of Object.entries(values)) {
    const el = $(field);
    if (!el) continue;
    if (el.type === "checkbox") el.checked = raw === "1" || raw === "true";
    else if (el.tagName === "SELECT" && !Array.from(el.options).some((option) => option.value === raw)) continue;
    else el.value = raw;
    applied++;
  }
  updateRangeValues();
  updateNightControlsFromStored();
  updateProviderFields();
  updatePageControls();
  updateHardwareInfo();
  if (language && language !== currentLanguage) setLanguage(language);
  updateDirtyState();
  schedulePreview();
  return applied;
}

async function importSettings(file) {
  if (!file) return;
  const applied = applySettingsImport(parseSettingsImport(await file.text()));
  messageText(trFormat("{count} Einstellungen übernommen. Prüfen und speichern.", { count: applied }));
}

async function startAuthenticatedApp() {
  setAuthenticatedView(true);
  try {
    await loadConfig();
  } catch (error) {
    if (error.message !== "Unauthorized") message("Konfiguration konnte nicht geladen werden.");
    return;
  }
  if (languageChosenBeforeLogin && languageChosenBeforeLogin !== currentLanguage) setLanguage(languageChosenBeforeLogin);
  languageChosenBeforeLogin = "";
  await loadStatus().catch(() => {});
  scheduleStatusRefresh();
  if (!authHeaderValue()) return;
  // A preview left over from a closed tab would otherwise run until it times out.
  if (lastStatus?.displayPreviewActive && capabilities.displayPreview) {
    previewActive = true;
    cancelPreview();
  }
  if (typeof maybeOpenSetupWizard === "function") maybeOpenSetupWizard();
}

async function login(event) {
  event.preventDefault();
  if ($("loginBtn").disabled) return;
  $("loginBtn").disabled = true;
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 12000);
  const username = sanitizeName($("loginUsername").value, "");
  const password = $("loginPassword").value;
  const auth = basicAuthValue(username, password);
  setLoginMessage("Anmeldung läuft...");
  try {
    const res = await fetch("/api/status", {
      signal: controller.signal,
      cache: "no-store",
      credentials: "same-origin",
      headers: { Authorization: auth }
    });
    if (!res.ok) {
      const data = res.status === 429 ? await res.json().catch(() => ({})) : {};
      if (res.status === 429 && data.retryAfterSeconds) {
        setLoginMessage("Zu viele Fehlversuche. Bitte in {seconds} Sekunden erneut versuchen.", { seconds: data.retryAfterSeconds });
      } else {
        setLoginMessage("Anmeldung fehlgeschlagen.");
      }
      return;
    }
    setAuthHeader(auth);
    setLoginMessage("Angemeldet.");
    $("loginPassword").value = "";
    await startAuthenticatedApp();
  } catch (_) {
    setLoginMessage("Anmeldung fehlgeschlagen.");
  } finally {
    clearTimeout(timeout);
    $("loginBtn").disabled = false;
  }
}

// A new clock (no Wi-Fi, default password) needs no login: the firmware says
// so on the public /api/setup and the default credentials are used. Older
// firmware answers 401 there, which keeps the normal login page.
async function tryFirstSetupLogin() {
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 5000);
  try {
    const res = await fetch("/api/setup", { cache: "no-store", signal: controller.signal });
    if (!res.ok) return false;
    const data = await res.json();
    if (!data.firstSetup) return false;
    setAuthHeader(basicAuthValue(sanitizeName(data.adminUsername, "admin"), defaultAdminPassword));
    return true;
  } catch (_) {
    return false;
  } finally {
    clearTimeout(timeout);
  }
}

// Password recovery without login: the firmware shows a code on the matrix,
// so only someone standing at the clock can set a new admin password.
function setRecoveryMessage(text, values = {}) {
  recoveryMessage = { text, values };
  $("recoveryMessage").textContent = trFormat(text, values);
}

function showRecovery(show) {
  $("loginForm").hidden = show;
  $("recoveryForm").hidden = !show;
  if (!show) return;
  for (const id of ["recoveryCode", "recoveryPassword", "recoveryPassword2"]) $(id).value = "";
  setRecoveryMessage("Tippe auf „Code auf der Uhr anzeigen“.");
  $("recoveryStartBtn").focus();
}

function recoveryInputError(code, first, second) {
  if (!/^\d{6}$/.test(code)) return "Bitte den 6-stelligen Code von der Uhr eingeben.";
  if (first.length < 8) return "Das Admin-Passwort muss mindestens 8 Zeichen lang sein.";
  if (first.length > 64) return "Das Admin-Passwort darf höchstens 64 Zeichen lang sein.";
  if (first === defaultAdminPassword) return "Bitte ein anderes Passwort als das Standardpasswort wählen.";
  if (first !== second) return "Die Passwörter stimmen nicht überein.";
  return "";
}

async function postRecovery(path, params) {
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 12000);
  try {
    const res = await fetch(path, { method: "POST", body: new URLSearchParams(params), cache: "no-store", signal: controller.signal });
    return { status: res.status, ok: res.ok, data: await res.json().catch(() => ({})) };
  } catch (_) {
    return { status: 0, ok: false, data: {} };
  } finally {
    clearTimeout(timeout);
  }
}

function showRecoveryFailure({ status, data }) {
  if (!status) setRecoveryMessage("Die Uhr ist nicht erreichbar.");
  else if (status === 401 || status === 404 || status === 405) {
    setRecoveryMessage("Diese Firmware kann das Passwort noch nicht zurücksetzen. Halte die BOOT-Taste 10 Sekunden gedrückt, um die Uhr zurückzusetzen.");
  } else if (status === 429 && data.retryAfterSeconds) {
    setRecoveryMessage("Bitte in {seconds} Sekunden erneut versuchen.", { seconds: data.retryAfterSeconds });
  } else setRecoveryMessage(data.error || "Admin-Passwort konnte nicht gespeichert werden.");
}

async function startRecovery() {
  $("recoveryStartBtn").disabled = true;
  setRecoveryMessage("Code wird angefordert...");
  const result = await postRecovery("/api/recovery/start", {});
  $("recoveryStartBtn").disabled = false;
  if (!result.ok) {
    showRecoveryFailure(result);
    return;
  }
  setRecoveryMessage("Die Uhr zeigt jetzt {minutes} Minuten lang einen Code.", { minutes: Math.max(1, Math.round((result.data.expiresInSeconds || 300) / 60)) });
  $("recoveryCode").focus();
}

async function finishRecovery(event) {
  event.preventDefault();
  if ($("recoverySaveBtn").disabled) return;
  const code = $("recoveryCode").value.trim();
  const password = $("recoveryPassword").value;
  const error = recoveryInputError(code, password, $("recoveryPassword2").value);
  if (error) {
    setRecoveryMessage(error);
    return;
  }
  $("recoverySaveBtn").disabled = true;
  setRecoveryMessage("Passwort wird gespeichert...");
  const result = await postRecovery("/api/recovery/finish", { code, adminPassword: password });
  $("recoverySaveBtn").disabled = false;
  if (!result.ok) {
    showRecoveryFailure(result);
    return;
  }
  setAuthHeader(basicAuthValue(sanitizeName(result.data.adminUsername || $("loginUsername").value, "admin"), password));
  showRecovery(false);
  setLoginMessage("Angemeldet.");
  await startAuthenticatedApp();
}

async function logout() {
  await cancelPreview();
  showLogin("Bitte anmelden.");
}

function initUi() {
  fillPins();
  fillWifiCountries();
  fillWeatherIntervals();
  fillWeatherModels();
  initPasswordReveal();
  for (const id of ["firmwareLine", "littleFsLine", "networks", "wizardNetworks", "message", "loginMessage", "recoveryMessage", "locationResult", "firmwareSelectedVersion", "webSelectedVersion"]) $(id).dataset.noI18n = "";
  showPage(pageFromHash());
  if (typeof initSetupWizard === "function") initSetupWizard();
}
initUi();
resetUpdateInputs();
applyLanguage();
updateStaticVersionLines();
if (authHeaderValue()) {
  startAuthenticatedApp();
} else {
  tryFirstSetupLogin().then((firstSetup) => (firstSetup ? startAuthenticatedApp() : showLogin()));
}

$("loginForm").addEventListener("submit", login);
$("recoveryForm").addEventListener("submit", finishRecovery);
$("recoveryOpenBtn").addEventListener("click", () => showRecovery(true));
$("recoveryBackBtn").addEventListener("click", () => showRecovery(false));
$("recoveryStartBtn").addEventListener("click", startRecovery);
$("logoutBtn").addEventListener("click", logout);
$("languageSelect").addEventListener("change", (event) => setLanguage(event.target.value));
$("loginLanguage").addEventListener("change", (event) => setLoginLanguage(event.target.value));
$("setupCardPasswordBtn").addEventListener("click", openAdminAccess);
$("setupCardHideBtn").addEventListener("click", () => {
  writeStorage(setupCardHiddenStorageKey, "1");
  updateSetupCard();
  message("Der Assistent ist unter System jederzeit erneut verfügbar.");
});
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
for (const id of ["width", "height", "origin", "wiringMode"]) {
  for (const eventName of ["input", "change"]) $(id).addEventListener(eventName, updateHardwareInfo);
}
$("cityName").addEventListener("input", renderLocationResult);
$("setupApPassword").addEventListener("input", () => {
  const length = $("setupApPassword").value.length;
  $("setupApPassword").setCustomValidity(length && (length < 8 || length > 63) ? tr("Das Setup-WLAN-Passwort muss 8 bis 63 Zeichen lang sein.") : "");
});
$("hostname").addEventListener("change", () => { $("hostname").value = sanitizeName($("hostname").value); });
$("adminUsername").addEventListener("change", () => { $("adminUsername").value = sanitizeName($("adminUsername").value, ""); });
bindAction("testBtn", runTestPattern);
bindAction("weatherBtn", refreshWeather);
bindAction("restartBtn", () => postAction("/api/restart", "Neustart läuft...", { restart: true }));
bindAction("restartRequiredBtn", () => postAction("/api/restart", "Neustart läuft...", { restart: true }));
bindAction("settingsResetBtn", resetSettings);
$("factoryResetBtn").addEventListener("click", openFactoryReset);
$("factoryResetCancel").addEventListener("click", closeFactoryReset);
$("factoryResetConfirm").addEventListener("change", () => { $("factoryResetGo").disabled = !$("factoryResetConfirm").checked; });
bindAction("factoryResetGo", factoryReset);
$("factoryResetModal").addEventListener("keydown", (event) => { if (event.key === "Escape") closeFactoryReset(); });
$("exportBtn").addEventListener("click", exportSettings);
$("importFile").addEventListener("change", async (event) => {
  try {
    await importSettings(event.target.files[0]);
  } catch (error) {
    message(error.message);
  } finally {
    event.target.value = "";
  }
});
for (const canvas of [$("liveMatrix"), $("liveMatrixDisplay")]) canvas.addEventListener("click", noteInteraction);
for (const eventName of ["pointerdown", "keydown", "touchstart"]) document.addEventListener(eventName, noteInteraction, { passive: true });
window.addEventListener("resize", () => { if (lastFrame) drawLiveMatrix(lastFrame); });
$("firmwareUpdateBtn").addEventListener("click", uploadFirmware);
$("firmwareFile").addEventListener("change", updateFirmwareSelectionInfo);
$("webUpdateBtn").addEventListener("click", uploadWebInterface);
$("webFile").addEventListener("change", updateWebSelectionInfo);
window.addEventListener("pageshow", resetUpdateInputs);
window.addEventListener("hashchange", () => showPage(pageFromHash()));

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
$("discardBtn").addEventListener("click", () => {
  if (!savedConfig) return;
  setForm({ ...savedConfig });
  message("");
  cancelPreview();
});
for (const eventName of ["input", "change"]) {
  $("appShell").addEventListener(eventName, (event) => {
    if (!event.target.matches("input:not([type=file]), select") || event.target.id === "languageSelect") return;
    updateDirtyState();
    if (event.target.closest('.page[data-page="display"]')) schedulePreview();
  });
}
window.addEventListener("beforeunload", (event) => {
  if (dirty) { event.preventDefault(); event.returnValue = ""; }
});
document.addEventListener("visibilitychange", () => {
  clearTimeout(statusRefreshTimer);
  scheduleClockTick();
  if (!document.hidden) lastInteraction = Date.now();
  scheduleLiveMatrix();
  if (!document.hidden && authHeaderValue()) loadStatus().catch(() => {}).finally(scheduleStatusRefresh);
});
