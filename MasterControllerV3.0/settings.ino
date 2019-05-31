
/**
   This file handles the settings menus and changing of v
   @author Thor Lancaster
*/


/** To create new menu options:
    Create a xxxSettingsIcons[] PROGMEM byte array, 8 bytes per icon
    Create const char PROGMEM v for prompts
    Create a *const PROMGEM char pointer array to hold the above "strings"
    Define the length of said char pointer array with a const byte
    Use the main settings menu below as a template
*/

const byte mainSettingsIcons[] PROGMEM {
  B01111, B01110, B11100, B11111, B00111, B01110, B01100, B10000,
  B01110, B01010, B01010, B01110, B01110, B11111, B11111, B01110,
  B01110, B11111, B11111, B11111, B11111, B11111, B11111, B11111,
  B01110, B10001, B00001, B00010, B00100, B00100, B00000, B10101,
  B00000, B01110, B01010, B11111, B11111, B11111, B11111, B11111,
  B11111, B11001, B10111, B11011, B11101, B10011, B11111, B01110
};
const char s_ST0[] PROGMEM = "Power";
const char s_ST1[] PROGMEM = "Temperature";
const char s_ST2[] PROGMEM = "Battery";
const char s_ST3[] PROGMEM = "Misc";
const char s_ST4[] PROGMEM = "Security";
const char s_ST5[] PROGMEM = "Sudo";
const char *const settingsTxt[] PROGMEM = {s_ST0, s_ST1, s_ST2, s_ST3, s_ST4, s_ST5};
const byte settingsTxtLen = 6;

void settingsMainMenu(Settings &s) {
  while (true) {
    int8_t len = settingsTxtLen;
    if (!s.sudoEnabled)
      len = settingsTxtLen - 1;

    int8_t res = showMenu(settingsTxt, mainSettingsIcons, len);
    switch (res) {
      case -1:
        writeSettings(s, EEP_OFFSET);
        saveTrip(s, EEP_OFFSET + EEP_TRIP_OFFSET);
        return;
      case 0:
        settingsPowerMenu(s);
        break;
      case 1:
        settingsTemperatureMenu(s);
        break;
      case 2:
        settingsBatteryMenu(s);
        break;
      case 3:
        settingsMiscMenu(s);
        break;
      case 4:
        if (s.lockLevel & 4) {
          if (lockPassword(progStr(MSG_SETTINGS_SECURITY_PASSWORD), s.password, s.pwLength))
            settingsSecurityMenu(s);
          else {
            alert(progStr(MSG_WRONG_PASSWORD), 2000);
          }
        } else {
          settingsSecurityMenu(s);
        }
        break;
      case 5:
        if (s.lockLevel & 4) {
          if (lockPassword(progStr(MSG_SETTINGS_SUDO_PASSWORD), s.password, s.pwLength))
            settingsSudoMenu(s);
          else {
            alert(progStr(MSG_WRONG_PASSWORD), 2000);
          }
        } else {
          settingsSudoMenu(s);
        }
        break;
    }
  }
}


const byte powerSettingsIcons[] PROGMEM {
  B10101, B11111, B11011, B10011, B11011, B11011, B10001, B11111,
  B10101, B11111, B10011, B11101, B11011, B10111, B10001, B11111,
  B01010, B11111, B11011, B10011, B11011, B11011, B10001, B11111,
  B01010, B11111, B10011, B11101, B11011, B10111, B10001, B11111,
  B00100, B01110, B11111, B11111, B11011, B11011, B11011, B11111,
  B00100, B01110, B11111, B11111, B10101, B10101, B10101, B11111,
  B10101, B01010, B10101, B01010, B10101, B10000, B10000, B10000
};
const char sp_ST0[] PROGMEM = "Stall Watts Frnt";
const char sp_ST1[] PROGMEM = "Stall Watts Rear";
const char sp_ST2[] PROGMEM = "Max Amps  Frnt";
const char sp_ST3[] PROGMEM = "Max Amps  Rear";
const char sp_ST4[] PROGMEM = "Boost Power Frnt";
const char sp_ST5[] PROGMEM = "Boost Power Rear";
const char sp_ST6[] PROGMEM = "Max Speed";
const char *const settingsPowerTxt[] PROGMEM = {sp_ST0, sp_ST1, sp_ST2, sp_ST3, sp_ST4, sp_ST5, sp_ST6};
const byte settingsPowerTxtLen = 7;

void settingsPowerMenu(Settings &s) {
  while (true) {
    int8_t res = showMenu(settingsPowerTxt, powerSettingsIcons, settingsPowerTxtLen);
    switch (res) {
      case -1:
        return;
      case 0:
        s.stallWatts1 = promptInt(s.stallWatts1, getStr(settingsPowerTxt, 0), s.sudoMaxWatts * 0.75);
        break;
      case 1:
        s.stallWatts2 = promptInt(s.stallWatts2, getStr(settingsPowerTxt, 1), s.sudoMaxWatts * 0.75);
        break;
      case 2:
        s.maxAmps1 = promptInt(s.maxAmps1, getStr(settingsPowerTxt, 2), s.sudoMaxAmps);
        break;
      case 3:
        s.maxAmps2 = promptInt(s.maxAmps2, getStr(settingsPowerTxt, 3), s.sudoMaxAmps);
        break;
      case 4:
        s.boostWatts1 = promptInt(s.boostWatts1, getStr(settingsPowerTxt, 4), 250);
        break;
      case 5:
        s.boostWatts2 = promptInt(s.boostWatts2, getStr(settingsPowerTxt, 5), 250);
        break;
      case 6:
        s.maxSpeed = promptInt(s.maxSpeed, getStr(settingsPowerTxt, 6), s.sudoMaxSpeed);
        break;
    }
  }
}

const byte temperatureSettingsIcons[] PROGMEM {
  B10101, B11111, B11011, B10011, B11011, B11011, B10001, B11111,
  B10101, B11111, B10011, B11101, B11011, B10111, B10001, B11111,
  B01010, B11111, B11011, B10011, B11011, B11011, B10001, B11111,
  B01010, B11111, B10011, B11101, B11011, B10111, B10001, B11111
};
const char st_ST0[] PROGMEM = "Max ESC1 Temp";
const char st_ST1[] PROGMEM = "Max Motor1 Temp";
const char st_ST2[] PROGMEM = "Max ESC2 Temp";
const char st_ST3[] PROGMEM = "Max Motor2 Temp";
const char *const settingsTemperatureTxt[] PROGMEM = {st_ST0, st_ST1, st_ST2, st_ST3};
const byte settingsTemperatureTxtLen = 4;

void settingsTemperatureMenu(Settings &s) {
  while (true) {
    int8_t res = showMenu(settingsTemperatureTxt, temperatureSettingsIcons, settingsTemperatureTxtLen); // TODO change
    switch (res) {
      case -1:
        return;
      case 0:
        s.maxESCTemp1 = promptInt(s.maxESCTemp1, getStr(settingsTemperatureTxt, 0), s.sudoMaxESCTemp);
        break;
      case 1:
        s.maxMotorTemp1 = promptInt(s.maxMotorTemp1, getStr(settingsTemperatureTxt, 1), s.sudoMaxMotorTemp);
        break;
      case 2:
        s.maxESCTemp2 = promptInt(s.maxESCTemp2, getStr(settingsTemperatureTxt, 2), s.sudoMaxESCTemp);
        break;
      case 3:
        s.maxMotorTemp2 = promptInt(s.maxMotorTemp2, getStr(settingsTemperatureTxt, 3), s.sudoMaxMotorTemp);
        break;
    }
  }
}


const byte batterySettingsIcons[] PROGMEM {
  B01110, B10001, B10001, B10001, B10001, B10001, B10001, B11111,
  B01110, B11111, B11111, B10101, B10001, B10001, B10001, B11111,
  B00100, B11111, B01110, B00100, B00000, B01100, B11110, B11110,
  B01110, B10101, B11011, B10101, B11011, B10101, B11011, B11111,
  B01110, B10001, B10001, B10001, B11011, B10101, B11011, B11111,
  B01110, B10001, B10001, B10001, B11111, B10001, B11111, B11111
};
const char sb_ST0[] PROGMEM = "Battery LVC";
const char sb_ST1[] PROGMEM = "Battery HVC";
const char sb_ST2[] PROGMEM = "Battery CHG Amps";
const char sb_ST3[] PROGMEM = "Battery Full AH";
const char sb_ST4[] PROGMEM = "Battery Used AH";
const char sb_ST5[] PROGMEM = "Battery Used WH";
const char *const settingsBatteryTxt[] PROGMEM = {sb_ST0, sb_ST1, sb_ST2, sb_ST3, sb_ST4, sb_ST5};
const byte settingsBatteryTxtLen = 6;

void settingsBatteryMenu(Settings &s) {
  while (true) {
    int8_t res = showMenu(settingsBatteryTxt, batterySettingsIcons, settingsBatteryTxtLen); // TODO change
    switch (res) {
      case -1:
        return;
      case 0:
        s.batteryLVC = promptInt(s.batteryLVC, getStr(settingsBatteryTxt, 0), 100);
        break;
      case 1:
        s.maxChargeVoltage = promptInt(s.maxChargeVoltage, getStr(settingsBatteryTxt, 1), 120);
        break;
      case 2:
        s.maxChargeCurrent = promptInt(s.maxChargeCurrent, getStr(settingsBatteryTxt, 2), 12);
        break;
      case 3:
        s.batteryAH = promptInt(s.batteryAH, getStr(settingsBatteryTxt, 3), 500);
        break;
      case 4:
        s.batteryUsedAH = promptInt(s.batteryUsedAH, getStr(settingsBatteryTxt, 4), s.batteryAH);
        break;
      case 5:
        s.batteryUsedWH = promptInt(s.batteryUsedWH, getStr(settingsBatteryTxt, 5), 30000);
        break;
    }
  }
}

const byte miscSettingsIcons[] PROGMEM {
  B10101, B00000, B01110, B11111, B11111, B11111, B01010, B01010,
  B11100, B10000, B10000, B11000, B10000, B10000, B11100, B00000,
  B00010, B00101, B00100, B00100, B00100, B00100, B10100, B01000,
  B00000, B01110, B10101, B10111, B10001, B01110, B00000, B00000,
  B01110, B10101, B10111, B10001, B01110, B00100, B01110, B11111
};
const char sm_ST0[] PROGMEM = "Backlight";
const char sm_ST1[] PROGMEM = "CC tolerance";
const char sm_ST2[] PROGMEM = "CC Integral Mult";
const char sm_ST3[] PROGMEM = "Set Clock";
const char sm_ST4[] PROGMEM = "Clock spd trim";
const char *const settingsMiscTxt[] PROGMEM = {sm_ST0, sm_ST1, sm_ST2, sm_ST3, sm_ST4};
const byte settingsMiscTxtLen = 5;

void settingsMiscMenu(Settings &s) {
  while (true) {
    int8_t res = showMenu(settingsMiscTxt, miscSettingsIcons, settingsMiscTxtLen);
    switch (res) {
      case -1:
        return;
      case 0:
        s.backlight = promptInt(s.backlight, getStr(settingsMiscTxt, 0), 15);
        cb420_setBacklight(s.backlight);
        break;
      case 1:
        s.ccTolerance = promptInt(s.ccTolerance, getStr(settingsMiscTxt, 1), 10);
        s.ccTolerance = max(1, s.ccTolerance);
        break;
      case 2:
        s.ccIntegralMult = promptInt(s.ccIntegralMult, getStr(settingsMiscTxt, 2), 200);
        break;
      case 3:
        if (isTrialEnabled(s))
          lolAccessDenied();
        else
          setClockUI();
        break;
      case 4:
        s.ds3231Offset = promptInt(s.ds3231Offset, getStr(settingsMiscTxt, 4), 255);
        ds3231_setCalibrationOffset(s.ds3231Offset);
        break;
    }
  }
}

const byte securitySettingsIcons[] PROGMEM {
  B11100, B10100, B11100, B01000, B01000, B01011, B01010, B01011,
  B01110, B01010, B01110, B00100, B00100, B00110, B00101, B00110,
  B01110, B01010, B01110, B00111, B01000, B01100, B00010, B11100,
  B01110, B01010, B01110, B00100, B00000, B01110, B01010, B11111,
  B01100, B00100, B01110, B10101, B11111, B10001, B10101, B01110,
  B00000, B01110, B01010, B11111, B11111, B11111, B11111, B11111
};
const char sc_ST0[] PROGMEM = "Set Password";
const char sc_ST1[] PROGMEM = "Boot Password";
const char sc_ST2[] PROGMEM = "Settings PWord";
const char sc_ST3[] PROGMEM = "Security PWord";
const char sc_ST4[] PROGMEM = "Restrictions...";
const char sc_ST5[] PROGMEM = "Enable SUDO mode";
const char *const settingsSecTxt[] PROGMEM = {sc_ST0, sc_ST1, sc_ST2, sc_ST3, sc_ST4, sc_ST5};
const byte settingsSecTxtLen = 6;

const char sc_msg_enter_master_code[] PROGMEM = " Enter Master Code     For Sudo Mode";
const char sc_msg_sudo_enabled[] PROGMEM = " Sudo Mode Enabled";
const char sc_msg_set_pw[] PROGMEM = " Enter New Password";
const char sc_msg_cfm_pw[] PROGMEM = "Confirm New Password";
const char sc_msg_chg_pw[] PROGMEM = "  Password Changed";
const char sc_msg_nchg_pw[] PROGMEM = " Passwords Not Same";
void settingsSecurityMenu(Settings &s) {
  while (true) {
    byte tmp = settingsSecTxtLen;
    if (s.sudoEnabled) tmp--;
    int8_t res = showMenu(settingsSecTxt, securitySettingsIcons, tmp);
    switch (res) {
      case -1:
        return;
      case 0:
        byte len1;
        unsigned long pw1;
        enterPassword(progStr(sc_msg_set_pw));
        len1 = getPWL();
        pw1 = getPW();
        enterPassword(progStr(sc_msg_set_pw));
        cb420_clear();
        cb420_setCursor(0, 0);
        if (getPW() == pw1 && getPWL() == len1) {
          cb420_print(progStr(sc_msg_chg_pw));
          s.password = pw1;
          s.pwLength = len1;
        } else {
          cb420_print(progStr(sc_msg_nchg_pw));
        }
        cb420_updateAll();
        delay(2000);
        break;
      case 1:
        // Set Password on boot (Y/N)
        bitWrite(s.lockLevel, 0, promptInt(bitRead(s.lockLevel, 0), getStr(settingsSecTxt, 1), 1));
        break;
      case 2:
        // Set password to enter settings (Y/N)
        bitWrite(s.lockLevel, 1, promptInt(bitRead(s.lockLevel, 1), getStr(settingsSecTxt, 2), 1));
        break;
      case 3:
        // Set password to enter settings.security (Y/N)
        bitWrite(s.lockLevel, 2, promptInt(bitRead(s.lockLevel, 2), getStr(settingsSecTxt, 3), 1));
        break;
      case 4:
        settingsRestrictionsMenu(s);
        break;
      case 5:
        if (lockPassword(progStr(sc_msg_enter_master_code), MASTER_PW, MASTER_PWL)) {
          s.sudoEnabled = true;
          alert(progStr(sc_msg_sudo_enabled), 2000);
        } else {
          alert(progStr(MSG_WRONG_PASSWORD), 2000);
        }
        delay(2000);
        break;
    }
  }
}




const byte restrictionsSettingsIcons[] PROGMEM {
  B01110, B01010, B11111, B11011, B11011, B10001, B11011, B11111,
  B01110, B01010, B11111, B10011, B10101, B10101, B10011, B11111,
  B01110, B01010, B11111, B10001, B11011, B11011, B11011, B11111
};

const char scr_ST0[] PROGMEM = "Trial Enable";
const char scr_ST1[] PROGMEM = "Trial Distance";
const char scr_ST2[] PROGMEM = "Trial Time";

const char scr_ST_TD[] PROGMEM = "   Trial Distance       (Miles/10)";
const char scr_ST_TT[] PROGMEM = "Trial Time (Minutes)";

const char *const settingsRestrictionsTxt[] PROGMEM = {scr_ST0, scr_ST1, scr_ST2};
const byte settingsRestrictionsTxtLen = 3;

void settingsRestrictionsMenu(Settings &s) {
  while (true) {
    int8_t res = showMenu(settingsRestrictionsTxt, restrictionsSettingsIcons, settingsRestrictionsTxtLen);
    switch (res) {
      case -1:
        return;
      case 0:
        // Set Trial Enabled
        bitWrite(s.lockLevel, 3, promptInt(bitRead(s.lockLevel, 3), getStr(settingsRestrictionsTxt, 0), 1));
        bitWrite(s.lockLevel, 4, 0);
        break;
      case 1:
        // Set Trial distance
        s.trialMiles10 = promptInt(s.trialMiles10, progStr(scr_ST_TD), 1000);
        bitWrite(s.lockLevel, 4, 0);
        break;
      case 2:
        // Set Trial Time
        s.trialMinutes = promptInt(s.trialMinutes, progStr(scr_ST_TT), 20160);
        bitWrite(s.lockLevel, 4, 0);
        break;

    }
  }
}





const byte sudoSettingsIcons[] PROGMEM {
  B10101, B01010, B10101, B01010, B10101, B10000, B10000, B10000,
  B00100, B11111, B11111, B01010, B01010, B10101, B11111, B00100,
  B01111, B01110, B11100, B11111, B00111, B01110, B01100, B10000,
  B01110, B01010, B01010, B01110, B01110, B11111, B11111, B01110,
  B01110, B01010, B01010, B01110, B01110, B11111, B11111, B01110,
  B00000, B01110, B01010, B11111, B11111, B11111, B11111, B11111
};
const char su_ST0[] PROGMEM = "MAX SPEED";
const char su_ST1[] PROGMEM = "MAX MTR WATTS";
const char su_ST2[] PROGMEM = "MAX MTR AMPS";
const char su_ST3[] PROGMEM = "MAX ESC TEMP";
const char su_ST4[] PROGMEM = "MAX MOTOR TEMP";
const char su_ST5[] PROGMEM = "DISABLE SUDO";
const char *const settingsSudoTxt[] PROGMEM = {su_ST0, su_ST1, su_ST2, su_ST3, su_ST4, su_ST5};
const byte settingsSudoTxtLen = 6;

void settingsSudoMenu(Settings &s) {
  while (true) {
    int8_t res = showMenu(settingsSudoTxt, sudoSettingsIcons, settingsSudoTxtLen);
    switch (res) {
      case -1:
        return;
      case 0:
        s.sudoMaxSpeed = promptInt(s.sudoMaxSpeed, getStr(settingsSudoTxt, 0), 200);
        break;
      case 1:
        s.sudoMaxWatts = promptInt(s.sudoMaxWatts, getStr(settingsSudoTxt, 1), 4000);
        break;
      case 2:
        s.sudoMaxAmps = promptInt(s.sudoMaxAmps, getStr(settingsSudoTxt, 2), 150);
        break;
      case 3:
        s.sudoMaxESCTemp = promptInt(s.sudoMaxESCTemp, getStr(settingsSudoTxt, 3), 90);
        break;
      case 4:
        s.sudoMaxMotorTemp = promptInt(s.sudoMaxMotorTemp, getStr(settingsSudoTxt, 4), 150);
        break;
      case 5:
        s.sudoEnabled = false;
        return;
    }
  }
}


const byte quickSettingsIcons[] PROGMEM {
  B00000, B00001, B00010, B11100, B11011, B10100, B00010, B00001,
  B00000, B00100, B01110, B00100, B00000, B00000, B01110, B00000,
  B01100, B00100, B01110, B10101, B11111, B10001, B10101, B01110
};
const char qu_ST0[] PROGMEM = "Headlight toggle";
const char qu_ST1[] PROGMEM = "Start Charging";
const char qu_ST2[] PROGMEM = "Start Trial Mode";
const char *const settingsQuickTxt[] PROGMEM = {qu_ST0, qu_ST1, qu_ST2};
const byte settingsQuickTxtLen = 3;

void settingsQuickMenu(Settings &s, values &v) {
  cb420_clear();
  cb420_updateAll();
  delay(750);
  byte temp = settingsQuickTxtLen;
  if ((s.trialMinutes == 0 && s.trialMiles10 == 0) || isTrialEnabled(s))
    temp--;
  while (true) {
    int8_t res = showMenu(settingsQuickTxt, quickSettingsIcons, temp);
    switch (res) {
      case -1:
        return;
      case 0:
        v.headlight = !v.headlight;
        return;
      case 1:
        v.mpptCharge = true;
        return;
      case 2:
        if (bitRead(s.lockLevel, 3)) return;
        bitWrite(s.lockLevel, 3, 1);
        bitWrite(s.lockLevel, 4, 0);
        writeSettings(s, EEP_OFFSET);
        return;
    }
  }
}
