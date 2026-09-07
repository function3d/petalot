#pragma once

#if defined(ESP8266) && VERSION == 1405 // D1 mini

  #define PIN_EN    D1
  #define PIN_STEP  D2
  #define PIN_DIR   D3
  #define PIN_THERMISTER A0
  #define PIN_HEATER D0
  #define PIN_FILAMENT D7
  #define PIN_SCL 99
  #define PIN_SDA 99

#elif defined(ESP8266) && VERSION == 1501  // D1 mini, Transitional PCB, Display but no buttons

  #define PIN_EN    D1
  #define PIN_STEP  D2
  #define PIN_DIR   D6
  #define PIN_THERMISTER A0
  #define PIN_HEATER D5
  #define PIN_FILAMENT D7
  #define PIN_SCL D4
  #define PIN_SDA D3

#elif defined(ESP8266) && VERSION >= 1502

  #define PIN_EN          D1
  #define PIN_STEP        D2
  #define PIN_DIR         99   // False pin, fixed to LOW via the PCB
  #define PIN_THERMISTER  A0
  #define PIN_HEATER      D0   // 1405 D0
  #define PIN_FILAMENT    D7
  #define PIN_SCL         D4
  #define PIN_SDA         D3   // 1405 PIN_DIR
  #define PIN_BTN1        D6   // 1501 PIN_DIR
  #define PIN_BTN2        D5
  #define PIN_MSI3        D8
  
// TODO
/*#elif defined(ESP32) && VERSION == 1502 

  #define LED_BUILTIN     15 
  #define PIN_EN          35
  #define PIN_STEP        33   
  #define PIN_DIR         99   // False pin
  #define PIN_THERMISTER  3
  #define PIN_HEATER      7
  #define PIN_FILAMENT    11
  #define PIN_SCL         16
  #define PIN_SDA         18
  #define PIN_BTN1        9
  #define PIN_BTN2        13
  #define PIN_BTN3        37
  #define PIN_BTN4        39
*/
#else

  #error "Board/version not supported"

#endif