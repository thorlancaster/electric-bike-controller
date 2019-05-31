
/**
   This file handles loading/saving settings to/from EEPROM
   and pulling strings and data from PROGMEM
   @author Thor Lancaster
*/

char gspStrBuf[80];

char* getStr(const char* const* arr, int8_t idx) {
  strcpy_P(gspStrBuf, (char *)pgm_read_word(&(arr[idx])));
  return gspStrBuf;
}

char* progStr(const char* ptr) {
  byte len = strlen_P(ptr);
  for (byte k = 0; k < len; k++) {
    gspStrBuf[k] = pgm_read_byte_near(ptr + k);
  }
  gspStrBuf[len] = 0;
  return gspStrBuf;
}

byte gotCustChar[8];
void getCustChar(const byte* iconSet, uint16_t index) {
  for (byte x = 0; x < 8; x++) {
    gotCustChar[x] = pgm_read_byte_near(iconSet + (index * 8) + x);
  }
}

int promptInt(int num, char* message, int maxVal) {
  num = constrain(num, 0, 30000);
  cb420_clear();
  if (strlen(message) > 20)
    cb420_setCursor(0, 0);
  else
    cb420_setCursor(9 - (strlen(message) - 1) / 2, 0);
  cb420_print(message);
  cb420_updateAll();
  float setNum = num;
  float inc = 0;
  boolean pLeft = false;
  boolean pRight = true;
  while (true) {
    cb420_setCursor(8, 2);
    cb420_clearRow();
    cb420_printInt(floor(setNum + 0.5));
    boolean left = getBrake(false);
    boolean right = getBrake(true);
    if (left)
      setNum -= inc;
    if (right)
      setNum += inc;
    if (left && !pLeft)
      setNum--;
    if (right && !pRight)
      setNum++;
    if (left || right)
      inc *= 1.03;
    else {
      inc = 0.03;
      setNum = floor(setNum + 0.5);
    }
    if (getThrottle() > 0.5) {
      cb420_setCursor(0, 0);
      cb420_clearRow();
      cb420_updateAll();
      while (getThrottle() > 0.2);
      return floor(setNum + 0.5);
    }
    if (getThrBtn()) {
      cb420_setCursor(0, 0);
      cb420_clearRow();
      cb420_updateAll();
      delay(50);
      while (getThrBtn());
      delay(20);
      return floor(setNum + 0.5);
    }
    pLeft = left;
    pRight = right;
    setNum = constrain(setNum, 0, maxVal);
    cb420_updateAll();
  }
}

//float promptFloat(int num, char* message, int maxval) {
//  cb420_clear();
//  cb420_setCursor(0, 0);
//  cb420_print(message);
//  cb420_updateAll();
//  delay(2000);
//}

int showMenu(const char* const* options, const byte* iconStrip, byte len) {
  int8_t optPtr = 0;
  int8_t selNum = 0;
  boolean pLeft = false;
  boolean pRight = false;
  cb420_clear();
  while (true) {
    cb420_clear();
    cb420_setCustChar(6, iconLeftArrow, true);
    cb420_setCustChar(7, iconRightArrow, true);

    if (selNum < 0) {
      optPtr += selNum;
      selNum = 0;
    } if (selNum > 3) {
      optPtr += (selNum - 3);
      selNum = 3;
    } if (optPtr > len - 4)
      optPtr = len - 4;
    if (optPtr < 0)
      optPtr = 0;

    for (byte y = 0; y < 4; y++) {
      byte pos = y + optPtr;
      if (pos < len) {
        getCustChar(iconStrip, pos);
        cb420_setCustChar(y, gotCustChar, true);
        char* thisItem = getStr(options, pos);
        cb420_setCursor(9 - ((strlen(thisItem) - 1) / 2), y);
        cb420_print(thisItem);
        cb420_setCursor(1, y);
        cb420_printChar(y);
        cb420_setCursor(18, y);
        cb420_printChar(y);
      }
    }
    cb420_setCursor(0, selNum);
    cb420_printChar(7);
    cb420_setCursor(19, selNum);
    cb420_printChar(6);
    cb420_updateAll();
    while (true) {
      if (getThrottle() > 0.5) {
        cb420_setCursor(0, selNum);
        cb420_printChar('X');
        cb420_setCursor(19, selNum);
        cb420_printChar('X');
        cb420_updateAll();
        while (getThrottle() > 0);
        return -1;
      }
      boolean right = getBrake(false);
      boolean left = getBrake(true);
      byte br = 0;
      if (left && !pLeft)
        br = 1;
      if (right && !pRight)
        br = 2;
      pLeft = left;
      pRight = right;
      if (br == 1) {
        selNum++; break;
      } else if (br == 2) {
        selNum--; break;
      }

      if (getThrBtn()) {
        cb420_setCursor(0, selNum);
        cb420_printChar(6);
        cb420_setCursor(19, selNum);
        cb420_printChar(7);
        cb420_updateAll();
        delay(200);
        if (!getThrBtn)
          break;
        while (getThrBtn()) {
        }
        delay(50);
        return optPtr + selNum;
      }
    }
  }
}

void setClockUI() {
  byte iconPtr1[] = {B00000, B00000, B00001, B00011, B00111, B01111, B11111, B11111};
  byte iconPtr2[] = {B00000, B00000, B10000, B11000, B11100, B11110, B11111, B11111};
  int8_t second, minute, hour, day, month, year;
  byte cursorPos = 0;
  boolean pressed = false;
  boolean exitFlag = false;
  cb420_clear();
  cb420_setCustChar(1, iconPtr1, true);
  cb420_setCustChar(2, iconPtr2, true);
  cb420_updateAll();
  ds3231_getTimeInt(&second, &minute, &hour, &day, &month, &year);
  byte sec = second;
  while (true) { // Sync setting clock and RTC exactly
    ds3231_getTime(&sec, 0, 0, 0, 0, 0);
    if (sec != second) break;
  }
  unsigned long msTicker = millis();
  ds3231_getTimeInt(&second, &minute, &hour, &day, &month, &year);
  while (true) {
    cb420_clear();
    // Display Time and Date
    cb420_setCursor(0, 0);
    cb420_print("00:00:00 \x02\x03 00/00/00");
    cb420_setCursor(1, 0);
    cb420_printIntR(hour);
    cb420_setCursor(4, 0);
    cb420_printIntR(minute);
    cb420_setCursor(7, 0);
    cb420_printIntR(second);
    cb420_setCursor(13, 0);
    cb420_printIntR(day);
    cb420_setCursor(16, 0);
    cb420_printIntR(month);
    cb420_setCursor(19, 0);
    cb420_printIntR(year);

    // Display setting cursor
    cb420_setCursor(cursorPos * 3, 1);
    cb420_printChar(1);
    cb420_printChar(2);

    // User Interface
    boolean leftBrake = getBrake(false);
    boolean rightBrake = getBrake(true);
    boolean throttle = getThrottle() > 0.3;
    boolean button = getThrBtn();
    if (!leftBrake && !rightBrake && !throttle && !button)
      pressed = false;

    if (throttle && !pressed && cursorPos > 0)
      cursorPos--;
    if (button && !pressed && cursorPos < 6)
      cursorPos++;
    if (leftBrake && !pressed) {
      switch (cursorPos) {
        case 0: hour--; break;
        case 1: minute--; break;
        case 2: second--; msTicker = millis(); break;
        case 4: day--; break;
        case 5: month--; break;
        case 6: year--; break;
      }
    }
    if (rightBrake && !pressed) {
      switch (cursorPos) {
        case 0: hour++; break;
        case 1: minute++; break;
        case 2: second++; msTicker = millis(); break;
        case 4: day++; break;
        case 5: month++; break;
        case 6: year++; break;
      }
    }

    if (leftBrake || rightBrake || throttle || button)
      pressed = true;
    // Make clock act like clock
    boolean tickedThisCycle = false;
    if (millis() - msTicker > 1000) {
      msTicker += 1000;
      second++;
      tickedThisCycle = true;
    }
    if (second > 59) {
      minute++;
      second = 0;
    }
    if (second < 0) {
      minute--;
      second = 59;
    }
    if (minute > 59) {
      hour++;
      minute = 0;
    }
    if (minute < 0) {
      hour--;
      minute = 59;
    }
    if (hour < 0) hour = 23;
    if (hour > 23) hour = 0;
    if (day < 0) day = 31;
    if (day > 31) day = 0;
    if (month < 0) month = 12;
    if (month > 12) month = 0;
    if (year < 0) year = 99;
    if (year > 99) year = 0;

    // Set clock and exit to sync clocks exactly
    if (tickedThisCycle && exitFlag) {
      ds3231_setTime(second, minute, hour, day, month, year);
      return;
    }

    // Update display
    if (!exitFlag)
      cb420_sendPacket(2);
    if (cb420_wptr > 40)
      cb420_wptr = 0; // Hack to only update first two rows

    // Handle exit condition
    if (cursorPos == 3 && leftBrake && rightBrake) {
      exitFlag = true; // Set flag to sync clock and exit on next second tick
    }
  }
}

void saveTrip(Settings & s, int offset) {
  offset += EEPROMWriteDecimal(offset, s.odometer);
  offset += EEPROMWriteDecimal(offset, s.batteryUsedAH);
  offset += EEPROMWriteDecimal(offset, s.batteryUsedWH);
}
void loadTrip(Settings & s, int offset) {
  s.odometer = EEPROMReadDecimal(offset); offset += 4;
  s.batteryUsedAH = EEPROMReadDecimal(offset); offset += 4;
  s.batteryUsedWH = EEPROMReadDecimal(offset); offset += 4;
}

int writeSettings(Settings & s, int offset) {
  offset++;
  offset += EEPROMWriteLong(offset, s.password);
  offset += EEPROMWrite(offset, s.pwLength);
  offset += EEPROMWriteLong(offset, s.sudoPassword);
  offset += EEPROMWrite(offset, s.sudoPwLength);
  offset += EEPROMWrite(offset, s.lockLevel);
  offset += EEPROMWrite(offset, s.maxSpeed);
  offset += EEPROMWriteInt(offset, s.stallWatts1);
  offset += EEPROMWriteInt(offset, s.stallWatts2);
  offset += EEPROMWriteInt(offset, s.boostWatts1);
  offset += EEPROMWriteInt(offset, s.boostWatts2);
  offset += EEPROMWriteInt(offset, s.maxAmps1);
  offset += EEPROMWriteInt(offset, s.maxAmps2);
  offset += EEPROMWrite(offset, s.maxESCTemp1);
  offset += EEPROMWrite(offset, s.maxESCTemp2);
  offset += EEPROMWrite(offset, s.maxMotorTemp1);
  offset += EEPROMWrite(offset, s.maxMotorTemp2);
  offset += EEPROMWriteInt(offset, s.batteryLVC);
  offset += EEPROMWriteDecimal(offset, s.batteryAH);
  offset += EEPROMWrite(offset, s.backlight);
  offset += EEPROMWriteDecimal(offset, s.speedDiv);
  offset += EEPROMWrite(offset, s.ccTolerance);
  offset += EEPROMWrite(offset, s.ccIntegralMult);
  offset += EEPROMWrite(offset, s.maxChargeCurrent);
  offset += EEPROMWrite(offset, s.maxChargeVoltage);

  offset += EEPROMWriteBoolean(offset, s.sudoEnabled);
  offset += EEPROMWriteInt(offset, s.sudoMaxSpeed);
  offset += EEPROMWriteInt(offset, s.sudoMaxWatts);
  offset += EEPROMWrite(offset, s.ds3231Offset);
  offset += EEPROMWrite(offset, s.sudoMaxAmps);
  offset += EEPROMWrite(offset, s.sudoMaxESCTemp);
  offset += EEPROMWrite(offset, s.sudoMaxMotorTemp);

  offset += EEPROMWriteInt(offset, s.trialMinutes);
  offset += EEPROMWriteDecimal(offset, s.trialMiles10);
  offset += EEPROMWriteDateTime(offset, s.trialEndDate);
  offset += EEPROMWriteDecimal(offset, s.trialEndMiles);
  return offset;
}

int loadSettings(Settings & s, int offset) {
  offset++;
  s.password = EEPROMReadLong(offset); offset += 4;
  s.pwLength = EEPROMRead(offset++);
  s.sudoPassword = EEPROMReadLong(offset); offset += 4;
  s.sudoPwLength = EEPROMRead(offset++);
  s.lockLevel = EEPROMRead(offset++);
  s.maxSpeed = EEPROMRead(offset++);
  s.stallWatts1 = EEPROMReadInt(offset); offset += 2;
  s.stallWatts2 = EEPROMReadInt(offset); offset += 2;
  s.boostWatts1 = EEPROMReadInt(offset); offset += 2;
  s.boostWatts2 = EEPROMReadInt(offset); offset += 2;
  s.maxAmps1 = EEPROMReadInt(offset); offset += 2;
  s.maxAmps2 = EEPROMReadInt(offset); offset += 2;
  s.maxESCTemp1 = EEPROMRead(offset++);
  s.maxESCTemp2 = EEPROMRead(offset++);
  s.maxMotorTemp1 = EEPROMRead(offset++);
  s.maxMotorTemp2 = EEPROMRead(offset++);
  s.batteryLVC = EEPROMReadInt(offset); offset += 2;
  s.batteryAH = EEPROMReadDecimal(offset); offset += 4;
  s.backlight = EEPROMRead(offset++);
  s.speedDiv = EEPROMReadDecimal(offset); offset += 4;
  s.ccTolerance = EEPROMRead(offset++);
  s.ccIntegralMult = EEPROMRead(offset++);
  s.maxChargeCurrent = EEPROMRead(offset++);
  s.maxChargeVoltage = EEPROMRead(offset++);

  s.sudoEnabled = EEPROMReadBoolean(offset++);
  s.sudoMaxSpeed = EEPROMReadInt(offset); offset += 2;
  s.sudoMaxWatts = EEPROMReadInt(offset); offset += 2;
  s.ds3231Offset = EEPROMRead(offset++);
  s.sudoMaxAmps = EEPROMRead(offset++);
  s.sudoMaxESCTemp = EEPROMRead(offset++);
  s.sudoMaxMotorTemp = EEPROMRead(offset++);

  s.trialMinutes = EEPROMReadInt(offset); offset += 2;
  s.trialMiles10 = EEPROMReadDecimal(offset); offset += 4;
  offset += EEPROMReadDateTime(offset, s.trialEndDate);
  s.trialEndMiles = EEPROMReadDecimal(offset); offset += 4;

  return offset;
}

void defaultSettings(Settings & s) {
  s.password = 0;
  s.pwLength = 0;
  s.sudoPassword = 0;
  s.sudoPwLength = 0;
  s.sudoEnabled = true;
  s.lockLevel = 0;
  s.maxSpeed = 28;
  s.stallWatts1 = 750;
  s.stallWatts2 = 750;
  s.boostWatts1 = 20;
  s.boostWatts2 = 20;
  s.maxAmps1 = 35;
  s.maxAmps2 = 35;
  s.maxESCTemp1 = 65;
  s.maxESCTemp2 = 65;
  s.maxMotorTemp1 = 105;
  s.maxMotorTemp2 = 105;
  s.batteryLVC = 40;
  s.batteryAH = 20;
  s.batteryUsedAH = 0;
  s.batteryUsedWH = 0;
  s.backlight = 6;
  s.speedDiv = 5.165;
  s.ccTolerance = 1;
  s.ccIntegralMult = 10;
  s.maxChargeCurrent = 6;
  s.maxChargeVoltage = 50;
  s.ds3231Offset = 72654
                   ; // Increase to speed up // TODO is this correct?
  // 127 is the default value
  // 97 is 2 seconds a week slow
  // 69 is right on

  s.sudoMaxSpeed = 45;
  s.sudoMaxWatts = 1600;
  s.sudoMaxAmps = 42;
  s.sudoMaxESCTemp = 72;
  s.sudoMaxMotorTemp = 95;
}
byte verifySettings(Settings & s, int offset, byte sig) {
  if (EEPROMRead(offset) != sig) {
    // E2END is EEPROM size, defined by Arduino
    for (int x = 0; x < E2END; x++) {
      EEPROMWrite(x, 0); // Clear the EEPROM
    }
    defaultSettings(s);
    writeSettings(s, offset);
    EEPROMWrite(offset, sig);
    return 1;
  }
  return 0;
}

float getBattery(settings & s) {
  return (s.batteryAH - s.batteryUsedAH) / s.batteryAH;
}
