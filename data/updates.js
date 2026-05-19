const littleFsVersionMarkerPrefix = "PIXEL_CLOCK_LITTLEFS_VERSION=";
const littleFsVersionMarker = "PIXEL_CLOCK_LITTLEFS_VERSION=0.1.9";
const littleFsVersion = littleFsVersionMarker.slice(littleFsVersionMarkerPrefix.length);
let currentFirmwareVersion = "";
let selectedFirmwareVersion = "";

function formatVersion(version) {
  return version ? `v${version}` : "";
}

function versionParts(version) {
  return String(version || "")
    .trim()
    .replace(/^v/i, "")
    .split(/[._+-]/)
    .filter(Boolean);
}

function compareVersions(left, right) {
  const leftParts = versionParts(left);
  const rightParts = versionParts(right);
  if (!leftParts.length || !rightParts.length) return null;

  const maxLength = Math.max(leftParts.length, rightParts.length);
  for (let i = 0; i < maxLength; i++) {
    const leftPart = leftParts[i] ?? "0";
    const rightPart = rightParts[i] ?? "0";
    const leftIsNumber = /^\d+$/.test(leftPart);
    const rightIsNumber = /^\d+$/.test(rightPart);

    if (leftIsNumber && rightIsNumber) {
      const leftNumber = Number.parseInt(leftPart, 10);
      const rightNumber = Number.parseInt(rightPart, 10);
      if (leftNumber < rightNumber) return -1;
      if (leftNumber > rightNumber) return 1;
      continue;
    }

    const result = leftPart.localeCompare(rightPart, "en", { sensitivity: "base", numeric: true });
    if (result < 0) return -1;
    if (result > 0) return 1;
  }
  return 0;
}

function isOlderFirmwareVersion(version) {
  if (!version || !currentFirmwareVersion) return false;
  return compareVersions(version, currentFirmwareVersion) === -1;
}

function firmwareDowngradeWarning(version) {
  if (!isOlderFirmwareVersion(version)) return "";
  return `${tr("Warnung: Die ausgewählte Firmware-Version ist älter als die aktuell installierte Version.")} ${tr("Installierte Firmware-Version:")} ${formatVersion(currentFirmwareVersion)}`;
}

function renderFirmwareSelectionVersion(version) {
  const line = $("firmwareSelectedVersion");
  if (!line || !version) return;
  line.classList.remove("isMuted");
  const warning = firmwareDowngradeWarning(version);
  if (warning) {
    line.classList.add("isWarning");
    line.textContent = `${tr("Ausgewählte Firmware-Version:")} ${formatFirmwareVersion(version)}. ${warning}`;
  } else {
    line.classList.remove("isWarning");
    line.textContent = `${tr("Ausgewählte Firmware-Version:")} ${formatFirmwareVersion(version)}`;
  }
}

function updateStaticVersionLines() {
  $("littleFsLine").textContent = formatVersion(littleFsVersion);
}

function setUploadEnabled(fileId, buttonId, enabled) {
  $(fileId).disabled = !enabled;
  $(buttonId).disabled = !enabled;
}

const firmwareVersionMarker = "PIXEL_CLOCK_FIRMWARE_VERSION=";
const espAppDescMagic = [0x32, 0x54, 0xcd, 0xab];

function textBytes(text) {
  return Array.from(text, (char) => char.charCodeAt(0));
}

function indexOfBytes(bytes, needle, start = 0) {
  for (let i = start; i <= bytes.length - needle.length; i++) {
    let found = true;
    for (let j = 0; j < needle.length; j++) {
      if (bytes[i + j] !== needle[j]) {
        found = false;
        break;
      }
    }
    if (found) return i;
  }
  return -1;
}

function readAsciiString(bytes, start, maxLength) {
  const chars = [];
  for (let i = start; i < Math.min(bytes.length, start + maxLength); i++) {
    const byte = bytes[i];
    if (byte === 0) break;
    if (byte < 32 || byte > 126) break;
    chars.push(String.fromCharCode(byte));
  }
  return chars.join("").trim();
}

function readVersionString(bytes, start, maxLength) {
  const chars = [];
  for (let i = start; i < Math.min(bytes.length, start + maxLength); i++) {
    const char = String.fromCharCode(bytes[i]);
    if (!/[0-9A-Za-z._+-]/.test(char)) break;
    chars.push(char);
  }
  return chars.join("").trim();
}

function isLikelyFirmwareVersion(version) {
  return /^[0-9A-Za-z][0-9A-Za-z._+-]*$/.test(version) && /\d/.test(version);
}

function findMarkedVersion(bytes, markerText) {
  const marker = textBytes(markerText);
  let markerIndex = indexOfBytes(bytes, marker);
  while (markerIndex >= 0) {
    const version = readVersionString(bytes, markerIndex + marker.length, 48);
    if (isLikelyFirmwareVersion(version)) return version;
    markerIndex = indexOfBytes(bytes, marker, markerIndex + marker.length);
  }
  return "";
}

function findMarkedFirmwareVersion(bytes) {
  return findMarkedVersion(bytes, firmwareVersionMarker);
}

function findEspAppDescFirmwareVersion(bytes) {
  const limit = Math.min(bytes.length - 48, 8192);
  for (let i = 0; i <= limit; i++) {
    if (
      bytes[i] === espAppDescMagic[0] &&
      bytes[i + 1] === espAppDescMagic[1] &&
      bytes[i + 2] === espAppDescMagic[2] &&
      bytes[i + 3] === espAppDescMagic[3]
    ) {
      const version = readAsciiString(bytes, i + 16, 32);
      if (isLikelyFirmwareVersion(version)) return version;
    }
  }
  return "";
}

async function readFirmwareVersion(file) {
  const bytes = new Uint8Array(await file.arrayBuffer());
  return findMarkedFirmwareVersion(bytes) || findEspAppDescFirmwareVersion(bytes);
}

async function readLittleFsVersion(file) {
  const bytes = new Uint8Array(await file.arrayBuffer());
  return findMarkedVersion(bytes, littleFsVersionMarkerPrefix);
}

function formatFirmwareVersion(version) {
  return formatVersion(version);
}

let firmwareVersionReadRequest = 0;
let webVersionReadRequest = 0;

async function updateFirmwareSelectionInfo() {
  const line = $("firmwareSelectedVersion");
  const file = $("firmwareFile").files[0];
  if (!line || !file || !file.name.toLowerCase().endsWith(".bin")) {
    selectedFirmwareVersion = "";
    if (line) {
      line.hidden = true;
      line.textContent = "";
    }
    return "";
  }

  const requestId = ++firmwareVersionReadRequest;
  line.hidden = false;
  line.classList.remove("isWarning");
  line.classList.add("isMuted");
  line.textContent = tr("Firmware-Version wird gelesen...");

  let version = "";
  try {
    version = await readFirmwareVersion(file);
  } catch (_) {
    version = "";
  }
  if (requestId !== firmwareVersionReadRequest) return "";
  selectedFirmwareVersion = version;

  if (version) {
    renderFirmwareSelectionVersion(version);
  } else {
    line.classList.remove("isWarning");
    line.classList.add("isMuted");
    line.textContent = tr("Firmware-Version konnte nicht aus der Datei gelesen werden.");
  }
  return version;
}

async function updateWebSelectionInfo() {
  const line = $("webSelectedVersion");
  const file = $("webFile").files[0];
  if (!line || !file || !file.name.toLowerCase().endsWith(".bin")) {
    if (line) {
      line.hidden = true;
      line.textContent = "";
    }
    return "";
  }

  const requestId = ++webVersionReadRequest;
  line.hidden = false;
  line.classList.add("isMuted");
  line.textContent = tr("Weboberflächen-Version wird gelesen...");

  let version = "";
  try {
    version = await readLittleFsVersion(file);
  } catch (_) {
    version = "";
  }
  if (requestId !== webVersionReadRequest) return "";

  if (version) {
    line.classList.remove("isMuted");
    line.textContent = `${tr("Ausgewählte Weboberflächen-Version:")} ${formatVersion(version)}`;
  } else {
    line.classList.add("isMuted");
    line.textContent = tr("Weboberflächen-Version konnte nicht aus der Datei gelesen werden.");
  }
  return version;
}

function resetUpdateInputs() {
  for (const id of ["firmwareFile", "webFile"]) {
    const input = $(id);
    if (input) input.value = "";
  }
  for (const id of ["firmwareProgress", "webProgress"]) {
    const progress = $(id);
    if (!progress) continue;
    progress.value = 0;
    progress.hidden = true;
  }
  firmwareVersionReadRequest++;
  webVersionReadRequest++;
  selectedFirmwareVersion = "";
  const firmwareVersionLine = $("firmwareSelectedVersion");
  if (firmwareVersionLine) {
    firmwareVersionLine.hidden = true;
    firmwareVersionLine.textContent = "";
    firmwareVersionLine.classList.remove("isWarning");
  }
  const webVersionLine = $("webSelectedVersion");
  if (webVersionLine) {
    webVersionLine.hidden = true;
    webVersionLine.textContent = "";
  }
  setUploadEnabled("firmwareFile", "firmwareUpdateBtn", true);
  setUploadEnabled("webFile", "webUpdateBtn", true);
}

async function uploadBinaryUpdate(options) {
  const file = $(options.fileId).files[0];
  if (!file) {
    message(options.noFileText);
    return;
  }
  if (!file.name.toLowerCase().endsWith(".bin")) {
    message(options.invalidFileText);
    return;
  }
  const uploadInfo = options.readUploadInfo ? await options.readUploadInfo(file) : {};
  const confirmText = options.confirmMessage ? options.confirmMessage(uploadInfo) : tr(options.confirmText);
  if (!confirm(confirmText)) return;

  const form = new FormData();
  form.append("file", file, file.name);
  const progress = $(options.progressId);
  progress.hidden = false;
  progress.value = 0;
  setUploadEnabled(options.fileId, options.buttonId, false);
  if (options.uploadingMessage) messageText(options.uploadingMessage(uploadInfo));
  else message(options.uploadingText);

  await new Promise((resolve) => {
    const xhr = new XMLHttpRequest();
    xhr.open("POST", options.url);
    xhr.withCredentials = true;
    xhr.upload.addEventListener("progress", (event) => {
      if (event.lengthComputable) progress.value = Math.round(event.loaded * 100 / event.total);
    });
    xhr.addEventListener("load", () => {
      let data = {};
      try {
        data = JSON.parse(xhr.responseText || "{}");
      } catch (_) {
        data = {};
      }
      if (xhr.status >= 200 && xhr.status < 300 && data.ok) {
        progress.value = 100;
        if (options.doneMessage) messageText(options.doneMessage(uploadInfo));
        else message(options.doneText);
        setTimeout(() => location.reload(), 9000);
      } else {
        message(data.error || (xhr.status === 401 ? "Admin-Anmeldung erforderlich." : options.failedText));
        setUploadEnabled(options.fileId, options.buttonId, true);
      }
      resolve();
    });
    xhr.addEventListener("error", () => {
      message(options.failedText);
      setUploadEnabled(options.fileId, options.buttonId, true);
      resolve();
    });
    xhr.send(form);
  });
}

async function uploadFirmware() {
  await uploadBinaryUpdate({
    fileId: "firmwareFile",
    buttonId: "firmwareUpdateBtn",
    progressId: "firmwareProgress",
    url: "/api/update/firmware",
    noFileText: "Bitte zuerst eine firmware.bin auswählen.",
    invalidFileText: "Nur .bin-Dateien können als Firmware hochgeladen werden.",
    confirmText: "Firmware-Update starten?",
    uploadingText: "Firmware wird hochgeladen...",
    doneText: "Firmware installiert. Neustart läuft...",
    failedText: "Firmware-Update fehlgeschlagen.",
    readUploadInfo: async () => {
      const firmwareVersion = await updateFirmwareSelectionInfo();
      return {
        firmwareVersion,
        firmwareDowngradeWarning: firmwareDowngradeWarning(firmwareVersion)
      };
    },
    confirmMessage: (info) => {
      const version = formatFirmwareVersion(info.firmwareVersion);
      const parts = [tr("Firmware-Update starten?")];
      if (version) parts.push(`${tr("Ausgewählte Firmware-Version:")} ${version}`);
      if (info.firmwareDowngradeWarning) parts.push(info.firmwareDowngradeWarning);
      return parts.join("\n\n");
    },
    uploadingMessage: (info) => {
      const version = formatFirmwareVersion(info.firmwareVersion);
      return version ? `${tr("Firmware wird hochgeladen:")} ${version}` : tr("Firmware wird hochgeladen...");
    },
    doneMessage: (info) => {
      const version = formatFirmwareVersion(info.firmwareVersion);
      return version
        ? `${tr("Firmware installiert:")} ${version}. ${tr("Neustart läuft...")}`
        : tr("Firmware installiert. Neustart läuft...");
    }
  });
}

async function uploadWebInterface() {
  await uploadBinaryUpdate({
    fileId: "webFile",
    buttonId: "webUpdateBtn",
    progressId: "webProgress",
    url: "/api/update/web",
    noFileText: "Bitte zuerst eine Weboberflächen-Datei auswählen.",
    invalidFileText: "Nur .bin-Dateien können als Weboberfläche hochgeladen werden.",
    confirmText: "Weboberflächen-Update starten?",
    uploadingText: "Weboberfläche wird hochgeladen...",
    doneText: "Weboberfläche installiert. Neustart läuft...",
    failedText: "Weboberflächen-Update fehlgeschlagen.",
    readUploadInfo: async () => ({ littleFsVersion: await updateWebSelectionInfo() }),
    confirmMessage: (info) => {
      const version = formatVersion(info.littleFsVersion);
      return version
        ? `${tr("Weboberflächen-Update starten?")}\n\n${tr("Ausgewählte Weboberflächen-Version:")} ${version}`
        : tr("Weboberflächen-Update starten?");
    },
    uploadingMessage: (info) => {
      const version = formatVersion(info.littleFsVersion);
      return version ? `${tr("Weboberfläche wird hochgeladen:")} ${version}` : tr("Weboberfläche wird hochgeladen...");
    },
    doneMessage: (info) => {
      const version = formatVersion(info.littleFsVersion);
      return version
        ? `${tr("Weboberfläche installiert:")} ${version}. ${tr("Neustart läuft...")}`
        : tr("Weboberfläche installiert. Neustart läuft...");
    }
  });
}
