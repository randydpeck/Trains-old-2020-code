// Comment out MASTER to compile as SLAVE

//#define MASTER

#include "Train_Consts_Global.h"
#define _Digole_Serial_UART_                  // Tell compiler to use only the serial part of the DigoleSerial class library
#include "DigoleSerial.h"                     // Tell the compiler to use the DigoleSerial class library
#include "Display_2004.h"                     // Our LCD message-display library; requires DigoleSerialDisp object "digoleLCD"//char APPVERSION[LCD_WIDTH + 1] = "A-TS1 Rev. 07/27/18";

//const byte THIS_MODULE = ARDUINO_BTN;  // Not sure if/where I will use this - intended if I call a common function but will this "global" be seen there?
byte THIS_MODULE = ARDUINO_BTN;  // Not sure if/where I will use this - intended if I call a common function but will this "global" be seen there?
char lcdString[LCD_WIDTH + 1];                // Global array to hold strings sent to Digole 2004 LCD; last char is for null terminator.
                                              // *** RS485 & Digital Pin MESSAGE CLASS (Inter-Arduino communications):
#ifdef MASTER
  char APPVERSION[LCD_WIDTH + 1] = "Master Rev. 07/28/18";
  #include "Message_MAS.h"                      // This module's communitcation class, in its .ini directory
#else
  char APPVERSION[LCD_WIDTH + 1] = "Slave Rev. 07/28/18";
  #include "Message_BTN.h"
#endif

void setup() { 
  pinMode(16, OUTPUT);         // PINMODE seems to have fixed my problems with phantom input appearing in the RS485 input buffers.  Sheesh.
  pinMode(17, INPUT_PULLUP);

}

void loop() {

  // Apparently Serial.begin() does *not* work inside of constructors.  Nor does delay().
  // So create an object.init() function that gets called in setup() (or at the top of loop()) instead.

  // *** SERIAL LCD DISPLAY CLASS:
  // We will create an object called "LCD" that displays messages on the 2004 display (i.e. LCD.send)  Although this object requires that we have
  // previously created a "digoleLCD" object from the DigoleSerial class library, "LCD" is not a child class/object of digoleLCD.  When we instantiate
  // our LCD object, we'll pass it a pointer to the digoleLCD object, and then we can forget about the Digole class/object altogether.
  //#define _Digole_Serial_UART_                  // Tell compiler to use only the serial part of the DigoleSerial class library
  //#include "DigoleSerial.h"                     // Tell the compiler to use the DigoleSerial class library
  //#include "Display_2004.h"                     // Our LCD message-display library; requires DigoleSerialDisp object "digoleLCD"
  //char lcdString[LCD_WIDTH + 1];                // Global array to hold strings sent to Digole 2004 LCD; last char is for null terminator.

  // DigoleSerialDisp needs serial port address i.e. &Serial, &Serial1, &Serial2, or &Serial3.
  // DigoleSerialDisp also needs a valid baud rate, typically 9600 or 115200.
  DigoleSerialDisp digoleLCD(&Serial1, SERIAL1_SPEED);  // Instantiate and name the DigoleSerial object, required for the Display_2004 class.
  Display_2004 LCD(&digoleLCD);                 // Finally, instantiate our LCD object "LCD" by passing a pointer to the Digole LCD object.

  // *** RS485 & Digital Pin MESSAGE CLASS (Inter-Arduino communications):
  #ifdef MASTER
    Message_MAS Message(&Serial2, SERIAL2_SPEED, &LCD);  // Instantiate our RS485/digital communications object "Message."
  #else
    Message_BTN Message(&Serial2, SERIAL2_SPEED, &LCD);  // Instantiate our RS485/digital communications object "Message."
  #endif

  //  LCD.init();  (moved into constructor since this is now in loop()     // Init the 2004 Digole LCD object; must be done here, not in constructor, because it includes delay().

  Serial.begin(SERIAL0_SPEED);          // Serial0 a.k.a. Serial, is the PC serial monitor window
  // Serial1 is for the Digole 20x4 LCD debug display, and the port was initialized when we instantiated the dogoleLCD object, above.
  // Serial2 is for our RS485 message bus, and was initialized when we instantiated the Message object, above.

  sprintf(lcdString, "%.20s", APPVERSION);       // Display the application version number on the LCD display
  LCD.send(lcdString);

  //Serial.print(F("This is a really long debug message that I'm using to test how much memory is saved with the F() macro."));

  while (Serial2.read() != -1);  // Clear the RS485 input buffer NOT NEEDED, NOT THE PROBLEM ************************************************************

  while (true) {

    #ifdef MASTER  // Master will watch for RTX pin pulled low by slave, then send ack RS485 msg, then wait for RS485 button number msg.

      pinMode(PIN_REQ_TX_A_BTN_IN, INPUT);
//delay(1000);
      sprintf(lcdString, "%.20s", "Master checking...");
      LCD.send(lcdString);
      byte buttonPressed = 0;
      while (true) {
//delay(1000);
        buttonPressed = Message.getTurnoutButtonPress();
        if (buttonPressed > 0) {  // We got a button press!
          sprintf(lcdString, "Button %2i pressed.", buttonPressed);
          LCD.send(lcdString);
          delay(2000);
        } else {
          sprintf(lcdString, "%.20s", "No press.");
          LCD.send(lcdString);
        }
      }

    #else  // Must be SLAVE.  Slave will RTX by pulling a pin low, then wait to receive ack from A_MAS, then send RS485 button number.

      pinMode(PIN_REQ_TX_A_BTN_OUT, OUTPUT);
      digitalWrite(PIN_REQ_TX_A_BTN_OUT, HIGH);
//delay(1000);
      sprintf(lcdString, "%.20s", "BTN sending RTX...");
      LCD.send(lcdString);
      byte buttonPressed = 17;
      Message.sendTurnoutButtonPress(buttonPressed);  // Don't even need to send it the message buffer!
      sprintf(lcdString, "%.20s", "Back in loop()");
      LCD.send(lcdString);

    #endif  // MASTER vs SLAVE
  }
}

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

//void loop() {

void endWithFlashingLED(int x) {}

void checkIfHaltPinPulledLow() {}