#include <WiFi.h>
#include <nvs_flash.h>

#include "app_state.h"

// Factory reset for handing the clock on. Holding the BOOT button works without
// a login, e.g. when the admin password is unknown.
static uint32_t resetPressedSince = 0;

void serviceResetButton() {
  const bool pressed = digitalRead(RESET_BUTTON_PIN) == LOW;
  const uint32_t now = millis();
  StateLock lock;
  if (pendingFactoryWipe) return;
  if (!pressed) {
    resetPressedSince = 0;
    if (resetCountdownSeconds) {
      resetCountdownSeconds = 0;
      lastRender = 0;
    }
    return;
  }
  if (!resetPressedSince) resetPressedSince = now ? now : 1;
  const uint32_t held = now - resetPressedSince;
  if (held < RESET_BUTTON_NOTICE_MS) return;
  if (held >= RESET_BUTTON_HOLD_MS) {
    Serial.println("Reset button held: factory reset");
    pendingFactoryWipe = true;
    resetCountdownSeconds = 0;
    lastRender = 0;
    scheduleRestart(500);
    return;
  }
  const uint8_t secondsLeft = (RESET_BUTTON_HOLD_MS - held + 999) / 1000;
  if (secondsLeft != resetCountdownSeconds) {
    resetCountdownSeconds = secondsLeft;
    lastRender = 0;
  }
}

// Erases the whole NVS partition: settings, Wi-Fi, admin login, API keys, the
// setup network password and Wi-Fi driver data. Firmware and web UI remain.
// Called right before ESP.restart(); NVS is reinitialized on the next boot.
void performFactoryWipe() {
  Serial.println("Factory reset: erasing NVS");
  fill_solid(leds, ledCount, CRGB::Black);
  FastLED.show();
  WiFi.disconnect(true, true);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  const esp_err_t err = nvs_flash_erase();
  if (err != ESP_OK) Serial.printf("NVS erase failed: %d\n", err);
}
