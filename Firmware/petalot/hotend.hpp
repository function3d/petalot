#include <PID_v1.h>


double T;          //current temp

bool F = false;
bool Fc = false;
bool Fi = false; 

double Output;  //pid output

PID myPID(&T, &Output, &To, Kp, Ki, Kd, DIRECT);

double tempLastSample;
double tempLastFilament;
double tempLastNoFilament;
double tempLastStart;

//thermistor
float logR2, R2;
//steinhart-hart coeficients for thermistor
float c1 = 0.8438162826e-03, c2 = 2.059601750e-04, c3 = 0.8615484887e-07;

double Thermistor(float Volts) {
  R2 = R1 * (1023.0 / (float)Volts - 1.0); //calculate resistance on thermistor

  logR2 = log(R2);
  T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2)); // temperature in Kelvin
  T = T - 273.15; //convert Kelvin to Celcius
  return T;
}

void start(){
    if (tempLastStart==0){
      status = "working";
      V = Vo;
      tempLastStart = millis();
      if (tempLastStart==0) tempLastStart = 1;
    }
}

void stop(){
    status = "stopped";
    V = 0;
    tempLastStart = 0;
    ifttt();
}

void initHotend(){
  myPID.SetTunings(Kp, Ki, Kd);
  myPID.SetOutputLimits(0,Max);
  pinMode(LED_BUILTIN , OUTPUT);
  pinMode(PIN_FILAMENT , INPUT);
  if (status=="") start();
}




void hotendReadTempTask() {
  if (status == "stopped" && myPID.GetMode() == AUTOMATIC){
    myPID.SetMode(MANUAL);
    Output = 0;
  }
  if (status == "working" && myPID.GetMode() != AUTOMATIC){
    myPID.SetMode(AUTOMATIC);
  }
  if (millis() >= tempLastSample + 100)
  {
    Thermistor(analogRead(PIN_THERMISTOR)); //Volt to temp, update T
    if (T > Tm || isnan(T)){
      Output = 0;
    } else {
      myPID.Compute();
    }
    if (status == "working"){
      start();
      if (T > 150 || T > To + 20 ) {
        digitalWrite(LED_BUILTIN , LOW);// target temperature ready
      } else {
        digitalWrite(LED_BUILTIN , !digitalRead(LED_BUILTIN));//reaching tarjet temp
      }
    } else {
        digitalWrite(LED_BUILTIN , HIGH);
    }

    analogWrite(PIN_HEATER, Output);
    
    Fc = digitalRead(PIN_FILAMENT);
    
    if (Fc && !F) {
      tempLastFilament = millis();
      start();
    }
    
    if (!Fc && F) {
      tempLastFilament = 0;
      tempLastNoFilament = millis();
    }

    F = Fc;

    if (Fc && tempLastFilament > 0 && millis() >= tempLastFilament + 3*1000){
      Fi = true;
    }
    
    if (!Fc && Fi && tempLastNoFilament > 0 && millis() >= tempLastNoFilament + 500) { // no filament
      stop();
      tempLastNoFilament = 0;
      Fi = false;
    }
    
    if (!Fc && !Fi && tempLastStart > 0 && millis() >= tempLastStart + 5*60*1000) { // no filament for 5 min
      stop();
    }

    tempLastSample = millis();
    
  }
}
