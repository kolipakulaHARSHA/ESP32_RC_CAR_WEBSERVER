#include "motor_driver.h"

#include <Arduino.h>

#include "config.h"

namespace motor {
namespace {

struct HBridgeChannel {
  uint8_t in1;
  uint8_t in2;
};

// Indexed by Side.
constexpr HBridgeChannel kChannels[] = {
    {config::kLeftMotorIn1, config::kLeftMotorIn2},    // Side::Left
    {config::kRightMotorIn1, config::kRightMotorIn2},  // Side::Right
};

const HBridgeChannel& channelFor(Side side) {
  return kChannels[static_cast<uint8_t>(side)];
}

}  // namespace

void init() {
  for (const HBridgeChannel& channel : kChannels) {
    pinMode(channel.in1, OUTPUT);
    pinMode(channel.in2, OUTPUT);
  }
  stopAll();
}

void set(Side side, Direction direction) {
  const HBridgeChannel& channel = channelFor(side);

  switch (direction) {
    case Direction::Forward:
      digitalWrite(channel.in1, HIGH);
      digitalWrite(channel.in2, LOW);
      break;
    case Direction::Reverse:
      digitalWrite(channel.in1, LOW);
      digitalWrite(channel.in2, HIGH);
      break;
    case Direction::Stop:
    default:
      // Both inputs low = coast (motor terminals floating).
      digitalWrite(channel.in1, LOW);
      digitalWrite(channel.in2, LOW);
      break;
  }
}

void stopAll() {
  set(Side::Left, Direction::Stop);
  set(Side::Right, Direction::Stop);
}

}  // namespace motor
