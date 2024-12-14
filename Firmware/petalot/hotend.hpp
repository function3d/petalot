
#define T_THRESHOLD 50

bool F = false;
bool Fc = false;
bool Fi = false; 

double Output;  //pid output

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
      Fs = 0;
      Ts = 0;
      status = "working";
      tempLastStart = millis();
      if (tempLastStart==0) tempLastStart = 1;
    }
}

void stop(){
    
    status = "stopped";
    analogWrite(PIN_HEATER, 0);
    digitalWrite(LED_BUILTIN , HIGH);
    tempLastStart = 0;
    ifttt();
}

void initHotend(){
  pinMode(LED_BUILTIN , OUTPUT);
  pinMode(PIN_FILAMENT , INPUT);
  if (status=="") start();
}

double control(){

  if (status == "stopped"){
    return 0;
  }
  if (T > Tm || isnan(T)){
    return 0;
  }
  if (T < 0){
    stop();
    return 0;
  }
  if(T > To){
    return 0;
  }
   if(T < To - T_THRESHOLD){
    return Max;
  }
  return Max/2;
}

void hotendReadTempTask() {
  
  if (millis() >= tempLastSample + 250)
  {
    Thermistor(analogRead(PIN_THERMISTOR)); //Volt to temp, update T
    Output = control();
    analogWrite(PIN_HEATER, Output);
    if (status == "working"){
      start();
      if (T > Tmi && T < Tm + 30) {
        digitalWrite(LED_BUILTIN , LOW);// target temperature ready
      } else {
        digitalWrite(LED_BUILTIN , !digitalRead(LED_BUILTIN));//reaching tarjet temp
      }
    } else {
        digitalWrite(LED_BUILTIN , HIGH);
    }

    
    
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
    if (Fe) {
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
    }

    

    tempLastSample = millis();
    
  }
}
