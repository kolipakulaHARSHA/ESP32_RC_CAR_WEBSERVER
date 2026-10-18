// Low-level driver for a dual H-bridge (L298N / L293D) with on/off direction control.
// This layer knows about pins only - it has no idea what "turn left" means.
#pragma once

#include <stdint.h>

namespace motor {

enum class Side : uint8_t { Left, Right };
enum class Direction : uint8_t { Stop, Forward, Reverse };

// Configures the H-bridge pins as outputs and leaves both motors stopped.
void init();

// Drives one motor in the given direction.
void set(Side side, Direction direction);

// Stops both motors.
void stopAll();

}  // namespace motor
