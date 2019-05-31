/**
  Miscellaneous code that does not belong anywhere else
  @author Thor Lancaster
*/


/**
   Display a message for a set time, then return
   @param message message to display
   @param milliseconds how long to halt for while displaying the message
*/
void alert(char* message, unsigned int milliseconds) {
  cb420_clear();
  cb420_setCursor(0, 0);
  cb420_print(message);
  cb420_updateAll();
  delay(milliseconds);
}

void zeroDate(DateTime &dt) {
  dt.hour = 0;
  dt.minute = 0;
  dt.second = 0;
  dt.day = 0;
  dt.month = 0;
  dt.year = 0;
}
boolean isDateZero(DateTime &dt) {
  return dt.hour == 0 && dt.minute == 0 && dt.second == 0 && dt.day == 0 && dt.month == 0 && dt.year == 0;
}

/**
   Compare two dates
   @param d1 first date
   @param d2 second date
   @return positive if d2 > d1, 0 if d2 == d1, negative if d2 < d1
*/
int compareDates(DateTime &d1, dateTime &d2) {
  if (d1.year < d2.year)
    return 6;
  if (d1.year > d2.year)
    return -6;

  if (d1.month < d2.month)
    return 5;
  if (d1.month > d2.month)
    return -5;

  if (d1.day < d2.day)
    return 4;
  if (d1.day > d2.day)
    return -4;

  if (d1.hour < d2.hour)
    return 3;
  if (d1.hour > d2.hour)
    return -3;

  if (d1.minute < d2.minute)
    return 2;
  if (d1.minute > d2.minute)
    return -2;

  if (d1.second < d2.second)
    return 1;
  if (d1.second > d2.second)
    return -1;

  return 0;
}



unsigned int getMinutesBetweenDates(DateTime &d1, DateTime &d2) {
  if (abs(compareDates(d1, d2) <= 1))
    return 0;
  DateTime temp;
  int offset = 16384;
  int addend = 8192;
  byte quit = 0;
  do {
    addMinutesToDate(d1, temp, offset);
    int res = compareDates(temp, d2);

    if (res <= 1 && res >= -1) {
      return offset;
    }
    if (res < 1) {
      offset -= addend;
    }
    if (res > 1) {
      offset += addend;
    }
    if (addend > 1)
      addend /= 2;
    else
      quit++;
  } while (addend > 0 && quit < 2);
  return -1;
}

void printDate(DateTime &dt) {
  Serial1.print(dt.hour);
  Serial1.print(' ');
  Serial1.print(dt.minute);
  Serial1.print(' ');
  Serial1.print(dt.second);
  Serial1.print(' ');
  Serial1.print(dt.day);
  Serial1.print(' ');
  Serial1.print(dt.month);
  Serial1.print(' ');
  Serial1.print(dt.year);
}

/**
    Add minutes to a DateTime
    @param src source date
    @param dest destiniation date objet to write to
    @param minutes minutes to add
*/
void addMinutesToDate(DateTime &src, DateTime &dest, int minutes) {
  unsigned int minute = src.minute;
  unsigned int hour = src.hour;
  unsigned int day = src.day;
  unsigned int month = src.month;
  unsigned int year = src.year;
  unsigned int carry = 0;

  minute += minutes;
  if (minute > 59) {
    carry = minute / 60;
    minute -= carry * 60;
    hour += carry;
  }
  if (hour > 23) {
    carry = hour / 24;
    hour -= carry * 24;
    day += carry;
  }
  // Months are really REALLY fun
  while (true) {
    if (month == 1 && day > 31) {
      day -= 31;
      month++;
    } else if (month == 2 && year % 4 != 0 && day > 28) {
      day -= 28;
      month++;
    } else if (month == 2 && year % 4 == 0 && day > 29) {
      day -= 29; // Leap Year
      month++;
    } else if (month == 3 && day > 31) {
      day -= 31;
      month++;
    } else if (month == 4 && day > 30) {
      day -= 30;
      month++;
    } else if (month == 5 && day > 31) {
      day -= 31;
      month++;
    } else if (month == 6 && day > 30) {
      day -= 30;
      month++;
    } else if (month == 7 && day > 31) {
      day -= 31;
      month++;
    } else if (month == 8 && day > 31) {
      day -= 31;
      month++;
    } else if (month == 9 && day > 30) {
      day -= 30;
      month++;
    } else if (month == 10 && day > 31) {
      day -= 31;
      month++;
    } else if (month == 11 && day > 30) {
      day -= 30;
      month++;
    } else if (month == 12 && day > 31) {
      day -= 30;
      month++;
    } else break;
    if (month > 12) {
      month = 1;
      year++;
    }
  }
  dest.second = src.second;
  dest.minute = minute;
  dest.hour = hour;
  dest.day = day;
  dest.month = month;
  dest.year = year;
}


void calcVals(values &v, Settings &s) {
  float throttle = v.throttleIn;
  if (v.ccSetSpeed >= 1) {
    throttle = 0.5 + (v.ccSetSpeed - v.speed + v.ccIntegral) / s.ccTolerance;
    if (throttle > 0 && throttle < 1) { // If in range, use integral to maintain exact speed
      v.ccIntegral += (v.ccSetSpeed - v.speed) * s.ccIntegralMult / 40000.0;
    } // Otherwise, don't use integral because it would cause initial cruise control overshoot
    else {
      v.ccIntegral *= 0.99;
    }
    v.ccIntegral = constrain(v.ccIntegral, -s.ccTolerance, s.ccTolerance);
    throttle = max(throttle, v.throttleIn);
  }
  throttle = constrain(throttle, 0, 1);
  // Decrease current if battery nears or hits LVC. vBatt>LVC+1= No Restriction; vBatt < LVC-1= Full Restriction
  float battAtten = constrain((v.volts + 1 - s.batteryLVC) / 2, 0, 1);
  if (s.batteryUsedAH >= s.batteryAH)
    battAtten = 0; // Disable throttle if battery's Amp Hours are depleted
  float speedAtten = constrain((s.maxSpeed + 1 - v.speed) / 2, 0, 1);
  float fullAmps1 = min(s.maxAmps1, (s.stallWatts1 + (s.boostWatts1 * v.speed)) / v.volts);
  float fullAmps2 = min(s.maxAmps2, (s.stallWatts2 + (s.boostWatts2 * v.speed)) / v.volts);
  fullAmps1 *= battAtten;
  fullAmps2 *= battAtten;
  fullAmps1 *= speedAtten;
  fullAmps2 *= speedAtten;
  v.setAmps1 = throttle * fullAmps1;
  v.setAmps2 = throttle * fullAmps2;
  if (v.speed > 0) {
    // Calculate Watt-hours per Mile
    float whpm = (v.volts * (v.amps1 + v.amps2)) / v.speed;
    // Don't let WHPM get too large to fit on the display and smooth it a little
    v.wattHoursPerMile = min(v.wattHoursPerMile * 0.95 + whpm * 0.05, 999);
  }
}

void dispVals(values &v) {
  float dispVolts;
  float dispAmps1;
  float dispAmps2;
  float dispSpeed;

  v.dispVolts = v.dispVolts * 0.9 + v.volts * 0.1;
  v.dispAmps1 = v.dispAmps1 * 0.9 + v.amps1 * 0.1;
  v.dispAmps2 = v.dispAmps2 * 0.9 + v.amps2 * 0.1;
  v.dispSpeed = v.dispSpeed * 0.85 + v.speed * 0.15;
  if (v.speed < 0.5)
    v.dispSpeed = 0;
}



/*
   Get the brake button status
   @param right true to read right brake, false to read left
   @return true if specified brake handle is depressed
*/
boolean getBrake(boolean right) {
  if (right)
    return !digitalRead(BRAKE_R);
  else
    return !digitalRead(BRAKE_L);
}

/*
   Get the state of the throttle button
   @return true if pressed
*/
boolean getThrBtn() {
  return digitalRead(THR_BTN) ^ THR_BUTTON_INVERTED ^ 1;
}
/**
   @return throttle position,from 0.0 to 1.0
*/
float getThrottle() {
  //return 0;
  int inSig = analogRead(THR_IN);
  if(inSig > 950 || inSig < 204)
    return 0;
  return (inSig-204)/634.0;
}

float getResistanceH(float reading, float bottom) {
  if (reading > 0.995)
    return -1;
  return bottom * (1 / reading - 1);
}
float getResistanceL(float reading, float top) {
  if (reading > 0.995)
    return -1;
  return (-top * reading) / (reading - 1);
}

/**
   Zero out v in v object (normally they'd be undefined)
   @param v v object to zero out
*/
void zeroValues(values & v) {
  v.volts = 0;
  v.amps1 = 0;
  v.amps2 = 0;
  v.setAmps1 = 0;
  v.setAmps2 = 0;
  v.speed = 0;
  v.dispVolts = 0;
  v.dispAmps1 = 0;
  v.dispAmps2 = 0;
  v.escTemp1 = 0;
  v.motorTemp1 = 0;
  v.escTemp2 = 0;
  v.motorTemp2 = 0;
  v.throttleIn = 0;
  v.regenMode = false;
  v.setRegen = 0;
  v.setAmps1 = 0;
  v.setAmps2 = 0;
  v.ccSetSpeed = 0;
  v.ccIntegral = 0;
  v.brake = false;
  v.tripometer = 0;
  v.wattHoursPerMile = 0;
  v.regenErr = false;
  v.headlight = false;
  v.busyMillis = 0;
  v.mpptCharge = false;

//  v.dt.second = 0; // These are initially set when the clock is read for the first time
//  v.dt.minute = 0;
//  v.dt.hour = 0;
//  v.dt.day = 0;
//  v.dt.month = 0;
//  v.dt.year = 0;
}

/**
   Convert from decimal to BCD
   @param val decimal value, 0 to 99
   @return BCD value
*/
byte decToBcd(byte val) {
  return ((val / 10 * 16) + (val % 10));
}

/**
   Convert from BCD to decimal
   @param val BCD value
   @return decimal value
*/
byte bcdToDec(byte val) {
  return ((val / 16 * 10) + (val % 16));
}

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
