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

// Menu button state (BTN3 cycles the selected zone)
bool btnMenuLastState = HIGH;
unsigned long btnMenuPressTime = 0;

// Selected LCD zone: 0 = RUNNING/STOPPED, 1 = target temp, 2 = speed.
// BTN1/BTN2 act as +/- on zones 1-2, and toggle start/stop on zone 0.
int menuSel = 0;

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
  bool selState = (menuSel == 0);

  // Inverted pill marks the selected menu zone.
  if (selState) display.fillRect(84, 0, 52, 9, SSD1306_WHITE);
  display.setTextColor(selState ? SSD1306_BLACK : SSD1306_WHITE);
  display.setCursor(86, 1);
  display.print(working ? "RUNNING" : "STOPPED");

  // Blinking dot left of the state while working (drawUI runs ~1 Hz).
  static bool dotOn = false;
  if (working) dotOn = !dotOn;
  else dotOn = false;
  if (dotOn) display.fillCircle(78, 4, 2, selState ? SSD1306_BLACK : SSD1306_WHITE);
  display.setTextColor(SSD1306_WHITE);

  display.drawFastHLine(0, 12, SCREEN_WIDTH, SSD1306_WHITE);

  // --- TEMPERATURES ---
  display.setCursor(0, 18);
  display.print("TEMP");
  display.setCursor(66, 18);
  display.print("SPEED");

  display.setTextSize(2);
  display.setCursor(0, 32);
  if (isnan(T)) {
    display.print("--");
  } else {
    display.print(T, 0);
  }
  display.setTextSize(1);
  display.write(247); // degree glyph
  display.print("C");

  // Target small next tu "TEMP" (inverted pill when selected).
  bool selTarget = (menuSel == 1);
  if (selTarget) display.fillRect(26, 18, 31, 9, SSD1306_WHITE);
  display.setTextColor(selTarget ? SSD1306_BLACK : SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(27, 19);
  display.print(To, 0);
  display.write(247);
  display.print("C");

    // --- SPEED --- (inverted pill when selected)
  bool selSpeed = (menuSel == 2);
  if (selSpeed) display.fillRect(65, 31, 24, 16, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(66, 32);
  display.setTextColor(selSpeed ? SSD1306_BLACK : SSD1306_WHITE);
  display.setTextSize(2);
  if (Vo < 10)
    display.print(" ");
  display.print(Vo, 0);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.print("cm/min");
  display.drawFastHLine(0, 52, SCREEN_WIDTH, SSD1306_WHITE);

  // --- RUN TIMER ---
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

  // --- RUN LENGHT ---
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 57);
  if (Fs > 0) {
    display.print(round(Fs) / 100);
  } else {
    display.print("--");
  }
  display.print("m");

  display.display();
}

#if PCB > 1501
// Apply a +/- step (BTN1 = +, BTN2 = -) to the selected menu zone.
// Long press steps 20 instead of 5.
void menuAction(int dir, bool longPress) {
  int step = longPress ? 20 : 5;
  if (menuSel == 0) {
    // Either button toggles: start when stopped, stop when working.
    if (status == "working") stop();
    else start();
  } else if (menuSel == 1) {
    To += dir * step;
    if (To > maxT) To = maxT;
    if (To < minT) To = minT;
  } else {
    Vo += dir * step;
    if (Vo > maxV) Vo = maxV;
    if (Vo < minV) Vo = minV;
  }
  drawUI();
}

void checkButtons() {
  static bool btnInit = false;
  if (!btnInit) {
    // RX floats when USB is unplugged, so it needs its own pull-up.
    pinMode(PIN_BTN1, INPUT_PULLUP);
    pinMode(PIN_BTN2, INPUT_PULLUP);
    pinMode(PIN_BTN3, INPUT_PULLUP);
    btnInit = true;
  }
  unsigned long currentMillis = millis();

  // --- + BUTTON (BTN1) ---
  bool btnPlusState = digitalRead(PIN_BTN1);

  // Falling edge: button pressed
  if (btnPlusState == LOW && btnTempLastState == HIGH) {
    btnTempPressTime = currentMillis;
  }
  // Rising edge: button released
  else if (btnPlusState == HIGH && btnTempLastState == LOW) {
    unsigned long pressDuration = currentMillis - btnTempPressTime;

    if (pressDuration > 50) { // debounce
      menuAction(+1, pressDuration >= LONG_PRESS_TIME);
    }
  }
  btnTempLastState = btnPlusState;

  // --- - BUTTON (BTN2) ---
  bool btnMinusState = digitalRead(PIN_BTN2);

  if (btnMinusState == LOW && btnSpeedLastState == HIGH) {
    btnSpeedPressTime = currentMillis;
  } else if (btnMinusState == HIGH && btnSpeedLastState == LOW) {
    unsigned long pressDuration = currentMillis - btnSpeedPressTime;

    if (pressDuration > 50) { // debounce
      menuAction(-1, pressDuration >= LONG_PRESS_TIME);
    }
  }
  btnSpeedLastState = btnMinusState;

  // --- MENU BUTTON (BTN3): cycle the selected zone ---
  bool btnMenuState = digitalRead(PIN_BTN3);

  if (btnMenuState == LOW && btnMenuLastState == HIGH) {
    btnMenuPressTime = currentMillis;
  } else if (btnMenuState == HIGH && btnMenuLastState == LOW) {
    if (currentMillis - btnMenuPressTime > 50) { // debounce
      menuSel = (menuSel + 1) % 3;
      drawUI();
    }
  }
  btnMenuLastState = btnMenuState;
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