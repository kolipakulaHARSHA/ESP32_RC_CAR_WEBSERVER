// System diagnostics: boot reset-reason decoding and Task Watchdog Timer (TWDT).
#pragma once

#include <esp_err.h>

namespace diag {

// Logs human-readable reset reason on boot (power-on, software restart, TWDT, brownout, etc.)
void logResetReason();

// Configures and initializes the FreeRTOS Task Watchdog Timer.
// Panic flag triggers core dump / reboot if any monitored task stalls.
void initWatchdog();

// Registers the calling FreeRTOS task to the Task Watchdog.
void subscribeCurrentTask();

// Feeds the Task Watchdog for the calling task.
void feedWatchdog();

}  // namespace diag
