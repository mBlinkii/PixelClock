// Guided setup. Wizard controls mirror regular settings fields through
// data-bind="<field id>", so saving reuses formBody(), validation, secret
// handling and the normal /api/config request. Only existing endpoints are used,
// which keeps this web UI compatible with already installed firmware.
// The basic setup covers what every user needs. The LED matrix step is only
// added when the advanced setup is switched on in the summary.
const wizardBasicSteps = ["welcome", "wifi", "location", "security", "summary"];
let wizardAdvanced = false;

function wizardSteps() {
  return wizardAdvanced ? ["welcome", "wifi", "location", "security", "display", "summary"] : wizardBasicSteps;
}
const matrixPresets = { "32x8": [32, 8], "32x16": [32, 16], "64x8": [64, 8] };
const providerNames = { 0: "Open-Meteo", 1: "OpenWeatherMap", 2: "DWD (Bright Sky)", 3: "MET Norway", 4: "WeatherAPI" };
let wizardStepIndex = 0;
let wizardNetworks = [];
let wizardScanned = false;
let wizardScanInFlight = false;
let wizardProbeTimer = 0;
let wizardStart = null;
let wizardFinished = false;

function wizardStepElement(name) {
  return document.querySelector(`.wizardStep[data-step="${name}"]`);
}

function setWizardError(text) {
  $("wizardError").textContent = text;
  $("wizardError").hidden = !text;
}

function setPlaceholder(el, source) {
  el.dataset.i18nPlaceholder = source;
  el.placeholder = source ? tr(source) : "";
}

function setFormField(id, value) {
  const target = $(id);
  if (target.type === "checkbox") target.checked = Boolean(value);
  else target.value = String(value);
  target.dispatchEvent(new Event("change", { bubbles: true }));
}

function pushWizardField(el) {
  if (el.type === "radio" && !el.checked) return;
  setFormField(el.dataset.bind, el.type === "checkbox" ? el.checked : el.value);
}

function pullWizardFields(root) {
  for (const el of root.querySelectorAll("[data-bind]")) {
    const source = $(el.dataset.bind);
    if (!source) continue;
    if (el.type === "radio") {
      const option = source.tagName === "SELECT" ? Array.from(source.options).find((item) => item.value === el.value) : null;
      el.disabled = Boolean(option?.disabled);
      el.checked = el.value === source.value;
    } else if (el.type === "checkbox") {
      el.checked = source.checked;
    } else {
      if (el.id === "wizardWifiCountry" || el.id === "wizardDataPin" || el.id === "wizardWeatherModel") el.innerHTML = source.innerHTML;
      el.value = source.value;
    }
  }
}

function selectedSizeChoice() {
  const key = `${$("width").value}x${$("height").value}`;
  return matrixPresets[key] ? key : "custom";
}

function applySizeChoice(value) {
  const preset = matrixPresets[value];
  if (preset) {
    setFormField("width", preset[0]);
    setFormField("height", preset[1]);
  }
  $("wizardCustomSize").hidden = value !== "custom";
  pullWizardFields($("wizardCustomSize"));
  setWizardError("");
}

function renderWizardDiagram() {
  renderWiringDiagram($("wizardWiringDiagram"), Number($("origin").value), Number($("wiringMode").value));
}

function updateWizardPasswordPlaceholder() {
  const keepsPassword = $("ssid").value === (savedConfig?.ssid || "") && savedConfig?.hasPassword;
  setPlaceholder($("wizardWifiPassword"), keepsPassword ? "Leer lassen zum Beibehalten" : "");
}

function updateWizardProviderFields() {
  const provider = Number($("weatherProvider").value);
  $("wizardModelField").hidden = provider !== 0 || savedConfig?.weatherModel === undefined;
  $("wizardOwmKeyField").hidden = provider !== 1;
  $("wizardWaKeyField").hidden = provider !== 4;
  setPlaceholder($("wizardOwmKey"), hasProviderKeys[1] ? "Leer lassen zum Beibehalten" : "API-Key eingeben");
  setPlaceholder($("wizardWaKey"), hasProviderKeys[4] ? "Leer lassen zum Beibehalten" : "API-Key eingeben");
}

// Firmware before 0.1.18 shows its WIFI/AP prompt instead of the test pattern while no SSID is saved.
function updateWizardTestHint() {
  $("wizardTestApHint").hidden = Boolean(capabilities.setupTestPattern) ||
    !(lastStatus?.setupMode && !String(savedConfig?.ssid || "").trim() && !$("ssid").value.trim());
}

function renderWizardNetworks(collapsed = true) {
  renderNetworks($("wizardNetworks"), wizardNetworks, (network) => {
    $("wizardSsid").value = network.ssid;
    pushWizardField($("wizardSsid"));
    updateWizardPasswordPlaceholder();
    updateWizardTestHint();
    setWizardError("");
    $("wizardWifiPassword").focus();
  }, $("ssid").value, collapsed);
}

async function wizardScan() {
  if (wizardScanInFlight) return;
  wizardScanInFlight = true;
  $("wizardScanBtn").disabled = true;
  $("wizardNetworks").textContent = tr("Suche...");
  try {
    wizardNetworks = await fetchNetworks();
    wizardScanned = true;
    renderWizardNetworks(false);
  } catch (error) {
    if (error.message !== "Unauthorized") {
      $("wizardNetworks").textContent = tr(error.message === "Failed to fetch" || error.name === "AbortError" ? "Uhr nicht erreichbar" : error.message);
    }
  } finally {
    wizardScanInFlight = false;
    $("wizardScanBtn").disabled = false;
  }
}

function selectedOptionText(id) {
  const select = $(id);
  return select.options[select.selectedIndex]?.textContent.trim() || select.value;
}

function renderWizardSummary() {
  const ssid = $("ssid").value;
  const password = $("adminPassword").value
    ? ["Wird geändert", false]
    : savedConfig?.adminPasswordIsDefault ? ["Standardpasswort bleibt aktiv", true] : ["Eigenes Passwort ist gesetzt", false];
  const rows = [
    ["WLAN", ssid ? `${ssid} · ${$("wifiCountry").value}` : tr("Nicht festgelegt"), "wifi", !ssid],
    ["Browser-Adresse", `http://${sanitizeName($("hostname").value)}.local`, "wifi"],
    ["Standort", `${$("cityName").value.trim()} · ${providerNames[$("weatherProvider").value] || ""}`, "location"],
    ["Format", `${$("hourFormat").value} h · °${$("temperatureUnit").value === "1" ? "F" : "C"}`, "location"],
    ["Admin-Passwort", tr(password[0]), "security", password[1]]
  ];
  if (wizardAdvanced) {
    rows.splice(2, 0, ["Display", `${$("width").value} × ${$("height").value} · GPIO ${$("dataPin").value} · ` +
      `${$("colorOrder").value} · ${selectedOptionText("origin")} · ${selectedOptionText("wiringMode")}`, "display"]);
  }
  const list = $("wizardSummary");
  list.replaceChildren();
  for (const [label, value, step, warn] of rows) {
    const row = document.createElement("div");
    const term = document.createElement("dt");
    term.textContent = tr(label);
    const detail = document.createElement("dd");
    detail.textContent = value;
    if (warn) detail.className = "isWarn";
    const edit = document.createElement("button");
    edit.type = "button";
    edit.className = "ghost small";
    edit.textContent = tr("Ändern");
    edit.addEventListener("click", () => showWizardStep(wizardSteps().indexOf(step)));
    row.append(term, detail, edit);
    list.append(row);
  }
}

function prepareWizardStep(name) {
  pullWizardFields(wizardStepElement(name));
  if (name === "welcome") {
    for (const radio of document.querySelectorAll('input[name="wizardLanguage"]')) radio.checked = radio.value === currentLanguage;
  } else if (name === "wifi") {
    const ssid = savedConfig?.ssid || "";
    $("wizardWifiIntro").textContent = lastStatus?.wifiConnected && ssid
      ? trFormat("Die Uhr ist mit „{ssid}“ verbunden. Du kannst das WLAN beibehalten oder ein anderes auswählen.", { ssid })
      : tr("Wähle das WLAN, mit dem sich die Uhr verbinden soll. Unterstützt werden 2,4-GHz-Netze.");
    updateWizardPasswordPlaceholder();
    if (wizardScanned) renderWizardNetworks();
    else wizardScan();
  } else if (name === "display") {
    const choice = selectedSizeChoice();
    for (const radio of document.querySelectorAll('input[name="wizardSize"]')) radio.checked = radio.value === choice;
    $("wizardCustomSize").hidden = choice !== "custom";
    renderWizardDiagram();
    updateWizardTestHint();
  } else if (name === "location") {
    updateWizardProviderFields();
  } else if (name === "security") {
    const isDefault = Boolean(savedConfig?.adminPasswordIsDefault);
    $("wizardDefaultPwNote").hidden = !isDefault;
    $("wizardAdminUser").textContent = $("adminUsername").value || "admin";
  } else if (name === "summary") {
    $("wizardAdvancedToggle").checked = wizardAdvanced;
    renderWizardSummary();
  }
}

function renderWizardChrome() {
  const name = wizardSteps()[wizardStepIndex];
  $("wizardStepLabel").textContent = trFormat("Schritt {n} von {total}", { n: wizardStepIndex + 1, total: wizardSteps().length });
  $("wizardProgressBar").style.width = `${((wizardStepIndex + 1) / wizardSteps().length) * 100}%`;
  $("wizardBack").hidden = wizardStepIndex === 0;
  $("wizardNext").className = "primary";
  $("wizardNext").textContent = tr(name === "welcome" ? "Los geht's" : name === "summary" ? "Speichern und abschließen" : "Weiter");
}

function showWizardStep(index) {
  wizardFinished = false;
  wizardStepIndex = Math.max(0, Math.min(index, wizardSteps().length - 1));
  const name = wizardSteps()[wizardStepIndex];
  for (const step of document.querySelectorAll(".wizardStep")) step.hidden = step.dataset.step !== name;
  setWizardError("");
  prepareWizardStep(name);
  renderWizardChrome();
  document.querySelector(".wizardBody").scrollTop = 0;
  wizardStepElement(name).querySelector("h3")?.focus({ preventScroll: true });
}

// A new clock is opened without a login, so an own admin password is mandatory
// while the default is active; the default itself is not accepted again.
function applyWizardPassword() {
  const first = $("wizardAdminPassword").value;
  const second = $("wizardAdminPassword2").value;
  if (!first && !second) {
    if (savedConfig?.adminPasswordIsDefault) return tr("Bitte ein eigenes Admin-Passwort festlegen.");
    setFormField("adminPassword", "");
    return "";
  }
  if (first === defaultAdminPassword) return tr("Bitte ein anderes Passwort als das Standardpasswort wählen.");
  if (first.length < (savedConfig?.minAdminPasswordLength || 8)) return tr("Das Admin-Passwort muss mindestens 8 Zeichen lang sein.");
  if (first !== second) return tr("Die Passwörter stimmen nicht überein.");
  setFormField("adminPassword", first);
  return "";
}

// Programmatic values skip minlength checks in browsers, so lengths are checked explicitly.
function validateWizardStep(name) {
  if (name === "wifi") {
    const ssid = $("ssid").value;
    if (!ssid.trim()) return tr("Bitte ein WLAN auswählen oder den Namen eingeben.");
    const network = wizardNetworks.find((item) => item.ssid === ssid);
    const keepsPassword = ssid === (savedConfig?.ssid || "") && savedConfig?.hasPassword;
    if (network?.secure && !$("password").value && !keepsPassword) return tr("Bitte das WLAN-Passwort eingeben.");
  } else if (name === "display") {
    for (const id of ["width", "height"]) {
      if (!$(id).checkValidity()) return invalidFieldMessage($(id));
    }
  } else if (name === "location") {
    if ($("cityName").value.trim().length < 2) return tr("Bitte eine Stadt oder Postleitzahl eingeben.");
    const provider = Number($("weatherProvider").value);
    const keyField = provider === 1 ? $("openWeatherApiKey") : provider === 4 ? $("weatherApiKey") : null;
    if (keyField && !hasProviderKeys[provider] && !keyField.value.trim()) return tr("Für diesen Wetterdienst wird ein API-Key benötigt.");
  } else if (name === "security") {
    return applyWizardPassword();
  }
  return "";
}

async function wizardApplyAndTest() {
  const error = validateWizardStep("display");
  const invalid = error ? null : findInvalidField();
  if (error || invalid) {
    setWizardError(error || invalidFieldMessage(invalid));
    return;
  }
  const button = $("wizardTestBtn");
  button.disabled = true;
  try {
    if (dirty && !(await saveConfig())) {
      setWizardError(tr("Speichern fehlgeschlagen."));
      return;
    }
    await runTestPattern();
    updateWizardTestHint();
  } catch (failure) {
    if (failure.message !== "Unauthorized") setWizardError(tr("Uhr nicht erreichbar"));
  } finally {
    button.disabled = false;
  }
}

function showWizardDoneView(title, nextLabel) {
  wizardFinished = true;
  message("");
  for (const step of document.querySelectorAll(".wizardStep")) step.hidden = step.dataset.step !== "done";
  $("wizardDoneTitle").textContent = tr(title);
  $("wizardStepLabel").textContent = tr("Fertig");
  $("wizardProgressBar").style.width = "100%";
  $("wizardBack").hidden = true;
  $("wizardNext").className = "primary";
  $("wizardNext").textContent = tr(nextLabel);
  $("wizardProbeState").hidden = true;
  $("wizardDoneLink").hidden = true;
  setWizardError("");
}

function textElement(tag, text) {
  const el = document.createElement(tag);
  el.textContent = text;
  return el;
}

function showWizardHandoff(ssid, url, reconnectDevice) {
  showWizardDoneView("Die Uhr startet neu", "Schließen");
  const steps = document.createElement("ol");
  if (reconnectDevice) steps.append(textElement("li", trFormat("Verbinde dieses Gerät wieder mit „{ssid}“.", { ssid })));
  const open = document.createElement("li");
  const link = textElement("a", url.replace(/\/$/, ""));
  link.href = url;
  open.append(`${tr("Öffne danach")} `, link);
  steps.append(open, textElement("li", trFormat(
    "Klappt die .local-Adresse nicht, findest du die IP-Adresse im Router unter „{name}“.",
    { name: lastStatus?.routerHostname || "pixelclock-…" })));
  $("wizardDoneText").replaceChildren(
    textElement("p", trFormat("Die Uhr verbindet sich jetzt mit dem WLAN „{ssid}“.", { ssid })),
    steps,
    textElement("p", trFormat(
      "Kann sich die Uhr nicht verbinden, öffnet sie nach etwa 20 Sekunden wieder das WLAN „{ap}“. Prüfe dann das WLAN-Passwort.",
      { ap: lastStatus?.setupApSsid || "PixelClock-Setup" }))
  );
  startWizardProbe(url);
}

function stopWizardProbe() {
  clearTimeout(wizardProbeTimer);
  wizardProbeTimer = 0;
}

// favicon.svg is public, so an image load shows when the restarted clock is reachable
// under its new address, even from the setup access point's origin.
function startWizardProbe(url) {
  stopWizardProbe();
  const state = $("wizardProbeState");
  state.hidden = false;
  state.className = "probeState";
  state.textContent = tr("Warte auf die Uhr im Netzwerk...");
  const startedAt = Date.now();
  const attempt = () => {
    const image = new Image();
    let timeout = 0;
    let settled = false;
    const settle = (found) => {
      if (settled) return;
      settled = true;
      clearTimeout(timeout);
      image.onload = null;
      image.onerror = null;
      if (found) {
        state.className = "probeState isFound";
        state.textContent = tr("Uhr gefunden!");
        $("wizardDoneLink").href = url;
        $("wizardDoneLink").textContent = tr("Weboberfläche öffnen");
        $("wizardDoneLink").hidden = false;
        $("wizardNext").className = "ghost";
      } else if (Date.now() - startedAt < 180000) {
        wizardProbeTimer = setTimeout(attempt, 3000);
      } else {
        state.className = "probeState isIdle";
        state.textContent = tr("Noch nicht gefunden. Nutze die Adresse oben oder die IP-Adresse aus dem Router.");
      }
    };
    timeout = setTimeout(() => settle(false), 4000);
    image.onload = () => settle(true);
    image.onerror = () => settle(false);
    image.src = `${url}favicon.svg?probe=${Date.now()}`;
  };
  wizardProbeTimer = setTimeout(attempt, 12000);
}

async function finishSetupWizard() {
  const invalid = findInvalidField();
  if (invalid) {
    setWizardError(invalidFieldMessage(invalid));
    return;
  }
  const start = wizardStart || {};
  const ssid = $("ssid").value;
  const host = sanitizeName($("hostname").value);
  const wifiChanged = ssid !== start.ssid || start.wifiPasswordEntered;
  $("wizardNext").disabled = true;
  $("wizardBack").disabled = true;
  try {
    let result = { restartRequired: Boolean(savedConfig?.restartRequired) };
    if (dirty) {
      result = await saveConfig();
      if (!result) {
        setWizardError(tr("Speichern fehlgeschlagen."));
        return;
      }
    }
    writeStorage(setupDismissedStorageKey, "1");
    // In setup mode a restart retries the saved Wi-Fi even when nothing changed.
    if (!result.restartRequired && !(lastStatus?.setupMode && ssid.trim())) {
      showWizardDoneView("Einrichtung abgeschlossen", "Zur Übersicht");
      $("wizardDoneText").replaceChildren(textElement("p", tr("Alle Einstellungen sind gespeichert.")));
      return;
    }
    const res = await apiFetch("/api/restart", { method: "POST" });
    if (!res.ok) {
      setWizardError(tr("Aktion fehlgeschlagen."));
      return;
    }
    const addressChanged = host !== start.hostname && location.hostname.endsWith(".local");
    if (wifiChanged || start.setupMode || addressChanged) {
      clearTimeout(statusRefreshTimer);
      showWizardHandoff(ssid, `http://${host}.local/`, start.setupMode || ssid !== start.ssid);
    } else {
      closeSetupWizard(false);
      showRestartOverlay("Neustart läuft...");
    }
  } catch (error) {
    if (error.message !== "Unauthorized") setWizardError(tr("Uhr nicht erreichbar"));
  } finally {
    $("wizardNext").disabled = false;
    $("wizardBack").disabled = false;
  }
}

async function wizardNext() {
  if (wizardFinished) {
    closeSetupWizard(false);
    showPage("overview");
    return;
  }
  const name = wizardSteps()[wizardStepIndex];
  const error = validateWizardStep(name);
  if (error) {
    setWizardError(error);
    return;
  }
  if (name === "summary") await finishSetupWizard();
  else showWizardStep(wizardStepIndex + 1);
}

function openSetupWizard() {
  if (!savedConfig) return;
  wizardStart = {
    ssid: savedConfig.ssid || "",
    hostname: savedConfig.hostname || "",
    setupMode: Boolean(lastStatus?.setupMode),
    wifiPasswordEntered: false
  };
  wizardScanned = false;
  wizardAdvanced = false;
  $("wizardAdminPassword").value = "";
  $("wizardAdminPassword2").value = "";
  $("setupWizard").hidden = false;
  document.documentElement.classList.add("noScroll");
  showWizardStep(0);
}

function closeSetupWizard(remember = true) {
  stopWizardProbe();
  $("setupWizard").hidden = true;
  document.documentElement.classList.remove("noScroll");
  $("wizardAdminPassword").value = "";
  $("wizardAdminPassword2").value = "";
  if (remember) writeStorage(setupDismissedStorageKey, "1");
  if (!wizardFinished && dirty) message("Deine Eingaben aus dem Assistenten sind noch nicht gespeichert.");
  else if (remember && !wizardFinished) message("Der Assistent ist unter System jederzeit erneut verfügbar.");
  wizardFinished = false;
}

function maybeOpenSetupWizard() {
  if (!isFirstSetup() || readStorage(setupDismissedStorageKey) === "1") return false;
  openSetupWizard();
  return true;
}

function refreshSetupWizardText() {
  if ($("setupWizard").hidden || wizardFinished) return;
  prepareWizardStep(wizardSteps()[wizardStepIndex]);
  renderWizardChrome();
}

function onWizardInput(el) {
  pushWizardField(el);
  setWizardError("");
  if (el.id === "wizardSsid") {
    updateWizardPasswordPlaceholder();
    updateWizardTestHint();
    if (wizardScanned) renderWizardNetworks(false);
  } else if (el.id === "wizardWifiPassword" && el.value && wizardStart) {
    wizardStart.wifiPasswordEntered = true;
  } else if (el.id === "wizardOrigin" || el.id === "wizardWiring") {
    renderWizardDiagram();
  } else if (el.name === "wizardProvider") {
    updateWizardProviderFields();
  }
}

function initSetupWizard() {
  for (const heading of document.querySelectorAll(".wizardStep h3")) heading.tabIndex = -1;
  for (const el of $("setupWizard").querySelectorAll("[data-bind]")) {
    el.addEventListener(el.tagName === "SELECT" || el.type === "radio" ? "change" : "input", () => onWizardInput(el));
  }
  for (const radio of document.querySelectorAll('input[name="wizardLanguage"]')) {
    radio.addEventListener("change", () => { if (radio.checked) setLanguage(radio.value); });
  }
  for (const radio of document.querySelectorAll('input[name="wizardSize"]')) {
    radio.addEventListener("change", () => { if (radio.checked) applySizeChoice(radio.value); });
  }
  $("wizardScanBtn").addEventListener("click", wizardScan);
  // Switching the advanced setup on continues with the LED matrix step.
  $("wizardAdvancedToggle").addEventListener("change", () => {
    wizardAdvanced = $("wizardAdvancedToggle").checked;
    if (wizardAdvanced) showWizardStep(wizardSteps().indexOf("display"));
    else showWizardStep(wizardSteps().indexOf("summary"));
  });
  $("wizardTestBtn").addEventListener("click", wizardApplyAndTest);
  $("wizardBack").addEventListener("click", () => showWizardStep(wizardStepIndex - 1));
  $("wizardNext").addEventListener("click", wizardNext);
  $("wizardClose").addEventListener("click", () => closeSetupWizard(true));
  $("setupCardBtn").addEventListener("click", openSetupWizard);
  $("wizardStartBtn").addEventListener("click", openSetupWizard);
  $("setupWizard").addEventListener("keydown", (event) => {
    if (event.key === "Escape") {
      closeSetupWizard(!wizardFinished);
    } else if (event.key === "Enter" && event.target.matches("input:not([type=radio]):not([type=checkbox])")) {
      event.preventDefault();
      wizardNext();
    }
  });
}
