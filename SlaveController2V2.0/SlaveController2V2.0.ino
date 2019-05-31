/**
   Slave controller for CBEv2 electric bike
   This controller shall accept commands over Serial
   and apply and reply to commands sent to it.
   All commands not directed to this controller shall
   be ignored.

   This is a real-time application and thus the HARD
   response time shall not be greater than 15ms worst case
*/
byte SLAVE_ID = 0x04; // 0x03 to 0x0F, id of this controller
byte MAX_ALLOWED_CURRENT = 50; // Max allowed current in amps
byte MAX_REGEN = 200; // Max regen analogWrite
byte THROTTLE_MIN = 73; // Minumim PWM output value to engage controller
byte THROTTLE_ZERO = 40; // Used instead of no voltage. Required for some controllers

float CURRENT_SENSE_ZERO = 511.2; // Zero current
float CURRENT_SENSE_MULT = 0.140; // Measure current

#define START_PACKET 0x0E
#define END_PACKET 0x0F
#define LEN_PACKET 6

// Pin definitions
#define THROTTLE_OUT 11 // Connected to RC filter and then connected to EBC's throttle input wire, must be pin 3 or 11
#define CURRENT_SENSE A7 // Conencted to output pin of hall-effect current sensor
#define TEMP_ESC A4 // Connected VIA resistor bridge to a thermistor attached to the controller FETs
#define TEMP_MOTOR A5 // Connected VIA resistor bridge to a thermistor in the motor and a high-value RC filter for noise
#define EBRAKE 3 // To control pin of ebrake gate driver IC
#define LED 13 // Intrinsic led on Arduino to indicate controller is applying power to motor
#define PIN_SAFT_TRIG 2 // If high, controller goes into FAULT mode until power is cycled

#define BAUD_SERIAL 9600
#define CAPACITY_SERIAL 63

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

void setup() {
  // set ADC clock to F_CPU/16 (1MHZ on Nano)
  // This allows AnalogRead() of around 16 microseconds
  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);

  // Set current sensor zero position and add a tiny bit of bias
  delay(200);
  float inc = 0;
  for(int x = 0; x < 100; x++)
    inc+=accuAnalogRead(CURRENT_SENSE);
  CURRENT_SENSE_ZERO = inc/100 + 0.2;

  pinMode(THROTTLE_OUT, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(EBRAKE, OUTPUT);
  //  digitalWrite(EBRAKE, HIGH);
  //  while (true);
  Serial.begin(BAUD_SERIAL);

}
unsigned long lastMillis = 0;

float current = 0; // Measured Current flowing right now
float PCurrent = 0;
float PPCurrent = 0;
float PPPCurrent = 0;
byte regenMode = 0; // 0 is normal, 1 is slow regen, 2 is fast regen
float setCurrent = 0; // Desired current (or regen percent)
float escTemp = 0; // Measured temperature of ESC
float motorTemp = 0; // Measured temperature of motor
float speed = 0; // Speed of wheel
float voltage = 0; // Voltage coming into ESC


float error = 0;
float pCurrent = 0;
float iCurrent = 0;
float dCurrent = 0;
float throttleOut = 0; // 0 to 1

float PC_CURRENT = 0.0003; // P-constant for PID control
float IC_CURRENT = 0.0012; // I-constant for PID control
float DC_CURRENT = 0.0005; // D-constant for PID control
float IM_CURRENT = 0.9; // MAX integrator value
float MAX_ERROR = 4; // MAX error value
float addlI = 0;

float ESC_TEMP_RES = 8050; // Used to calibrate ESC temperature reading. Can go either way
float MOTOR_TEMP_RES = 2086; // Used to calibrate Motor temperature reading. Can go either way


unsigned long spd_lastMicros = 0;
unsigned long spd_thisMicros = 0;
unsigned long spd_lastDelta = 0;
void speedISR() {
  spd_thisMicros = micros();
  spd_lastDelta = spd_thisMicros - spd_lastMicros;
  spd_lastMicros = spd_thisMicros;
}

float getSpeed() {
  if (spd_lastDelta < 10)
    return 0;
  if (micros() > spd_lastMicros + 300000L)
    return 0;
  return 1000000.0 / (spd_lastDelta);
}

void setFastPWM(boolean enable) {
  if (enable) {
    // Set PWM on pins 3 and 11 for ~31 KHZ
    TCCR2B = TCCR2B & B11111000 | B00000001;
  } else {
    // Set PWM on pins 3 and 11 for ~4 KHZ
    TCCR2B = TCCR2B & B11111000 | B00000011;
  }
}

int timeToLive = 0;
int regenEnCooldown = 200;
boolean saftLockout = false;
void loop() {
  // Read state
  if (timeToLive < 1)
    setCurrent = 0;
  current = getCurrent();
  speed = getSpeed();
  PCurrent = current;
  PPCurrent = PPCurrent;
  PPPCurrent = PPCurrent;

  float nEscTemp = getESCTemp();
  float nMotorTemp = getMotorTemp();
  if (abs(escTemp - nEscTemp) > 5)
    escTemp = escTemp * 0.8 + nEscTemp * 0.2;
  if (abs(motorTemp - nMotorTemp) > 5)
    motorTemp = motorTemp * 0.8 + nMotorTemp * 0.2;
  escTemp = escTemp * 0.97 + getESCTemp() * 0.03;
  motorTemp = motorTemp * 0.97 + getMotorTemp() * 0.03;

  // Process state
  if (current < 0.5)
    addlI = 1;
  else
    addlI = 0.13;

  error = setCurrent - current;
  error = constrain(error, -MAX_ERROR, MAX_ERROR);
  pCurrent = (error * PC_CURRENT);
  iCurrent += (error * IC_CURRENT * addlI);
  dCurrent = (PPPCurrent - current) * DC_CURRENT;
  iCurrent = constrain(iCurrent, -IM_CURRENT, IM_CURRENT);
  throttleOut = constrain(pCurrent + iCurrent + dCurrent, 0, 1);
  if (setCurrent < 0.1) {
    pCurrent *= 0.9;
    iCurrent *= 0.9;
    dCurrent *= 0.9;
  }
  // Write state
  if (saftLockout)
    digitalWrite(LED, millis() % 600 > 300);
  else
    digitalWrite(LED, timeToLive > 0);
  //digitalWrite(LED, regenMode);

  if (saftLockout) {
    setThrottle(0);
    setRegen(0);
    setFastPWM(false);
  } else {
    if (regenMode > 0) {
      setFastPWM(regenMode == 2);
      pCurrent = iCurrent = dCurrent = 0;
      setThrottle(0);
      if (regenEnCooldown == 0)
        setRegen(setCurrent/100);
      else
        setRegen(0);
    }
    else {
      regenEnCooldown = 30; // 0.3 seconds from normal to regen (for safety)
      setRegen(0);
      setFastPWM(false);
      setThrottle(throttleOut);
    }
  }


  // Wait and handle communication. Desired FPS = 100
  do {
    // Detect SAFT trigger going high and shutdown if that happens
    if (digitalRead(PIN_SAFT_TRIG) && digitalRead(PIN_SAFT_TRIG)) {
      saftLockout = true;
      setThrottle(0);
      setRegen(0);
    }
    while (Serial.available() > LEN_PACKET && Serial.peek() != START_PACKET)
      Serial.read(); // Clear out data until start of packet encountered
    if (Serial.peek() == START_PACKET && Serial.available() >= LEN_PACKET) { // Start of packet
      int val = checkPacket(SLAVE_ID);
      if (val == 1) {
        sendResponsePacket();
        if (setCurrent > 0)
          timeToLive = 50;
      }
    }
    if (Serial.available() >= CAPACITY_SERIAL) // Should never happen
      while (Serial.available()) {
        Serial.read(); // Flush buffer if it becomes too big. This should never happen.
      }
  } while (millis() - lastMillis < 10);
  lastMillis = millis();

  if (timeToLive > 0)
    timeToLive--;
  if (regenEnCooldown > 0)
    regenEnCooldown--;
}

void setRegen(float pct) {
  if (pct <= 0)
    analogWrite(EBRAKE, 0);
  else {
    pct = constrain(pct, 0, 1);
    analogWrite(EBRAKE, pct*MAX_REGEN);
  }
}

void setThrottle(float pct) {
  analogWrite(THROTTLE_OUT, getThrottle(pct));
}

byte getThrottle(float pct) {
  if (pct < 0)
    pct = 0;
  if (pct > 1)
    pct = 1;
  if (pct < 0.005)
    return THROTTLE_ZERO;
  return THROTTLE_MIN + (137 * pct);
}


/**
   Check and apply an incoming Serial data packet
   @return 1 if addressed to this slave, 0 if not, -1 if invalid
*/
int checkPacket(byte t_slaveID) {
  // Read in the packet
  byte startVal = Serial.read();
  byte slaveID = Serial.read();
  byte regenTemp = Serial.read();
  byte sa1 = decode99(Serial.read());
  byte sa2 = decode99(Serial.read());
  byte endVal = Serial.read();

  byte regenMd = (regenTemp - 41); // 41 for no regen, 42 for regen mode, 43 for fast regen mode

  // Clear the Serial buffer
  while (Serial.available())
    Serial.read();

  // Validate the packet
  if (endVal != END_PACKET || startVal != START_PACKET) {
    return -1; // Bad headers
  }
  if (sa1 == 255 || sa2 == 255 || (regenTemp != 41 && regenTemp != 42 && regenTemp != 43)) {
    return -1; // Bad value
  }
  // Check if packet addressed to this slave
  if (slaveID != t_slaveID) {
    return 0; // Not addressed to this slave
  }

  // Once we get here, packet must be valid and belong to this slave

  setCurrent = (sa1 * 10.0) + (sa2 / 10.0); // Set global desired current
  if (setCurrent > MAX_ALLOWED_CURRENT && regenMd == 0)
    setCurrent = MAX_ALLOWED_CURRENT;
    
  regenMode = regenMd;


  return 1; // Success
}


byte format99(byte num) {
  if (num > 99)
    num = 99;
  return num + 33;
}

byte decode99(byte code) {
  if (code < 33 || code > 132)
    return 255;
  return code - 33;
}

/**
   Send a response to a received packet as per communication protocol
*/
void sendResponsePacket() {
  byte packet[9];
  packet[0] = 0x0E;
  packet[1] = SLAVE_ID;
  float eTemp = constrain(escTemp, 0, 999);
  float mTemp = constrain(motorTemp, 0, 999);
  float curr = constrain(abs(current), 0, 999);

  packet[2] = format99((int)(eTemp / 10) % 100);
  packet[3] = format99((int)(eTemp * 10) % 100);

  if (saftLockout) {
    packet[2] = format99(99);
    packet[3] = format99(99);
  }

  packet[4] = format99((int)(mTemp / 10) % 100);
  packet[5] = format99((int)(mTemp * 10) % 100);

  packet[6] = format99((int)(curr / 10) % 100);
  packet[7] = format99((int)(curr * 10) % 100);

  packet[8] = 0x0F;
  Serial.write(packet, 9);
}

/**
   Accurate, oversampled analog read
   @return analog reading between 0.0 1023.0
*/
float accuAnalogRead(int pin) {
  float sum = 0;
  for (int x = 0; x < 32; x++)
    sum += analogRead(pin);
  return sum / 32.0;
}

float getCurrent() {
  float rtn = (accuAnalogRead(CURRENT_SENSE) - CURRENT_SENSE_ZERO) * CURRENT_SENSE_MULT;
  if (abs(rtn) < 0.3)
    return 0;
  else
    return rtn;
}

float getESCTemp() {
  float reading = accuAnalogRead(TEMP_ESC);
  float rtn = getTemperature(getResistanceH(reading / 1023.0, ESC_TEMP_RES));
  if (isnan(rtn))
    return 0;
  else
    return rtn;
}

float getMotorTemp() {
  float reading = accuAnalogRead(TEMP_MOTOR);
  float rtn = getTemperature(getResistanceL(reading / 1023.0, MOTOR_TEMP_RES));
  if (isnan(rtn))
    return 0;
  else
    return rtn;
}

float getTemperature(double resistance) {
  double LnR = log(resistance);
  return (1 / (0.001125308852122 + 0.000234711863267 * LnR + 0.000000085663516 * LnR * LnR * LnR)) - 273.15;
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
