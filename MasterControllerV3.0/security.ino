/**
  This file handles security, password, limiting, and locking functions
  @author Thor Lancaster
*/

#define MASTER_PW 510732947UL
#define MASTER_PWL 30

byte sc_pwLength = 0;
unsigned long sc_pword = 0;


/*
   Get a password from the user and compare it to pw and len
   @param pw password
   @param len password length
   @return true if password entered matches provided password
*/
boolean lockPassword(char* prompt, unsigned long pw, byte len) {
  enterPassword(prompt);
  return sc_pwLength == len && sc_pword == pw;
}

/*
   Get a password from the user and set sc_pwLength and sc_pword
   @param prompt password prompt, up to 40 characters
*/

void enterPassword(char* prompt) {
  cb420_clear();
  cb420_setCursor(0, 0);
  cb420_print(prompt);
  cb420_updateAll();
  cb420_setCursor(0, 2);
  unsigned long pw = 0;
  byte len = 0;
  boolean pressed = false;
  boolean left, right, button;
  while (true) {
    left = getBrake(false);
    right = getBrake(true);
    button = getThrBtn();
    if (!pressed) {
      if (left && len < 30) {
        cb420_printChar('0');
      } else if (right && len < 30) {
        cb420_printChar('1');
        pw |= (1UL << len);
      } else if (button && len > 0) {
        cb420_wptr--;
        cb420_printChar(' ');
        cb420_wptr--;
        len--;
        cb420_updateAll();
        pw &= ~(1UL << len);
      }
      if ((left || right) && len < 30) {
        cb420_updateAll();
        len++;
      }
    }
    pressed = left || right || button;
    if (getThrottle() > 0.3) {
      sc_pwLength = len;
      sc_pword = pw;
      cb420_setCursor(0, 0);
      cb420_clearRow();
      cb420_setCursor(0, 1);
      cb420_clearRow();
      cb420_updateAll();
      while (getThrottle() > 0);
      return;
    }
  }
}
unsigned long getPW() {
  return sc_pword;
}
byte getPWL() {
  return sc_pwLength;
}
boolean isTrialEnabled(Settings &s) {
  return bitRead(s.lockLevel, 3);
}
boolean isTrialRunning(Settings &s) {
  return bitRead(s.lockLevel, 4);
}
boolean isTrialExpired(Settings &s, values &v) {
  if(!isTrialEnabled(s)) return false;
  if (s.trialEndMiles > 0 && s.odometer > s.trialEndMiles)
    return true;
  if (isDateZero(s.trialEndDate))
    return false;
  return compareDates(v.dt, s.trialEndDate) < 0;
}
void startTrial(Settings &s, values &v) {
  cb420_clear();
  cb420_setCursor(0, 0);
  cb420_print(" Starting Trial...");
  cb420_updateAll();
  delay(1000);
  // Indicate that the trial has started
  s.lockLevel |= 16;
  // Allocate trial time
  if (s.trialMinutes == 0)
    zeroDate(s.trialEndDate);
  else
    addMinutesToDate(v.dt, s.trialEndDate, s.trialMinutes);
  // Allocate trial mileage
  if (s.trialMiles10 == 0)
    s.trialEndMiles = 0;
  else
    s.trialEndMiles = s.trialMiles10 / 10.0 + s.odometer;
  // Write the settings to EEPROM
  // to prevent trial tampering by power cycling
  writeSettings(s, EEP_OFFSET);
}
const char MSG_LOL_DENIED[] PROGMEM = "  You're tricky...     But you aren't      tricky enough!";
void lolAccessDenied(){
  cb420_clear();
  cb420_setCursor(0, 0);
  cb420_print(progStr(MSG_LOL_DENIED));
  cb420_updateAll();
  delay(2000);
}
