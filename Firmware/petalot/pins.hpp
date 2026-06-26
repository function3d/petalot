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

#elif defined(ESP8266) // D1 mini  1.5.2

  #define PIN_EN          5    // D1
  #define PIN_STEP        4    // D2
  #define PIN_DIR         99   // False pin,  12 o D6
  #define PIN_THERMISTER  A0
  #define PIN_HEATER      16   // go back to D0, D0 funciona para el gate del mostfet
  #define PIN_FILAMENT    13   // D7
  #define PIN_SCL         2    // D4
  #define PIN_SDA         0    // D3
  #define PIN_BTN1        12   // D6 previously PIN_DIR
  #define PIN_BTN2        14   // D5
  //#define PIN_BTN3      RX   // Cannot be used on D1 Mini
  //#define PIN_BTN4      RX   // Cannot be used on D1 Mini

#elif defined(ESP32) // S2 mini 

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

#else

  #error "Board/version not supported"

#endif