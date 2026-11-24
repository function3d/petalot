#pragma once

#include <Arduino.h>

#include <AccelStepper.h>

// ============================================================================
// MOTOR CONFIGURATION
// ============================================================================
const uint16_t STEPS_PER_REVOLUTION = 200;
const uint8_t MICROSTEPPING = 16;
const uint16_t REAL_STEPS_PER_REV = STEPS_PER_REVOLUTION * MICROSTEPPING; // 3200

// Gearbox calibration lives in conf.hpp: GearRatio (motor revolutions per
// output revolution) and DriveCmPerRev (filament cm per output revolution).
// Defaults are 46/29 on older boards and 50/29 on PCB 1503.
inline float stepperRevsPerCm() {
  return (float)(GearRatio / DriveCmPerRev);
}

// ============================================================================
// STEPPER CONTROLLER CLASS
// ============================================================================
class StepperController {
private:
  uint16_t targetRPM = 0;

  AccelStepper accelStepper;

  uint32_t rpmToHz(uint16_t rpm) const {
    return (uint32_t)((float)rpm * REAL_STEPS_PER_REV / 60.0f);
  }

  void applySpeed() {
    uint32_t hz = rpmToHz(targetRPM);
    accelStepper.setSpeed(-(float)hz);
    accelStepper.runSpeed();
  }

public:
  bool motorEnabled = false;

  StepperController()
    : accelStepper(AccelStepper::DRIVER, PIN_STEP, PIN_DIR) {}

  void init() {
    pinMode(PIN_EN, OUTPUT);
#if PCB >= 1502
    // Configure microstepping (MSI pins) for the 1.5.2 PCB and later
    pinMode(PIN_MSI3, OUTPUT);
    digitalWrite(PIN_MSI3, HIGH);
#endif
    digitalWrite(PIN_EN, HIGH); // Disabled at start (active LOW)

    accelStepper.setEnablePin(PIN_EN);
    accelStepper.setPinsInverted(false, false, true); // Enable is active LOW
    accelStepper.setAcceleration(10000);
    applyCalibration();
  }

  // Recompute the speed ceiling after GearRatio/DriveCmPerRev change. The
  // AccelStepper speed setter clamps to this ceiling, so it must leave room
  // for maxV at the current gearbox ratio.
  void applyCalibration() {
    float maxRpm = (float)maxV * stepperRevsPerCm() * 1.15f;
    if (maxRpm < 60.0f) maxRpm = 60.0f;
    accelStepper.setMaxSpeed(rpmToHz((uint16_t)(maxRpm + 0.5f)));
    targetRPM = 0;
  }

  void setSpeed(float cmPerMin) {
    if (cmPerMin <= 0) return;

    uint16_t newRPM = (uint16_t)(cmPerMin * stepperRevsPerCm());
    if (newRPM != targetRPM && newRPM > 0) {
      targetRPM = newRPM;
      applySpeed();
    }
  }

  void enable() {
    if (motorEnabled) return;

    digitalWrite(PIN_EN, LOW);
    motorEnabled = true;

    applySpeed();
    accelStepper.runSpeed();
  }

  void disable() {
    if (!motorEnabled) return;

    accelStepper.stop();

    digitalWrite(PIN_EN, HIGH);
    motorEnabled = false;
  }

  void task() {
    // MotorOnTo waits for temperature. An OTA resume waits too, even when
    // MotorOnTo is off: the heater cools during the update and PET strip must
    // not be driven cold.
    if ((MotorOnTo || resumeWaitTemp) && (isnan(T) || T < To - 3.0f)) {
      disable();
      return;
    }
    if (resumeWaitTemp && !isnan(T) && T >= To - 3.0f) resumeWaitTemp = false;

    // Sync motor state with the global status
    if (status == "working" && !motorEnabled) {
      enable();
    } else if (status == "stopped" && motorEnabled) {
      disable();
      return;
    }

    // Only run if the motor is enabled
    if (!motorEnabled) return;

    // Update speed from Vo (cm/min)
    setSpeed(Vo);

    accelStepper.runSpeed();
  }

  bool isEnabled() const { return motorEnabled; }
  uint16_t getTargetRPM() const { return targetRPM; }
};

StepperController stepper;