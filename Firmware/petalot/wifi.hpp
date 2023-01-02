#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>

bool apmode = false;
IPAddress local_ip;



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
  
  unsigned char mac[6];
  char APNAME[40];
  WiFi.macAddress(mac);

  sprintf(APNAME, "PETALOT-%02X%02X%02X", mac[3], mac[4], mac[5]);
  if (WiFi.softAP(APNAME)) {
    Serial.println("AP Ready");
    apmode = true;
  }else{
    Serial.println("AP Failed!");
  }
}

void initWiFi()
{     
      
     if (!ssid){
        AP();
        //status = "stopped";
        return;
     } else {
      IPAddress localip;
      localip.fromString(LocalIP.c_str());
      IPAddress subnet;
      subnet.fromString(Subnet.c_str());
      IPAddress gatewayip;
      gatewayip.fromString(Gateway.c_str());

     WiFi.begin(ssid, password); //Conexión a la red
     if (!WiFi.config(localip, gatewayip, subnet,IPAddress(8, 8, 8, 8))) {
      Serial.println("config wifi ips failed");
     }
     unsigned long wifiConnectStart = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if (WiFi.status() == WL_CONNECT_FAILED) {
        AP();
        //status = "stopped";
        Serial.println("Error conectado a la  wifi");
        return;
      }
      if (millis() - wifiConnectStart > 10000) {
        AP();
        //status = "stopped";
        return;
      }
      delay(500);
      Serial.print(".");
    }
    if (!MDNS.begin("Petalot")) 
   {             
     Serial.println("Error iniciando mDNS");
   }
   //Serial.println("mDNS iniciado");
   Serial.println();
   Serial.println(WiFi.localIP());
   }
}

int ifttt(String value_1="", String value_2="", String value_3="")
{
  if (ifttt_event_name=="" || ifttt_api_key=="") return 0;
  WiFiClient client;
  HTTPClient http;
  http.begin(client, "http://maker.ifttt.com/trigger/" +
              ifttt_event_name+"/with/key/"+ifttt_api_key+
              "?value1="+value_1+"&value2="+value_2+"&value3="+value_3);
  int httpCode = http.GET();
  http.end();
  return httpCode;
}
