// Rev: 07/14/18
// Message_RS485 handles RS485 (and *not* digital-pin) communications, via a specified serial port.

#include <Message_RS485.h>

Message_RS485::Message_RS485(HardwareSerial * hdwrSerial, long unsigned int baud, Display_2004 * LCD2004) {  // Constructor
  mySerial = hdwrSerial;    // Pointer to the serial port we want to use for RS485.
  myBaud = baud;            // Serial port baud rate.
  myLCD = LCD2004;          // Pointer to the LCD display for error messages.
  mySerial->begin(myBaud);  // Initialize the serial port that this object will be using.
}

bool Message_RS485::RS485GetMessage(byte tMsg[]) {
  // Returns true or false, depending if a complete message was read.
  // If this function returns true, then we are guaranteed to have a real/accurate message in the buffer, including good CRC.
  // However, this function does not check if it is to "us" (this Arduino) or not.  That's because 1) the child class will do that,
  // and because some modules snoop the messages, and act even if the message isn't addressed to them.
  // tmsg[] is also "returned" by the function since arrays are automatically passed by reference.
  // If the whole message is not available, tMsg[] will not be affected, so ok to call any time.
  // Does not require any data in the incoming RS485 serial buffer when it is called.
  // Detects incoming serial buffer overflow and fatal errors.
  // If there is a fatal error, calls endWithFlashingLED() (in calling module, so it can also invoke emergency stop if applicable.)
  // This only reads and returns one complete message at a time, regardless of how much more data may be in the incoming buffer.
  // Input byte tmsg[] is the initialized incoming byte array whose contents may be filled.
  
  byte tMsgLen = mySerial->peek();     // First byte will be message length
  byte tBufLen = mySerial->available();  // How many bytes are waiting?  Size is 64.

  sprintf(lcdString, "Msg len %3i.", tMsgLen);
  myLCD->send(lcdString);
  sprintf(lcdString, "Buf len %3i.", tBufLen);
  myLCD->send(lcdString);

  if (tBufLen >= tMsgLen) {  // We have at least enough bytes for a complete incoming message
    digitalWrite(PIN_RS485_RX_LED, HIGH);       // Turn on the receive LED
    if (tMsgLen < 5) {                 // Message too short to be a legit message.  Fatal!
      sprintf(lcdString, "%.20s", "RS485 msg too short!");
      myLCD->send(lcdString);
      Serial.println(lcdString);
      endWithFlashingLED(1);
    } else if (tMsgLen > RS485_MAX_LEN) {            // Message too long to be any real message.  Fatal!
      sprintf(lcdString, "%.20s", "RS485 msg too long!");
      myLCD->send(lcdString);
      Serial.println(lcdString);
      endWithFlashingLED(1);
    } else if (tBufLen > 60) {            // RS485 serial input buffer should never get this close to overflow.  Fatal!
      sprintf(lcdString, "%.20s", "RS485 in buf ovrflw!");
      myLCD->send(lcdString);
      Serial.println(lcdString);
      endWithFlashingLED(1);
    }
    for (int i = 0; i < tMsgLen; i++) {   // Get the RS485 incoming bytes and put them in the tMsg[] byte array
      tMsg[i] = mySerial->read();
    }
    if (tMsg[tMsgLen - 1] != calcChecksumCRC8(tMsg, tMsgLen - 1)) {   // Bad checksum.  Fatal!
      sprintf(lcdString, "%.20s", "RS485 bad checksum!");
      myLCD->send(lcdString);
      Serial.println(lcdString);
      endWithFlashingLED(1);
    }
// DEBUG: Keep the receive LED lit for 500ms after read is completed ************************************************************************************************************
delay(500);
    // At this point, we have a complete and legit message with good CRC, which may or may not be for us.
    digitalWrite(PIN_RS485_RX_LED, LOW);       // Turn off the receive LED
    return true;
  } else {     // We don't yet have an entire message in the incoming RS485 bufffer
    return false;
  }
}

void Message_RS485::RS485SendMessage(byte tMsg[]) {
  // This routine must *only* be called when an entire message is ready to write, not a byte at a time.
  // This version, as part of the RS485 message class, automatically calculates and adds the CRC checksum.
  digitalWrite(PIN_RS485_TX_LED, HIGH);       // Turn on the transmit LED
  digitalWrite(PIN_RS485_TX_ENABLE, RS485_TRANSMIT);  // Turn on transmit mode
  byte tMsgLen = getLen(tMsg);
  tMsg[tMsgLen - 1] = calcChecksumCRC8(tMsg, tMsgLen - 1);  // Insert the checksum into the message
sprintf(lcdString, "%.20s", "Before write.");
myLCD->send(lcdString);

sprintf(lcdString, "Writing %2i bytes.", tMsgLen);
myLCD->send(lcdString);

  mySerial->write(tMsg, tMsgLen);  // flush() makes it impossible to overflow the outgoing serial buffer, which CAN happen in my test code.
                                   // Although it is BLOCKING code, we'll use it at least for now.  Output buffer overflow is unpredictable without it.
                                   // Alternative would be to check available space in the outgoing serial buffer and stop on overflow, but how?
  sprintf(lcdString, "%.20s", "Before flush.");
  myLCD->send(lcdString);

  mySerial->flush();               // wait for transmission of outgoing serial data to complete.  Takes about 0.1ms/byte.

  sprintf(lcdString, "%.20s", "After flush.");
  myLCD->send(lcdString);

  digitalWrite(PIN_RS485_TX_ENABLE, RS485_RECEIVE);  // receive mode
// DEBUG: Keep the transmit LED lit for 500ms after write is completed*************************************************************************************************************

delay(500);
  digitalWrite(PIN_RS485_TX_LED, LOW);       // Turn off the transmit LED
  return;
}

byte Message_RS485::calcChecksumCRC8(const byte data[], byte len) {
  // calcChecksumCRC8 returns the CRC-8 checksum to use or confirm.
  // Used for RS485 messages
  // Sample call: msg[msgLen - 1] = calcChecksumCRC8(msg, msgLen - 1);
  // We will send (sizeof(msg) - 1) and make the LAST byte
  // the CRC byte - so not calculated as part of itself ;-)
  byte crc = 0x00;
  while (len--) {
    byte extract = *data++;
    for (byte tempI = 8; tempI; tempI--) {
      byte sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  return crc;
}

byte Message_RS485::getLen(byte tMsg[]) {
  return tMsg[RS485_LEN_OFFSET];
}

byte Message_RS485::getTo(byte tMsg[]) {
  return tMsg[RS485_TO_OFFSET];
}

byte Message_RS485::getFrom(byte tMsg[]) {
  return tMsg[RS485_FROM_OFFSET];
}

char Message_RS485::getType(byte tMsg[]) {
  return tMsg[RS485_TYPE_OFFSET];
}

void Message_RS485::setLen(byte tMsg[], byte len) {
  tMsg[RS485_LEN_OFFSET] = len;
  return;
}

void Message_RS485::setTo(byte tMsg[], byte to) {
  tMsg[RS485_TO_OFFSET] = to;
  return;
}

void Message_RS485::setFrom(byte tMsg[], byte from) {
  tMsg[RS485_FROM_OFFSET] = from;
  return;
}

void Message_RS485::setType(byte tMsg[], char type) {
  tMsg[RS485_TYPE_OFFSET] = type;
  return;
}