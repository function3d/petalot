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

// Fallback AP is not a dead end: with a home SSID configured, retry joining
// it every AP_RETRY_INTERVAL (only when nobody is using the AP, so an active
// configuration session is never kicked). The retry is non-blocking: the
// heater loop keeps running while the association is in flight.
bool apRetrying = false;
unsigned long apRetryStart = 0;
unsigned long apRetryLast = 0;
const unsigned long AP_RETRY_INTERVAL = 5 * 60 * 1000;
const unsigned long AP_RETRY_TIMEOUT = 15 * 1000;

// Boot is patient: a weak radio often needs several association attempts,
// so re-issue WiFi.begin() every STA_BEGIN_INTERVAL and only fall back to
// the AP after STA_BOOT_TIMEOUT. Once associated, the SDK auto-reconnects
// short drops in seconds without leaving STA mode.
const unsigned long STA_BOOT_TIMEOUT = 3 * 60 * 1000;
const unsigned long STA_BEGIN_INTERVAL = 15 * 1000;
unsigned long staBeginLast = 0;

// SSID the access point will use: AP_PREFIX + DeviceName, or the MAC tail when
// no name is set. Exposed to the web UI through /get so it shows the real name.
String apSsidName() {
  String apName = String(AP_PREFIX);
  if (DeviceName.length() > 0) {
    apName += DeviceName;
    return apName;
  }
  // No name set: fall back to the MAC tail so APs stay unique.
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
  char macSuffix[8];
  sprintf(macSuffix, "%02X%02X%02X", mac[3], mac[4], mac[5]);
  apName += macSuffix;
  return apName;
}

void AP() {
  apmode = true;
  WiFi.disconnect(true);

  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);

  String apName = apSsidName();

  const char *apPass = (APPassword.length() > 0) ? APPassword.c_str() : NULL;
  if (WiFi.softAP(apName.c_str(), apPass)) {
    Serial.print("AP Ready: ");
    Serial.print(apName);
    Serial.println(apPass ? " (password protected)" : " (open)");
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
      if (millis() >= tempStartWifiTask + STA_BOOT_TIMEOUT) {
        AP();
        wifiReady = true;
        return;
      }
      // Still within the boot window: keep trying. Re-issue begin every
      // interval (a new attempt often succeeds where the first one stalls),
      // sooner if the stack already gave up with a failure status.
      if (millis() - staBeginLast >= STA_BEGIN_INTERVAL ||
          (WiFi.status() == WL_CONNECT_FAILED && millis() - staBeginLast >= 3000)) {
        staBeginLast = millis();
        WiFi.disconnect();
        WiFi.begin(ssid, password);
      }
      Serial.print(".");
      tempLastWifiTask = millis();
    }
  }
  if (wifiReady && !apmode) {
    MDNS.update();
    return;
  }

  if (wifiReady && apmode && !apRetrying && strlen(ssid) > 0 &&
      millis() - apRetryLast >= AP_RETRY_INTERVAL &&
      WiFi.softAPgetStationNum() == 0 && !Update.isRunning()) {
    // Nobody on the fallback AP: drop it momentarily and try the home network.
    apRetrying = true;
    apRetryStart = millis();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    return;
  }

  if (wifiReady && apmode && apRetrying) {
    if (WiFi.status() == WL_CONNECTED) {
      apRetrying = false;
      apRetryLast = millis();
      apmode = false;
      tempLastWifiTask = millis();
      Serial.println();
      Serial.println(WiFi.localIP());
      if (!MDNS.begin("petalot")) {
        Serial.println("Error mDNS");
      } else {
        Serial.println("mDNS: http://petalot.local");
      }
      MDNS.addService("http", "tcp", 80);
      return;
    }
    if (millis() - apRetryStart >= AP_RETRY_TIMEOUT) {
      // Still no home network: back to the fallback AP.
      apRetrying = false;
      apRetryLast = millis();
      AP();
    }
    return;
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