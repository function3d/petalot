#include <AccelStepper.h>

AccelStepper stepper(AccelStepper::FULL2WIRE,PIN_STEP,PIN_DIR);

int stepsPerRevolution = 200;
double tempLastSampleStepper;

bool stepperEnable = false;

void initStepper(){
  stepper.setPinsInverted(true,false,true); //set enable pin inverted
  stepper.setEnablePin(PIN_EN);
  stepper.disableOutputs();
  stepper.setMaxSpeed(60*stepsPerRevolution+1);
  stepper.setSpeed(Vo*stepsPerRevolution);
}

void stepperRunTask(){

    if (status == "stopped" && stepperEnable) {
      stepper.disableOutputs();
      stepperEnable = false;
    }
    if (status == "working" && !stepperEnable) {
      stepper.enableOutputs();
      stepperEnable = true;
    }
    if (status == "working") { //&& T > Tmi
      if ((MotorOnTo && T > To - 6) || !MotorOnTo){
        stepper.setSpeed(Vo*stepsPerRevolution);
        stepper.runSpeed();
      }
    }
}
