
/**
   This file handles and provides a buffered interface to a 4x20 LCD display
   Supports 8 custom characters and printing of various v
   @author Thor Lancaster
*/

/*
  cb420 interface consists of the following methods:
  cb420_init()
  cb420_clear()
  cb420_updateAll()
  cb420_setCustChar(int slot, byte* character, boolean immediate)
  cb420_updateCustChar(byte slot)
  cb420_updateCustChar()
  cb420_setCursor(int x, int y)
  cb420_clearRow()
  cb420_printChar(char ch)
  cb420_print(char* str)
  cb420_printFloat100(float number)
  cb420_printFloat(float number)
  cb420_printFloat(float number, int digits)
  cb420_printDigit(unsigned int num)
  cb420_printIntR(unsigned long num)
  cb420_printInt(long num)
  cb420_sendPacket(byte numPackets)
  cb420_setBacklight(byte brightness) [0-16]
*/

char cb420_chars[80];
byte cb420_custChars[8][8];
byte cb420_wptr = 0; // write pointer
byte cb420_rptr = 0; // read pointer
byte cb420_ccuptr = 0; // custom char update pointer
//byte iconXXX[] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};

/**
   Initialize the CB420 interface and the LCD display
*/
void cb420_init() {
  lcd.begin(20, 4);
  Wire.setClock(400000);
  lcd.clear();
  lcd.setBacklight(HIGH);
  cb420_clear();
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      cb420_custChars[x][y] = 0;
    }
  }
}

/**
   Clear the internal display buffer
*/
void cb420_clear() {
  for (int x = 0; x < 80; x++)
    cb420_chars[x] = ' ';
}

/**
   Write the entire buffer to the LCD display
*/
void cb420_updateAll() {
  cb420_rptr = 0;
  cb420_sendPacket(80);
}

/**
   Set a custom character on the LCD display
   @param slot slot to set to custom character
   @param ch 8-byte array representing custom characters
   @param immediate true to update LCD immediately
*/
void cb420_setCustChar(int slot, byte* ch, boolean immediate) {
  for (int x = 0; x < 8; x++) {
    cb420_custChars[slot][x] = ch[x];
  }
  if (immediate)
    cb420_updateCustChar(slot);
}

/**
   Update a given LCD custom character
   @param slot slot of character to update
*/
void cb420_updateCustChar(byte sl) {
  lcd.createChar(sl, cb420_custChars[sl]);
}

/**
   Update a rotating-index custom character
   8 invocations of this method will update all of them
*/
void cb420_updateCustChar() {
  lcd.createChar(cb420_ccuptr, cb420_custChars[cb420_ccuptr]);
  cb420_ccuptr++;
  if (cb420_ccuptr >= 8)
    cb420_ccuptr = 0;
}

/**
   Set the cursor to write to the buffer
   @param x X-position (0 to 20)
   @param y Y-position (0 to 3)
*/
void cb420_setCursor(byte x, byte y) {
  x = constrain(x, 0, 19);
  y = constrain(y, 0, 3);
  cb420_wptr = (y * 20 + x);
}

/**
   Set the update cursor to update from the buffer to the display
   @param x X-position (0 to 20)
   @param y Y-position (0 to 3)
*/
void cb420_setUpdateCursor(byte x, byte y) {
  x = constrain(x, 0, 19);
  y = constrain(y, 0, 3);
  cb420_rptr = (y * 20 + x);
}

/**
   Print a character to the buffer
   @param ch char to print
*/
void cb420_printChar(char ch) {
  cb420_chars[cb420_wptr] = ch;
  cb420_wptr++;
  if (cb420_wptr >= 80)
    cb420_wptr = 0;
}

/**
   Print a string to the buffer
   @param str string to print
*/
void cb420_print(char* str) {
  for (int x = 0; str[x] != 0; x++) {
    cb420_chars[cb420_wptr] = str[x];
    if (cb420_chars[cb420_wptr] <= 8 && cb420_chars[cb420_wptr] > 0)
      cb420_chars[cb420_wptr]--;

    cb420_wptr++;
    if (cb420_wptr >= 80)
      cb420_wptr = 0;
  }
}

/**
   Clear the display's current row, set by y-position of cursor
*/
void cb420_clearRow() {
  int row = cb420_wptr / 20;
  for (int x = row * 20; x < (row + 1) * 20; x++)
    cb420_chars[x] = ' ';
}

/*
   Print a floating-point number with 3 places to the buffer
   @param number number to print
*/
void cb420_printFloat100(float number) {
  if (number < 10 - 0.005)
    cb420_printFloat(number + 0.005, 2, false);
  else if (number < 100 - 0.05)
    cb420_printFloat(number + 0.05, 1, false);
  else if (number > 1000 - 0.5) {
    cb420_printChar(' ');
    cb420_printFloat(number + 0.5, 0, false);
  }
  else
    cb420_printFloat(number, 0);
}

/*
   Print a floating-point number with 4 places to the buffer
   @param number number to print
*/
void cb420_printFloat1000(float number) {
  if (number < 10 - 0.0005)
    cb420_printFloat(number + 0.0005, 3, false);
  else if (number < 100 - 0.005)
    cb420_printFloat(number + 0.005, 2, false);
  else {
    cb420_printChar(' ');
    cb420_printFloat(number + 0.05, 1, false);
  }
}

/*
   Print a floating-point number to the buffer with 2 digits to the right of the decimal point
   @param number number to print
*/
void cb420_printFloat(float number) {
  cb420_printFloat(number, 2, true);
}

/*
   Print a floating-point number to the buffer
   @param number number to print
   @param digits digits to the right of the decimal point
*/
void cb420_printFloat(float number, byte digits) {
  cb420_printFloat(number, digits, true);
}

/*
   Print a floating-point number to the buffer, with or without default rounding
   @param number number to print
   @param digits digits to the right of the decimal point
   @param rounded true to use default rounding
*/
void cb420_printFloat(float number, byte digits, boolean rounded) {
  if (isnan(number)) return cb420_print("nan");
  if (isinf(number)) return cb420_print("inf");
  // handle negative numbers
  if (number < 0.0)
  {
    cb420_print("-");
    number = -number;
  }

  if (rounded) {
    // Round correctly so that print(1.999, 2) prints as "2.00"
    double rounding = 0.5;
    for (uint8_t i = 0; i < digits; ++i)
      rounding /= 10.0;
    number += rounding;
  }

  // Extract the integer part of the number and print it
  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;
  cb420_printInt(int_part);

  // Print the decimal point, but only if there are digits beyond
  if (digits > 0) {
    cb420_print(".");
  }

  // Extract digits from the remainder one at a time
  while (digits-- > 0)
  {
    remainder *= 10.0;
    unsigned int toPrint = (unsigned int)(remainder);
    cb420_printDigit(toPrint);
    remainder -= toPrint;
  }
}

/*
   Print a digit to the buffer
   @param num digit to print. If outside (0, 9) range nothing will happen
*/
void cb420_printDigit(byte num) { // TODO this was unsigned int
  if (num < 0 || num > 9)
    return;
  cb420_printChar('0' + num);
}

/**
   Print a number to the buffer, right-aligned at cursor
   @param num number to print
*/
void cb420_printIntR(unsigned long num) {
  int sz = log(num) / log(10);
  cb420_wptr -= sz;
  cb420_printInt(num);
  cb420_wptr += sz;
}

/**
   Print a number to the buffer, left-aligned at cursor
   @param num number to print
*/
void cb420_printInt(long num) {
  if (num < -10000000L || num > 10000000L) {
    cb420_print("OOR");
    return;
  }
  if (num < 0) {
    cb420_printChar('-');
    cb420_printInt(-num);
    return;
  }
  byte len = ((byte)(log(num) / 2.30258509)) + 1;
  for (int x = len - 1; x >= 0; x--) {
    cb420_printDigit(((int)(num / pow(10, x))) % 10);
  }
}

/**
   Update characters on the LCD based on the buffer
   @param numPackets number of characters to update
*/
void cb420_sendPacket(byte numPackets) {
  if (numPackets >= 80) {
    numPackets = 80;
    cb420_rptr = 0;
  }
  lcd.setCursor(cb420_rptr % 20, cb420_rptr / 20);
  for (int x = 0; x < numPackets; x++) {
    if (cb420_rptr % 20 == 0) {
      lcd.setCursor(0, cb420_rptr / 20);
    }
    lcd.print(cb420_chars[cb420_rptr]);
    cb420_rptr++;
    if (cb420_rptr >= 80)
      cb420_rptr = 0;
  }
}

/**
 * Set the LCD backlight brightness from 1 to 16, using an apparent-equal-brightness quadratic scale
 * @param brightness brightness of LCD backlight
 */
void cb420_setBacklight(byte brightness) {
  if (brightness >= 16) {
    analogWrite(BACKLIGHT, 0);
    return;
  }
  analogWrite(BACKLIGHT, 255 - (brightness * brightness));
}
