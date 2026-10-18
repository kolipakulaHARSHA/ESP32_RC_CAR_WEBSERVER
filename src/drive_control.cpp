#include "drive_control.h"

#include <string.h>

#include "motor_driver.h"

namespace drive {
namespace {

struct CommandEntry {
  const char* text;
  Command command;
};

// Wire protocol: plain-text WebSocket messages sent by the web UI.
constexpr CommandEntry kCommandTable[] = {
    {"up", Command::Forward},
    {"down", Command::Reverse},
    {"left", Command::TurnLeft},
    {"right", Command::TurnRight},
    {"off", Command::Stop},
};

}  // namespace

bool parseCommand(const uint8_t* payload, size_t length, Command& out) {
  if (payload == nullptr) {
    return false;
  }

  for (const CommandEntry& entry : kCommandTable) {
    // Length-checked compare: no reliance on a terminating NUL in the payload.
    if (strlen(entry.text) == length && memcmp(payload, entry.text, length) == 0) {
      out = entry.command;
      return true;
    }
  }
  return false;
}

void apply(Command command) {
  using motor::Direction;
  using motor::Side;

  switch (command) {
    case Command::Forward:
      motor::set(Side::Left, Direction::Forward);
      motor::set(Side::Right, Direction::Forward);
      break;
    case Command::Reverse:
      motor::set(Side::Left, Direction::Reverse);
      motor::set(Side::Right, Direction::Reverse);
      break;
    case Command::TurnLeft:
      // Right wheel drives, left wheel coasts.
      motor::set(Side::Left, Direction::Stop);
      motor::set(Side::Right, Direction::Forward);
      break;
    case Command::TurnRight:
      // Left wheel drives, right wheel coasts.
      motor::set(Side::Left, Direction::Forward);
      motor::set(Side::Right, Direction::Stop);
      break;
    case Command::Stop:
    default:
      motor::stopAll();
      break;
  }
}

const char* toString(Command command) {
  switch (command) {
    case Command::Forward:   return "FORWARD";
    case Command::Reverse:   return "REVERSE";
    case Command::TurnLeft:  return "TURN_LEFT";
    case Command::TurnRight: return "TURN_RIGHT";
    case Command::Stop:      return "STOP";
  }
  return "UNKNOWN";
}

}  // namespace drive
