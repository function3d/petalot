#include <ArduinoJson.h>
#include "FS.h"

  String msg;
  String status;
  double To;
  int Vo = 0;
  int Tm;
  double Kp;
  double Ki;
  double Kd;
  double Max;
  String LocalIP;
  String gateway;
  String Subnet;
  int R1;
  char ssid[64];
  char password[64];
  String ifttt_event_name = "petalot_stopped";
  String ifttt_api_key = "";
  
  StaticJsonDocument<512> doc;


const char *confFile = "/config.json";


String printConf() {
  String confString;
  serializeJson(doc, confString);
  return confString;
  
}

void loadConfiguration() {
  // Open file for reading
  File file = SPIFFS.open("/config.json", "r");

  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    msg = "Failed to read file, using default configuration";
  }

  strlcpy(ssid,                  
          doc["ssid"] | "",  
          sizeof(ssid));         

  strlcpy(password,                  
          doc["password"] | "",  
          sizeof(password));         

  To = doc["To"] | 220;
  //To = Tco;
  Vo = doc["Vo"] | 32;
  //Vo = Vco;
  Tm = doc["Tm"] | 250;
  Kp = doc["Kp"]?doc["Kp"].as<double>():23.0;
  Ki = doc["Ki"]?doc["Ki"].as<double>():0.043;
  Kd = doc["Kd"]?doc["Kd"].as<double>():160.0;
  Max = doc["Max"]?doc["Max"].as<double>():255;
  LocalIP = doc["LocalIP"] | "";
  Subnet = doc["Subnet"] | "255.255.255.0";
  gateway = doc["gateway"] | "";
  R1 = doc["R1"] | 10000;
  ifttt_event_name = doc["ifttt_event_name"] | "";
  ifttt_api_key = doc["ifttt_api_key"] | "";

  file.close();

}


void saveConfiguration() {

  File file = SPIFFS.open("/config.json", "w");
  if (!file) {
    msg = "Failed to create file";
    return;
  }

  doc["ssid"] = ssid;
  doc["password"] = password;
  doc["To"] = To;
  doc["Vo"] = Vo;
  doc["Tm"] = Tm;
  doc["Kp"] = Kp;
  doc["Ki"] = Ki;
  doc["Kd"] = Kd;
  doc["Max"] = Max;
  doc["LocalIP"] = LocalIP;
  doc["Subnet"] = Subnet;
  doc["gateway"] = gateway;
  doc["R1"] = R1;
  doc["ifttt_event_name"] = ifttt_event_name;
  doc["ifttt_api_key"] = ifttt_api_key;
  
  if (serializeJson(doc, file) == 0) {
    msg = "Failed to write to file";
  }

  file.close();

  
}


void initConf() {
  
  if (!SPIFFS.begin()) {
    msg = "Error mounting the file system";
    return;
  }

  loadConfiguration();
  

}
