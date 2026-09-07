#pragma once

// ============================================================================
// INCLUDES (ESP32 / ESP8266 compatible)
// ============================================================================
#ifdef ESP32
  #include <AsyncTCP.h>
#elif defined(ESP8266)
  #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <ESPAsyncHTTPUpdateServer.h>

#include "web_ui.h"

// ============================================================================
// GLOBAL SERVER OBJECTS
// ============================================================================
AsyncWebServer server(80);
ESPAsyncHTTPUpdateServer httpUpdater;

// ============================================================================
// OPTIONAL WEB AUTH
// Define WEB_USER/WEB_PASSWORD before including this file to require a login
// on /set, /reset and /update. Empty password = auth disabled (default).
// ============================================================================
#ifndef WEB_USER
  #define WEB_USER "admin"
#endif
#ifndef WEB_PASSWORD
  #define WEB_PASSWORD ""
#endif

static bool isAuthorized(AsyncWebServerRequest *request) {
  if (WEB_PASSWORD[0] == '\0') return true;
  return request->authenticate(WEB_USER, WEB_PASSWORD);
}

// ============================================================================
// HANDLERS
// ============================================================================
void handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void tele(AsyncWebServerRequest *request) {
  StaticJsonDocument<384> teleData;
  teleData["status"]        = (status == "working") ? ((stepper.motorEnabled) ? 2 : 1) : 0;
  teleData["T"]             = T;
  teleData["AR"]            = AR;
  teleData["To"]            = To;
  teleData["Vo"]            = Vo;
  teleData["F"]             = F;
  teleData["Fenable"]       = Fenable;
  teleData["Ft"]            = Ft;
  teleData["Fs"]            = Fs;
  teleData["Tt"]            = Tt;
  teleData["Ts"]            = Ts;
  teleData["LastStopReason"] = LastStopReason;
  teleData["Output"]        = String(map((int)Output, 0, 255, 0, 100)) + "%";
  String r;
  serializeJson(teleData, r);
  request->send(200, "application/json", r);
}

void get(AsyncWebServerRequest *request) {
  request->send(200, "application/json", printConf());
}

void reset(AsyncWebServerRequest *request) {
  if (!isAuthorized(request)) {
    request->requestAuthentication();
    return;
  }
  bool wipeStats = request->arg("stats") == "1";
  request->redirect("/");
  wipeStats ? factoryReset(true) : factoryReset();
}

void set(AsyncWebServerRequest *request) {
  if (!isAuthorized(request)) {
    request->requestAuthentication();
    return;
  }

  // Single +/- actions keep the live telemetry response
  String ToChange = request->arg("To");
  if (ToChange != "") {
    if (ToChange.toInt() + To <= maxT && ToChange.toInt() + To >= minT) {
      To += ToChange.toInt();
      saveConfiguration(false);
    }
    tele(request);
    return;
  }

  String VoChange = request->arg("Vo");
  if (VoChange != "") {
    if (VoChange.toInt() + Vo <= maxV && VoChange.toInt() + Vo >= minV) {
      Vo += VoChange.toInt();
      saveConfiguration(false);
    }
    tele(request);
    return;
  }

  String statusChange = request->arg("status");
  if (statusChange != "") {
    if (statusChange.toFloat() || statusChange == "1" || statusChange == "true") start();
    else stop();
    saveConfiguration(false);
    tele(request);
    return;
  }

  String FeChange = request->arg("Fenable");
  if (FeChange != "") {
    Fenable = (FeChange.toFloat() || FeChange == "true");
    saveConfiguration(false);
    tele(request);
    return;
  }

  // Bulk update of settings form fields
  if (request->hasArg("Gate")) Gate = request->arg("Gate").toInt();
  if (request->hasArg("TOffset")) TOffset = request->arg("TOffset").toInt();

  if (request->hasArg("StartOnPower")) StartOnPower = (request->arg("StartOnPower") == "true");
  if (request->hasArg("MotorOnTo")) MotorOnTo = (request->arg("MotorOnTo") == "true");
  if (request->hasArg("UseDisplay")) UseDisplay = (request->arg("UseDisplay") == "true");
  if (request->hasArg("UseDisplay")) displayInitialized = false;

  if (request->hasArg("ssid")) request->arg("ssid").toCharArray(ssid, sizeof(ssid));
  if (request->hasArg("password")) request->arg("password").toCharArray(password, sizeof(password));
  if (request->hasArg("LocalIP")) LocalIP = request->arg("LocalIP");
  if (request->hasArg("Subnet")) Subnet = request->arg("Subnet");
  if (request->hasArg("Gateway")) Gateway = request->arg("Gateway");
  if (request->hasArg("Stopdelay")) Stopdelay = request->arg("Stopdelay").toInt();
  if (request->hasArg("Maxtime")) Maxtime = request->arg("Maxtime").toInt();
  if (request->hasArg("NoFilamentTime")) NoFilamentTime = request->arg("NoFilamentTime").toInt();

  if (request->arg("reboot") == "1") {
    saveConfiguration(true);
  } else {
    saveConfiguration(false);
    loadConfiguration();
  }
  get(request);
}

void handleRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", INDEX_HTML);
}

// ============================================================================
// SERVER INIT
// ============================================================================
void InitServer() {
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/get", HTTP_GET, get);
  server.on("/tele", HTTP_GET, tele);
  server.on("/set", HTTP_GET, set);
  server.on("/reset", HTTP_GET, reset);

  server.onNotFound(handleNotFound);

  httpUpdater.setup(&server, WEB_USER, WEB_PASSWORD);
  server.begin();
}

void serverTask() {}