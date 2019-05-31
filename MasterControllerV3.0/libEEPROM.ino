
/**
  This file handles EEPROM operations to allow writing of various v
  and prevent unnecessary EEPROM wear.
  @author Thor Lancaster
 */
 

/**
 * Write a Long to the EEPROM
 * @param offset offset to write to
 * @param value number to write
 * @return 4 (number of bytes used)
 */
byte EEPROMWriteLong(int offset, long value) {
  EEPROMWrite(offset, (value >> 24) & 255);
  EEPROMWrite(offset + 1, (value >> 16) & 255);
  EEPROMWrite(offset + 2, (value >> 8) & 255);
  EEPROMWrite(offset + 3, value & 255);
  return 4;
}

/**
 * Write a DateTime to the EEPROM
 * @param offset offset to write to
 * @param value number to write
 * @return 6 (number of bytes used)
 */
byte EEPROMWriteDateTime(int offset, DateTime &value){
  EEPROMWrite(offset, value.second);
  EEPROMWrite(offset + 1, value.minute);
  EEPROMWrite(offset + 2, value.hour);
  EEPROMWrite(offset + 3, value.day);
  EEPROMWrite(offset + 4, value.month);
  EEPROMWrite(offset + 5, value.year);
  return 6;
}

byte EEPROMReadDateTime(int offset, DateTime &value){
  value.second = EEPROMRead(offset);
  value.minute = EEPROMRead(offset + 1);
  value.hour = EEPROMRead(offset + 2);
  value.day = EEPROMRead(offset + 3);
  value.month = EEPROMRead(offset + 4);
  value.year = EEPROMRead(offset + 5);
  return 6;
}


long EEPROMReadLong(int offset) {
  return ((long) EEPROMRead(offset)) << 24 | ((long) EEPROMRead(offset + 1)) << 16 | ((long) EEPROMRead(offset + 2)) << 8 | ((long) EEPROMRead(offset + 3));
}

/**
 * Write a float to the EEPROM
 * @param offset offset to write to
 * @param value number to write
 * @return 4 (number of bytes used)
 */
int EEPROMWriteDecimal(int offset, float value) {
  EEPROMWriteLong(offset, (value+0.0005)*1000.0);
  return 4;
}

float EEPROMReadDecimal(int offset) {
  return EEPROMReadLong(offset)/1000.0;
}

/**
 * Write an Int to the EEPROM
 * @param offset offset to write to
 * @param value number to write
 * @return 2 (number of bytes used)
 */
byte EEPROMWriteInt(int offset, long value) {
  EEPROMWrite(offset, (value >> 8) & 255);
  EEPROMWrite(offset + 1, (value) & 255);
  return 2;
}

int EEPROMReadInt(int offset) {
  return (((int) EEPROMRead(offset)) << 8 | ((int) EEPROMRead(offset + 1)));
}

boolean EEPROMReadBoolean(int offset){
  return EEPROMRead(offset) > 0;
}

/**
 * Write a Boolean to the EEPROM
 * @param offset offset to write to
 * @param value value to write, true or false
 * @return 1 (number of bytes used)
 */
byte EEPROMWriteBoolean(int offset, boolean val){
  if(val != 0)
    val = 1;
  EEPROMWrite(offset, val);
  return 1;
}

/**
 * Safe, EEPROM-wear preventing write. Only updates if needed
 * @param addr EEPROM address
 * @param val byte to write
 */
byte EEPROMWrite(int addr, byte val) {
  if (EEPROM.read(addr) != val)
    EEPROM.write(addr, val);
  return 1;
}
byte EEPROMRead(int addr) {
  return EEPROM.read(addr);
}
