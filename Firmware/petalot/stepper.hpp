#pragma once

#include <Arduino.h>

// ============================================================================
// PLATFORM-SPECIFIC INCLUDES
// ============================================================================
#ifdef ESP32
    #include <FastAccelStepper.h>
#elif defined(ESP8266)
    #include <AccelStepper.h>
#else
    #error "Unsupported platform. This code requires ESP32 or ESP8266."
#endif

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
    
    
    // Platform-specific stepper instances
    #ifdef ESP32
        FastAccelStepperEngine engine;
        FastAccelStepper *fasStepper = nullptr;
    #elif defined(ESP8266)
        AccelStepper accelStepper;
    #endif
    
    uint32_t rpmToHz(uint16_t rpm) const {
        return (uint32_t)((float)rpm * REAL_STEPS_PER_REV / 60.0f);
    }
    
    void applySpeed() {
        uint32_t hz = rpmToHz(targetRPM);
        #ifdef ESP32
            if (fasStepper) {
                fasStepper->setSpeedInHz(-hz);
                fasStepper->runForward();
            }
        #elif defined(ESP8266)
            accelStepper.setSpeed(-(float)hz);
            accelStepper.runSpeed();
        #endif
    }
    
public:
    bool motorEnabled = false;
    
    StepperController() 
    #ifdef ESP32
        : engine(FastAccelStepperEngine())
    #elif defined(ESP8266)
        : accelStepper(AccelStepper::DRIVER, PIN_STEP, PIN_DIR)
    #endif
    {}
    
    void init() {
        pinMode(PIN_EN, OUTPUT);
        digitalWrite(PIN_EN, HIGH); // Disabled at start (active LOW)
        
        #ifdef ESP32
            engine.init();
            fasStepper = engine.stepperConnectToPin(PIN_STEP);
            if (fasStepper) {
                fasStepper->setDirectionPin(PIN_DIR);
                fasStepper->setEnablePin(PIN_EN);
                fasStepper->setAutoEnable(false); // We control enable manually
                fasStepper->setAcceleration(10000);
                fasStepper->setSpeedInHz(0);
            } else {
                Serial.println("Error: Could not connect stepper to pin");
            }
        #elif defined(ESP8266)
            accelStepper.setEnablePin(PIN_EN);
            accelStepper.setPinsInverted(false, false, true); // Enable is active LOW
            accelStepper.setAcceleration(10000);
            accelStepper.setMaxSpeed(rpmToHz(60));
        #endif
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
        
        #ifdef ESP32
            applySpeed();
            fasStepper->runForward();
        #elif defined(ESP8266)
            applySpeed();
            accelStepper.runSpeed();
        #endif
    }
    
    void disable() {
        if (!motorEnabled) return;
        
        #ifdef ESP32
            fasStepper->stopMove();
        #elif defined(ESP8266)
            accelStepper.stop();
        #endif
        
        digitalWrite(PIN_EN, HIGH);
        motorEnabled = false;
    }
    
    void task() {
        // Sync motor state with global status
        if (status == "working" && !motorEnabled) {
            enable();
        } else if (status == "stopped" && motorEnabled) {
            disable();
            return;
        }
        
        // Only run if motor is enabled
        if (!motorEnabled) return;
        
        // Temperature safety: if MotorOnTo is true, wait until T >= To - 6°C
        if (MotorOnTo && T < To - 6.0f) {
            disable();
            return;
        }
        
        // Update speed from Vo (cm/min)
        setSpeed(Vo);
        
        // Platform-specific task handling
        #ifdef ESP32
            fasStepper->runForward();
        #elif defined(ESP8266)
            accelStepper.runSpeed();
        #endif
    }
    
    bool isEnabled() const { return motorEnabled; }
    uint16_t getTargetRPM() const { return targetRPM; }
};

StepperController stepper;