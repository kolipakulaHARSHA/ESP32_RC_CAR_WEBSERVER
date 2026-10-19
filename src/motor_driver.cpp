#include "motor_driver.h"

#include <Arduino.h>

#include "config.h"

namespace motor {
namespace {

struct HBridgePwmChannel {
  uint8_t pin;
  uint8_t channel;
};

struct MotorSideMapping {
  HBridgePwmChannel fwd;
  HBridgePwmChannel rev;
};

// Indexed by Side (Left = 0, Right = 1)
constexpr MotorSideMapping kMotors[] = {
    // Side::Left (IN1, IN2)
    {{config::kLeftMotorIn1, config::kLeftPwmChan1},
     {config::kLeftMotorIn2, config::kLeftPwmChan2}},
    // Side::Right (IN3, IN4)
    {{config::kRightMotorIn1, config::kRightPwmChan1},
     {config::kRightMotorIn2, config::kRightPwmChan2}},
};

inline const MotorSideMapping& mappingFor(Side side) {
  return kMotors[static_cast<uint8_t>(side)];
}

uint32_t clampPwm(int16_t val) {
  if (val <= 0) return 0;
  if (val > config::kMaxPwmValue) return config::kMaxPwmValue;
  return static_cast<uint32_t>(val);
}

}  // namespace

void init() {
  for (const MotorSideMapping& motor : kMotors) {
    ledcSetup(motor.fwd.channel, config::kPwmFrequency, config::kPwmResolutionBits);
    ledcAttachPin(motor.fwd.pin, motor.fwd.channel);
    ledcWrite(motor.fwd.channel, 0);

    ledcSetup(motor.rev.channel, config::kPwmFrequency, config::kPwmResolutionBits);
    ledcAttachPin(motor.rev.pin, motor.rev.channel);
    ledcWrite(motor.rev.channel, 0);
  }
}

void setMotor(Side side, int16_t speed) {
  const MotorSideMapping& motor = mappingFor(side);

  if (speed > 0) {
    // Forward: drive forward channel with PWM, reverse channel grounded
    ledcWrite(motor.rev.channel, 0);
    ledcWrite(motor.fwd.channel, clampPwm(speed));
  } else if (speed < 0) {
    // Reverse: drive reverse channel with PWM, forward channel grounded
    ledcWrite(motor.fwd.channel, 0);
    ledcWrite(motor.rev.channel, clampPwm(-speed));
  } else {
    // Stop: coast both channels low
    ledcWrite(motor.fwd.channel, 0);
    ledcWrite(motor.rev.channel, 0);
  }
}

void setSpeeds(int16_t leftSpeed, int16_t rightSpeed) {
  setMotor(Side::Left, leftSpeed);
  setMotor(Side::Right, rightSpeed);
}

void set(Side side, Direction direction) {
  switch (direction) {
    case Direction::Forward:
      setMotor(side, config::kDefaultSpeed);
      break;
    case Direction::Reverse:
      setMotor(side, -config::kDefaultSpeed);
      break;
    case Direction::Stop:
    default:
      setMotor(side, 0);
      break;
  }
}

void stopAll() {
  setSpeeds(0, 0);
}

}  // namespace motor
