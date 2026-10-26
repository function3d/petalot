#pragma once

#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128      // Screen width in pixels
#define SCREEN_HEIGHT 64      // Screen height in pixels
#define OLED_RESET -1         // Reset pin (-1 if shared with the MCU)
#define SCREEN_ADDRESS 0x3C   // I2C address (0x3C for 128x64, sometimes 0x3D)

// Temperature button state
bool btnTempLastState = HIGH;
unsigned long btnTempPressTime = 0;

// Speed button state
bool btnSpeedLastState = HIGH;
unsigned long btnSpeedPressTime = 0;

const unsigned long LONG_PRESS_TIME = 500; // ms to consider a long press

unsigned long lastUpdate = 0;
const unsigned long UpdateTimeout = 1000;

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
  display.invertDisplay(false);
  displayInitialized = true;
  display.clearDisplay();
  display.display();
}

String toHHMMSS(unsigned long seconds) {
  int hours = seconds / 3600;
  int min = (seconds % 3600) / 60;
  int sec = seconds % 60;

  String result = "";
  result += String(hours) + ":";
  result += String(min) + ":";
  result += String(sec) + "";

  return result;
}

void drawUI() {
  if (OTA_update) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // --- HEADER: PETALOT vX.Y | o RUNNING / STOPPED ---
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("PETALOT v");
  display.print(VERSION / 1000);
  display.print(".");
  display.print((VERSION % 1000) / 100);

  bool working = (status == "working");

  // Inverted state pill on the right (contrasts in normal and inverted mode).
  display.fillRect(84, 0, 52, 9, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(86, 1);
  display.print(working ? "RUNNING" : "STOPPED");

  // Blinking dot left of the state while working (drawUI runs ~1 Hz).
  static bool dotOn = false;
  if (working) dotOn = !dotOn;
  else dotOn = false;
  if (dotOn) display.fillCircle(78, 4, 2, SSD1306_WHITE);
  display.setTextColor(SSD1306_WHITE);

  display.drawFastHLine(0, 12, SCREEN_WIDTH, SSD1306_WHITE);

  // --- TEMPERATURES ---
  display.setCursor(0, 18);
  display.print("TEMP");
  display.setCursor(84, 18);
  display.print("TARGET");

  display.setTextSize(2);
  display.setCursor(0, 32);
  if (isnan(T)) {
    display.print("--");
  } else {
    display.print(T, 0);
  }
  display.write(247); // degree glyph
  display.print("C");

  // Target small, vertically centred with the big actual value.
  display.fillRect(83, 31, 31, 9, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(84, 32);
  display.print(To, 0);
  display.write(247);
  display.print("C");

  display.drawFastHLine(0, 52, SCREEN_WIDTH, SSD1306_WHITE);

  // --- SPEED + RUN TIMER ---
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.setTextColor(SSD1306_WHITE);
  display.print("PACE:");
  display.fillRect(29, 55, 50, 9, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.print(Vo, 0);
  display.print("cm/min");
  
  display.setTextColor(SSD1306_WHITE);
  if (Ts > 1) {
    long seconds = Ts;
    int hours = seconds / 3600;
    int min = (seconds % 3600) / 60;
    int sec = seconds % 60;
    display.setCursor(128 - 7*6+2, 57);
    display.print(hours>9?String(hours):"0"+String(hours));
    display.setCursor(128 - 7*4 - 2, 57);
    display.print(":");
    display.setCursor(128 - 7*4 + 2, 57);
    display.print(min>9?String(min):"0"+String(min));
    display.setCursor(128 - 7*2 - 2, 57);
    display.print(":");
    display.setCursor(128 - 7*2 + 2, 57);
    display.print(sec>9?String(sec):"0"+String(sec));
  } else {
    display.setCursor(128 - 7*6+2, 57);
    display.print("--");
    display.setCursor(128 - 7*4 - 2, 57);
    display.print(":");
    display.setCursor(128 - 7*4 + 2, 57);
    display.print("--");
    display.setCursor(128 - 7*2 - 2, 57);
    display.print(":");
    display.setCursor(128 - 7*2 + 2, 57);
    display.print("--");
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