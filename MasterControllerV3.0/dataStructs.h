
/**
  Data structures to handle data in the program and prevent spaghetti
*/

struct dateTime{
  byte second;
  byte minute;
  byte hour;
  byte day;
  byte month;
  byte year;
};
typedef struct dateTime DateTime;

struct values{
  DateTime dt;
  float volts;
  float amps1;
  float amps2;
  float setAmps1;
  float setAmps2;
  float speed;
  float dispVolts;
  float dispAmps1;
  float dispAmps2;
  float dispSpeed;
  float escTemp1;
  float motorTemp1;
  float escTemp2;
  float motorTemp2;
  float throttleIn;
  byte regenMode;
  float setRegen;
  boolean mpptCharge;
  boolean brake;
  float tripometer;
  float wattHoursPerMile;
  float ccSetSpeed;
  float ccIntegral;
  boolean regenErr;
  boolean headlight;
  int busyMillis;
};
typedef struct values Values;

struct settings{
  long password;
  byte pwLength;
  long sudoPassword;
  long sudoPwLength;
  boolean sudoEnabled;
  /*
   * LockLevel bits:
   * 0 (LSB): Password On Boot
   * 1: Password to enter settings
   * 2: Password to enter settings.security
   * 3: Cry Baby restrictions set
   * 4: Cry Baby restrictions running (counting down)
   */
  byte lockLevel;
  byte maxSpeed;
  int stallWatts1;
  int stallWatts2;
  int boostWatts1;
  int boostWatts2;
  int maxAmps1;
  int maxAmps2;
  byte maxESCTemp1;
  byte maxESCTemp2;
  byte maxMotorTemp1;
  byte maxMotorTemp2;
  int batteryLVC;
  float batteryAH;
  byte backlight;
  float speedDiv;
  byte ccTolerance;
  byte ccIntegralMult;
  byte maxChargeCurrent;
  byte maxChargeVoltage;
  float batteryUsedAH; //*
  float batteryUsedWH; //*
  float odometer; //*
  // *Save to EEPROM on power loss

  byte ds3231Offset;
  int sudoMaxSpeed;
  int sudoMaxWatts;
  byte sudoMaxAmps;
  byte sudoMaxESCTemp;
  byte sudoMaxMotorTemp;
  
  // The following two v are set when trial is configured
  int trialMinutes; // Initial minutes in trial
  float trialMiles10; // Initial 10ths of a mile in trial
  // The following two v are set when trial is started
  // Stays the same from when trial is started
  DateTime trialEndDate;
  // When odometer hits this, trial is over
  float trialEndMiles;
};
typedef struct settings Settings;
