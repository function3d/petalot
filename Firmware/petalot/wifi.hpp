#pragma once

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <esp_mac.h>
  #include <ESPmDNS.h>
#endif

// ============================================================================
// WIFI + MDNS
// ============================================================================
bool apmode = false;

unsigned long tempLastWifiTask;
unsigned long tempStartWifiTask;
bool wifiReady = false;

void AP() {
  apmode = true;
  WiFi.disconnect(true);

  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);

  char APNAME[40];

#if defined(ESP8266)
  unsigned char mac[6];
  WiFi.macAddress(mac);
#elif defined(ESP32)
  uint8_t mac[6];
  esp_err_t err = esp_read_mac(mac, ESP_MAC_WIFI_STA);
  if (err != ESP_OK) {
    Serial.println("Failed to read MAC address");
  }
#endif

  sprintf(APNAME, "PETALOT-%02X%02X%02X", mac[3], mac[4], mac[5]);
  if (WiFi.softAP(APNAME)) {
    Serial.print("AP Ready: ");
    Serial.println(APNAME);
    apmode = true;
  } else {
    Serial.println("AP Failed!");
  }
}

void wifiTask() {
  if (!wifiReady) {
    if (millis() >= tempLastWifiTask + 500) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println(WiFi.localIP());
        wifiReady = true;
        return;
      }
      if (WiFi.status() == WL_CONNECT_FAILED) {
        AP();
        wifiReady = true;
        return;
      }
      if (millis() >= tempStartWifiTask + 10000) {
        AP();
        wifiReady = true;
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

void initWiFi() {
  if (strlen(ssid) == 0) {
    AP();
    wifiReady = true;
    return;
  }

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

  // With a static IP the DHCP client is stopped, so DNS must be provided
  // explicitly or hostname lookups (online update) fail. Try the gateway first
  // (local/hosts names) and fall back to a public resolver.
  IPAddress dns1 = gatewayip.isSet() ? gatewayip : IPAddress(1, 1, 1, 1);
  IPAddress dns2 = (dns1 == IPAddress(1, 1, 1, 1)) ? IPAddress(8, 8, 8, 8) : IPAddress(1, 1, 1, 1);

  if (localip && !WiFi.config(localip, gatewayip, subnet ? subnet : IPAddress(255, 255, 255, 0), dns1, dns2)) {
    Serial.println("config wifi ips failed");
    AP();
    wifiReady = true;
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