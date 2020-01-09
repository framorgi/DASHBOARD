/******************************************************************************
COMP100, Arduino Radio Module Transponder and Light  for FSX/P3D
Version   :  Alfa 0.1 (c) _morgi 2019
 CPU:     :  Arduino Micro 
 switches :  5 3-position switches 
 SW01-07  :  COM1, COM2, BOTH, NAV1, NAV2,  MKR, DME, GPS/NAV (toggle like function)
 LEDs     :   A,    F,    B,    G,    C,     E,   DP    on   DIG4
 Display  :  MAX7219 8-segment array
 Link2FS  :  MUST use "Include CR/LF at end" or won't work, this code relies on NL char (dec 10) for EOL
*******************************************************************************/
String ProjVersion = "/******************************************************************************\n COMP100, Arduino Radio Module Transponder and Light  for FSX/P3D\nVersion   :  Alfa 0.1 (c) _morgi 2019\n CPU:     :  Arduino Micro\n switches :  5 3-position switches\nSW01-07  :  COM1, COM2, BOTH, NAV1, NAV2,  MKR, DME, GPS/NAV (toggle like function)\n LEDs     :   A,    F,    B,    G,    C,     E,   DP    on   DIG4\n Display  :  MAX7219 8-segment array\n Link2FS  :  MUST use 'Include CR/LF at end' or won't work, this code relies on NL char (dec 10) for EOL\n*******************************************************************************\n";


// LIBRARIES
// #include "math.h"            // quadrature needs math lib
// #include "SoftReset.h"          // SoftReset lib,   https://github.com/WickedDevice/SoftReset
//#include "LedControl.h"         // MAX7219 library, http://www.wayoda.org/arduino/ledcontrol/index.html
// #include "ClickEncoder.h"     // Rotary switch encoder lib, https://github.com/0xPIT/encoder




// ~~~~~~~ MAX7219 pins   We need 4 8-seg display   Uncomment for 8-seg display  development
// Probably we will need to search for another library for driving 8-seg  
/*const uint8_t MAX7219_PIN_MOSI = 8;
const uint8_t MAX7219_PIN_CS   = 9;
const uint8_t MAX7219_PIN_CK   = 10;
const uint8_t MAX7219_NUMCHIPS = 1;  // one chip
*/

// STRUCTS

// ~~~~~~~ MAX7219 Driver class  We need 4 8-seg display   Uncomment for 8-seg display  development
// Probably we will need to search for another library for driving 8-seg
//LedControl lc = LedControl(MAX7219_PIN_MOSI, MAX7219_PIN_CK, MAX7219_PIN_CS, MAX7219_NUMCHIPS);
//uint8_t LEDIntensity = 10;




// Typedef for switch data structure used in function ProcessMomentary

//   swmom_  for functions, SW_MOMENTARY_TYPE for variables
typedef struct swmom_ {
  uint8_t pin;
  unsigned long millis;
  uint8_t state;
} SW_MOMENTARY_TYPE;


// Typedef for switch data structure used in function ProcessStatic
//   swstatic_  for functions, SW_STATIC_TYPE for variables
typedef struct swstatic_ {
  uint8_t pin;

  uint8_t state;
} SW_STATIC_TYPE;

//VARIABLES 
// Switch variables, structures  We need 2 mom sw for toggle stdby and active freq 
//                 var,    pin, 0, 0
SW_MOMENTARY_TYPE SW_S1 = { 4, 0, 0};
SW_MOMENTARY_TYPE SW_S2 = { 5, 0, 0};


// debounce and pushed long time values
long               debouncesw=50;               // switch debounce time in milliseconds (1000 millis per second)
const unsigned int PUSH_LONG_LEVEL1  = 2000;    // Encoder Switch pressed LONG time value (5000 = five seconds)
unsigned long      millisSinceLaunch = 1;       // previous reading of millis, seed with 1 to init, set to 0 if don't want to use
unsigned long      delaytime=250;
unsigned long      shortdel=50;
unsigned long      mydelay=100;

//  Main loop millies comparators  
unsigned long ul_PreviousMillis;
unsigned long ul_CurrentMillis;

// Pin 13 has an LED connected on most Arduino boards, light it up
uint8_t led = 13;

// variables for master and avionics switches or states, test state, lights, etc
boolean gbMasterVolts=false;


// variables for SerialIO (reserve 200 chars in SETUP)
String  SerialInString = "";        // a string to hold incoming data (reserved in SETUP)
boolean SerialInReady = false;      // whether the string is complete
String  SerialCmdString = "";       // a string to hold broken up commands (reserved in SETUP)
boolean SerialCmdReady = false;     // whether the command is ready

// variable for watchdog timer (eg SIM up or down) MAYBE UNUSEFUL
/*long lWatchDog;
uint8_t  iCntWatchDog;
uint8_t  giDoingDemo;*/


/*
 ####  ###### ##### #    # #####  
#      #        #   #    # #    # 
 ####  #####    #   #    # ##### 
     # #        #   #    # #    
 ####  ######   #    ####  #      
*/
 void setup() {

  // Light up the LED on pin 13
  // pinMode(led, OUTPUT); 
  // digitalWrite(led, HIGH);           // turn the LED on (HIGH is the voltage level)

  // MAX7219 wake up
 /*lc.shutdown(0,false);              // wake up MAX7219
  lc.setIntensity(0,10);             // set intensity
  lc.clearDisplay(0);                // clear display
*/
  // Switches, set the pins for pullup mode  READ THIS!!! --> https://www.arduino.cc/en/Tutorial/DigitalPins
  pinMode(SW_S1.pin, INPUT_PULLUP);
  pinMode(SW_S2.pin, INPUT_PULLUP);


  // light GPS on GPS/NAV
  // lc.setRow(0,0, SEGE);   SW_S8_GPSNAV=1;
 
  // Seed the millies
  // BlinkLast = lWatchDog = ul_PreviousMillis = millis();
  // BlinkLast = millis();
  // lWatchDog = millis();

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
  uint8_t disp = 0;
  // light up everything
//  lc.setRow(disp,4,B11111111);   // SWITCH LEDS
//  lc.setRow(disp,5,B11111111);   // TOP ROW LEDS
//  lc.setRow(disp,1,B11111111);   // BOT ROW LEDS
//  lc.setRow(disp,7,B11111111);   // OMI ROW LEDS
//  lc.setRow(disp,0,B11111111);   // GPS/NAV switch

  // Wait X milliseconds then powerdown displays and wait for commands from host
  // turn everything off
//  lc.setRow(disp,4,B00000000);   // SWITCH LEDS
//  lc.setRow(disp,5,B00000000);   // TOP ROW LEDS
//  lc.setRow(disp,1,B00000000);   // BOT ROW LEDS
//  lc.setRow(disp,7,B00000000);   // OMI ROW LEDS
//  lc.setRow(disp,0,B00000000);   // GPS/NAV switch

} // end of SETUP 



/* 
#       ####   ####  #####  
#      #    # #    # #    # 
#      #    # #    # #####   
#      #    # #    # #      
######  ####   ####  #      
*/
void loop() {
  //TIME SNAPSHOTS FOR LOOP
  ul_CurrentMillis = millis();

//FOR NOW, NO READ FROM SERIAL --> later

// *************************************************************
  // READ and PARSE SERIAL INPUT
  // void SerialEvent used to gather data up to NL
  // *************************************************************

  // *************************************************************
  // READ and PARSE SERIAL INPUT (version 2)
  // Jim's program (Link2FS) sends commands batched up /A1/F1/J1/K1 (no NL separator)
  // added IsSerialCommand routine to fetch them one at a time up to separator like / or =
  // leaving rest on serialbuffer for next loop to process
  // *************************************************************




// *************************************************************
  // Respond to switch button presses 
  // processing 2 mom sw in radio panel
  // *************************************************************

// Check using pointers instead of big chunks o code
  ProcessMomentary(&SW_S1);          // TOGGLE STDBY COM SW
  ProcessMomentary(&SW_S2);          // TOGGLE STDBY INSTRUM SW
  
// ***************************************************
  // Switch action, state==2 means pressed and debounced, ready to read
  // ***************************************************
 
 
 // SEND CMD AND CHANGE TO READY STATE  Act on regular button presses
  if(SW_S1.state==2) { Serial.println("A06"); SW_S1.
  state=3; }   // TOGGLE STDBY COM SW
  if(SW_S2.state==2) { Serial.println("A12"); SW_S2.state=3; }   // TOGGLE STDBY INSTRUM SW
    
}




/*
###### #    # #    #  ####  ##### #  ####  #    #  ####  
#      #    # ##   # #    #   #   # #    # ##   # #      
#####  #    # # #  # #        #   # #    # # #  #  ####  
#      #    # #  # # #        #   # #    # #  # #      # 
#      #    # #   ## #    #   #   # #    # #   ## #    # 
#       ####  #    #  ####    #   #  ####  #    #  ####  
*/

/*-----------------------------------
 ProcessMomentary(pointer to swmom_ struct)
  Brian McMullan SIMAV8 2016
  Processes the specified switch state of momentary buttons
  Read the state of the switch to get results
  - state 0, switch not pressed
  - state 2, switch pressed and debounced (short press)
  - state 4, switch pressed and held for PUSH_LONG_LEVEL1 (eg 3 seconds, 5 seconds)
  - state 6, switch pressed and held for 2x PUSH_LONG_LEVEL1
------------------------------------*/
void ProcessMomentary(struct swmom_ *AnySwitch)
{
  if (digitalRead(AnySwitch->pin) == LOW) {       // switch pressed
    switch(AnySwitch->state) {
      case 0: /* first instance of press */ AnySwitch->millis = ul_CurrentMillis; AnySwitch->state = 1; break;
      case 1: /* debouncing counts */       if (ul_CurrentMillis - AnySwitch->millis > debouncesw) { AnySwitch->state = 2; break; }
      case 2: /* DEBOUNCED, read valid */   break;
      case 3: /* For LONG press  _not in this code_*/          if (ul_CurrentMillis - AnySwitch->millis > PUSH_LONG_LEVEL1) { AnySwitch->state = 4; break; }
      case 4: /* LONG PRESS, read valid */  break;
      case 5: /* For LONG * 2 press  _not in this code_*/      if (ul_CurrentMillis - AnySwitch->millis > (PUSH_LONG_LEVEL1*2)) { AnySwitch->state = 6; break; }
      case 6: /* LONG PRESS * 2, read  */   break;
      case 7: /* LONG PRESS * 2, hold  */   break;      
    } // end switch
  } else {    // button went high, reset the state machine
    AnySwitch->state = 0;
  }
}

/*********************************************************************************************************
SerialEvent gathers incoming serial string, but with Jim's can be multiple commands per line like
  =M0=N1=O0
that needs to be broken up and fetched as =M0, then =N1, then =O0 as if came that way
this routine does that breaking up
*********************************************************************************************************/
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
    if (SerialInString.length() == 0) {  SerialInReady = false;}
    return(true);
    
  } else {
    return(false);
  }
} // end function

/*********************************************************************************************************
SerialEvent occurs whenever a new data comes in the hardware serial RX.  This routine is run each
time loop() runs, so using delay inside loop can delay response.  Multiple bytes of data may be available.
 reads one char at a time
 when NL is found
   calls that string ready for processing
   sets SerialInReady to true
   no more chars will be read while SerialInReady is true
 to break up single line multiple command strings, use external routine
*********************************************************************************************************/
void serialEvent() {
  while (Serial.available() && !SerialInReady) {
      // get the new byte(s), stopping when CR or LF read
      char inChar = (char)Serial.read(); 
      // static boolean EOL = false;
   
      // debugging, dump every character back
      //Serial.print("[");  Serial.print(inChar,DEC); Serial.println("]");

      // Windows Serial.writeline sends NL character (ASCII 10)
      // Putty and other terms send CR character (ASCII 13)
      
      if (inChar == 13) { break; }      // ignore CR

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
