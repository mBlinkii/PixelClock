#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Update.h>

#include "app_state.h"
#include "web_updates.h"

String firmwareUpdateError;
bool firmwareUpdateStarted = false;
bool firmwareUpdateComplete = false;
String webUpdateError;
bool webUpdateStarted = false;
bool webUpdateComplete = false;

void handleFirmwareUpdateUpload(
  AsyncWebServerRequest *request,
  String filename,
  size_t index,
  uint8_t *data,
  size_t len,
  bool final) {
  if (!request->authenticate(config.adminUsername.c_str(), config.adminPassword.c_str())) {
    return;
  }

  if (index == 0) {
    firmwareUpdateStarted = true;
    firmwareUpdateComplete = false;
    firmwareUpdateError = "";
    String lowerFilename = filename;
    lowerFilename.toLowerCase();
    if (filename.length() == 0 || !lowerFilename.endsWith(".bin")) {
      firmwareUpdateError = "Firmware-Datei muss eine .bin-Datei sein.";
      return;
    }
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      firmwareUpdateError = Update.errorString();
      return;
    }
  }

  if (firmwareUpdateError.length() == 0 && len > 0) {
    const size_t written = Update.write(data, len);
    if (written != len) {
      firmwareUpdateError = Update.errorString();
    }
  }

  if (final && firmwareUpdateError.length() == 0) {
    if (!Update.end(true)) {
      firmwareUpdateError = Update.errorString();
    } else {
      firmwareUpdateComplete = true;
    }
  }
}

void handleFirmwareUpdateDone(AsyncWebServerRequest *request) {
  if (!requireAdminAuth(request)) return;

  JsonDocument doc;
  if (!firmwareUpdateStarted) {
    firmwareUpdateError = "Keine Firmware-Datei empfangen.";
  } else if (!firmwareUpdateComplete && firmwareUpdateError.length() == 0) {
    firmwareUpdateError = "Firmware-Upload wurde nicht abgeschlossen.";
  }

  if (Update.hasError() || firmwareUpdateError.length() > 0) {
    Update.abort();
    doc["ok"] = false;
    doc["error"] = firmwareUpdateError.length() > 0 ? firmwareUpdateError : Update.errorString();
    String body;
    serializeJson(doc, body);
    request->send(400, "application/json", body);
    return;
  }

  doc["ok"] = true;
  doc["restart"] = true;
  String body;
  serializeJson(doc, body);
  request->send(200, "application/json", body);
  scheduleRestart(1200);
}

void handleWebUpdateUpload(
  AsyncWebServerRequest *request,
  String filename,
  size_t index,
  uint8_t *data,
  size_t len,
  bool final) {
  if (!request->authenticate(config.adminUsername.c_str(), config.adminPassword.c_str())) {
    return;
  }

  if (index == 0) {
    webUpdateStarted = true;
    webUpdateComplete = false;
    webUpdateError = "";
    String lowerFilename = filename;
    lowerFilename.toLowerCase();
    if (filename.length() == 0 || !lowerFilename.endsWith(".bin")) {
      webUpdateError = "Weboberflächen-Datei muss eine .bin-Datei sein.";
      return;
    }

    LittleFS.end();
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)) {
      webUpdateError = Update.errorString();
      return;
    }
  }

  if (webUpdateError.length() == 0 && len > 0) {
    const size_t written = Update.write(data, len);
    if (written != len) {
      webUpdateError = Update.errorString();
    }
  }

  if (final && webUpdateError.length() == 0) {
    if (!Update.end(true)) {
      webUpdateError = Update.errorString();
    } else {
      webUpdateComplete = true;
    }
  }
}

void handleWebUpdateDone(AsyncWebServerRequest *request) {
  if (!requireAdminAuth(request)) return;

  JsonDocument doc;
  if (!webUpdateStarted) {
    webUpdateError = "Keine Weboberflächen-Datei empfangen.";
  } else if (!webUpdateComplete && webUpdateError.length() == 0) {
    webUpdateError = "Weboberflächen-Upload wurde nicht abgeschlossen.";
  }

  if (Update.hasError() || webUpdateError.length() > 0) {
    Update.abort();
    doc["ok"] = false;
    doc["error"] = webUpdateError.length() > 0 ? webUpdateError : Update.errorString();
    String body;
    serializeJson(doc, body);
    request->send(400, "application/json", body);
    return;
  }

  doc["ok"] = true;
  doc["restart"] = true;
  String body;
  serializeJson(doc, body);
  request->send(200, "application/json", body);
  scheduleRestart(1200);
}
