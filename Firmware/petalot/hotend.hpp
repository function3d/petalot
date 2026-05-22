#include <math.h>

bool F = false;
bool Fcurrent = false;
bool Finsert = false;
bool Fworking = false;

double Output;  //pid output

double tempLastSample;
double tempLastFilament;
double tempLastNoFilament;
double tempLastStart;
double tempLastFilamentCheck;
double temperatureStart;


short AR;

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
{ 87,  55 },
{ 74,  50 },
{ 61,  45 },
{ 51,  40 },
{ 43,  35 },
{ 31,  30 },
{ 29,  25 },
{ 27,  20 },
{ 17,  17 },
{ 14,  17 },
{ 10,  17 },
{ 5,   17 }
};

double Thermister(int Volts) {
  AR = Volts;
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
  return T;
}

void start(){
    if (tempLastStart==0){
      Fs = 0;
      Ts = 0;
      status = "working";
      tempLastStart = millis();
      Thermister(analogRead(PIN_THERMISTER));
      temperatureStart = T;
      if (tempLastStart==0) tempLastStart = 1;
      LastStopReason = "";
    }
}

void stop(){
    status = "stopped";
    analogWrite(PIN_HEATER, 0);
    digitalWrite(LED_BUILTIN , HIGH);
    tempLastStart = 0;
}

void initHotend(){
  status = "stopped";
  pinMode(LED_BUILTIN , OUTPUT);
  pinMode(PIN_FILAMENT , INPUT);
  if (StartOnPower) start();
  
  

  //Tm  = 240; //temptable[0][1];
  if (To > Tm) To = 185;
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
  if(T < 100){
    return 255;
  }
  if(T < 150){
    return Max;
  }
  return Max/100*Gate;

  
}


void hotendReadTempTask() {

  if (millis() >= tempLastSample + 250) //250 is 4 times per second
  {
    Thermister(analogRead(PIN_THERMISTER)); //Volt to temp, update T
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
        Fworking = false;
      }

      if (Fcurrent && tempLastFilament > 0 && millis() >= tempLastFilament + 30*1000){
        Fworking = true;
      }
      
      if (!Fcurrent && Finsert && !Fworking && tempLastNoFilament > 0 && millis() >= tempLastNoFilament + 500) {
        LastStopReason = "Filament stop by user.";
        stop();
        tempLastNoFilament = 0;
        Fworking = false;
        Finsert = false;
      }

      if (!Fcurrent && Fworking && tempLastNoFilament > 0 && millis() >= tempLastNoFilament + Stopdelay * 1000) {
        LastStopReason = "Filament run out.";
        stop();
        tempLastNoFilament = 0;
        Fworking = false;
        Finsert = false;
      }
      
      if (!Fcurrent && !Finsert && tempLastStart > 0 && millis() >= tempLastStart + NoFilamentTime*60*1000) {
        LastStopReason = "The filament sensor did not detect filament for " + String(NoFilamentTime) + " minutes.";
        stop();
      }

      if (tempLastStart > 0 && millis() >= tempLastStart + Maxtime * 60 * 1000) {
        LastStopReason = "The machine has been working for " + String(Maxtime) + " minutes (Max Time reached).";
        stop();
      }
    }
    if (tempLastStart > 0 && T < To-20 && T-10 < temperatureStart && millis() >= tempLastStart + 30*1000 ) { 
        LastStopReason = "30 seconds without temperature increase, something wrong with termistor o heater.";
        stop();
    }

  }
}
