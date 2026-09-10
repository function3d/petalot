#pragma once

#include <ArduinoJson.h>
#include <LittleFS.h>

String msg;
String status;
double To;
int Vo = 0;
bool Fenable = true;  //filament sensor enabled
double T;             //current temp
int Gate;
int MaxGate;
double HYS = 0.5;   // deadband below To before reheating (°C)
double RAMP = 6;    // approach ramp window above the deadband (°C)
double HOLD = 28;   // minimum hold duty (%) near the target so To is reachable
int TOffset = 0;
bool MotorOnTo = 0;
bool StartOnPower = 1;
int maxT = 220;
int minT = 180;
int workT = 210;
int maxV = 35;
int minV = 5;
int workV = 25;
String LocalIP;
String Gateway;
int Stopdelay = 14;
int Maxtime = 120;
int NoFilamentTime = 6;
bool UseDisplay = 0;
String Subnet;
char ssid[64];
char password[64];
String LastStopReason = "";
int pcbVer = 0; // Registered PCB version (locked at first boot)

// Embedded marker so /updatecheck can identify the PCB version of a binary
// without writing it to flash. Kept in the image even with --gc-sections.
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
const char PCB_MAGIC[] PROGMEM __attribute__((used)) = "PETALOT-PCB-" STRINGIFY(VERSION);

StaticJsonDocument<512> doc;

String printConf(bool plus = true) {
  String confString;
  StaticJsonDocument<512> conf;
  conf = doc;
  if (plus) {
    conf["version"] = version;
    conf["minT"] = minT;
    conf["maxT"] = maxT;
    conf["minV"] = minV;
    conf["maxV"] = maxV;
  } else {
    conf.remove("version");
    conf.remove("minT");
    conf.remove("maxT");
    conf.remove("minV");
    conf.remove("maxV");
  }
  serializeJson(conf, confString);
  return confString;
}

void saveConfiguration(bool reset = true) {
  LittleFS.remove("/config.json");
  File file = LittleFS.open("/config.json", "w");
  if (!file) {
    msg = "Failed to create file";
    return;
  }
  doc["To"] = To;
  doc["Vo"] = Vo;
  doc["Fenable"] = Fenable;
  doc["ssid"] = ssid;
  doc["password"] = password;
  doc["LocalIP"] = LocalIP;
  doc["Subnet"] = Subnet;
  doc["Gateway"] = Gateway;
  doc["Gate"] = Gate;
  doc["MaxGate"] = MaxGate;
  doc["HYS"] = HYS;
  doc["RAMP"] = RAMP;
  doc["HOLD"] = HOLD;
  doc["TOffset"] = TOffset;
  doc["Stopdelay"] = Stopdelay;
  doc["Maxtime"] = Maxtime;
  doc["NoFilamentTime"] = NoFilamentTime;
  doc["UseDisplay"] = UseDisplay;
  doc["StartOnPower"] = StartOnPower;
  doc["MotorOnTo"] = MotorOnTo;
  if (pcbVer > 0) doc["pcbVer"] = pcbVer;
  if (serializeJson(doc, file) == 0) {
    msg = "Failed to write to file";
  }
  Serial.println(printConf(false));
  file.close();
  if (reset) {
    analogWrite(PIN_HEATER, 0);
    ESP.restart();
  }
}

void resetConfiguration() {
  Serial.println("reset");
  strcpy(ssid, "");
  strcpy(password, "");
  To = workT;
  Vo = workV;
  Fenable = true;
  LocalIP = "";
  Subnet = "";
  Gateway = "";
  Gate = 50;
  MaxGate = 255;
  HYS = 1.5;
  RAMP = 6;
  HOLD = 25;
  TOffset = 0;
  Stopdelay = 14;
  Maxtime = 120;
  NoFilamentTime = 6;
  UseDisplay = 0;
  StartOnPower = 1;
  MotorOnTo = 0;
}

void loadConfiguration(bool reset = false) {
  File file = LittleFS.open("/config.json", "r");
  if (!file) {
    msg = "Failed to open /config.json";
    Serial.println("Failed to open /config.json");
    resetConfiguration();
    saveConfiguration(false);
    return;
  }
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) {
    msg = "Failed to read file, using default configuration";
    Serial.println("Failed to read file, using default configuration");
    resetConfiguration();
    saveConfiguration(false);
    return;
  }

  strlcpy(ssid,
          doc["ssid"],
          sizeof(ssid));

  strlcpy(password,
          doc["password"],
          sizeof(password));

  To = doc["To"] | workT;
  Vo = doc["Vo"] | workV;
  Fenable = doc["Fenable"];
  LocalIP = doc["LocalIP"] | "";
  Subnet = doc["Subnet"] | "";
  Gateway = doc["Gateway"] | "";
  if (doc.containsKey("Gate"))
    Gate = doc["Gate"];
  else {
    Gate = 50;
    doc["Gate"] = Gate;
  }
  if (doc.containsKey("MaxGate"))
    MaxGate = doc["MaxGate"];
  else {
    MaxGate = 255;
    doc["MaxGate"] = MaxGate;
  }
  if (doc.containsKey("HYS"))
    HYS = doc["HYS"];
  else {
    HYS = 1.5;
    doc["HYS"] = HYS;
  }
  if (doc.containsKey("RAMP"))
    RAMP = doc["RAMP"];
  else {
    RAMP = 6;
    doc["RAMP"] = RAMP;
  }
  if (doc.containsKey("HOLD"))
    HOLD = doc["HOLD"];
  else {
    HOLD = 25;
    doc["HOLD"] = HOLD;
  }
  if (HYS < 0) HYS = 0;
  if (RAMP < 1) RAMP = 1;
  if (HOLD < 0) HOLD = 0;
  if (HOLD > 100) HOLD = 100;
  
  if (doc.containsKey("TOffset"))
    TOffset = doc["TOffset"];
  else {
    TOffset = 0;
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
  if (doc.containsKey("pcbVer"))
    pcbVer = doc["pcbVer"];
  else
    pcbVer = 0;
}

void factoryReset(bool stats = false) {
  analogWrite(PIN_HEATER, 0);
  int TOffset_old = TOffset;
  int workT_old = workT;
  resetConfiguration();
  TOffset = TOffset_old;
  workT = workT_old;
  saveConfiguration(false);
  if (stats) {
    LittleFS.remove("/stats.json");
    ESP.restart();
  }
}

void readConfigurationSerial() {
  if (Serial.available() > 0) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) {
      return;
    }

    if (line.equalsIgnoreCase("conf")) {
      Serial.println();
      Serial.println("To: Temperature");
      Serial.println("Vo: Speed");
      Serial.println("Fenable: Filament enabled");
      Serial.println("Gate: Target approach gate drive percentage relative to MaxGate");
      Serial.println("MaxGate: Maximum MOSFET gate drive limit (0-255)");
      Serial.println("HYS: Deadband below target temp before reheating (deg C)");
      Serial.println("RAMP: Approach ramp window above the deadband (deg C)");
      Serial.println("HOLD: Minimum hold duty near the target so To is reachable (%)");
      Serial.println("TOffset: Temperature Offset");
      Serial.println("Stopdelay: Stop Delay (s)");
      Serial.println("Maxtime: Max Time (min)");
      Serial.println("NoFilamentTime: Minutes to stop if no filament is detected");
      Serial.println("UseDisplay: Use OLED display");
      Serial.println("StartOnPower: Start up at power on");
      Serial.println("MotorOnTo: Motor starting at target temperature");
      Serial.println("ssid: SSID");
      Serial.println("password: SSID Password");
      Serial.println("LocalIP: IP address");
      Serial.println("Gateway: Gateway address");
      Serial.println("Subnet: Subnet mask");
      Serial.println("pcbVer: Registered PCB version (locked at first boot)");
      Serial.println(printConf(true));
      return;
    }

    if (!line.startsWith("{")) {
      Serial.println("Unknown command. Type 'conf' to dump config or send a JSON config");
      return;
    }

    StaticJsonDocument<512> docInput;
    DeserializationError error = deserializeJson(docInput, line);
    if (error) {
      Serial.println("Invalid JSON");
      return;
    }
    doc = docInput;
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

void listFiles() {
  Serial.println("------ FILES IN LITTLEFS ------");
  File root = LittleFS.open("/", "r");
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

  if (pcbVer == 0) {
    Serial.print("[INFO] First boot: locking PCB version to ");
    Serial.println(VERSION);
    pcbVer = VERSION;
    doc["pcbVer"] = pcbVer;
    saveConfiguration(false);
  }
  //listFiles();
  Serial.println();
  Serial.println("Type 'conf' to dump config or send a JSON config");
}
