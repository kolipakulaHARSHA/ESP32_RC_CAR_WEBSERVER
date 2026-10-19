// ESP32 RC Car - Firmware Entry Point (Tier 1 Architecture).
//
// Layered Architecture:
//   system_diagnostics - Boot diagnostics, reset-reason decoding, FreeRTOS Task Watchdog (TWDT)
//   motor_driver       - ESP32 LEDC hardware PWM timer driver for dual H-bridge
//   drive_control      - FreeRTOS motor control task (Core 1), queue, state machine,
//                        acceleration ramp, link-loss heartbeat fail-safe
//   wifi_station       - Wi-Fi station connection (supports hardware secrets and Wokwi virtual Wi-Fi)
//   web_server         - Async HTTP + WebSocket command & telemetry channel (Core 0)

#include <Arduino.h>

#include "config.h"
#include "drive_control.h"
#include "system_diagnostics.h"
#include "web_server.h"
#include "wifi_station.h"

void setup() {
  Serial.begin(config::kSerialBaud);
  delay(100);

  // 1. Diagnostics & Reliability: Log reset reason (power-on, watchdog, panic, brownout)
  diag::logResetReason();

  // 2. Reliability: Initialize FreeRTOS Task Watchdog Timer (TWDT)
  diag::initWatchdog();

  // 3. Concurrency & Control: Initialize hardware PWM, FreeRTOS queue, and driveTask (Core 1)
  drive::init();

  // 4. Networking & Telemetry: Connect to Wi-Fi and launch asynchronous web server
  net::connectWifi();
  web::begin();

  Serial.println("[SYSTEM] Tier 1 firmware initialization complete. Ready for commands.");
}

void loop() {
  web::loop();
  delay(10);
}
