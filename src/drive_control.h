// Drive control: translates high-level drive commands into motor actions,
// and parses the text commands sent by the web UI.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace drive {

enum class Command : uint8_t { Stop, Forward, Reverse, TurnLeft, TurnRight };

// Parses a raw command payload. The payload does NOT need to be NUL-terminated
// and is never written to. Returns false for unknown commands.
bool parseCommand(const uint8_t* payload, size_t length, Command& out);

// Applies a command to the motors.
void apply(Command command);

// Human-readable command name, for logging.
const char* toString(Command command);

}  // namespace drive
