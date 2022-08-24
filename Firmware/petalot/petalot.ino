#include "conf.hpp"
#include "pins.hpp"
#include "wifi.hpp"
#include "stepper.hpp"
#include "hotend.hpp"
#include "server.hpp"
#include "ota.hpp"

void setup() {
  
  
  Serial.begin(115200);
  delay(1000);
  initConf();
  initWiFi();
  initOTA();
  initHotend();
  initStepper();
  InitServer();
}

void loop() {
  server.handleClient();
  if (!apmode) {
    hotendReadTempTask();
    stepperRunTask();
  }
  ArduinoOTA.handle();

}
