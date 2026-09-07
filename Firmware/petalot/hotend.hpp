const float VCC = 3.3;

const int NUM_READINGS = 2;
float readings[NUM_READINGS];
int readIndex = 0;
float total = 0;
float average = 0;

short AR;
bool F = false;
bool Fcurrent = false;
bool Finsert = false;
bool Fworking = false;
double Output;
double tempLastSample;
double tempLastFilament;
double tempLastNoFilament;
double tempLastStart;
double tempLastFilamentCheck;
double temperatureStart;

//3V3 → NTC → A0 → 2KΩ → GND
int temptable[8][2] = {
  { 960, 250 },
  { 922, 230 },
  { 833, 200 },
  { 703, 170 },
  { 454, 130 },
  { 157, 80 },
  { 31, 30 },
  { 17, 10 }
};

void Thermister_ESP8266() {
  int i = 0;

  for (i = 0; i < 52; i++) {
    if (AR >= temptable[i][0]) {
      break;
    }
  }

  if (i==0)
    T = temptable[i][1];
  else
    T = map(AR, temptable[i-1][0], temptable[i][0], temptable[i-1][1], temptable[i][1]);
  int toffset = map (T, 0, To, 0, TOffset);
  T = T + toffset;
  if (AR < temptable[7][0])
    T = 0;
}

void start(){
    if (tempLastStart==0){
      Fs = 0;
      Ts = 0;
      status = "working";
      tempLastStart = millis();
      AR = analogRead(PIN_THERMISTER);
      Thermister_ESP8266();
      temperatureStart = T;
      if (tempLastStart==0) tempLastStart = 1;
      LastStopReason = "";
    }
}

void stop(){
    status = "stopped";
    analogWrite(PIN_HEATER, 0);
    if (!UseDisplay)
      digitalWrite(LED_BUILTIN , HIGH);
    tempLastStart = 0;
}

float readVoltage() {
  AR = analogRead(PIN_THERMISTER);
  return (AR * VCC) / 1023.0;
}

void initHotend() {
  status = "stopped";
  if (!UseDisplay){
    pinMode(LED_BUILTIN, OUTPUT);
  }
  pinMode(PIN_FILAMENT, INPUT);
  if (StartOnPower) start();

  if (To > maxT) To = workT;

  for (int i = 0; i < NUM_READINGS; i++) readings[i] = 0;

  for (int i = 0; i < NUM_READINGS; i++) {
    readVoltage();
    delay(10);
  } 
}

double control(){
  if (status == "stopped"){
    return 0;
  }
  if (isnan(T)){
    return 0;
  }
  if (T == 0){
    LastStopReason = "Anomalous temperature reading, something wrong with termistor";
    stop();
    return 0;
  }
  if(T >= To){
    return 0;
  }
  if(T < minT){
    return MaxGate;
  }
  return MaxGate/100*Gate;
}

void hotendReadTempTask() {
  if (millis() >= tempLastSample + 250) {
    total = total - readings[readIndex];
    float voltage = readVoltage();
    readings[readIndex] = voltage;
    total = total + readings[readIndex];
    readIndex = (readIndex + 1) % NUM_READINGS;
    average = total / NUM_READINGS;

    if (average > 0.05 && average < VCC - 0.05) {
      Thermister_ESP8266();
    }

    Output = control();
    analogWrite(PIN_HEATER, Output);

    if (status == "working"){
      if (T > minT) {
        if (!UseDisplay)
          digitalWrite(LED_BUILTIN, LOW);// target temperature ready
      } else {
        if (!UseDisplay)
          digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));//reaching tarjet temp
      }
    } else {
        if (!UseDisplay)
          digitalWrite(LED_BUILTIN, HIGH);
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
        Fworking = false;
      }

      if (Fcurrent && tempLastFilament > 0 && millis() >= tempLastFilament + 30*1000){
        Fworking = true;
      }
      
      if (!Fcurrent && Finsert && !Fworking && tempLastNoFilament > 0 && millis() >= tempLastNoFilament + 500) {
        LastStopReason = "Stop by user";
        stop();
        tempLastNoFilament = 0;
        Fworking = false;
        Finsert = false;
      }

      if (!Fcurrent && Fworking && tempLastNoFilament > 0 && millis() >= tempLastNoFilament + (Stopdelay + (25 - Vo) / 5 * 1.5) * 1000) {
        LastStopReason = "Run out";
        stop();
        tempLastNoFilament = 0;
        Fworking = false;
        Finsert = false;
      }
      
      if (!Fcurrent && !Finsert && tempLastStart > 0 && millis() >= tempLastStart + NoFilamentTime*60*1000) {
        LastStopReason = "No sensor detection for " + String(NoFilamentTime) + " min.";
        stop();
      }

      if (tempLastStart > 0 && millis() >= tempLastStart + Maxtime * 60 * 1000) {
        LastStopReason = "Max time reached: " + String(Maxtime) + " min.";
        stop();
      }
    }
    if (tempLastStart > 0 && T < To-20 && T-10 < temperatureStart && millis() >= tempLastStart + 30*1000 ) { 
        LastStopReason = "30s no heat: thermistor/heater issue";
        stop();
    }
  }
}