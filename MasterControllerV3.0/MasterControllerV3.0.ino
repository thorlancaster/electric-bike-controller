
/**
   Main driver file for ElectricBikeController4CryBabyElectric
   This is one of three separate programs required for operation
   @author Thor Lancaster
*/
// Baud that communication happens at. Must be the same for all three controllers
#define BAUD_SERIAL 9600
// True if throttle button is normally closed, false otherwise
boolean THR_BUTTON_INVERTED = true;

#include "dataStructs.h"
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

byte EEP_RSTNUM = 4; // Change this to reset EEPROM
int EEP_OFFSET = 3; // First EEPROM byte to use
int EEP_TRIP_OFFSET = 100; // Offset between first EEPROM settings byte and first EEPROM trip data byte
byte ODO_SAVE_VOLTS = 32; // When vBatt gets this low, the system will save trip data to EEPROM and halt

// Global Flash messages for LCD display.
// Placed in Flash to save RAM, they must be
// at the beginning of the program
const char MSG_WRONG_PASSWORD[] PROGMEM = "   WRONG PASSWORD";
const char MSG_BOOT_PASSWORD[] PROGMEM = " Enter Password To      Start Bike";
const char MSG_SETTINGS_PASSWORD[] PROGMEM = " Enter Password To     Enter Settings";
const char MSG_SETTINGS_SECURITY_PASSWORD[] PROGMEM = " Enter Password For  Security Settings";
const char MSG_SETTINGS_SUDO_PASSWORD[] PROGMEM = " Enter Password For    Sudo Settings";
const char MSG_TRIAL_ACTIVATION[] PROGMEM = " Twist throttle to    Start Trial Ride";
Values v;
Settings s;
byte BRAKE_L = 2; // Left eBrake switch
byte BRAKE_R = 3; // Right eBrake switch
byte THR_IN = A2; // Throttle input
byte THR_BTN = 11; // Throttle pushbutton
byte THR_LED = 10; // Throttle LED output line
byte BACKLIGHT = 5; // Active-low LCD backlight
byte HEADLIGHT = A3; // To FET to activate headlight, active-high
byte SPEAKER = A6; // To piezo speaker for sound effects

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// Communication stuff
byte LEN_PACKET_MIN = 9;
byte LEN_PACKET_MAX = 13;
byte END_PACKET = 0x0F;
byte START_PACKET = 0x0E;
byte CAPACITY_SERIAL = 63;


void setup() {
  pinMode(BRAKE_L, INPUT_PULLUP); // EXT pullup recommended
  pinMode(BRAKE_R, INPUT_PULLUP); // EXT pullup recommended
  pinMode(THR_IN, INPUT);
  pinMode(THR_BTN, INPUT_PULLUP); // EXT pullup recommended
  pinMode(THR_LED, OUTPUT);
  digitalWrite(BACKLIGHT, HIGH);
  pinMode(BACKLIGHT, OUTPUT);
  pinMode(HEADLIGHT, OUTPUT);
  pinMode(SPEAKER, OUTPUT);
  analogWriteFrequency(BACKLIGHT, 29297); // Faster PWM looks better, especially in vehicle applications

  Serial1.begin(BAUD_SERIAL);

  // Initialize the LCD (and the I2C bus as well)
  cb420_init();
  cb420_setBacklight(8);
  zeroValues(v);


  // Load settings from EEPROM
  loadSettings(s, EEP_OFFSET);
  loadTrip(s, EEP_OFFSET + EEP_TRIP_OFFSET);
  cb420_setBacklight(s.backlight);

  // Verify settings and reset them if they are set to change
  if (verifySettings(s, EEP_OFFSET, EEP_RSTNUM) != 0) {
    boolean bl = false;
    while (1) {
      cb420_clear();
      cb420_setCursor(0, 0);
      cb420_print("EEPROM values reset");
      cb420_setCursor(4, 2);
      if (bl)
        cb420_print("Cycle Power");
      bl = !bl;
      cb420_updateAll();
      delay(500);
    }
  }
  // Start in MPPT mode if right brake held
  if (getBrake(true)) {
    v.mpptCharge = true;
    alert(" Starting MPPT Mode", 1200);
  }
  // Set the clock calibration offset to the one in Settings
  ds3231_setCalibrationOffset(s.ds3231Offset);
  ds3231_getTime(v.dt);
  // Require password on boot if specified
  if (s.lockLevel & 1) {
    bootPassword();
  }
}


void bootPassword() {
  while (!lockPassword(progStr(MSG_BOOT_PASSWORD), s.password, s.pwLength));
}

// Variables to regulate frame rate of main loop
unsigned long frRegMillis = 0; unsigned long lastFrameMillis = 0; unsigned long frameCount = 0;
// Set to true when vBatt passes ODO_SAVE_VOLTS. When vBatt drops again, the program halts and trip data is saved
boolean odoPassed = false;
// Increments when the throttle button is held
int thrBtnHeld = 0;

// Variables used to keep track of right-brake shortcut triggering
byte rbsc_presses = 0;
byte rbsc_nopress = 0;
byte rbsc_press = 0;

// Variables used to increase precision of integration
float ampSeconds = 0; // Add all controller amps together
float wattSeconds = 0; // Add all controller watts together
float m1kth = 0; // 1000th of a mile units
// Variables used to determine delta-time in integration
unsigned long lastTallyMicros = 0;
unsigned long thisTallyMicros = 0;

// Variables used for MPPT
float MPPT_SEEK = 0.51; // MPPT default for seeking power
float mpptSetting = MPPT_SEEK; // MPPT current output setting
float mpptMerit = 0;  // MPPT Merit (volts * (amps+1)
float pMpptSetting = 0; // last Checked MPPT output setting
float pMpptMerit = 0; // Last Checked MPPT Merit
float mpptAdd = 0.001; // Desired MPPT addend
float mpptRealAdd = 0; // Add this to MPPTSetting every cycle
void loop() {
  // Handle communication
  if (Serial1.available()) {
    while (Serial1.available() > LEN_PACKET_MIN && Serial1.peek() != START_PACKET)
      Serial1.read(); // Clear out data until start of packet encountered
    if (Serial1.peek() == START_PACKET && Serial1.available() >= LEN_PACKET_MIN) { // Start of packet
      int val = checkPacket(v);
    }
    if (Serial1.available() >= CAPACITY_SERIAL) // Should never happen
      while (Serial1.available()) {
        Serial1.read(); // Flush buffer if it becomes too big. This should never happen.
      }
  }
  // Regulate periodic activities so they don't happen all at once
  // Front ESC control packet
  if (!v.mpptCharge) {
    if (frameCount % 4 == 0)
      sendCtrlPacket(0x03, v.setAmps1, 0, false);
    // Rear ESC control packet
    if (frameCount % 4 == 2)
      sendCtrlPacket(0x04, v.setAmps2, v.setRegen, v.regenMode);
    // Get time from RTC
    if (frameCount % 4 == 1)
      ds3231_getTime(v.dt);
  } else { // For MPPT, constant communication both ESCs is a must
    if (frameCount % 2 == 0)
      sendCtrlPacket(0x03, 0, 0, false);
    else
      sendCtrlPacket(0x04, 0, v.setRegen, v.regenMode);
  }

  //////////////////////////////// Begin MPPT code ////////////////////////////////
  if (v.mpptCharge) {
    // Headlight off during charging as it uses significant power
    digitalWrite(HEADLIGHT, 0);
    // Use high-frequency regen mode to eliminate noise and increase efficiency
    v.regenMode = 2;
    // Don't let brown-outs cause controller to freeze
    odoPassed = false;
    // Recalculate values every 512 ticks
    int mpptCycle = frameCount & 511;

    // Calculate MPPT setting
    mpptSetting += (mpptRealAdd / 16.0); // Increase divider for faster framerates
    if (mpptSetting > 0.95)
      mpptAdd = min(0, mpptAdd);
    if (mpptSetting < 0.25)
      mpptAdd = max(0, mpptAdd);
    // Calculate and smooth MPPT merit
    mpptMerit = (mpptMerit * 19 / 20) + (v.volts * (v.amps2 + 1)) / 20.0;

    // Perform MPPT every few seconds
    if (mpptCycle == 0) {
      if (abs(mpptSetting - pMpptSetting) < 0.005) { // Prevent unreasonable division
        if (mpptSetting > pMpptSetting)
          pMpptSetting = mpptSetting - 0.005;
        else
          pMpptSetting = mpptSetting + 0.005;
      }
      // Calculate derivative of merit with respect to setting
      float dm_ds = ((mpptMerit / pMpptMerit) - 1) / (mpptSetting - pMpptSetting);
      mpptAdd = dm_ds / 1500; // Determined through experimentation. Make larger for >200 watt panels
      mpptAdd = constrain(mpptAdd, -0.002, 0.005); // Keep adjustment at a reasonable rate
      if (v.amps2 < 0.05) // If not receiving power, go to default setting for low-light conditions
        if (mpptSetting < MPPT_SEEK) // Oscillate around set point
          mpptAdd = 0.001;
        else
          mpptAdd = -0.001;
      pMpptMerit = mpptMerit;
      pMpptSetting = mpptSetting;
    }
    mpptRealAdd = mpptAdd;
    // Prevent overcurrent and overcharging
    if (v.amps2 > s.maxChargeCurrent / 2)
      mpptRealAdd = min(0.001, mpptRealAdd);
    if (v.amps2 > s.maxChargeCurrent * 4 / 5) {
      mpptRealAdd = min(0.0004, mpptRealAdd);
      mpptAdd = max(0.0001, mpptAdd); // If regulating current, prevent power dips from MPPT attempts
    }
    if (v.amps2 > s.maxChargeCurrent)
      mpptRealAdd = min(-0.0005, mpptRealAdd);
    if (v.amps2 > s.maxChargeCurrent * 1.3)
      mpptRealAdd = min(-0.01, mpptRealAdd);
    if (v.volts > s.maxChargeVoltage)
      mpptRealAdd = min(-0.0005, mpptRealAdd);

    mpptSetting = constrain(mpptSetting, 0.2, 1);
    v.setRegen = mpptSetting;
    // Exit from MPPT by twisting throttle. The brakes must be released.
    if (getThrottle() > 0.5 && !getBrake(true) && !getBrake(false)) {
      v.setRegen = 0;
      mpptSetting = MPPT_SEEK;
      cb420_clear();
      cb420_updateAll();
      while (getThrottle() > 0);
      v.mpptCharge = false;
    }
  }
  ///////////////////////////////// End MPPT code /////////////////////////////////
  else {
    digitalWrite(HEADLIGHT, v.headlight);

    // Read values
    v.regenMode = 0;
    v.brake = false;
    v.throttleIn = getThrottle();
    if (getBrake(false) || getBrake(true)) {
      v.setRegen = v.throttleIn;
      v.throttleIn = 0;
      v.ccSetSpeed = 0;
      v.regenMode = 1;
      v.brake = true;
    }
    Serial.println(v.brake);

    // Handle Trial
    if (isTrialEnabled(s) && (!isTrialRunning(s)) && v.throttleIn > 0 && frameCount > 20) {
      startTrial(s, v); // Twist throttle to activate trial
    }
    if (isTrialExpired(s, v)) {
      v.throttleIn = 0; // Disable throttle once trial expired
    }

    // Handle right-brake quick menu
    if (getBrake(true)) {
      if (!rbsc_press) {
        rbsc_presses++;
        rbsc_nopress = 0;
        rbsc_press = 1;
      }
    } else {
      rbsc_press = 0;
    }
    if (rbsc_presses >= 3) {
      rbsc_presses = 0;
      settingsQuickMenu(s, v);
    }
    if (rbsc_nopress > 60)
      rbsc_presses = 0;
    else
      rbsc_nopress++;
    if (getBrake(false))
      rbsc_presses = 0;

    // Calculate v
    calcVals(v, s);
  }
  // Smooth v
  dispVals(v);

  thisTallyMicros = micros();
  float quiescent = 0.1; // Quiescent current of all always-on accessories. Used for AH calculation
  if (v.volts < ODO_SAVE_VOLTS) // No quiescent battery consumption when it's not plugged in
    quiescent = 0;

  // Perform integration on amps, watts, and speed
  float asAddend = (v.amps1 + v.amps2 + quiescent);
  if (v.regenMode)
    asAddend -= ((v.amps2) * 2); // Negate rear current used, then subtract it again. Regen refills the battery.
  asAddend  *= ((thisTallyMicros - lastTallyMicros) / 1000000.0);

  ampSeconds += asAddend;
  wattSeconds += asAddend * v.volts;
  m1kth += v.speed * ((thisTallyMicros - lastTallyMicros) / 1000000.0);
  if (abs(ampSeconds) > 10) {
    s.batteryUsedAH += (ampSeconds / 3600);
    if (s.batteryUsedAH > s.batteryAH)
      s.batteryUsedAH = s.batteryAH;
    ampSeconds = 0;
  }
  if (wattSeconds > 400) {
    s.batteryUsedWH += (wattSeconds / 3600);
    wattSeconds = 0;
  }
  if (m1kth > 20) {
    v.tripometer += (m1kth / 3600);
    s.odometer += (m1kth / 3600);
    m1kth = 0;
  }
  lastTallyMicros = thisTallyMicros;

  // Display information to user
  if (v.mpptCharge)
    mpptDisplay(v, s, mpptMerit, mpptRealAdd);
  else
    mainDisplay(v, s);
  // Send pulses to illuminate throttle LEDs based on battery, if they are connected
  sendThrLights(getThrLights(getBattery(s), millis() % 600 > 300));

  // Handle edge cases
  if (v.volts > ODO_SAVE_VOLTS + 1)
    odoPassed = true;
  // Handle vBatt dropping and save trip data before the voltage is all gone
  if (frameCount > 200 && v.volts < ODO_SAVE_VOLTS && odoPassed && (!v.mpptCharge)) {
    saveTrip(s, EEP_OFFSET + EEP_TRIP_OFFSET);
    byte bl = 10;
    while (1) {
      cb420_clear();
      cb420_setCursor(1, 0);
      cb420_print("VBATT Undervoltage");
      cb420_setCursor(5, 1);
      cb420_print("Trip saved");
      cb420_setCursor(4, 3);
      if (bl == 0) {
        cb420_print("Cycle Power");
        bl = 2;
      }
      bl--;
      cb420_updateAll();
      delay(500);
    }
  }

  // Keep track of how long the throttle button is held
  if (getThrBtn())
    thrBtnHeld++;
  else thrBtnHeld = 0;
  if (thrBtnHeld > 10000)
    thrBtnHeld = 10000;

  // Push button while moving to engage cruse control
  if (thrBtnHeld > 16 && v.speed >= 1 && !v.mpptCharge) {
    v.ccSetSpeed = v.speed;
    v.ccIntegral = 0;
  }
  if (isTrialExpired(s, v)) {
    v.ccSetSpeed = 0;
    tone(SPEAKER, (sin(millis() / 150.0) + 2.5) * 400, 10);
  }
  // Push button (and hold a little bit) while stopped to enter settings menu
  if (thrBtnHeld > 32 && v.speed < 1 && !v.mpptCharge) {
    cb420_clear();
    cb420_setCursor(6, 0);
    cb420_print("Settings");
    cb420_updateAll();
    while (getThrBtn());
    delay(100);
    sendThrLights(0);
    if (s.lockLevel & 2) {
      if (lockPassword(progStr(MSG_SETTINGS_PASSWORD), s.password, s.pwLength))
        settingsMainMenu(s);
      else {
        alert(progStr(MSG_WRONG_PASSWORD), 2000);
      }
    } else
      settingsMainMenu(s);
    thrBtnHeld = 0;
  }

  // Regulate frame rate of main loop
  v.busyMillis = millis() - lastFrameMillis;
  do {
    cb420_sendPacket(2); // nothing to do
    cb420_updateCustChar(); // keep LCD up to date
  } while (millis() < frRegMillis + 5);

  if (millis() - frRegMillis > 100)
    frRegMillis = millis();
  else
    frRegMillis += 5; // guarantee accurate, consistent 200hz frame rate. Teensy32 can do it easily, nano can only do 50hz.
  lastFrameMillis = millis();
  frameCount++;
}
