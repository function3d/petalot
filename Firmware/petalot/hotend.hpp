bool F = false;
bool Fcurrent = false;
bool Finsert = false;

double Output;  //pid output

double tempLastSample;
double tempLastFilament;
double tempLastNoFilament;
double tempLastStart;
double tempLastFilamentCheck;

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
  if (isnan(T)){
    return 0;
  }
  if (T < -5){
    stop();
    return 0;
  }
  if(T > To){
    return 0;
  }
  if(T < 100){
    return 255;
  }
  if(T < 150){
    return Max;
  }
  return Max/3;

  
}


void hotendReadTempTask() {

  if (millis() >= tempLastSample + 250) //250 is 4 times per second
  {
    Thermistor(analogRead(PIN_THERMISTOR)); //Volt to temp, update T
    Output = control();
    analogWrite(PIN_HEATER, Output);
    if (status == "working"){
      if (T > Tmi) {
        digitalWrite(LED_BUILTIN , LOW);// target temperature ready
      } else {
        digitalWrite(LED_BUILTIN , !digitalRead(LED_BUILTIN));//reaching tarjet temp
      }
    } else {
        digitalWrite(LED_BUILTIN , HIGH);
    }  

    tempLastSample = millis();
    
    //filament
    Fcurrent = digitalRead(PIN_FILAMENT);
      
    if (Fcurrent && !F) {
      tempLastFilament = millis();
      start(); //start the machine with the filament sensor
    }
      
    if (!Fcurrent && F) {
      tempLastFilament = 0;
      tempLastNoFilament = millis();
    }

    F = Fcurrent;

    if (Fenable) {
      if (Fcurrent && tempLastFilament > 0 && millis() >= tempLastFilament + 3*1000){
        Finsert = true;
      }
      
      if (!Fcurrent && Finsert && tempLastNoFilament > 0 && millis() >= tempLastNoFilament + 500) { // no filament
        stop();
        tempLastNoFilament = 0;
        Finsert = false;
      }
      
      if (!Fcurrent && !Finsert && tempLastStart > 0 && millis() >= tempLastStart + 5*60*1000) { // no filament for 5 min
        stop();
      }
    }

  }
}
