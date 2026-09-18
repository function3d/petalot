#pragma once

#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128      // Screen width in pixels
#define SCREEN_HEIGHT 64      // Screen height in pixels
#define OLED_RESET -1         // Reset pin (-1 if shared with the MCU)
#define SCREEN_ADDRESS 0x3C   // I2C address (0x3C for 128x64, sometimes 0x3D)

#define FRAME_WIDTH (32)
#define FRAME_HEIGHT (32)
#define FRAME_COUNT 4

// Temperature button state
bool btnTempLastState = HIGH;
unsigned long btnTempPressTime = 0;

// Speed button state
bool btnSpeedLastState = HIGH;
unsigned long btnSpeedPressTime = 0;

const unsigned long LONG_PRESS_TIME = 500; // ms to consider a long press

unsigned long lastUpdate = 0;
const unsigned long UpdateTimeout = 1000;
int frame = 0;
const byte PROGMEM frames_speed[][128] = {
  {0,0,0,0,0,2,0,0,0,31,0,0,0,17,7,0,0,17,141,128,0,16,216,192,4,16,112,128,15,32,0,128,24,192,1,128,16,0,1,128,8,0,0,128,4,3,192,252,6,4,96,4,2,8,16,4,6,24,8,6,12,16,8,28,56,16,8,48,96,24,24,96,32,8,16,64,32,6,32,64,63,3,192,48,1,0,0,24,0,128,0,8,1,128,3,24,1,0,4,240,1,14,8,32,3,27,8,0,1,177,136,0,0,96,136,0,0,0,248,0,0,0,64,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,24,28,0,0,108,54,0,0,68,34,0,0,67,194,0,0,64,2,0,0,64,2,0,0,192,3,0,31,128,1,248,48,0,0,12,32,3,192,4,48,4,32,12,24,8,16,24,4,16,24,32,4,16,8,32,4,16,8,32,4,24,8,32,24,8,16,24,48,4,32,12,32,3,192,4,48,0,0,12,31,128,1,248,0,64,3,0,0,64,2,0,0,64,2,0,0,67,226,0,0,68,34,0,0,108,54,0,0,56,24,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,96,0,0,0,248,0,0,224,136,0,1,177,136,0,3,27,8,0,1,14,8,32,1,0,4,240,0,128,3,24,1,128,0,8,1,0,0,24,63,3,192,48,32,4,48,96,32,8,16,64,96,24,24,96,56,16,8,48,12,16,8,28,6,24,24,6,2,8,16,4,6,6,48,4,12,3,192,252,24,0,0,128,16,0,1,0,24,192,1,128,15,32,0,128,4,16,48,128,0,16,200,192,0,17,133,128,0,17,3,0,0,31,0,0,0,2,0,0,0,0,0,0},
  {0,0,0,0,0,3,192,0,0,2,64,0,0,6,96,0,1,4,32,128,3,132,33,192,4,124,62,32,12,32,12,48,4,0,0,32,2,0,0,64,3,0,0,192,3,3,192,192,2,12,96,64,30,8,16,124,96,24,8,14,64,16,8,6,64,16,8,6,112,16,24,14,30,8,16,120,2,6,48,64,3,3,192,192,3,0,0,192,2,0,0,64,4,0,0,32,12,48,4,48,4,124,62,32,3,132,33,192,1,4,32,128,0,6,96,0,0,2,64,0,0,3,192,0,0,0,0,0},
};

bool displayInitialized = false;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void initDisplay() {
  if (displayInitialized) return;
  if (!UseDisplay) return;

  Wire.begin(PIN_SDA, PIN_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED not found, disabling display");
    UseDisplay = false;
    displayInitialized = false;
    return;
  }
  displayInitialized = true;
  display.clearDisplay();
  display.display();
}

String toHHMMSS(unsigned long seconds) {
  int hours = seconds / 3600;
  int min = (seconds % 3600) / 60;
  int sec = seconds % 60;

  String result = "";
  result += String(hours) + "h";
  result += String(min) + "m";
  result += String(sec) + "s";

  return result;
}

void drawUI() {
  if (OTA_update) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // --- TEMPERATURE SECTION ---
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("TEMP");
  display.print(" (");
  display.print(To, 0);
  display.print(")");

  display.setTextSize(2);
  display.setCursor(0, 13);
  display.print(T, 0);
  display.setTextSize(1);
  display.print("C");

  // --- SPEED SECTION ---
  display.setCursor(0, 36);
  display.print("SPEED");

  display.setTextSize(2);
  display.setCursor(0, 50);
  display.print(Vo);

  // Position the "cm/min" unit dynamically based on how many digits Vo has
  int xWidth = String((int)Vo).length() * 12;
  display.setCursor(xWidth, 56);
  display.setTextSize(1);
  display.print("cm/min");

  if (status == "working") {
    display.drawBitmap(94, 34, frames_speed[frame], FRAME_WIDTH, FRAME_HEIGHT, 1);
    frame = (frame + 1) % FRAME_COUNT;
    if (F || !Fenable) {
      int timerWidth = toHHMMSS(Ts).length() * 6;
      display.setCursor(128 - timerWidth, 0);
      display.print(toHHMMSS(Ts));
    }
  }

  display.display();
}

#if PCB > 1501
void checkButtons() {
  unsigned long currentMillis = millis();

  // --- TEMPERATURE BUTTON ---
  bool btnTempState = digitalRead(PIN_BTN1);

  // Falling edge: button pressed
  if (btnTempState == LOW && btnTempLastState == HIGH) {
    btnTempPressTime = currentMillis;
  }
  // Rising edge: button released
  else if (btnTempState == HIGH && btnTempLastState == LOW) {
    unsigned long pressDuration = currentMillis - btnTempPressTime;

    if (pressDuration > 50) { // debounce
      if (pressDuration < LONG_PRESS_TIME) {
        To += 5; // short press: increase temperature
        if (To > maxT) To = maxT;
      } else {
        To -= 5; // long press: decrease temperature
        if (To < minT) To = minT;
      }
    }
  }
  btnTempLastState = btnTempState;

  // --- SPEED BUTTON ---
  bool btnSpeedState = digitalRead(PIN_BTN2);

  if (btnSpeedState == LOW && btnSpeedLastState == HIGH) {
    btnSpeedPressTime = currentMillis;
  } else if (btnSpeedState == HIGH && btnSpeedLastState == LOW) {
    unsigned long pressDuration = currentMillis - btnSpeedPressTime;

    if (pressDuration > 50) { // debounce
      if (pressDuration < LONG_PRESS_TIME) {
        Vo += 5; // short press: increase speed
        if (Vo > maxV) Vo = maxV;
      } else {
        Vo -= 5; // long press: decrease speed
        if (Vo < minV) Vo = minV;
      }
    }
    drawUI();
  }
  btnSpeedLastState = btnSpeedState;
}
#endif

void displayTask() {
#if PCB > 1501
  checkButtons();
#endif
  if (millis() - lastUpdate >= UpdateTimeout) {
    lastUpdate = millis();
    initDisplay();
    drawUI();
  }
}