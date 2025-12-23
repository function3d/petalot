#include <ArduinoJson.h>
#include "FS.h"

  String msg;
  String status;
  double To;
  int Vo = 0;
  bool Fenable = true; //filament sensor enabled
  double T;          //current temp
  int Gate;
  int TOffset = -9;
  bool MotorOnTo = 0;
  bool StartOnPower=1;
  int Tm = 210;
  int Tmi = 160;
  double Max;
  String LocalIP;
  String Gateway;
  String Subnet;
  int R1;
  char ssid[64];
  char password[64];
  String LastStopReason = "";
  
  StaticJsonDocument<512> doc;


const char *confFile = "/config.json";


String printConf() {
  String confString;
  StaticJsonDocument<512> conf;
  conf = doc;
  conf["version"] = version;
  conf["minT"] = Tmi;
  conf["maxT"] = Tm;
  serializeJson(conf, confString);
  return confString;
}

void saveConfiguration(bool reset=true) {
  SPIFFS.remove("/config.json");
  File file = SPIFFS.open("/config.json", "w");
  if (!file) {
    msg = "Failed to create file";
    return;
  }
  doc["To"] = To;
  doc["Vo"] = Vo;
  doc["Fenable"] = Fenable;
  doc["Max"] = Max;
  doc["ssid"] = ssid;
  doc["password"] = password;
  doc["LocalIP"] = LocalIP;
  doc["Subnet"] = Subnet;
  doc["Gateway"] = Gateway;
  doc["R1"] = R1;
  doc["Gate"] = Gate;
  doc["TOffset"] = TOffset;
  doc["StartOnPower"] = StartOnPower;
  doc["MotorOnTo"] = MotorOnTo;
  if (serializeJson(doc, file) == 0) {
    msg = "Failed to write to file";
  }
  Serial.println(printConf());
  file.close();
  if (reset) {
    analogWrite(PIN_HEATER, 0);
    ESP.restart();
  }
}

 void factoryReset() {
  SPIFFS.remove("/config.json");
  SPIFFS.remove("/stats.json");
  analogWrite(PIN_HEATER, 0);
  ESP.restart();
  } 	


void  resetConfiguration(){
    Serial.println("reset");
    strcpy(ssid, "");         
    strcpy(password, "");
    To = 185;
    Vo = 50;
    Fenable = true;
    Max = 255;
    LocalIP = "";
    Subnet = "255.255.255.0";
    Gateway = "";
    R1 = 2000;
    Gate = 55;
    //TOffset = -9;
    StartOnPower = 1;
    MotorOnTo = 0;
    saveConfiguration(true);
}

void readConfigurationSerial(){
  StaticJsonDocument<512> docInput;
  
  if (Serial.available() > 0)
  {
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(docInput, Serial);
    if (error)
    {
      return;
    } else {
      doc=docInput;
      File file = SPIFFS.open("/config.json", "w");
      if (!file) {
        msg = "Failed to create file";
        return;
      }
      if (serializeJson(doc, file) == 0) {
        msg = "Failed to write to file";
      }
      file.close();
      Serial.println("Configuration updated, restarting...");
      analogWrite(PIN_HEATER, 0);
      ESP.restart();
    }
  }
}

void loadConfiguration(bool reset=false) {
    File file = SPIFFS.open("/config.json", "r");
     if (!file) {
      msg = "Failed to open /config.json";
      Serial.println("Failed to open /config.json");
      resetConfiguration();
    }
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
      msg = "Failed to read file, using default configuration";
      Serial.println("Failed to read file, using default configuration");
      resetConfiguration();
      return;
    }
    file.close();

  strlcpy(ssid,                  
          doc["ssid"],  
          sizeof(ssid));         

  strlcpy(password,                  
          doc["password"],  
          sizeof(password));         

  To = doc["To"] | 185;
  Vo = doc["Vo"] | 50;
  Fenable = doc["Fenable"];
  Max = doc["Max"]?doc["Max"].as<double>():255;
  LocalIP = doc["LocalIP"] | "";
  Subnet = doc["Subnet"] | "255.255.255.0";
  Gateway = doc["Gateway"] | "";
  R1 = doc["R1"] | 2000;
  if (doc.containsKey("Gate"))
    Gate = doc["Gate"];
  else {
    Gate = 55;
    doc["Gate"] = Gate;
  }
  if (doc.containsKey("TOffset"))
    TOffset= doc["TOffset"];
  else {
    TOffset = -5;
    doc["TOffset"] = TOffset;
  }
  if (doc.containsKey("StartOnPower"))
    StartOnPower = doc["StartOnPower"];
  else {
    StartOnPower = 1;
    doc["StartOnPower"] = StartOnPower;
  }
  if (doc.containsKey("MotorOnTo"))
    MotorOnTo = doc["MotorOnTo"];
  else {
    MotorOnTo = 0;
    doc["MotorOnTo"] = MotorOnTo;
  }
  Serial.println();
  Serial.println("To:Temperature");
  Serial.println("Vo:Speed");
  Serial.println("Fenable:Filament enabled");
  Serial.println("R1:R1");
  Serial.println("Gate:Gate %");
  Serial.println("TOffset:Temperature Offset");
  Serial.print("StartOnPower: Start up at power on (");
  Serial.print(StartOnPower);
  Serial.print(")");
  Serial.println();
  Serial.print("MotorOnTo: Motor starting at target temperature (");
  Serial.print(MotorOnTo);
  Serial.print(")");
  Serial.println();
  Serial.println("Max:Maximum value for MOSFET (0-255)");
  Serial.println("ssid:SSID");
  Serial.println("password:SSID Password");
  Serial.println("LocalIP:IP address");
  Serial.println(printConf());
}

void initConf() {
  
  if (!SPIFFS.begin()) {
    msg = "Error mounting the file system";
    return;
  }

  loadConfiguration();
  

}
