#pragma once

#include <Arduino.h>

#include <AccelStepper.h>

// ============================================================================
// MOTOR CONFIGURATION
// ============================================================================
const uint16_t STEPS_PER_REVOLUTION = 200;
const uint8_t MICROSTEPPING = 16;
const uint16_t REAL_STEPS_PER_REV = STEPS_PER_REVOLUTION * MICROSTEPPING; // 3200

// Extruder calibration: 46 stepper revolutions = 29cm of filament extruded
const float STEPPER_REVS_PER_CM = 46.0f / 29.0f;
const float STEPS_PER_CM = REAL_STEPS_PER_REV * STEPPER_REVS_PER_CM;

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
#if VERSION == 1502
    // Configure microstepping (MSI pins) for the 1.5.2 PCB
    pinMode(PIN_MSI3, OUTPUT);
    digitalWrite(PIN_MSI3, HIGH);
#endif
    digitalWrite(PIN_EN, HIGH); // Disabled at start (active LOW)

    accelStepper.setEnablePin(PIN_EN);
    accelStepper.setPinsInverted(false, false, true); // Enable is active LOW
    accelStepper.setAcceleration(10000);
    accelStepper.setMaxSpeed(rpmToHz(60));
  }

  void setSpeed(float cmPerMin) {
    if (cmPerMin <= 0) return;

    uint16_t newRPM = (uint16_t)(cmPerMin * STEPPER_REVS_PER_CM);
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
    // Temperature safety: if MotorOnTo is set, wait until T >= To - 6°C
    if (MotorOnTo && T < To - 6.0f) {
      disable();
      return;
    }

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