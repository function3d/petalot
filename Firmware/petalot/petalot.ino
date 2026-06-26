#define VERSION 1405 // See pins.hpp

// Year: Extracts '2' and '6' from '2026'
#define BUILD_YEAR_DEC __DATE__[9]
#define BUILD_YEAR_UNI __DATE__[10]

// Day: Extracts tens and units (converts leading space to '0' if day < 10)
#define BUILD_DAY_DEC (__DATE__[4] == ' ' ? '0' : __DATE__[4])
#define BUILD_DAY_UNI __DATE__[5]

// Month: Compares characters to assign the corresponding two-digit string
#define BUILD_MONTH_DEC (__DATE__[0] == 'J' && __DATE__[1] == 'a' ? '0' : (__DATE__[0] == 'F' ? '0' : (__DATE__[0] == 'M' && __DATE__[2] == 'r' ? '0' : (__DATE__[0] == 'A' && __DATE__[1] == 'p' ? '0' : (__DATE__[0] == 'M' && __DATE__[2] == 'y' ? '0' : (__DATE__[0] == 'J' && __DATE__[2] == 'n' ? '0' : (__DATE__[0] == 'J' && __DATE__[2] == 'l' ? '0' : (__DATE__[0] == 'A' && __DATE__[1] == 'u' ? '0' : (__DATE__[0] == 'S' ? '0' : (__DATE__[0] == 'O' ? '1' : (__DATE__[0] == 'N' ? '1' : (__DATE__[0] == 'D' ? '1' : '0'))))))))))))
#define BUILD_MONTH_UNI (__DATE__[0] == 'J' && __DATE__[1] == 'a' ? '1' : (__DATE__[0] == 'F' ? '2' : (__DATE__[0] == 'M' && __DATE__[2] == 'r' ? '3' : (__DATE__[0] == 'A' && __DATE__[1] == 'p' ? '4' : (__DATE__[0] == 'M' && __DATE__[2] == 'y' ? '5' : (__DATE__[0] == 'J' && __DATE__[2] == 'n' ? '6' : (__DATE__[0] == 'J' && __DATE__[2] == 'l' ? '7' : (__DATE__[0] == 'A' && __DATE__[1] == 'u' ? '8' : (__DATE__[0] == 'S' ? '9' : (__DATE__[0] == 'O' ? '0' : (__DATE__[0] == 'N' ? '1' : (__DATE__[0] == 'D' ? '2' : '0'))))))))))))

// Compile-time concatenation into a single Arduino String object (e.g., "260624")
#define BUILD_VERSION (String("") + BUILD_YEAR_DEC + BUILD_YEAR_UNI + BUILD_MONTH_DEC + BUILD_MONTH_UNI + BUILD_DAY_DEC + BUILD_DAY_UNI)

int major = VERSION / 1000;
int minor = (VERSION % 1000) / 100;
int patch = VERSION % 100;
String version = String("") + major + "." + minor + "." + patch + "." + BUILD_VERSION;

double Ft = 0; //filament total
double Tt = 0; //time total
double Fs = 0; //filament total session
double Ts = 0; //time total session

bool OTA_update = false;

double tempLastStats;
double tempLastStatsSave;

#include "pins.hpp"
#include "conf.hpp"
#include "wifi.hpp"
#include "stepper.hpp"
#include "hotend.hpp"
#include "display.hpp"
#include "server.hpp"
#include "ota.hpp"

StaticJsonDocument<128> stats;

void setup() {
  Serial.begin(115200);
  delay(1500);
  initConf();
  initWiFi();
  initOTA();
  initHotend();
  stepper.init();
  InitServer();
  desbloquearLcdI2C();
  if (UseDisplay){
    initDisplay();
  }
  tempLastStats = millis();
  tempLastStatsSave = millis();
  File file = LittleFS.open("/stats.json", "r");
  DeserializationError error = deserializeJson(stats, file);
  if (error)
  {
      //Serial.println(F("deserializeJson() failed: "));
      //Serial.println(error.c_str());
  } else {
    Ft  = stats["Ft"]?stats["Ft"].as<double>():0.0;
    Tt  = stats["Tt"]?stats["Tt"].as<double>():0.0;
  }
  file.close();
}

void loop() {
  ArduinoOTA.handle();
  if (!OTA_update) {
    wifiTask();
    serverTask();
    hotendReadTempTask();
    stepper.task();
    readConfigurationSerial();
    if (UseDisplay)
      displayTask();
    if ((F || !Fenable) && status=="working" && millis() >= tempLastStats + 1000) {
      Fs = Fs + (float)Vo/60; //cm in 1 sec
      Ft = Ft + (float)Vo/60;
      Tt = Tt + 1;
      Ts = Ts + 1;
      if (millis() >= tempLastStatsSave + 10000) {
        File file = LittleFS.open("/stats.json", "w");
        if (!file) {
          msg = "Failed to create file";
        }
        stats["Ft"] = Ft;
        stats["Tt"] = Tt;
        if (serializeJson(stats, file) == 0) {
          msg = "Failed to write to file";
        }
        file.close();
      }
      tempLastStats = millis();
      tempLastStatsSave = millis();
    }
  }
}


