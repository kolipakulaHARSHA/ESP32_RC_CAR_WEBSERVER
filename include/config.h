// Compile-time configuration: pin map, network settings and timing constants.
// Keep all hardware-specific numbers here so the modules stay free of magic numbers.
#pragma once

#include <stdint.h>

namespace config {

// ---- Serial -----------------------------------------------------------------
constexpr uint32_t kSerialBaud = 115200;

// ---- Network ----------------------------------------------------------------
constexpr uint16_t kHttpPort        = 80;
constexpr char     kWebSocketPath[] = "/ws";
constexpr uint32_t kWifiRetryDelayMs = 1000;

// ---- H-bridge (L298N / L293D) -----------------------------------------------
// Left motor on channel A, right motor on channel B.
// If the car turns the wrong way, swap the left/right pin pairs below.
constexpr uint8_t kLeftMotorIn1  = 33;
constexpr uint8_t kLeftMotorIn2  = 25;
constexpr uint8_t kRightMotorIn1 = 26;
constexpr uint8_t kRightMotorIn2 = 27;

// ---- Main loop --------------------------------------------------------------
// Housekeeping period; also yields so the idle task on this core can run.
constexpr uint32_t kLoopIntervalMs = 10;

}  // namespace config
