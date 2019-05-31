/**
   This file contains code for various display views of data
   @author Thor Lancaster
*/


byte iconBattery[] =    {B01110, B11111, B11111, B11111, B11111, B11111, B11111, B11111};
byte iconESC1[] =       {B00100, B00100, B00100, B00100, B00000, B11010, B01011, B11010};
byte iconESC2[] =       {B01010, B01010, B01010, B01010, B00000, B11010, B01011, B11010};
byte iconMotor1[] =     {B00100, B00100, B00100, B00100, B00000, B11111, B01110, B11111};
byte iconMotor2[] =     {B01010, B01010, B01010, B01010, B00000, B11111, B01110, B11111};
byte iconCruise[] =     {B11100, B10000, B11100, B00000, B00000, B00111, B00100, B00111};
byte iconBrake[] =      {B00100, B00100, B11111, B10101, B11111, B00100, B00100, B00000};
byte iconBrakeRegen[] = {B00100, B00100, B11111, B10101, B11111, B00100, B11111, B01110};
byte iconDegC[] =       {B01000, B10100, B01000, B00000, B00111, B01000, B01000, B00111};
byte iconLeftArr[] =    {B00011, B00111, B01111, B11111, B01111, B00111, B00011, B00000};
byte iconRightArr[] =   {B11000, B11100, B11110, B11111, B11110, B11100, B11000, B00000};
byte iconPH[] =         {B11000, B10100, B11100, B10000, B10100, B00110, B00101, B00101};
byte iconRunning[] =    {B00000, B00000, B00000, B00100, B00110, B11111, B00110, B00100};

byte iconCryBaby[] =    {B01100, B00100, B01110, B10101, B11111, B10001, B10101, B01110};

byte iconLeftArrow[] =  {B00001, B00011, B00111, B01111, B00111, B00011, B00001, B00000};
byte iconRightArrow[] = {B10000, B11000, B11100, B11110, B11100, B11000, B10000, B00000};


/**
   Main display view. Shown during normal operation
   @param v v object
   @param s Settings object
*/
void mainDisplay(values & v, Settings & s) {
  unsigned long cMillis = millis();
  // Set custom characters
  setBatteryIcon(getBattery(s), millis() % 2000 > 1000);
  cb420_setCustChar(0, iconESC1, false);
  cb420_setCustChar(1, iconESC2, false);
  cb420_setCustChar(2, iconMotor1, false);
  cb420_setCustChar(3, iconMotor2, false);
  cb420_setCustChar(4, iconPH, false);
  cb420_setCustChar(5, iconBattery, false);
  cb420_setCustChar(6, iconDegC, false);
  if (v.regenMode)
    cb420_setCustChar(7, iconBrakeRegen, false);
  else if (v.brake)
    cb420_setCustChar(7, iconBrake, false);
  else if (v.ccSetSpeed > 0)
    cb420_setCustChar(7, iconCruise, false);
  else {
    cb420_setCustChar(7, iconRunning, false);
  }


  cb420_clear();
  cb420_setCursor(0, 0);
  cb420_printFloat100(v.dispVolts);
  cb420_printChar('V'); // Volts

  cb420_setCursor(6, 0);
  cb420_printFloat1000(s.batteryAH - s.batteryUsedAH);
  cb420_print("AH"); // Amp-hours remaining

  // Per-controller amps
  cb420_setCursor(15, 0);
  cb420_printIntR(v.dispAmps1 + 0.5);
  cb420_setCursor(16, 0);
  cb420_printChar(',');
  cb420_setCursor(18, 0);
  cb420_printIntR(v.dispAmps2 + 0.5);
  cb420_setCursor(19, 0);
  cb420_printChar('A');

  // Total system watts
  int watts = v.volts * (v.dispAmps1 + v.dispAmps2);
  cb420_setCursor(0, 1);
  cb420_printFloat100(watts / 1000.0);
  cb420_print("kW");

  // Speed
  cb420_setCursor(7, 1);
  cb420_printFloat100(v.speed);
  cb420_print("M\x05");

  // Tripometer / Trial Miles Left
  cb420_setCursor(14, 1);
  if (isTrialRunning(s) && s.trialEndMiles > 0)
    cb420_printFloat1000(max(0, s.trialEndMiles - s.odometer));
  else
    cb420_printFloat1000(v.tripometer);
  cb420_printChar('\xC5');


  // Handle Trial Mode

  if (isTrialEnabled(s)) {
    if (!isTrialRunning(s)) {
      cb420_setCursor(0, 2);
      cb420_print(progStr(MSG_TRIAL_ACTIVATION));
      return;
    }
  }

  // Display "REGEN FAULT" if there is one
  static byte rgeStatus = 0;
  static byte prgeStatus = 0; // Keep track of when quick LCD refresh is needed
  if (v.regenErr && cMillis % 5000 < 2000) {
    cb420_setCursor(4, 2);
    cb420_clearRow();
    rgeStatus = 0;
    cb420_print("REGEN FAULT");
  } else if (isTrialExpired(s, v)) {
    cb420_setCursor(3, 2);
    cb420_clearRow();
    rgeStatus = 1;
    cb420_print("TRIAL EXPIRED");
  } else if (isTrialEnabled(s) && cMillis % 5000 < 2000) {
    cb420_setCursor(4, 2);
    cb420_clearRow();
    rgeStatus = 2;
    cb420_print("Trial Active");
  } else {
    // Display temperatures
    rgeStatus = 3;
    cb420_setCursor(0, 2);
    cb420_printChar(0);
    cb420_printInt(v.escTemp1 + 0.5);
    cb420_setCursor(4, 2);
    cb420_printChar(2);
    cb420_printInt(v.motorTemp1 + 0.5);
    cb420_setCursor(9, 2);
    cb420_printChar(1);
    cb420_printInt(v.escTemp2 + 0.5);
    cb420_setCursor(13, 2);
    cb420_printChar(3);
    cb420_printInt(v.motorTemp2 + 0.5);
    cb420_setCursor(17, 2);
    cb420_print("[\x07]");
  }
  if (prgeStatus != rgeStatus) {
    prgeStatus = rgeStatus;
    cb420_setUpdateCursor(0, 2);
    cb420_sendPacket(20);
  }

  // Efficiency in watt-hours per mile
  cb420_setCursor(0, 3);
  cb420_printFloat100(v.wattHoursPerMile);
  cb420_print("Wh/m");
  cb420_setCursor(9, 3);
  cb420_printChar(5);
  cb420_setCursor(10, 3);
  cb420_printChar(7);

  // Real-time clock display / trial minutes left
  if (isTrialExpired(s, v)) {
    cb420_setCursor(13, 3);
    cb420_print("EXPIRED");
  }
  else if (isTrialRunning(s) && !isDateZero(s.trialEndDate)) {
    cb420_setCursor(12, 3);
    cb420_printInt(getMinutesBetweenDates(v.dt, s.trialEndDate));
    cb420_print("Min");
  } else {
    cb420_setCursor(12, 3);
    cb420_print("00:00:00");
    cb420_setCursor(13, 3);
    cb420_printIntR(v.dt.hour);
    cb420_setCursor(16, 3);
    cb420_printIntR(v.dt.minute);
    cb420_setCursor(19, 3);
    cb420_printIntR(v.dt.second);
  }
}



/**
   Charging display view. Shown during chargine
   @param v v object
   @param s Settings object
*/
void mpptDisplay(values & v, Settings & s, float & mpptMerit, float & mpptAdd) {
  unsigned long cMillis = millis();
  
  cb420_clear();
  cb420_setCursor(0, 0);
  cb420_printFloat100(v.dispVolts);
  cb420_printChar('V'); // Volts

  cb420_setCursor(6, 0);
  cb420_printFloat1000(s.batteryAH - s.batteryUsedAH);
  cb420_print("AH"); // Amp-hours remaining

  cb420_setCursor(15, 0);
  cb420_printFloat100(v.dispAmps2);
  cb420_printChar('A');

  cb420_setCursor(0, 1);
  cb420_printFloat1000(v.setRegen);
  cb420_print("RG");

  cb420_setCursor(12, 1);
  cb420_printFloat(mpptMerit, 2);

  cb420_setCursor(0, 2);
  cb420_printFloat(mpptAdd, 4);
}


/**
   Get throttle lights from battery capacity
   @param battery battery float from 0 to 1
   @param flash boolean used to blink when really low
*/
byte getThrLights(float battery, boolean flash) {
  if (battery < 0.1) {
    if (flash)
      return 1;
    else
      return 0;
  } else if (battery < 0.2)
    return 1;
  else if (battery < 0.4)
    return 3;
  else if (battery < 0.6)
    return 2;
  else if (battery < 0.8)
    return 6;
  else return 4;
}

/**
   Set lights on the throttle
   @param val light pattern (3 least significant bits)
*/
void sendThrLights(byte val) {
  for (int x = 0; x < 3; x++) {
    pulse_thr((val >> x) & 1);
    delayMicroseconds(50);
  }
  pulse_thr(2);
  delayMicroseconds(100);
}
void pulse_thr(byte val) {
  if (val == 0)
    pulse_dpx_thr(18);
  else if (val == 1)
    pulse_dpx_thr(43);
  else if (val == 2) {
    pulse_dpx_thr(84);
  }
}
inline void pulse_dpx_thr(byte us) {
  noInterrupts();
  digitalWrite(THR_LED, HIGH);
  delayMicroseconds(us);
  digitalWrite(THR_LED, LOW);
  interrupts();
}

// intelligent LCD battery icon
void setBatteryIcon(float pct, boolean strobe) {
  int bc = pct * 16;
  for (int x = 6; x > 1; x--) {
    iconBattery[x] = B10001;
    if (bc > 2) {
      iconBattery[x] |= B01110;
    } else if (bc == 2) {
      iconBattery[x] |= B01100;
    } else if (bc == 1) {
      iconBattery[x] |= B01000;
    }
    bc -= 3;
  }
  if (pct == 0 && strobe) {
    iconBattery[2] = B10001;
    iconBattery[3] = B01010;
    iconBattery[4] = B00100;
    iconBattery[5] = B01010;
    iconBattery[6] = B10001;
  }
}
