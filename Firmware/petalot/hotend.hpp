#pragma once

// ============================================================================
// THERMISTOR + HEATER (HOTEND)
// ============================================================================
short AR;                 // analog read value (0-1023)
int lastAR = 0;           // previous sample, used for a 2-sample average
bool F = false;           // filament present
bool Fcurrent = false;    // current filament pin state
bool Finsert = false;     // filament inserted & confirmed after 3s
bool Fworking = false;    // filament running confirmed after 30s
double Output;            // heater PWM output
unsigned long tempLastSample;
unsigned long tempLastFilament;
unsigned long tempLastNoFilament;
unsigned long tempLastStart;
double temperatureStart;  // temperature when the run started

// 3V3 -> NTC -> A0 -> 2KΩ -> GND
int temptable[8][2] = {
  { 960, 250 },
  { 922, 230 },
  { 833, 200 },
  { 703, 170 },
  { 454, 130 },
  { 157, 80 },
  { 31, 30 },
  { 17, 10 }
};
const int TEMP_TABLE_ROWS = sizeof(temptable) / sizeof(temptable[0]);

void Thermister_ESP8266() {
  int i = 0;

  for (i = 0; i < TEMP_TABLE_ROWS; i++) {
    if (AR >= temptable[i][0]) {
      break;
    }
  }

  if (i == 0) {
    T = temptable[0][1];
  } else {
    // Fractional linear interpolation between the bracketing table entries
    int hi = (i == TEMP_TABLE_ROWS) ? TEMP_TABLE_ROWS - 1 : i;
    int lo = hi - 1;
    double hiAR = temptable[hi][0], loAR = temptable[lo][0];
    double hiT = temptable[hi][1], loT = temptable[lo][1];
    T = loT + (double)(AR - loAR) * (hiT - loT) / (hiAR - loAR);
  }

  double toffset = TOffset * T / To;
  T = T + toffset;
  if (AR < temptable[TEMP_TABLE_ROWS - 1][0]) {
    T = 0;
  }
}

void start() {
  if (tempLastStart == 0) {
    Fs = 0;
    Ts = 0;
    status = "working";
    tempLastStart = millis();
    AR = analogRead(PIN_THERMISTER);
    Thermister_ESP8266();
    temperatureStart = T;
    if (tempLastStart == 0) tempLastStart = 1;
    LastStopReason = "";
  }
}

void stop() {
  status = "stopped";
  analogWrite(PIN_HEATER, 0);
  if (!UseDisplay) {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  tempLastStart = 0;
}

void initHotend() {
  status = "stopped";
  if (!UseDisplay) {
    pinMode(LED_BUILTIN, OUTPUT);
  }
  pinMode(PIN_FILAMENT, INPUT);
  if (StartOnPower) start();

  if (To > maxT) To = workT;

  AR = analogRead(PIN_THERMISTER);
  lastAR = AR;
}

double control() {
  if (status == "stopped") {
    return 0;
  }
  if (isnan(T)) {
    return 0;
  }
  if (T == 0) {
    LastStopReason = "Anomalous temperature reading, something wrong with thermistor";
    stop();
    return 0;
  }

  // Hysteresis around minT: use full power until minT is reached, then step
  // down to the working duty without flapping back and forth at the threshold.
  const double HYSTERESIS = 3.0;
  static bool boost = false;
  if (T >= minT) {
    boost = false;
  } else if (T < minT - HYSTERESIS) {
    boost = true;
  }
  if (boost) {
    return MaxGate;
  }

  // Regulation zone: linear approach ramp plus a HOLD floor so the heater
  // delivers enough power right at/below the target to reach and hold To.
  // Power is kept (floor) from the ramp up to To+HYS, and only cuts above,
  // so the setpoint is reached and gently held instead of sagging away.
  if (T >= (double)To + HYS) {
    return 0;
  }
  double ramp = (RAMP > 0) ? RAMP : 1.0;
  double approach = (double)MaxGate * Gate / 100.0;
  if (T <= (double)To - HYS - ramp) {
    return approach;
  }
  double err = ((double)To - HYS) - T; // in (0, ramp]
  double duty = approach * err / ramp;
  double floor = (double)MaxGate * HOLD / 100.0;
  if (floor > approach) floor = approach;
  if (duty < floor) duty = floor;
  return duty;
}

void hotendReadTempTask() {
  if (millis() >= tempLastSample + 250) {
    // 2-sample moving average using integer math (no FPU on ESP8266).
    // The sanity band 15..1007 is equivalent to the original 0.05..3.25 V check.
    AR = analogRead(PIN_THERMISTER);
    int avgAR = (AR + lastAR) / 2;
    lastAR = AR;

    if (avgAR > 15 && avgAR < 1007) {
      Thermister_ESP8266();
    }

    Output = control();
    analogWrite(PIN_HEATER, Output);

    if (status == "working") {
      if (T > minT) {
        if (!UseDisplay) {
          digitalWrite(LED_BUILTIN, LOW); // target temperature ready
        }
      } else {
        if (!UseDisplay) {
          digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // heating up
        }
      }
    } else {
      if (!UseDisplay) {
        digitalWrite(LED_BUILTIN, HIGH);
      }
    }

    tempLastSample = millis();

    // Filament sensor
    Fcurrent = digitalRead(PIN_FILAMENT);

    if (Fcurrent && !F) {
      tempLastFilament = millis();
      start(); // start the machine when filament is inserted
    }

    if (!Fcurrent && F) {
      tempLastFilament = 0;
      tempLastNoFilament = millis();
    }

    F = Fcurrent;

    if (Fenable) {
      // Filament inserted and kept for 3s: mark as inserted
      if (Fcurrent && tempLastFilament > 0 && millis() >= tempLastFilament + 3 * 1000) {
        Finsert = true;
        Fworking = false;
      }

      // Filament running for 30s: consider the job running
      if (Fcurrent && tempLastFilament > 0 && millis() >= tempLastFilament + 30 * 1000) {
        Fworking = true;
      }

      // Filament removed right after insertion: treat as user stop
      if (!Fcurrent && Finsert && !Fworking && tempLastNoFilament > 0 && millis() >= tempLastNoFilament + 500) {
        LastStopReason = "Stop by user";
        stop();
        tempLastNoFilament = 0;
        Fworking = false;
        Finsert = false;
      }

      // Filament removed while running: run out, wait Stopdelay
      if (!Fcurrent && Fworking && tempLastNoFilament > 0 && millis() >= tempLastNoFilament + (Stopdelay + (25 - Vo) / 5 * 1.5) * 1000) {
        LastStopReason = "Run out";
        stop();
        tempLastNoFilament = 0;
        Fworking = false;
        Finsert = false;
      }

      // No filament detected for NoFilamentTime while running
      if (!Fcurrent && !Finsert && tempLastStart > 0 && millis() >= tempLastStart + NoFilamentTime * 60 * 1000) {
        LastStopReason = "No sensor detection for " + String(NoFilamentTime) + " min.";
        stop();
      }
    }

    // Max run time applies regardless of the filament sensor state
    if (tempLastStart > 0 && millis() >= tempLastStart + (unsigned long)Maxtime * 60 * 1000) {
      LastStopReason = "Max time reached: " + String(Maxtime) + " min.";
      stop();
    }

    // Heater failed to reach temperature 30s after start
    if (tempLastStart > 0 && T < To - 20 && T - 10 < temperatureStart && millis() >= tempLastStart + 30 * 1000) {
      LastStopReason = "30s no heat: thermistor/heater issue";
      stop();
    }
  }
}