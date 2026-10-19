#include "system_diagnostics.h"

#include <Arduino.h>
#include <esp_system.h>
#include <esp_task_wdt.h>

#include "config.h"

namespace diag {

namespace {

const char* resetReasonToString(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON:   return "Power-on event";
    case ESP_RST_EXT:       return "External pin reset";
    case ESP_RST_SW:        return "Software reset via esp_restart";
    case ESP_RST_PANIC:     return "Software reset due to exception/panic";
    case ESP_RST_INT_WDT:   return "Interrupt watchdog reset";
    case ESP_RST_TASK_WDT:  return "Task watchdog reset (TWDT)";
    case ESP_RST_WDT:       return "Other watchdog reset";
    case ESP_RST_DEEPSLEEP: return "Reset after exit from deep sleep";
    case ESP_RST_BROWNOUT:  return "Brownout reset (unstable supply voltage)";
    case ESP_RST_SDIO:      return "Reset over SDIO";
    case ESP_RST_UNKNOWN:
    default:                return "Unknown / uninitialized reset";
  }
}

}  // namespace

void logResetReason() {
  esp_reset_reason_t reason = esp_reset_reason();
  Serial.println("==================================================");
  Serial.printf("[DIAG] Reset reason: %s (code %d)\n", resetReasonToString(reason), static_cast<int>(reason));
  Serial.printf("[DIAG] CPU Frequency: %u MHz | Free Heap: %u bytes\n",
                getCpuFrequencyMhz(), ESP.getFreeHeap());
  Serial.println("==================================================");
}

void initWatchdog() {
  // Initialize Task Watchdog with panic=true (forces reset on stall)
  esp_err_t err = esp_task_wdt_init(config::kWatchdogTimeoutSec, true);
  if (err != ESP_OK) {
    Serial.printf("[DIAG] Warning: esp_task_wdt_init returned %d\n", err);
  } else {
    Serial.printf("[DIAG] Task Watchdog Timer initialized (%u s timeout, panic enabled)\n",
                  config::kWatchdogTimeoutSec);
  }
}

void subscribeCurrentTask() {
  esp_err_t err = esp_task_wdt_add(NULL);
  if (err != ESP_OK) {
    Serial.printf("[DIAG] Warning: esp_task_wdt_add failed with %d\n", err);
  }
}

void feedWatchdog() {
  esp_task_wdt_reset();
}

}  // namespace diag
