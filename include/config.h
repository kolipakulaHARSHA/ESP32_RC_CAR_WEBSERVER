// Compile-time configuration: pin map, network settings, LEDC PWM, and safety timing.
// Keep all hardware-specific numbers here so the modules stay free of magic numbers.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace config {

// ---- Serial -----------------------------------------------------------------
constexpr uint32_t kSerialBaud = 115200;

// ---- Network ----------------------------------------------------------------
constexpr uint16_t kHttpPort         = 80;
constexpr char     kWebSocketPath[]  = "/ws";
constexpr uint32_t kWifiRetryDelayMs = 1000;

// ---- H-bridge (L298N / L293D) Pin Map ---------------------------------------
// Left motor on channel A, right motor on channel B.
constexpr uint8_t kLeftMotorIn1  = 33;
constexpr uint8_t kLeftMotorIn2  = 25;
constexpr uint8_t kRightMotorIn1 = 26;
constexpr uint8_t kRightMotorIn2 = 27;

// ---- LEDC PWM Speed Control -------------------------------------------------
// 5 kHz frequency is inaudible/quiet and well within L298N switching capabilities.
constexpr uint32_t kPwmFrequency       = 5000;
constexpr uint8_t  kPwmResolutionBits  = 8;     // 8-bit resolution (0..255)
constexpr int16_t  kMaxPwmValue        = 255;
constexpr int16_t  kDefaultSpeed       = 200;   // ~78% duty cycle default cruise speed

// LEDC hardware timer channels (0..15 available on ESP32)
constexpr uint8_t kLeftPwmChan1  = 0;  // GPIO 33
constexpr uint8_t kLeftPwmChan2  = 1;  // GPIO 25
constexpr uint8_t kRightPwmChan1 = 2;  // GPIO 26
constexpr uint8_t kRightPwmChan2 = 3;  // GPIO 27

// ---- Motion & Acceleration Ramp ---------------------------------------------
// Slew-rate limit per control loop tick.
// At 50 Hz (20ms/tick), 25 PWM units/tick achieves 0 -> 255 full speed in ~200ms.
constexpr int16_t kMaxSlewStepPerTick = 25;

// ---- FreeRTOS & Safety Timing -----------------------------------------------
// Control loop runs on Core 1 at 50 Hz (20ms period).
constexpr uint32_t kControlLoopPeriodMs = 20;

// Heartbeat fail-safe: if no packet received within 350ms, auto-stop motors.
constexpr uint32_t kHeartbeatTimeoutMs  = 350;
constexpr uint32_t kHeartbeatIntervalMs = 100;  // UI transmits heartbeat every 100ms

// FreeRTOS Task Watchdog Timer: panics/resets if driveTask hangs > 3 seconds.
constexpr uint32_t kWatchdogTimeoutSec  = 3;

// Capacity of the inter-task command queue.
constexpr size_t   kDriveQueueCapacity  = 16;

// Task stack depth & priority
constexpr uint32_t kDriveTaskStackDepth = 4096;
constexpr int      kDriveTaskPriority   = 2;
constexpr int      kDriveTaskCore       = 1;    // Core 1 (Wi-Fi/AsyncTCP runs on Core 0)

// Optional on-board status LED (GPIO 2 on ESP32 DevKit)
constexpr int8_t   kStatusLedPin        = 2;

}  // namespace config

