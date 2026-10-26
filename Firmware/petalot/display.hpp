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
  display.invertDisplay(InvertDisplay);
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

  // --- HEADER: PETALOT vX.Y | o RUNNING / STOPPED ---
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("PETALOT v");
  display.print(VERSION / 1000);
  display.print(".");
  display.print((VERSION % 1000) / 100);

  bool working = (status == "working");
  display.setCursor(86, 0);
  display.print(working ? "RUNNING" : "STOPPED");

  // Blinking dot left of the state while working (drawUI runs ~1 Hz).
  static bool dotOn = false;
  if (working) dotOn = !dotOn;
  else dotOn = false;
  if (dotOn) display.fillCircle(80, 3, 2, SSD1306_WHITE);

  // --- TEMPERATURES ---
  display.setCursor(0, 12);
  display.print("TEMP ACTUAL");
  display.setCursor(68, 12);
  display.print("TARGET:");

  display.setTextSize(2);
  display.setCursor(0, 24);
  if (isnan(T)) {
    display.print("--");
  } else {
    display.print(T, 0);
  }
  display.write(248); // degree glyph
  display.print("C");

  display.setCursor(68, 24);
  display.print(To, 0);
  display.write(248);
  display.print("C");

  // --- SPEED + RUN TIMER ---
  display.setTextSize(1);
  display.setCursor(0, 48);
  display.print("VEL: ");
  display.print(Vo, 0);
  display.print(" cm/min");

  if (working && (F || !Fenable)) {
    String timer = toHHMMSS(Ts);
    display.setCursor(128 - timer.length() * 6, 56);
    display.print(timer);
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