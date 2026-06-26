#include <ArduinoJson.h>
#include <LittleFS.h> 

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
  int Stopdelay = 14;
  int Maxtime = 120;
  int NoFilamentTime = 6;
  bool UseDisplay = 0;
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
  LittleFS.remove("/config.json");
  File file = LittleFS.open("/config.json", "w");
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
  doc["Stopdelay"] = Stopdelay;
  doc["Maxtime"]  = Maxtime;
  doc["NoFilamentTime"] = NoFilamentTime;
  doc["UseDisplay"] = UseDisplay;
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

void  resetConfiguration(){
    Serial.println("reset");
    strcpy(ssid, "");         
    strcpy(password, "");
    To = 195;
    Vo = 25;
    Fenable = true;
    Max = 255;
    LocalIP = "";
    Subnet = "";
    Gateway = "";
    R1 = 2000;
    Gate = 55;
    TOffset = -9;
    Stopdelay = 14;
    Maxtime = 120;
    NoFilamentTime = 6;
    UseDisplay = 0;
    StartOnPower = 1;
    MotorOnTo = 0;
    saveConfiguration(true);
}

void loadConfiguration(bool reset=false) {
    File file = LittleFS.open("/config.json", "r");
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

  To = doc["To"] | 195;
  Vo = doc["Vo"] | 25;
  Fenable = doc["Fenable"];
  Max = doc["Max"]?doc["Max"].as<double>():255;
  LocalIP = doc["LocalIP"] | "";
  Subnet = doc["Subnet"] | "";
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
    TOffset = -9;
    doc["TOffset"] = TOffset;
  }
  Stopdelay = doc["Stopdelay"] | 14;
  Maxtime = doc["Maxtime"] | 120;
  NoFilamentTime = doc["NoFilamentTime"] | 6;
  if (doc.containsKey("UseDisplay"))
    UseDisplay = doc["UseDisplay"];
  else {
    UseDisplay = 0;
    doc["UseDisplay"] = UseDisplay;
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
  Serial.println("Stopdelay:Stop Delay (s)");
  Serial.println("Maxtime:Max Time (min)");
  Serial.println("NoFilamentTime:Minutes to stop if no filament is detected");
  Serial.println("UseDisplay: Use OLED display");
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

 void factoryReset() {
  LittleFS.remove("/config.json");
  LittleFS.remove("/stats.json");
  analogWrite(PIN_HEATER, 0);
  ESP.restart();
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
      File file = LittleFS.open("/config.json", "w");
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

void listFiles() {
  Serial.println("------ ARCHIVOS EN SPIFFS ------");
  File root = LittleFS.open("/","r");
  File file = root.openNextFile();
  while (file) {
    Serial.print("  ");
    Serial.print(file.name());
    Serial.print("  -  ");
    Serial.print(file.size());
    Serial.println(" bytes");
    file = root.openNextFile();
  }
  Serial.println("--------------------------------");
}

void initConf() {
  
  #if defined(ESP8266)
    if (!LittleFS.begin()) {
      Serial.println("Error mounting the file system. Formating...");
      LittleFS.format(); 
      if (!LittleFS.begin()) {
        Serial.println("[ERROR] Error mounting the file system after format");
        return;
      }
    }
  #elif defined(ESP32)
    if (!LittleFS.begin(true)) {
      Serial.println("[Error mounting the file system");
      return;
    }
  #endif

  loadConfiguration();
  listFiles();
}
