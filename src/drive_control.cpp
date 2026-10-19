#include "drive_control.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "motor_driver.h"
#include "system_diagnostics.h"

namespace drive {
namespace {

QueueHandle_t       s_driveQueue       = nullptr;
TaskHandle_t        s_driveTaskHandle  = nullptr;
volatile VehicleState s_currentState   = VehicleState::Idle;
int16_t             s_cruiseSpeed      = config::kDefaultSpeed;

void transitionTo(VehicleState newState) {
  if (s_currentState != newState) {
    Serial.printf("[STATE] %s -> %s\n", stateToString(s_currentState), stateToString(newState));
    s_currentState = newState;
  }
}

int16_t rampStep(int16_t current, int16_t target, int16_t maxStep) {
  const int16_t delta = target - current;
  if (delta > maxStep) {
    return current + maxStep;
  }
  if (delta < -maxStep) {
    return current - maxStep;
  }
  return target;
}

void driveTaskLoop(void* /*arg*/) {
  Serial.printf("[TASK] driveTask started on Core %d (priority %d)\n",
                xPortGetCoreID(), uxTaskPriorityGet(NULL));

  // Subscribe this task to the FreeRTOS Task Watchdog Timer
  diag::subscribeCurrentTask();

  if (config::kStatusLedPin >= 0) {
    pinMode(config::kStatusLedPin, OUTPUT);
    digitalWrite(config::kStatusLedPin, LOW);
  }

  int16_t targetLeftPwm  = 0;
  int16_t targetRightPwm = 0;
  int16_t currentLeftPwm  = 0;
  int16_t currentRightPwm = 0;

  TickType_t lastHeartbeatTick = xTaskGetTickCount();
  TickType_t lastWakeTime      = xTaskGetTickCount();
  bool clientConnected         = false;
  uint32_t ledBlinkCounter     = 0;

  while (true) {
    // 1. Reset Task Watchdog Timer each iteration
    diag::feedWatchdog();

    // 2. Process all pending queue messages (non-blocking batch)
    DriveMessage msg;
    while (xQueueReceive(s_driveQueue, &msg, 0) == pdTRUE) {
      switch (msg.type) {
        case MessageType::ClientConnected:
          clientConnected   = true;
          lastHeartbeatTick = xTaskGetTickCount();
          targetLeftPwm     = 0;
          targetRightPwm    = 0;
          transitionTo(VehicleState::Armed);
          break;

        case MessageType::ClientDisconnected:
          clientConnected   = false;
          targetLeftPwm     = 0;
          targetRightPwm    = 0;
          transitionTo(VehicleState::Idle);
          break;

        case MessageType::SetSpeed:
          if (msg.speed >= 50 && msg.speed <= config::kMaxPwmValue) {
            s_cruiseSpeed = msg.speed;
            Serial.printf("[DRIVE] Cruise speed updated to %d / %d\n", s_cruiseSpeed, config::kMaxPwmValue);
          }
          break;

        case MessageType::Heartbeat:
          lastHeartbeatTick = xTaskGetTickCount();
          if (s_currentState == VehicleState::Failsafe && clientConnected) {
            transitionTo(VehicleState::Armed);
          }
          break;

        case MessageType::DriveCommand:
          lastHeartbeatTick = xTaskGetTickCount();
          if (s_currentState == VehicleState::Failsafe) {
            transitionTo(VehicleState::Armed);
          }

          switch (msg.direction) {
            case DirectionCommand::Forward:
              targetLeftPwm  = s_cruiseSpeed;
              targetRightPwm = s_cruiseSpeed;
              transitionTo(VehicleState::Driving);
              break;

            case DirectionCommand::Reverse:
              targetLeftPwm  = -s_cruiseSpeed;
              targetRightPwm = -s_cruiseSpeed;
              transitionTo(VehicleState::Driving);
              break;

            case DirectionCommand::TurnLeft:
              // Spin-turn / differential steering
              targetLeftPwm  = -s_cruiseSpeed / 2;
              targetRightPwm = s_cruiseSpeed;
              transitionTo(VehicleState::Driving);
              break;

            case DirectionCommand::TurnRight:
              targetLeftPwm  = s_cruiseSpeed;
              targetRightPwm = -s_cruiseSpeed / 2;
              transitionTo(VehicleState::Driving);
              break;

            case DirectionCommand::Stop:
            default:
              targetLeftPwm  = 0;
              targetRightPwm = 0;
              if (s_currentState == VehicleState::Driving) {
                transitionTo(VehicleState::Armed);
              }
              break;
          }
          break;
      }
    }

    // 3. Heartbeat link-loss fail-safe check
    const TickType_t now = xTaskGetTickCount();
    if ((s_currentState == VehicleState::Driving || s_currentState == VehicleState::Armed) && clientConnected) {
      const uint32_t elapsedMs = pdTICKS_TO_MS(now - lastHeartbeatTick);
      if (elapsedMs > config::kHeartbeatTimeoutMs) {
        if (s_currentState != VehicleState::Failsafe) {
          Serial.printf("[SAFETY] Heartbeat timed out (%lu ms > %u ms limit). Transitioning to FAILSAFE!\n",
                        static_cast<unsigned long>(elapsedMs), config::kHeartbeatTimeoutMs);
          targetLeftPwm  = 0;
          targetRightPwm = 0;
          transitionTo(VehicleState::Failsafe);
        }
      }
    }

    // If in failsafe, guarantee targets remain zero
    if (s_currentState == VehicleState::Failsafe || s_currentState == VehicleState::Idle) {
      targetLeftPwm  = 0;
      targetRightPwm = 0;
    }

    // 4. Smooth Acceleration / Deceleration Ramping (Slew-Rate Limiter)
    currentLeftPwm  = rampStep(currentLeftPwm, targetLeftPwm, config::kMaxSlewStepPerTick);
    currentRightPwm = rampStep(currentRightPwm, targetRightPwm, config::kMaxSlewStepPerTick);

    // 5. Output to hardware LEDC PWM
    motor::setSpeeds(currentLeftPwm, currentRightPwm);

    // 6. Status LED indication
    if (config::kStatusLedPin >= 0) {
      ledBlinkCounter++;
      if (s_currentState == VehicleState::Driving || s_currentState == VehicleState::Armed) {
        digitalWrite(config::kStatusLedPin, HIGH);
      } else if (s_currentState == VehicleState::Failsafe) {
        // Fast blink (100ms cycle = 5 ticks on / 5 ticks off)
        digitalWrite(config::kStatusLedPin, (ledBlinkCounter % 10 < 5) ? HIGH : LOW);
      } else {
        // Idle: off
        digitalWrite(config::kStatusLedPin, LOW);
      }
    }

    // 7. Precise fixed-frequency loop execution
    vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(config::kControlLoopPeriodMs));
  }
}

}  // namespace

void init() {
  motor::init();

  s_driveQueue = xQueueCreate(config::kDriveQueueCapacity, sizeof(DriveMessage));
  if (s_driveQueue == nullptr) {
    Serial.println("[ERROR] Failed to allocate FreeRTOS driveQueue!");
    return;
  }

  BaseType_t res = xTaskCreatePinnedToCore(
      driveTaskLoop,
      "driveTask",
      config::kDriveTaskStackDepth,
      nullptr,
      config::kDriveTaskPriority,
      &s_driveTaskHandle,
      config::kDriveTaskCore);

  if (res != pdPASS) {
    Serial.println("[ERROR] Failed to create driveTask!");
  } else {
    Serial.printf("[INIT] driveTask spawned on Core %d\n", config::kDriveTaskCore);
  }
}

bool postMessage(const DriveMessage& msg) {
  if (s_driveQueue == nullptr) return false;
  // Non-blocking send: drop if queue full to avoid blocking network task
  return (xQueueSend(s_driveQueue, &msg, 0) == pdTRUE);
}

bool parseCommand(const uint8_t* payload, size_t length, DriveMessage& outMsg) {
  if (payload == nullptr || length == 0) return false;

  outMsg.timestampMs = millis();

  // 1. Check for heartbeat
  if ((length == 2 && memcmp(payload, "hb", 2) == 0) ||
      (length == 9 && memcmp(payload, "heartbeat", 9) == 0)) {
    outMsg.type = MessageType::Heartbeat;
    return true;
  }

  // 2. Check for speed adjustment e.g. "speed:220"
  if (length >= 7 && memcmp(payload, "speed:", 6) == 0) {
    char buf[8] = {0};
    const size_t numLen = (length - 6 < sizeof(buf) - 1) ? (length - 6) : (sizeof(buf) - 1);
    memcpy(buf, payload + 6, numLen);
    buf[numLen] = '\0';
    int val = atoi(buf);
    if (val >= 0 && val <= 255) {
      outMsg.type = MessageType::SetSpeed;
      outMsg.speed = static_cast<int16_t>(val);
      return true;
    }
  }

  // 3. Directional movement commands
  outMsg.type = MessageType::DriveCommand;
  outMsg.speed = s_cruiseSpeed;

  if (length == 2 && memcmp(payload, "up", 2) == 0) {
    outMsg.direction = DirectionCommand::Forward;
    return true;
  }
  if (length == 4 && memcmp(payload, "down", 4) == 0) {
    outMsg.direction = DirectionCommand::Reverse;
    return true;
  }
  if (length == 4 && memcmp(payload, "left", 4) == 0) {
    outMsg.direction = DirectionCommand::TurnLeft;
    return true;
  }
  if (length == 5 && memcmp(payload, "right", 5) == 0) {
    outMsg.direction = DirectionCommand::TurnRight;
    return true;
  }
  if (length == 3 && memcmp(payload, "off", 3) == 0) {
    outMsg.direction = DirectionCommand::Stop;
    return true;
  }

  return false;
}

VehicleState getState() {
  return s_currentState;
}

const char* stateToString(VehicleState state) {
  switch (state) {
    case VehicleState::Idle:     return "IDLE";
    case VehicleState::Armed:    return "ARMED";
    case VehicleState::Driving:  return "DRIVING";
    case VehicleState::Failsafe: return "FAILSAFE";
    case VehicleState::Fault:    return "FAULT";
    default:                     return "UNKNOWN";
  }
}

int16_t getCurrentSpeed() {
  return s_cruiseSpeed;
}

}  // namespace drive
