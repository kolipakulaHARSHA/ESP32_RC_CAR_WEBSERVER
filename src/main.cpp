// ESP32 RC Car - firmware entry point.
//
// Module layout:
//   motor_driver   - H-bridge pin control (hardware layer)
//   drive_control  - command parsing + mapping commands to motors
//   wifi_station   - Wi-Fi connection
//   web_server     - HTTP (web UI from LittleFS) + WebSocket command channel

#include <Arduino.h>

#include "config.h"
#include "motor_driver.h"
#include "web_server.h"
#include "wifi_station.h"

void setup() {
  Serial.begin(config::kSerialBaud);

  // Drive the H-bridge inputs low first so the motors can't twitch while
  // Wi-Fi is connecting.
  motor::init();

  net::connectWifi();
  web::begin();
}

void loop() {
  web::loop();
  delay(config::kLoopIntervalMs);
}
