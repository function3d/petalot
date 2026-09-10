# Installing the Libraries to Compile the PETALOT Firmware

This guide explains how to set up the Arduino IDE so you can compile this
firmware for the **LOLIN (WEMOS) D1 mini**.

## 1. Add the ESP8266 board support

1. Open the Arduino IDE.
2. Go to **File > Preferences** (labeled **File > Settings** in some versions).
3. In the **Additional Boards Manager URLs** field, paste:

   ```
   https://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```

4. Click **OK** to save your preferences.
5. Go to **Tools > Board > Boards Manager**.
6. Type `esp8266` in the search box, locate **esp8266 by ESP8266 Community** and click **Install**.
7. Select the board: **Tools > Board > esp8266 > LOLIN(WEMOS) D1 mini (clone)**.

## 2. Install the required libraries

Go to **Sketch > Include Library > Manage Libraries...** and install each of the
following using the exact names shown. The Libraries Manager filter must be set
to **All** (or **Updatable** if you are reinstalling).

| Library                     | Purpose                                        |
| --------------------------- | ---------------------------------------------- |
| ArduinoJson                 | Configuration files (`.json`)                  |
| AccelStepper                | Stepper motor control                          |
| Adafruit SSD1306            | OLED display support (optional hardware)       |
| ESPAsyncTCP                 | Networking layer required by ESPAsyncWebServer |
| ESPAsyncWebServer           | Embedded web server                            |
| ESPAsyncHTTPUpdateServer (2.0.0) | Over-the-air (OTA) firmware updates        |

> **Note:** `ESPAsyncTCP` is a dependency of `ESPAsyncWebServer`; install it
> first. `ESPAsyncHTTPUpdateServer` provides the OTA update endpoint used from
> the *Settings* page. The built-in libraries used by the firmware
> (`ArduinoOTA`, `ESP8266WiFi`, `ESP8266mDNS`, `LittleFS`, `Wire`) are included
> with the ESP8266 board package installed in step 1, so you do not need to
> install them separately.

After the libraries are installed, restart the Arduino IDE and reopen the
project before compiling.