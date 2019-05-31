/**
   This file handles communication and control of the slave ESC modules
   @author Thor Lancaster
*/
byte SLAVE_1_ID = 0x03;
byte SLAVE_2_ID = 0x04;

void sendCtrlPacket(byte sid, float current, float brakeAmt, byte regen) {
  byte packet[6];
  current = min(current, 99);
  packet[0] = 0x0E;
  packet[1] = sid;

  packet[2] = 41 + regen;

  if (regen) {
    packet[3] = format99((int)(brakeAmt * 10) % 100);
    packet[4] = format99((int)(brakeAmt * 1000) % 100);
  } else {
    packet[3] = format99((int)(current / 10) % 100);
    packet[4] = format99((int)(current * 10) % 100);
  }
  packet[5] = 0x0F;
  Serial1.write(packet, 6);
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


int8_t checkPacket(values &v) {
  // Read in the packet
  byte startVal = Serial1.read();
  byte slaveID = Serial1.read();

  byte bx[10];

  for (byte x = 0; x < 6; x++) {
    bx[x] = decode99(Serial1.read());
    if (bx[x] > 99)
      return -1;
  }

  //    byte b0 // ESC Temp * 10
  //    byte b1 // ESC Temp / 10
  //    byte b2 // Motor Temp * 10
  //    byte b3 // Motor Temp / 10
  //    byte b4 // Current * 10
  //    byte b5 // Current / 10

  if (slaveID == SLAVE_1_ID) {
    for (byte x = 0; x < 20 && Serial1.available() < 5; x++) { // Wait for longer packet to finish arriving from slave 1
      delay(1);
    }
    for (byte x = 6; x < 10; x++) {
      bx[x] = decode99(Serial1.read());
      if (bx[x] > 99)
        return -1;
    }
    //    b6 = Speed * 10
    //    b7 = Speed / 10
    //    b8 = Voltage * 10
    //    b9 = Voltage / 10
  }

  byte endVal = Serial1.read();
  // Validate the packet
  if (endVal != END_PACKET || startVal != START_PACKET) {
    return -1; // Bad headers
  }
  // Check if packet from valid slave
  if (slaveID != SLAVE_1_ID && slaveID != SLAVE_2_ID) {
    return 0;
  }

  // Once we get here, packet must be valid and belong to a valid slave

  if (slaveID == SLAVE_1_ID) {
    v.escTemp1 = bx[0] * 10 + bx[1] / 10.0;
    v.motorTemp1 = bx[2] * 10 + bx[3] / 10.0;
    v.amps1 = bx[4] * 10 + bx[5] / 10.0;
    v.speed = (bx[6] * 10 + bx[7] / 10.0) / s.speedDiv;
    v.volts = bx[8] * 10 + bx[9] / 10.0;
  }
  if (slaveID == SLAVE_2_ID) {
    v.regenErr = (bx[0] == 99 && bx[1] == 99);
    v.escTemp2 = bx[0] * 10 + bx[1] / 10.0;
    if (v.regenErr)
      v.escTemp2 = 0;
    v.motorTemp2 = bx[2] * 10 + bx[3] / 10.0;
    v.amps2 = bx[4] * 10 + bx[5] / 10.0;
  }

  return 1; // Success
}
