const float VCC = 3.3;
const float R_SERIE = 2000.0;          // 1KΩ para rango 180-210°C
const float R_NTC_25 = 100000.0;
const float B_VALUE = 5540.0;  //3950.0;
const float T_0 = 298.15;

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

int temptable[52][2] = {
  { 1010, 255 },
  { 1005, 250 },
  { 996,  245 },
  { 991,  240 },
  { 982,  235 },
  { 973,  230 },
  { 961,  225 },
  { 950,  220 },
  { 934,  215 },
  { 923,  210 },
  { 911,  205 },
  { 895,  200 },
  { 879,  195 },
  { 858,  190 },
  { 840,  185 },
  { 815,  180 },
  { 794,  175 },
  { 775,  170 },
  { 760,  165 },
  { 735,  160 },
  { 703,  155 },
  { 670,  150 },
  { 642,  145 },
  { 606,  140 },
  { 569,  135 },
  { 541,  130 },
  { 500,  125 },
  { 462,  120 },
  { 423,  115 },
  { 386,  110 },
  { 350,  105 },
  { 310,  100 },
  { 280,  95 },
  { 253,  90 },
  { 219,  85 },
  { 198,  80 },
  { 168,  75 },
  { 144,  70 },
  { 120,  65 },
  { 103,  60 },
  { 87,   55 },
  { 74,   50 },
  { 61,   45 },
  { 51,   40 },
  { 43,   35 },
  { 31,   30 },
  { 29,   25 },
  { 27,   20 },
  { 17,   17 },
  { 14,   17 },
  { 10,   17 },
  { 5,    17 }
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
  if (AR == 0)
    T = -5;
}

void start(){
    if (tempLastStart==0){
      Fs = 0;
      Ts = 0;
      status = "working";
      tempLastStart = millis();
      #if defined(ESP8266)
      AR = analogRead(PIN_THERMISTER);
      Thermister_ESP8266();
      #endif
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
#if defined(ESP32)
  AR = analogRead(PIN_THERMISTER);
  return (AR * VCC) / 4095; 
#elif defined(ESP8266)
  AR = analogRead(PIN_THERMISTER);
  return (AR * VCC) / 1023.0;
#endif
}

void initHotend() {
  status = "stopped";
  if (!UseDisplay)
    pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_FILAMENT, INPUT);
  if (StartOnPower) start();

  if (To > Tm) To = 195;

  #if defined(ESP32)
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_THERMISTER, ADC_11db);
    pinMode(PIN_THERMISTER, INPUT);
  #endif

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
  if (T == -5){
    LastStopReason = "Anomalous temperature reading, something wrong with termistor.";
    stop();
    return 0;
  }
  if(T >= To){
    return 0;
  }
  if(T < 150){
    return Max;
  }
  return Max/100*Gate;
}

float getTempFromADC(float Vadc) {

  float r_ntc = R_SERIE * ((VCC / Vadc) - 1.0);

  // tabla mínima de corrección (puedes ampliarla luego)
  const int n = 5;
  float R[n] = {77500, 30000, 10000, 3000, 600};
  float T[n] = {30,    80,    120,   155,  180};

  // búsqueda
  for (int i = 0; i < n - 1; i++) {
    if (r_ntc <= R[i] && r_ntc >= R[i+1]) {

      float t = (r_ntc - R[i+1]) / (R[i] - R[i+1]);
      return T[i+1] + t * (T[i] - T[i+1]);
    }
  }

  return 0;
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
      #if defined(ESP8266)
      Thermister_ESP8266();
      #elif defined(ESP32)
      float r_ntc = R_SERIE * ((VCC / average) - 1.0);
      float steinhart = r_ntc / R_NTC_25;
      steinhart = log(steinhart);
      steinhart /= B_VALUE;
      steinhart += 1.0 / T_0;
      steinhart = 1.0 / steinhart;
      steinhart -= 273.15;
      T = steinhart;
      LastStopReason = average;
      #endif
    }

    Output = control();
    analogWrite(PIN_HEATER, Output);

    if (status == "working"){
      if (T > Tmi) {
        if (!UseDisplay)
          #if defined(ESP8266)
            digitalWrite(LED_BUILTIN, LOW);// target temperature ready
          #elif defined(ESP32)
            digitalWrite(LED_BUILTIN, HIGH);// target temperature ready
          #endif
      } else {
        if (!UseDisplay)
          digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));//reaching tarjet temp
      }
    } else {
        if (!UseDisplay)
          #if defined(ESP8266)
            digitalWrite(LED_BUILTIN, HIGH);
          #elif defined(ESP32)
            digitalWrite(LED_BUILTIN, LOW);
          #endif
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
        LastStopReason = "Stop by user.";
        stop();
        tempLastNoFilament = 0;
        Fworking = false;
        Finsert = false;
      }

      if (!Fcurrent && Fworking && tempLastNoFilament > 0 && millis() >= tempLastNoFilament + Stopdelay * 1000) {
        LastStopReason = "Run out.";
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
        LastStopReason = "30s no heat: thermistor/heater issue.";
        stop();
    }
  }
}