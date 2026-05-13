#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

void handleFirmwareUpdateUpload(
  AsyncWebServerRequest *request,
  String filename,
  size_t index,
  uint8_t *data,
  size_t len,
  bool final);
void handleFirmwareUpdateDone(AsyncWebServerRequest *request);
void handleWebUpdateUpload(
  AsyncWebServerRequest *request,
  String filename,
  size_t index,
  uint8_t *data,
  size_t len,
  bool final);
void handleWebUpdateDone(AsyncWebServerRequest *request);
