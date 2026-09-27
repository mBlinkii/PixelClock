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
  "ssid", "wifiCountry", "wifiPowerSave", "hostname", "cityName", "timezone", "weatherProvider", "weatherIntervalHalfHours", "width", "height", "dataPin",
  "brightness", "fullBrightnessUnlocked", "wiringMode", "origin", "displayMode", "colorOrder",
  "temperatureUnit", "weatherIconEnabled", "hourFormat", "colorWeekday", "colorText", "colorPoint", "colorColon", "timePageSeconds", "pageSeconds",
  "colorGradientMode", "autoPage", "selectedPage", "nightBrightness", "nightStart", "nightEnd"
];
const pageNames = ["overview", "display", "weather", "hardware", "network", "system"];

const $ = (id) => document.getElementById(id);
const adminReminderStorageKey = "pixelClockAdminReminderDismissed";
const setupDismissedStorageKey = "pixelClockSetupDismissed";
const authStorageKey = "pixelClockAuth";
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
let loginMessageSource = "";

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

function setLoginMessage(text) {
  loginMessageSource = text;
  $("loginMessage").textContent = tr(text);
}

function showLogin(text = "Bitte anmelden.") {
  setAuthHeader("");
  setAuthenticatedView(false);
  $("setupWizard").hidden = true;
  document.documentElement?.classList.remove("noScroll");
  setLoginMessage(text);
  $("loginPassword").value = "";
  $("loginUsername").focus();
  clearTimeout(clockTimer);
  clockTimer = 0;
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
  translateTextNodes(document.body);
  translateAttributes();
  if (loginMessageSource) $("loginMessage").textContent = tr(loginMessageSource);
  updateAdminPasswordPlaceholder();
  updateProviderFields();
  updateHardwareInfo();
  updateOverviewFromConfig();
  updateSetupCard();
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

function showAdminReminder(config) {
  if (!config?.adminPasswordIsDefault || readStorage(adminReminderStorageKey) === "1") return;
  $("adminReminder").hidden = false;
  $("adminReminderGo").focus();
}

function closeAdminReminder(rememberDismissal) {
  $("adminReminder").hidden = true;
  if (rememberDismissal) writeStorage(adminReminderStorageKey, "1");
}

function openAdminAccess() {
  closeAdminReminder(false);
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

function updateSetupCard() {
  const items = setupChecklist();
  $("setupCard").hidden = !items.some((item) => !item.done);
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
  updateSetupCard();
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
  for (const id of ["password", "adminPassword", "openWeatherApiKey", "weatherApiKey"]) $(id).value = "";
  hideRevealedPasswords();
  updateProviderFields();
  updatePageControls();
  updateHardwareInfo();
  updateOverviewFromConfig();
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
  if (status.weatherProvider) meta.push(status.weatherProvider);
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
  renderConnection(status);
  renderWeather(status);
  setDeviceClock(status);
  currentFirmwareVersion = status.firmwareVersion || currentFirmwareVersion;
  $("firmwareLine").textContent = formatVersion(status.firmwareVersion) || "-";
  updateStaticVersionLines();
  renderFirmwareSelectionVersion(selectedFirmwareVersion);
  if (status.locationLabel || status.cityName) $("locationLine").textContent = status.locationLabel || status.cityName;
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
    for (const id of ["password", "adminPassword", "openWeatherApiKey", "weatherApiKey"]) $(id).value = "";
    hideRevealedPasswords();
    const numeric = new Set(["latitude", "longitude"]);
    for (const field of fields) {
      const el = $(field);
      savedConfig[field] = el.type === "checkbox" ? el.checked : numeric.has(field) ? Number(el.value) : el.value;
    }
    if (data.hostname) {
      $("hostname").value = data.hostname;
      savedConfig.hostname = data.hostname;
    }
    if (data.url) savedConfig.url = data.url;
    if (data.locationLabel) savedConfig.locationLabel = data.locationLabel;
    if (body.has("password")) savedConfig.hasPassword = true;
    savedForm = formBody().toString();
    savedConfig.adminUsername = $("adminUsername").value;
    savedConfig.adminPasswordIsDefault = body.has("adminPassword") ? false : savedConfig.adminPasswordIsDefault;
    savedConfig.hasOpenWeatherApiKey = hasProviderKeys[1];
    savedConfig.hasWeatherApiKey = hasProviderKeys[4];
    savedConfig.restartRequired = Boolean(data.restartRequired);
    updateProviderFields();
    updateOverviewFromConfig();
    renderDeviceClock();
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
function renderNetworks(container, networks, onPick, selectedSsid) {
  container.replaceChildren();
  if (!networks.length) {
    container.textContent = tr("Keine Netzwerke gefunden.");
    return;
  }
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
      onPick(network);
    });
    container.append(button);
  }
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

async function factoryReset() {
  if (!confirm(tr("Werksreset ausführen? Dabei werden auch WLAN-Daten gelöscht."))) return;
  if (!confirm(tr("Wirklich alles löschen? Der ESP startet danach im Setup-Modus."))) return;
  writeStorage(setupDismissedStorageKey, null);
  writeStorage(adminReminderStorageKey, null);
  await postAction("/api/reset/factory", "Werksreset läuft...", { restart: true });
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
  const wizardOpened = typeof maybeOpenSetupWizard === "function" && maybeOpenSetupWizard();
  if (!wizardOpened) showAdminReminder(savedConfig);
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
      setLoginMessage("Anmeldung fehlgeschlagen.");
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

function logout() {
  showLogin("Bitte anmelden.");
}

function initUi() {
  fillPins();
  fillWifiCountries();
  fillWeatherIntervals();
  initPasswordReveal();
  for (const id of ["firmwareLine", "littleFsLine", "networks", "wizardNetworks", "message", "loginMessage", "firmwareSelectedVersion", "webSelectedVersion"]) $(id).dataset.noI18n = "";
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
  showLogin();
}

$("loginForm").addEventListener("submit", login);
$("logoutBtn").addEventListener("click", logout);
$("languageSelect").addEventListener("change", (event) => setLanguage(event.target.value));
$("loginLanguage").addEventListener("change", (event) => setLoginLanguage(event.target.value));
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
for (const id of ["width", "height", "origin", "wiringMode"]) {
  for (const eventName of ["input", "change"]) $(id).addEventListener(eventName, updateHardwareInfo);
}
$("hostname").addEventListener("change", () => { $("hostname").value = sanitizeName($("hostname").value); });
$("adminUsername").addEventListener("change", () => { $("adminUsername").value = sanitizeName($("adminUsername").value, ""); });
bindAction("testBtn", runTestPattern);
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
  scheduleClockTick();
  if (!document.hidden && authHeaderValue()) loadStatus().catch(() => {}).finally(scheduleStatusRefresh);
});
