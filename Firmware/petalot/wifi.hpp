#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <esp_mac.h>
  #include <ESPmDNS.h> 
#endif

IPAddress local_ip;

bool apmode = false;

double tempLastWifiTask;
double tempStartWifiTask;
bool wifiReady  =  false;
String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}

void AP(){
  apmode = true;
  WiFi.disconnect(true);
  IPAddress local_IP(192,168,4,1);
  IPAddress gateway(192,168,4,1);
  IPAddress subnet(255,255,255,0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  
  
  char APNAME[40];
  
  #if defined(ESP8266)
    unsigned char mac[6];
    WiFi.macAddress(mac);
  #elif defined(ESP32)
    uint8_t mac[6];
    esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
  #endif

  sprintf(APNAME, "PETALOT-%02X%02X%02X", mac[3], mac[4], mac[5]);
  if (WiFi.softAP(APNAME)) {
    Serial.print("AP Ready: ");
    Serial.println(APNAME);
    apmode = true;
  }else{
    Serial.println("AP Failed!");
  }
}

void wifiTask(){
  if  (!wifiReady){
    if (millis() >= tempLastWifiTask + 500){
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println(WiFi.localIP());
        wifiReady=true;
        return;
      }
      if (WiFi.status() == WL_CONNECT_FAILED) {
        AP();
        wifiReady=true;
        return;
      }
      if (millis() >= tempStartWifiTask + 10000){
        AP();
        wifiReady=true;
        return;
      }
      Serial.print(".");
      tempLastWifiTask = millis();
      
    }
  }
  if (wifiReady && !apmode) {
    MDNS.update();
  }
}

void initWiFi()
{
  if (strlen(ssid) == 0){
    AP();
    wifiReady=true;
    return;
  } else {
      IPAddress localip;
      localip.fromString(LocalIP.c_str());
      IPAddress subnet;
      subnet.fromString(Subnet.c_str());
      IPAddress gatewayip;
      gatewayip.fromString(Gateway.c_str());
      Serial.println("Connecting to ");
      Serial.print(ssid);
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);

      if (localip && !WiFi.config(localip, gatewayip, subnet?subnet:IPAddress(255, 255, 255, 0))) {
        Serial.println("config wifi ips failed");
        AP();
        wifiReady=true;
        return;
      }
      
      Serial.println("");
      if (!MDNS.begin("petalot")) {
        Serial.println("Error mDNS");
      } else {
        Serial.println("mDNS: http://petalot.local");
      }

      MDNS.addService("http", "tcp", 80);
          tempStartWifiTask = millis();
      }
}