/******************************************************************************
  Arduino Panel Module  for FSX/P3D
  Display  :  MAX7219 8-segment array
  Link2FS  :  MUST use "Include CR/LF at end" or won't work, this code relies on NL char (dec 10) for EOL
*******************************************************************************/
String ProjVersion = "/*********************************************************************************\n";

// *************************************************************
//  ~~~~~~~ LIBRARIES SECTION ~~~~~~~
//
// *************************************************************

// #include "math.h"            // quadrature needs math lib
// #include "SoftReset.h"       // SoftReset lib,   https://github.com/WickedDevice/SoftReset

#include "LedControl.h"         // MAX7219 library, http://www.wayoda.org/arduino/ledcontrol/index.html  
#include "ClickEncoder.h"       // Rotary switch encoder lib, https://github.com/0xPIT/encoder
#include <TimerOne.h>

// *************************************************************
//  ~~~~~~~ CONSTANTS DEFINITION SECTION ~~~~~~~
//
// *************************************************************


// *************************************************************
//  MAX7219 DISPLAY Constant setting
// *************************************************************
#define LED_INTENSITY 5
#define MAX_NUMCHIPS 2

// *************************************************************
//  PINOUT setting
// *************************************************************
#define ENC_E1_PIN_A 2
#define ENC_E1_PIN_B 3
#define ENC_E1_PIN_BTN 7
#define SW_S1_PIN 5
#define SW_S2_PIN 6
#define SW_ST1_PIN 4
#define MAX_PIN_MOSI  12
#define MAX_PIN_CS    10
#define MAX_PIN_CK  11
#define LED_PIN 13





// *************************************************************
//  ~~~~~~~ STRUCTS & VAR  DEFINITION SECTION ~~~~~~~
//
// *************************************************************

// *************************************************************
//  MAX7219 definition
// ~~~~~~~ MAX7219
// ~~~~~~~ MAX7219 Driver class  We need 4 8-seg display
// *************************************************************
LedControl lc = LedControl(MAX_PIN_MOSI, MAX_PIN_CK, MAX_PIN_CS, MAX_NUMCHIPS);


// *************************************************************
//  SW_MOMENTARY_TYPE definition
//  Typedef for switch data structure used in function ProcessMomentary
//  swmom_  for functions, SW_MOMENTARY_TYPE for variables
// *************************************************************
typedef struct swmom_ {
  uint8_t pin;
  unsigned long millis;
  uint8_t state;
} SW_MOMENTARY_TYPE;

// *************************************************************
//  MOMENTARY SWITCH Instance
// *************************************************************
SW_MOMENTARY_TYPE SW_S1 = { SW_S1_PIN, 0, 0};
SW_MOMENTARY_TYPE SW_S2 = { SW_S2_PIN, 0, 0};

// *************************************************************
//  SW_STATIC_TYPE definition
//  Typedef for switch data structure used in function ProcessStatic
//  swstatic_  for functions, SW_STATIC_TYPE for variables
// *************************************************************
typedef struct swstatic_ {
  uint8_t pin;
  uint8_t prec_state;
  uint8_t state;
} SW_STATIC_TYPE;

// *************************************************************
//  STATIC SWITCH Instance
// *************************************************************
SW_STATIC_TYPE SW_ST1 = { SW_ST1_PIN, 0, 0};




// *************************************************************
//   ClickEncoder definition
//   ~~~~~~~ ClickEncoder var
//   ~~~~~~~ ClickEncoder Driver class
// *************************************************************
ClickEncoder *ENC_E1;
int16_t lastEncoder_E1 = 0, valueEncoder_E1;


// *************************************************************
//   Timer ISR definition
//   ISR Call for encoder service every 1 ms( set call interval time in setup section)
// *************************************************************
void timerIsr() {
  ENC_E1->service();
}


// *************************************************************
//   TIMING variables
//   variables for Millis counter
// *************************************************************
unsigned long ul_PreviousMillis;
unsigned long ul_CurrentMillis;

// *************************************************************
//   DEBOUNCE variables
//   DEBOUNCE and pushed long time values
// *************************************************************

long               debouncesw = 50;             // switch debounce time in milliseconds (1000 millis per second)
const unsigned int PUSH_LONG_LEVEL1  = 2000;    // Encoder Switch pressed LONG time value (5000 = five seconds)
unsigned long      millisSinceLaunch = 1;       // previous reading of millis, seed with 1 to init, set to 0 if don't want to use
unsigned long      delaytime = 250;
unsigned long      shortdel = 50;
unsigned long      mydelay = 100;




// *************************************************************
//   Serial_IO variables
//   variables for Serial_IO (reserve 200 chars in SETUP)
// *************************************************************
String  SerialInString = "";        // a string to hold incoming data (reserved in SETUP)
boolean SerialInReady = false;      // whether the string is complete
String  SerialCmdString = "";       // a string to hold broken up commands (reserved in SETUP)
boolean SerialCmdReady = false;     // whether the command is ready



// *************************************************************
//   GENERIC variables
//   variables for flags and states
//   variables for master and avionics switches or states, test state, lights, etc
// *************************************************************
boolean gbMasterVolts = false;
boolean gbAvionicsVolts = false;

// *************************************************************
//   END PREPROCESSOR CODE
// *************************************************************







// *************************************************************
//
//  ####  ###### ##### #    # #####
//  #     #        #   #    # #    #
//  ####  #####    #   #    # #####
//      # #        #   #    # #
//  ####  ######   #    ####  #
//
//  ~~~~~~~~~~~~~~~~~~   SETUP SECTION   ~~~~~~~~~~~~~~~~~~~~~~
// *************************************************************
void setup() {

  //  MAX7219 wake up
  lc.shutdown(0, false);             // wake up MAX7219
  lc.setIntensity(0, LED_INTENSITY);            // set intensity
  lc.clearDisplay(0);                // clear display

  // Same routine for MAX nr 2 
  lc.shutdown(1, false);             // wake up MAX7219
  lc.setIntensity(1, LED_INTENSITY);            // set intensity
  lc.clearDisplay(1);                // clear display


  
  // Switches, set the pins for pullup mode  READ THIS!!! --> https://www.arduino.cc/en/Tutorial/DigitalPins
  pinMode(SW_S1.pin, INPUT_PULLUP);
  pinMode(SW_S2.pin, INPUT_PULLUP);
  pinMode(SW_ST1.pin, INPUT_PULLUP);
  //Encoder instantiation
  ENC_E1 = new ClickEncoder(ENC_E1_PIN_A, ENC_E1_PIN_B, ENC_E1_PIN_BTN, 4);


  //Timer1 setup
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);



  // ********* Serial communications to PC
  Serial.begin(115200);  // output
  SerialInString.reserve(200);        //  IN reserve 200 bytes for the inputString
  SerialCmdString.reserve(200);       //  OUT reserve another 200 for post processor
  /*
    while (!Serial) {
    ; // wait for serial port to connect (you have to send something to Arduino)
      // I just ignore it and press on assuming somethings listening
    }
    if (Serial.available() > 0) {
  */
  // whenever DTR brought high on PC side, this will spit out and program resets
  // PC side, VB.NET..   SerialPort1.DtrEnable = True      'raise DTR
  //Serial.println(ProjVersion);
  // Serial.println(millisSinceLaunch);
  // Serial.println("REF");     // request all new data from SIM
  // }

  // flicker everything really quickly just to show it's running to user

  // light up everything MAX NR 1

  for (int j = 0; j <= 1; j++) {
    for (int i = 7; i >= 0; i--) {
      lc.setRow(0, i, B10000000); // SWITCH LEDS
      delay(10);
      lc.setRow(0, i, B11111111); // SWITCH LEDS
      delay(100);
      lc.setRow(0, i, B00000000); // SWITCH LEDS
    }
 } 
  // light up everything  MAX NR 2
  for (int j = 0; j <= 1; j++) {
    for (int i = 7; i >= 0; i--) {
      lc.setRow(1, i, B10000000); // SWITCH LEDS
      delay(10);
      lc.setRow(1, i, B11111111); // SWITCH LEDS
      delay(100);
      lc.setRow(1, i, B00000000); // SWITCH LEDS
    }
  }

  // Wait X milliseconds then powerdown displays and wait for commands from host
  // turn everything off


  //InitStatic(&SW_ST1);
  // end of SETUP Light up the LED on pin 13
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);           // turn the LED on (HIGH is the voltage level)
} // end of SETUP
// *************************************************************
//   END SETUP CODE
// *************************************************************




// *************************************************************
//
//  #       ####   ####  #####
//  #      #    # #    # #    #
//  #      #    # #    # #####
//  #      #    # #    # #
//  ######  ####   ####  #
//
//  ~~~~~~~~~~~~~~~~~~   LOOP SECTION   ~~~~~~~~~~~~~~~~~~~~~~
// *************************************************************
void loop() {
  //TIME SNAPSHOTS FOR LOOP
  ul_CurrentMillis = millis();

  // *************************************************************
  // READ and PARSE SERIAL INPUT (version 2)
  // Jim's program (Link2FS) sends commands batched up /A1/F1/J1/K1 (no NL separator)
  // added IsSerialCommand routine to fetch them one at a time up to separator like / or =
  // leaving rest on serialbuffer for next loop to process
  // *************************************************************


    if ( IsSerialCommand() )  {
      int iLen = SerialCmdString.length();
      Serial.print("Got string len "); Serial.println(iLen); Serial.print("Got: [");  Serial.print(SerialCmdString); Serial.println("]");
      char cToken; char cAction; char *cParam; 
      cToken   = SerialCmdString[0];
      cAction  = SerialCmdString[1];
      cParam = &(SerialCmdString[2]);   //cParam is a pointer, assigned to address of 3rd char in SerialCmdString
      //Serial.print("cToken  = "); Serial.println(cToken);
      //Serial.print("cAction = "); Serial.println(cAction);
      //Serial.print("cParam  = "); Serial.println(cParam);
      //Quick monitor
      //Serial.print("TAP ["); Serial.print(cToken); Serial.print(cAction); Serial.print(cParam); Serial.println("]");
      // cToken  ?, /, <, = etc
      // cAction M, J, E, etc
      // cParam  0, 1, or longer values

      // Process the command sent in
      switch (cToken) {

        case '?' :  {    // case cToken = ?
            Serial.println("");
            Serial.println("------ INPUTS ------");
            Serial.println(" Power           @Pn  (0/1)");
            Serial.println(" REBOOT Arduino  @R");
            Serial.println(" TEST Mode       @Tn (0/1)");
            Serial.println(" Brightness      @Bnn (0-15) [10]");
            Serial.println(" MASTER power    <an  (0/1)");
            Serial.println(" AVIONICS power  <gn  (0/1)");
            Serial.println(" Switch lights   =Mn=Nn=On=Pn=Qn=Rn (0/1)");
            Serial.println(" NAV GPS switch  =ln (0/1)");
            Serial.println(" OMI lights      =Vn (0-3)");
            Serial.println(" FUEL L, R       /Jn /Kn");
            Serial.println(" VACUUM          /Nn");
            Serial.println(" OIL PRESS       /Fn");
            Serial.println(" VOLTS           /Rn");
            Serial.println("------ OUTPUTS ------");
            Serial.println(" Button Presses  (try them)");
            Serial.println("");
          }
          break;

        // NEXT TOKEN TO PROCESS "<"
       /* case '<' : {   // case cToken = <
            switch (cAction) {    //MASTER switch
              case 'a' : if (cParam[0] == '0') {
                  gbMasterVolts = false;
                }
                if (cParam[0] == '1') {
                  gbMasterVolts = true;
                }
                break;   //AVIONICS switch
              case 'g' : if (cParam[0] == '0') {
                  gbAvionicsVolts = false;
                }
                if (cParam[0] == '1') {
                  gbAvionicsVolts = true;
                }
                break;
              default:   // if other commands come in, ignore them
                Serial.println("Err <  '" + SerialCmdString + "'");
                //Serial.println("!<ERR!");
                break;
            } // end switch cAtion
          } // end switch cToken is '<'
          break;  // break case cToken '<'
*/
        // NEXT cToken to Process '='
        case '=' :  {
            switch (cAction) {    // =M1=N0=01=P1 etc, switch functions lights, set the DIG4, SEGx flags
              case 'A' : if (cParam[0] == '1') {
                  printNumber(999);
                }  break;   // COM1 
                
              case 'B' : 
                  printNumber(cParam);
                 break;   // COM1


   
              default:   // if other commands come in, ignore them
                // Serial.println("invalid = cmd, use '?' for help...");
                Serial.println("Err =  '" + SerialCmdString + "'");
                break;

                
            } // end switch (cAction)
          } // end switch Token is '='
          break;  // break case cToken '='

        // NEXT cToken to Process '/'
        case '/' :  {
            switch (cAction) {
              case 'J' : if (cParam[0] == '1') {
                  printNumber(200);;
                }  break;        
                
    

              default:   // if other commands come in, ignore them
                Serial.println("Err /  '" + SerialCmdString + "'");
                // Serial.println("invalid / cmd, use '?' for help...");
                break;
            } // end switch (cAction)
          } // end switch Token is '/'
          break;  // break case cToken '/'


      
    // ANY OTHER CTOKEN (unrecognized)
    default:
    Serial.println("Err '" + SerialCmdString + "'");
    // ignore null entry, otherwise
  
  /*
    if (int(cToken) == 0 ) {
      Serial.println("use '?' for help");
    } else {
      Serial.println("invalid token, use '?' for help");
    }
  */
  
    break;
    } // end switch cToken

    //
    // post processing, if any
    //

    } // END if SerialInReady

  


  // *************************************************************
  // Process incoming Serial Comm
  // *************************************************************






  // *************************************************************
  // Respond to switch button presses
  // processing 2 mom sw in radio panel
  // *************************************************************

  // Check using pointers instead of big chunks o code
  ProcessMomentary(&SW_S1);          // TOGGLE STDBY COM SW
  ProcessMomentary(&SW_S2);          // TOGGLE STDBY INSTRUM SW

  ProcessStatic(&SW_ST1);
  // ***************************************************
  // Switch action, state==2 means pressed and debounced, ready to read
  // ***************************************************


  // SEND CMD AND CHANGE TO READY STATE  Act on regular button presses
  if (SW_S1.state == 2) {
    Serial.println("A06"); SW_S1.
    state = 3;
  }   // TOGGLE STDBY COM SW
  if (SW_S2.state == 2) {
    Serial.println("A12");  // TOGGLE STDBY INSTRUM SW
    SW_S2.state = 3;
  }



  // SEND CMD AND CHANGE TO READY STATE  Act on rotary encoder inc/dec

  valueEncoder_E1 += ENC_E1->getValue();
  if (valueEncoder_E1 != lastEncoder_E1) {
    printNumber(valueEncoder_E1);
    if (valueEncoder_E1 > lastEncoder_E1)  Serial.println("++");
    else Serial.println("--");
  }
  lastEncoder_E1 = valueEncoder_E1;
}
// *************************************************************
//   END LOOP CODE
// *************************************************************




// *************************************************************
//
//  ###### #    # #    #  ####  ##### #  ####  #    #  ####
//  #      #    # ##   # #    #   #   # #    # ##   # #
//  #####  #    # # #  # #        #   # #    # # #  #  ####
//  #      #    # #  # # #        #   # #    # #  # #      #
//  #      #    # #   ## #    #   #   # #    # #   ## #    #
//  #       ####  #    #  ####    #   #  ####  #    #  ####
//
//  ~~~~~~~~~~~~~~~~~~   FUNCTION SECTION   ~~~~~~~~~~~~~~~~~~~~~~
// *************************************************************

// *************************************************************
//   ProcessMomentary(pointer to swmom_ struct)
//   Processes the specified switch state of momentary buttons
//   Read the state of the switch to get results
//   - state 0, switch not pressed
//   - state 2, switch pressed and debounced (short press)
//   - state 4, switch pressed and held for PUSH_LONG_LEVEL1 (eg 3 seconds, 5 seconds)
//   - state 6, switch pressed and held for 2x PUSH_LONG_LEVEL1
// *************************************************************
void ProcessMomentary(struct swmom_ * AnySwitch)
{
  if (digitalRead(AnySwitch->pin) == LOW) {       // switch pressed
    switch (AnySwitch->state) {
      case 0: /* first instance of press */ AnySwitch->millis = ul_CurrentMillis; AnySwitch->state = 1; break;
      case 1: /* debouncing counts */       if (ul_CurrentMillis - AnySwitch->millis > debouncesw) {
          AnySwitch->state = 2;
          break;
        }
      case 2: /* DEBOUNCED, read valid */   break;
      case 3: /* For LONG press  _not in this code_*/          if (ul_CurrentMillis - AnySwitch->millis > PUSH_LONG_LEVEL1) {
          AnySwitch->state = 4;
          break;
        }
      case 4: /* LONG PRESS, read valid */  break;
      case 5: /* For LONG * 2 press  _not in this code_*/      if (ul_CurrentMillis - AnySwitch->millis > (PUSH_LONG_LEVEL1 * 2)) {
          AnySwitch->state = 6;
          break;
        }
      case 6: /* LONG PRESS * 2, read  */   break;
      case 7: /* LONG PRESS * 2, hold  */   break;
    } // end switch
  } else {    // button went high, reset the state machine
    AnySwitch->state = 0;
  }
}



// *************************************************************
//   ProcessStatic(pointer to swmom_ struct)
//   Processes the specified switch state of static switch
//   Send serial value on state change
// *************************************************************
void ProcessStatic(struct swstatic_ * AnySwitch)
{
  AnySwitch->state = digitalRead(AnySwitch->pin);
  if (AnySwitch->state != AnySwitch->prec_state) {
    if ( AnySwitch->state  == LOW) Serial.println("A01");
    else Serial.println("A02");
  }
  AnySwitch->prec_state = AnySwitch->state;
}

// *************************************************************
//   InitStatic(pointer to swmom_ struct)
//   Read state of static switch and send consistent value
//   MAYbe Better call this during setup ending (not before)
// *************************************************************
void InitStatic(struct swstatic_ * AnySwitch)
{
  AnySwitch->state = digitalRead(AnySwitch->pin);
  AnySwitch->prec_state = AnySwitch->state;
  if ((AnySwitch->state == LOW))
    Serial.println("A01");

  else Serial.println("A02");


}


// *************************************************************
//   SerialEvent() gathers incoming serial string, but with Jim's can be multiple commands per line like
//   =M0=N1=O0
//   that needs to be broken up and fetched as =M0, then =N1, then =O0 as if came that way
//   this routine does that breaking up
// *************************************************************
boolean IsSerialCommand() {
  // sees if serial data is available (full line read up to NL)
  // busts it up as individual commands if so and feeds back to caller
  uint8_t iSISptr = 0;
  char cSISchar;
  uint8_t iLenTemp = SerialInString.length();
  String sTempStr;

  SerialCmdString = "";      // clear the command buffer

  if (SerialInReady) {       // data on the In buffer?
    //testing
    //Serial.print("ISC [");Serial.print(SerialInString); Serial.println("]");

    // copy from serial buffer up any tokens OR NL
    do {    // always do the first char
      SerialCmdString += SerialInString[iSISptr];    // add char to command
      iSISptr++;                                     // point to next char on IN buffer
      cSISchar = SerialInString[iSISptr];            // store next char
      //   keep doing this till hit a token or NL
    } while (cSISchar != '/' && cSISchar != '=' && cSISchar != '<' && cSISchar != 10 && cSISchar != 0);

    // copy primary string into a temp buffer CHOPPING off command already read
    sTempStr = &(SerialInString[iSISptr]);
    // copy temp buffer back to primary buffer (shortening it)
    SerialInString = sTempStr;

    // if finally emptied the existing IN buffer, set flag so more can be read
    if (SerialInString.length() == 0) {
      SerialInReady = false;
    }
    return (true);

  } else {
    return (false);
  }
} // end function


// *************************************************************
//   SerialEvent occurs whenever a new data comes in the hardware serial RX.  This routine is run each
//   time loop() runs, so using delay inside loop can delay response.  Multiple bytes of data may be available.
//   reads one char at a time
//   when NL is found
//   calls that string ready for processing
//   sets SerialInReady to true
//   no more chars will be read while SerialInReady is true
//   to break up single line multiple command strings, use external routine
// *************************************************************
void serialEvent() {
  while (Serial.available() && !SerialInReady) {
    // get the new byte(s), stopping when CR or LF read
    char inChar = (char)Serial.read();
    // static boolean EOL = false;

    // debugging, dump every character back
    //Serial.print("[");  Serial.print(inChar,DEC); Serial.println("]");

    // Windows Serial.writeline sends NL character (ASCII 10)
    // Putty and other terms send CR character (ASCII 13)

    if (inChar == 13) {
      break;  // ignore CR
    }

    // if NL (aka LF, aka ASCII 10) call that end of string, flag it
    if (inChar == 10) {
      SerialInReady = true;
      // if want to echo string back to sender, uncomment this
      // Serial.println(SerialInString);
    } else {
      // add it to the SerialInString:
      SerialInString += inChar;
    }
  } // end while
} // end SerialEvent handler

// *************************************************************
//   printNumber(num)
//   Convert integer
//
// *************************************************************
void printNumber(int v) {
  int ones;
  int tens;
  int hundreds;
  boolean negative;

  if (v < 0) {
    negative = true;
    lc.setChar(0, 3, '-', false);
    v = v * -1;
  }
  else {
    //print a blank in the sign column
    lc.setRow(0, 3, B00000000);
  }
  ones = v % 10;
  v = v / 10;
  tens = v % 10;
  v = v / 10;
  hundreds = v;




//Now print the number digit by digit
lc.setDigit(0, 2, (byte)hundreds, false);
lc.setDigit(0, 1, (byte)tens, false);
lc.setDigit(0, 0, (byte)ones, false);
}
