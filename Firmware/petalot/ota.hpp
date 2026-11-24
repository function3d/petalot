#pragma once

#include <ArduinoOTA.h>

void initOTA() {
  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    OTA_update = true;
    Serial.println("Start updating " + type);
    // Arduino IDE OTA bypasses the web UI, so capture the run state here too.
    if (ArduinoOTA.getCommand() == U_FLASH) {
      OtaResumeState = (status == "working") ? 1 : 2;
      OtaResumeElapsedMs = (status == "working" && tempLastStart > 0) ? (millis() - tempLastStart) : 0;
      statsDirty = true;
      saveStats();
      saveConfiguration(false);
    }
    stop();
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    // No reboot will happen: drop the resume flag so a later power cycle
    // starts with the normal StartOnPower setting.
    OtaResumeState = 0;
    OtaResumeElapsedMs = 0;
    saveConfiguration(false);
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}