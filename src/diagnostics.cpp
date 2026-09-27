#include <esp_system.h>

#include "app_state.h"

const char *resetReasonText() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON: return "Einschalten / Stromversorgung";
    case ESP_RST_EXT: return "Reset-Pin";
    case ESP_RST_SW: return "Software-Neustart";
    case ESP_RST_PANIC: return "Software-Absturz (Panic)";
    case ESP_RST_INT_WDT: return "Interrupt-Watchdog";
    case ESP_RST_TASK_WDT: return "Task-Watchdog";
    case ESP_RST_WDT: return "Watchdog";
    case ESP_RST_DEEPSLEEP: return "Aufwachen aus Tiefschlaf";
    case ESP_RST_BROWNOUT: return "Unterspannung (Brownout)";
    case ESP_RST_SDIO: return "SDIO-Reset";
    default: return "Unbekannt";
  }
}
