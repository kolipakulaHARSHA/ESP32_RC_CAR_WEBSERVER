// Low-level hardware driver for a dual H-bridge (L298N / L293D) using ESP32 LEDC PWM.
// Maps motor throttle/speed (-255..+255) to LEDC hardware timer channels.
#pragma once

#include <stdint.h>

namespace motor {

enum class Side : uint8_t { Left, Right };
enum class Direction : uint8_t { Stop, Forward, Reverse };

// Configures the ESP32 LEDC hardware PWM channels for the H-bridge pins.
void init();

// Sets raw PWM speed for a single motor side (-255..+255).
// Positive = Forward, Negative = Reverse, 0 = Stop/Coast.
void setMotor(Side side, int16_t speed);

// Sets raw PWM speed for both left and right motors simultaneously.
void setSpeeds(int16_t leftSpeed, int16_t rightSpeed);

// Legacy on/off interface with fixed default cruise speed.
void set(Side side, Direction direction);

// Immediately sets 0% duty cycle on all 4 H-bridge channels.
void stopAll();

}  // namespace motor
