// Drive control: FreeRTOS motor control task, command queue, state machine,
// acceleration ramping, and link-loss heartbeat fail-safe.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace drive {

enum class VehicleState : uint8_t {
  Idle,      // Motors stopped, waiting for client
  Armed,     // Client connected, heartbeat active, stopped
  Driving,   // Active motion with verified heartbeat
  Failsafe,  // Heartbeat lost (>350ms); motors safely locked stopped
  Fault      // Watchdog or system error
};

enum class MessageType : uint8_t {
  DriveCommand,       // Directional move command (up, down, left, right, off)
  Heartbeat,          // Keepalive ping from client
  SetSpeed,           // Adjust cruising speed (0..255)
  ClientConnected,    // WebSocket client connected
  ClientDisconnected  // WebSocket client disconnected
};

enum class DirectionCommand : uint8_t {
  Stop,
  Forward,
  Reverse,
  TurnLeft,
  TurnRight
};

struct DriveMessage {
  MessageType      type;
  DirectionCommand direction;
  int16_t          speed;        // 0..255 for SetSpeed or DriveCommand
  uint32_t         timestampMs;
};

// Initializes the FreeRTOS command queue and spawns the driveTask on Core 1.
void init();

// Thread-safe: posts a message to the drive command queue (non-blocking).
bool postMessage(const DriveMessage& msg);

// Parses an incoming WebSocket payload into a DriveMessage.
bool parseCommand(const uint8_t* payload, size_t length, DriveMessage& outMsg);

// Returns current vehicle state.
VehicleState getState();

// Returns human-readable string for a vehicle state.
const char* stateToString(VehicleState state);

// Returns current configured cruising speed.
int16_t getCurrentSpeed();

}  // namespace drive
