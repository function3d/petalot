double Ft = 0; //filament total
double Tt = 0; //filament total
double Fs = 0; //filament total session
double Ts = 0; //filament total session

double tempLastStats;

#include "pins.hpp"
#include "conf.hpp"
#include "wifi.hpp"
#include "stepper.hpp"
#include "hotend.hpp"
#include "server.hpp"
#include "ota.hpp"

 StaticJsonDocument<128> stats;

void setup() {
  Serial.begin(115200);
  delay(1000);
  initConf();
  initWiFi();
  initOTA();
  initHotend();
  initStepper();
  InitServer();
  tempLastStats = millis();
  File file = SPIFFS.open("/stats.json", "r");
  DeserializationError error = deserializeJson(stats, file);
  if (error)
  {
      Serial.println(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
  } else {
    Ft  = stats["Ft"]?stats["Ft"].as<double>():0.0;
    Tt  = stats["Tt"]?stats["Tt"].as<double>():0.0;
  }
  file.close();
}

void loop() {
  wifiTask();
  server.handleClient();
  hotendReadTempTask();
  stepperRunTask();
  ArduinoOTA.handle();
  readConfigurationSerial();
  if ((F || !Fe) && status=="working" && millis() >= tempLastStats + 5000) {
    Fs = Fs + (float)Vo/2/62*5;
    Ft = Ft + (float)Vo/2/62*5; // centimetros hechos cada 5000 milisegundos
    Tt = Tt + 5;
    Ts = Ts + 5;
    File file = SPIFFS.open("/stats.json", "w");
    if (!file) {
      msg = "Failed to create file";
    }
    stats["Ft"] = Ft;
    stats["Tt"] = Tt;
    if (serializeJson(stats, file) == 0) {
      msg = "Failed to write to file";
    }
    file.close();
    tempLastStats = millis();
  }
}


