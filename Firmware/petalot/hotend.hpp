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

void stripWatchReset();   // implemented below with the strip-watch instrumentation

// Heater watchdog state. Unlike temperatureStart (frozen at start), this
// tracks progress: heatWatchRef is the best T seen since (re)start and
// heatWatchTime the last time T rose by HEAT_WATCH_RISE. Raising To rearms
// the window instead of moving the trip threshold towards the live T.
double heatWatchRef;
unsigned long heatWatchTime;
unsigned long heatWatchBadSince;  // when the far-below-target state began (0 = clear)
const double HEAT_WATCH_RISE = 2.0;               // progress step that rearms the window
const double HEAT_WATCH_FAR = 20.0;               // "far below target" gate, as before
const double HEAT_WATCH_MIN_RISE = 10.0;          // required rise since reference, as before
const unsigned long HEAT_WATCH_TIMEOUT = 30 * 1000;
const unsigned long HEAT_WATCH_DEBOUNCE = 5 * 1000;

void heatWatchRearm() {
  if (status != "working") return;
  if (!isnan(T)) heatWatchRef = T;
  heatWatchTime = millis();
  if (heatWatchTime == 0) heatWatchTime = 1;
  heatWatchBadSince = 0;
}

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

// Valid ADC window for a connected thermistor. An unplugged probe reads ~0
// (A0 pulled to GND by the 2k) and a shorted one ~1023; anything in between is
// a real reading. The hotend only works around minT (>=180 C), so cold ambient
// is not measured accurately and is reported as T_AMBIENT_MIN below.
const int AR_MIN = 4;
const int AR_MAX = 1021;
const double T_AMBIENT_MIN = 25;

// Sub-degree fractional temperature interpolation gives smoother control but
// reacts to ADC noise (can show a 209 more often near 210). Uncomment to use
// it; leave commented out for the classic integer map() conversion.
// #define INTERPOLATION_FRACTIONAL

void Thermister_ESP8266() {
  int i = 0;

  for (i = 0; i < TEMP_TABLE_ROWS; i++) {
    if (AR >= temptable[i][0]) {
      break;
    }
  }

#ifdef INTERPOLATION_FRACTIONAL
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
#else
  if (i == 0) {
    T = temptable[0][1];
  } else if (i == TEMP_TABLE_ROWS) {
    int last = TEMP_TABLE_ROWS - 1;
    T = map(AR, temptable[last - 1][0], temptable[last][0], temptable[last - 1][1], temptable[last][1]);
  } else {
    T = map(AR, temptable[i - 1][0], temptable[i][0], temptable[i - 1][1], temptable[i][1]);
  }
#endif

  double toffset = TOffset * T / To;
  T = T + toffset;

  // Below the usable range the table is not calibrated (ambient reads well
  // under 25 C), so clamp instead of showing a bogus cold value. The machine
  // does not run cold anyway.
  if (T < T_AMBIENT_MIN) {
    T = T_AMBIENT_MIN;
  }
}

void start() {
  if (tempLastStart == 0) {
    Fs = 0;
    Ts = 0;
    bottleCounted = false;
    status = "working";
    tempLastStart = millis();
    AR = analogRead(PIN_THERMISTER);
    if (AR > AR_MIN && AR < AR_MAX) {
      Thermister_ESP8266();
    } else {
      T = NAN;
    }
    temperatureStart = T;
    stripWatchReset();
    heatWatchRearm();
    if (tempLastStart == 0) tempLastStart = 1;
    LastStopReason = "";
  }
}

// Count the bottle as soon as the threshold is crossed, so a reboot mid-run
// does not lose it. "Longest" and "last" are updated when the run stops, where
// the full length of the bottle is known.
void countBottle() {
  if (bottleCounted || Fs < BOTTLE_MIN_CM) return;
  Bt++;
  bottleCounted = true;
  statsDirty = true;
}

void finishBottle() {
  if (!bottleCounted) return;
  lastFs = (int)Fs;
  if ((int)Fs > maxFs) maxFs = (int)Fs;
  bottleCounted = false;
  statsDirty = true;
}

void stop() {
  if (status == "working") finishBottle();
  status = "stopped";
  analogWrite(PIN_HEATER, 0);
  if (!UseDisplay) {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  tempLastStart = 0;
  heatWatchBadSince = 0;
  resumeWaitTemp = false;
}

void initHotend() {
  status = "stopped";
  if (!UseDisplay) {
    pinMode(LED_BUILTIN, OUTPUT);
  }
  pinMode(PIN_FILAMENT, INPUT);

  // Only an OTA reboot carries a resume state. Restore the previous run, keep
  // the motor off until the heater reaches target, and consume the flag so a
  // later power cycle uses StartOnPower as usual.
  if (OtaResumeState == 1) {
    start();
    if (tempLastStart > 0) {
      // millis() starts at zero on boot; never let a long run underflow into
      // an immediate Maxtime trip.
      tempLastStart = (OtaResumeElapsedMs <= millis()) ? (millis() - OtaResumeElapsedMs) : 1;
    }
    resumeWaitTemp = true;
    OtaResumed = true;
  } else if (OtaResumeState == 2) {
    // Machine was stopped before the update: stay stopped.
  } else if (StartOnPower) {
    start();
  }
  OtaResumeState = 0;
  OtaResumeElapsedMs = 0;
  saveConfiguration(false);

  if (To > maxT) To = workT;

  AR = analogRead(PIN_THERMISTER);
  lastAR = AR;
}

// ============================================================================
// HEATER REGULATION — two selectable controllers (Ajustes / Avanzado / Heating
// control). PID is the default and works out of the box; Bang-bang uses
// HYS/RAMP/HOLD. Both keep full power below minT (boost hysteresis).
// PID fine-tune: Kp response (lower it if it oscillates), Ki reaches the
// target (raise it if the temperature stays below), Kd damping (raise it if
// it oscillates). Uses clamping anti-windup and a low-pass filtered
// derivative so the ±1°C ADC noise is not amplified into the heater.
// ============================================================================
double pidIntegral = 0, pidPrevT = 0, pidDeriv = 0;
bool pidFirst = true, pidBoost = true;

void pidReset() {
  pidIntegral = 0;
  pidDeriv = 0;
  pidPrevT = T;
  pidFirst = true;
  pidBoost = false;
}

double controlPID() {
  // Full power until minT, with hysteresis so it does not flap at the threshold.
  const double HYSTERESIS = 3.0;
  static bool boost = false;
  if (T >= minT) {
    boost = false;
  } else if (T < minT - HYSTERESIS) {
    boost = true;
  }
  if (boost) {
    pidBoost = true; // next regulation pass starts with a clean integral
    return MaxGate;
  }

  if (pidFirst) {
    pidPrevT = T;
    pidFirst = false;
  }
  if (pidBoost) {
    pidIntegral = 0;
    pidBoost = false;
  }

  const double dt = 0.25;                           // 250 ms control period
  const double maxOut = (double)MaxGate * Gate / 100.0;
  const double error = (double)To - T;

  // Filtered derivative of the measurement (°C/s): rising temp pulls the
  // output down (damping), falling temp pushes it back up.
  const double dPV = (T - pidPrevT) / dt;
  const double alpha = 0.25;                        // low-pass on derivative
  pidDeriv = alpha * dPV + (1.0 - alpha) * pidDeriv;
  pidPrevT = T;

  double outRaw = Kp * error + Ki * pidIntegral - Kd * pidDeriv;
  const bool saturated = (outRaw >= maxOut) || (outRaw <= 0.0);
  if (!saturated) pidIntegral += error * dt;        // clamping anti-windup
  if (outRaw > maxOut) outRaw = maxOut;
  if (outRaw < 0.0) outRaw = 0.0;
  return outRaw;
}

double controlBangBang() {
  // Full power until minT, with hysteresis so it does not flap at the threshold.
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

double control() {
  if (status == "stopped") {
    pidReset();
    return 0;
  }
  if (isnan(T)) {
    LastStopReason = "Anomalous temperature reading, something wrong with thermistor";
    stop();
    return 0;
  }
  if (ControlMode == 0) {
    return controlPID();
  }
  return controlBangBang();
}

// ============================================================================
// STRIP WATCH — strip-break detection from heater duty
// ============================================================================
// The heater power needed to hold To depends on how much heat the strip takes
// away, so a strip that stops moving makes the PID settle at a lower duty.
// Two exponential averages track that duty: fast (15 s) follows load changes,
// slow (120 s) holds the machine baseline. When fast falls well below slow
// and stays there, the strip is treated as broken: the machine stops and the
// reason is shown in the UI. Field measurements: drop 11 pts at Vo35, 7 at
// Vo25, 7 at Vo15, 2 at Vo5 (noise), hence StripMinVo defaults to 15.
double stripDuty = NAN;      // normalised duty of the last sample (%)
double stripFast = NAN;      // 15 s average, follows load changes
double stripSlow = NAN;      // 120 s average, machine baseline
double stripNoise = 0;       // dispersion of fast/slow while stable
bool stripGatesOk = false;   // gates passed on the last sample
bool stripArmed = false;     // baseline settled, ratio is trustworthy
bool stripTripped = false;   // a strip break has stopped the machine
bool stripLog = false;       // serial CSV logging, toggled with "strip"
int stripVoRef = -1;         // Vo used when the baseline was built
unsigned long stripStableSince = 0;
unsigned long stripBadSince = 0;   // start of a transient gate failure
unsigned long stripDropSince = 0;  // start of the sustained duty drop
unsigned long stripDropBadSince = 0;  // start of a brief sub-threshold gap
unsigned long stripLastLog = 0;

const double STRIP_TAU_FAST = 15.0;
const double STRIP_TAU_SLOW = 120.0;
const double STRIP_SAMPLE_S = 0.25;  // hotendReadTempTask period
const double STRIP_ALPHA_FAST = STRIP_SAMPLE_S / (STRIP_TAU_FAST + STRIP_SAMPLE_S);
const double STRIP_ALPHA_SLOW = STRIP_SAMPLE_S / (STRIP_TAU_SLOW + STRIP_SAMPLE_S);
const double STRIP_BAND = 5.0;                // °C around To to call it "in regulation"
const double STRIP_MIN_DROP = 3.0;            // points, floor for the drop threshold
const double STRIP_DROP_FRAC = 0.08;          // 8 % of slow, relative threshold
const unsigned long STRIP_BAD_REBASE_MS = 3UL * 60 * 1000;  // long gate loss
const unsigned long STRIP_DROP_GAP_MS = 5000;  // brief dips do not reset the hold

unsigned long stripSettleMs() { return (unsigned long)StripSettleS * 1000UL; }
unsigned long stripArmMs() { return (unsigned long)StripArmS * 1000UL; }

// Only samples taken in steady regulation are meaningful: the machine must be
// running with the motor on, the temperature on target and the heater neither
// fully off nor saturated. Anything else is a transient, not a load change.
bool stripGatesPass() {
  if (!StripWatch) return false;
  if (status != "working") return false;
  if (!stepper.motorEnabled) return false;
  if (isnan(T)) return false;
  if (fabs(T - To) > STRIP_BAND) return false;
  double maxOut = (double)MaxGate * Gate / 100.0;
  if (maxOut <= 2.0) return false;
  if (Output <= 1.0 || Output >= maxOut - 1.0) return false;
  if (tempLastStart == 0 || millis() - tempLastStart < stripSettleMs()) return false;
  if (Vo < StripMinVo) return false;
  return true;
}

void stripWatchReset() {
  stripDuty = NAN;
  stripFast = NAN;
  stripSlow = NAN;
  stripNoise = 0;
  stripGatesOk = false;
  stripArmed = false;
  stripTripped = false;
  stripStableSince = 0;
  stripBadSince = 0;
  stripDropSince = 0;
  stripDropBadSince = 0;
  stripLastLog = 0;
}

double stripRatio() {
  return (!isnan(stripFast) && stripSlow > 0.5) ? stripFast / stripSlow : 0.0;
}

double stripDropThreshold() {
  return max(STRIP_MIN_DROP, STRIP_DROP_FRAC * stripSlow);
}

double stripDropAgeSec() {
  return (stripDropSince && !stripTripped) ? (millis() - stripDropSince) / 1000.0 : 0.0;
}

void stripPrintLine() {
  Serial.print("strip,");
  Serial.print(millis() / 1000);
  Serial.print(",");
  Serial.print(stripDuty, 1);
  Serial.print(",");
  Serial.print(stripFast, 1);
  Serial.print(",");
  Serial.print(stripSlow, 1);
  Serial.print(",");
  Serial.print(stripRatio(), 3);
  Serial.print(",");
  Serial.print(stripNoise, 3);
  Serial.print(",");
  Serial.print(T, 1);
  Serial.print(",");
  Serial.print(To, 1);
  Serial.print(",");
  Serial.print(Vo);
  Serial.print(",");
  Serial.print(stripArmed ? 1 : 0);
  Serial.print(",");
  Serial.print(stripDropAgeSec(), 1);
  Serial.print(",");
  Serial.println(stripTripped ? 1 : 0);
}

void stripCommand(const String &arg) {
  String a = arg;
  a.trim();
  a.toLowerCase();
  if (a == "on") {
    stripLog = true;
  } else if (a == "off") {
    stripLog = false;
  } else {
    stripLog = !stripLog;
  }
  Serial.print("strip log ");
  Serial.println(stripLog ? "on" : "off");
  Serial.println("strip,sec,duty,fast,slow,ratio,noise,T,To,Vo,armed,dropSec,tripped");
  stripPrintLine();
}

void stripWatchSample() {
  double maxOut = (double)MaxGate * Gate / 100.0;
  stripDuty = (maxOut > 0) ? 100.0 * Output / maxOut : 0.0;

  // A speed change redefines the load, so the old baseline is worthless.
  if (Vo != stripVoRef) {
    stripVoRef = Vo;
    stripWatchReset();
  }

  unsigned long now = millis();
  stripGatesOk = stripGatesPass();

  if (!stripGatesOk) {
    // A short transient (the T bump after a strip stop, a sensor glitch) must
    // not re-baseline the slow average, or the drop we are looking for would
    // be erased. Freeze both averages and keep the armed state while it lasts.
    // Only a long loss of regulation rebuilds the baseline from scratch.
    if (stripBadSince == 0) stripBadSince = now;
    if (now - stripBadSince >= STRIP_BAD_REBASE_MS) {
      if (!isnan(stripSlow)) {
        stripSlow += STRIP_ALPHA_SLOW * (stripDuty - stripSlow);
        stripFast = stripSlow;
      }
      stripStableSince = 0;
      stripArmed = false;
      stripDropSince = 0;
      stripDropBadSince = 0;
    }
  } else {
    stripBadSince = 0;
    if (stripStableSince == 0) stripStableSince = now;

    if (isnan(stripFast)) {
      stripFast = stripSlow = stripDuty;
    } else {
      stripFast += STRIP_ALPHA_FAST * (stripDuty - stripFast);
      stripSlow += STRIP_ALPHA_SLOW * (stripDuty - stripSlow);
    }

    bool wasArmed = stripArmed;
    stripArmed = (now - stripStableSince >= stripArmMs());

    // At arm time fast is the settled duty and slow may still be climbing from
    // the first heat-up sample. Snap the baseline so a strip stop does not have
    // to wait for the two averages to meet before the drop becomes visible.
    if (!wasArmed && stripArmed && !isnan(stripFast)) {
      stripSlow = stripFast;
      stripNoise = 0;
    }

    if (stripArmed && stripSlow > 5.0) {
      stripNoise += 0.02 * (fabs(stripRatio() - 1.0) - stripNoise);
    }
  }

  // Detection also runs while gates are momentarily false: a strip break can
  // itself push T briefly out of band, and that must not hide the event.
  if (StripWatch && stripArmed && status == "working" && stepper.motorEnabled &&
      Vo >= StripMinVo && !isnan(stripFast) && !isnan(stripSlow) && stripSlow > 5.0) {
    if (stripSlow - stripFast > stripDropThreshold()) {
      stripDropBadSince = 0;
      if (stripDropSince == 0) stripDropSince = now;
      else if (now - stripDropSince >= (unsigned long)StripHoldS * 1000UL) {
        stripTripped = true;
        LastStopReason = "Strip broken: heater load dropped";
        stop();
      }
    } else if (stripDropSince) {
      if (stripDropBadSince == 0) stripDropBadSince = now;
      else if (now - stripDropBadSince >= STRIP_DROP_GAP_MS) {
        stripDropSince = 0;
        stripDropBadSince = 0;
      }
    }
  } else {
    stripDropSince = 0;
    stripDropBadSince = 0;
  }

  if (stripLog && millis() >= stripLastLog + 5000) {
    stripLastLog = millis();
    stripPrintLine();
  }
}

void hotendReadTempTask() {
  if (millis() >= tempLastSample + 250) {
    // 2-sample moving average using integer math (no FPU on ESP8266).
    AR = analogRead(PIN_THERMISTER);
    int avgAR = (AR + lastAR) / 2;
    lastAR = AR;

    if (avgAR > AR_MIN && avgAR < AR_MAX) {
      Thermister_ESP8266();
    } else {
      T = NAN; // open/short thermistor: invalid reading
    }

    Output = control();
    analogWrite(PIN_HEATER, Output);
    stripWatchSample();

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

    // Heater watchdog: stop only if T makes no progress for HEAT_WATCH_TIMEOUT
    // while far below target, sustained for HEAT_WATCH_DEBOUNCE. Progress
    // (+HEAT_WATCH_RISE) and any To increase rearm the window, and samples
    // near the target keep it disarmed, so normal ripple or a slow climb
    // after raising To cannot stop the machine on a single 250 ms sample.
    if (status == "working" && !isnan(T)) {
      unsigned long now = millis();
      if (T > heatWatchRef + HEAT_WATCH_RISE) {
        heatWatchRef = T;
        heatWatchTime = now;
      }
      if (T < To - HEAT_WATCH_FAR && (now - heatWatchTime) >= HEAT_WATCH_TIMEOUT &&
          T < heatWatchRef + HEAT_WATCH_MIN_RISE) {
        if (heatWatchBadSince == 0) heatWatchBadSince = now;
        if ((now - heatWatchBadSince) >= HEAT_WATCH_DEBOUNCE) {
          LastStopReason = "30s no heat: thermistor/heater issue (T=" + String(T, 0) +
                           " To=" + String(To, 0) + " ref=" + String(heatWatchRef, 0) + ")";
          stop();
        }
      } else {
        heatWatchBadSince = 0;
      }
    }
  }
}