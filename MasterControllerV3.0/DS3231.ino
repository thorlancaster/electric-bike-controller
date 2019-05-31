#define DS3231_I2C_ADDRESS 0x68


/**
   This file handles a DS3231 RTC for accurate timekeeping
   Supports 8 custom characters and printing of various v
   @author Thor Lancaster
*/


/**
 * Set the time on the DS3231
 */
void ds3231_setTime(byte second, byte minute, byte hour, byte day, byte month, byte year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0);
  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(1);
  Wire.write(decToBcd(day));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}

/**
 * Get the time on the DS321 using int v
 */
void ds3231_getTimeInt(int8_t *second, int8_t *minute, int8_t *hour, int8_t *day, int8_t *month, int8_t *year) {
  byte sc, mn, hr, dy, mt, yr;
  ds3231_getTime(&sc, &mn, &hr, &dy, &mt, &yr);
  *second = sc; *minute = mn; *hour = hr; *day = dy; *month = mt; *year = yr;
}
/**
 * Get the time on the DS321 using byte v (default)
 */
void ds3231_getTime(byte *second, byte *minute, byte *hour, byte *day, byte *month, byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  Wire.read();
  *day = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

/**
 * Get the time on the DS321 using a DateTime
 */
void ds3231_getTime(DateTime &dt) {
  ds3231_getTime(&dt.second, &dt.minute, &dt.hour, &dt.day, &dt.month, &dt.year);
}

/**
 * Set the calibration offset to the DS3231 and apply it immediately
 * @param offset calibration offset, 127 by default
 */
void ds3231_setCalibrationOffset(byte offset) {
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0x10);
  Wire.write(offset);
  Wire.endTransmission();
  delay(1);
  Wire.beginTransmission(DS3231_I2C_ADDRESS); // Make changes take effect right now
  Wire.write(0x0E);
  Wire.write(B00111100);
  Wire.endTransmission();
}
//byte ds3231_getCalibrationOffset() {
//  Wire.beginTransmission(DS3231_I2C_ADDRESS);
//  Wire.write(0x10);
//  Wire.endTransmission();
//  Wire.requestFrom(DS3231_I2C_ADDRESS, 1);
//  return Wire.read();
//}
