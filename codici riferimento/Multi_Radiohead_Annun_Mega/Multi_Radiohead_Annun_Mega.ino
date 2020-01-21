/*
 
 This sketch is a complete radio head for FSX
 It needs to be used with Link2fs_Multi
 It was compiled with Arduino version 1.1
 
 Full project info at http://www.jimspage.co.nz/intro.htm 
 This example code is in the public domain.
 July 2013
 Once you get it going you may be able to remove some DELAY's
 */



#include "math.h"
#include "Quadrature.h"
#include <LiquidCrystal.h>
Quadrature quad1(8, 9);
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
int CodeIn;// used on all serial reads
int X;// a rotary variable
int Xold;// the old reading
int Xdif;// the difference since last loop
int active; // the mode thats active
int activeold;
int mark; // shows where the cursor is in the likes of ADF etc
int pulseOn = 0;// the loop that pulses the LED's
unsigned long TimeStart = 0; //used in pulsing LED's
unsigned long TimeNow = 0;
unsigned long TimeInterval = 500;

String oldpinStateSTR, pinStateSTR, pinStateString, APsp, APspold, stringnewstate,stringoldstate, com1, com1old, com1sb, com1sbold,com2, com2old, com2sb,com2sbold, airsp, airspold, altit, altitold;
String nav1, nav1old, nav1sb, nav1sbold, nav2, nav2old, nav2sb, nav2sbold;
String adf, adfold, xpdr, xpdrold, dme1, dme1old, dme2, dme2old, pitot, pitotold;
String AnunB = "0", AnunC = "0", AnunD = "0", AnunE = "0", AnunH = "0";
String AnunBx = "0", AnunCx = "0", AnunDx = "0", AnunEx = "0", AnunHx = "0";
String gearSimple;
char blank = 255;

void setup() 
{
  lcd.begin(16, 2); // set up the LCD's number of columns and rows: 
  stringoldstate = "111111111111111111111111111111111111111111111111111111111111111111111";
  for (int doutPin = 10; doutPin <= 53; doutPin++) // setup the input pins 
  {  
    pinMode(doutPin, INPUT);
    digitalWrite(doutPin, HIGH);  
  } 
  for (int PinNo = 54; PinNo <= 69; PinNo++)// Get all the OUTPUT pins ready.  
  {
    pinMode(PinNo, OUTPUT);
  }
  mark = 10;
  lcd.clear();
  Serial.begin(115200); 
}

void loop() {
   {INPUTPINS();} //Check the "button pressed" section
   {LCDMODE();}  // Sets up the LCD for the mode it's in (From the rotary switch)
   {ROTARYS();} // go and check the rotary encoder
   {PULSE_LEDs();} //Check the pulsing void
  if (Serial.available()) { // now lets check the serial buffer for any input data
    CodeIn = getChar();
    if (CodeIn == '=') {EQUALS();}// The first identifier is "="
    if (CodeIn == '?') {QUESTION();}// The first identifier is "?"
    if (CodeIn == '/') {SLASH();}// The first identifier is "/" (Annunciators)
  } // end of serial available
 }// edn of void loop
 
char getChar()// Get a character from the serial buffer
{
  while(Serial.available() == 0);// wait for data
  return((char)Serial.read());// Thanks Doug
}//end of getchar
 
void QUESTION(){    // The first identifier was "?"
CodeIn = getChar(); // Get the second identifier
  switch(CodeIn) {// Now lets find what to do with it
    case 'Y': // found the second identifier (the "Gear simple")
       gearSimple = "";
      gearSimple += getChar();// get first charactor (Nose gear)
      if (gearSimple == "2"){digitalWrite(54, HIGH);}else{digitalWrite(54, LOW);}//Nose gear
      if (gearSimple == "1"){digitalWrite(57, HIGH);}else{digitalWrite(57, LOW);}//nose gear moving
      gearSimple += getChar(); // get the second charactor (gear left)
      if (gearSimple.endsWith("2")){digitalWrite(55, HIGH);}else{digitalWrite(55, LOW);}//left gear
      if (gearSimple.endsWith("1")){digitalWrite(58, HIGH);}else{digitalWrite(58, LOW);}//left gear moving
    gearSimple += getChar(); // get the third charactor  (gear right)
      if (gearSimple.endsWith("2")){digitalWrite(56, HIGH);}else{digitalWrite(56, LOW);}//right gear
      if (gearSimple.endsWith("1")){digitalWrite(59, HIGH);}else{digitalWrite(59, LOW);}//right gear moving
    break;
     }
}// end of question void
 
 void SLASH(){    // The first identifier was "/" (Annunciator)
  CodeIn = getChar(); // Get another character
  switch(CodeIn) {// Now lets find what to do with it
  case 'B'://Found the second identifier
      AnunB = String(getChar());// get state if /B  (Overspeed flaps)
      if (AnunB == "1"){
      if (AnunBx != "3"){// checks it's not flashing
      digitalWrite(60, HIGH);
      delay(11);
      digitalWrite(65, HIGH);}}
    else{digitalWrite(60, LOW);
      AnunBx = "0";
      digitalWrite(65, LOW);}
    break;
    
  case 'C':
      AnunC = String(getChar());// get state if /C  (Overspeed gear)
      if (AnunC == "1"){
      if (AnunCx != "3"){
      digitalWrite(61, HIGH);
     delay(11); 
      digitalWrite(65, HIGH);}}
    else{digitalWrite(61, LOW);
      AnunCx = "0";
     digitalWrite(65, LOW);}
    break;
    
  case 'D':
      AnunD = String(getChar());// get state if /D  (Overspeed frame)
      if (AnunD == "1"){
      if (AnunDx != "3"){
      digitalWrite(62, HIGH);
      delay(11);
      digitalWrite(65, HIGH);}}
    else{digitalWrite(62, LOW);
      AnunDx = "0";
      digitalWrite(65, LOW);}
    break;
    
case 'E'://Found the second identifier
      AnunE = String(getChar());// get state if /E  (Stall warning)
      if (AnunE == "1"){
      if (AnunEx != "3"){
      digitalWrite(63, HIGH);
      delay(11);
      digitalWrite(65, HIGH);}}
    else{digitalWrite(63, LOW);
      AnunEx = "0";
      digitalWrite(65, LOW);}
    break;

  case 'H':
      AnunH = String(getChar());// get state if /H  (Low fuel)
      if (AnunH == "1"){
      if (AnunHx != "3"){
      digitalWrite(64, HIGH);
      delay(11);
      digitalWrite(65, HIGH);}}
    else{digitalWrite(64, LOW);
      AnunHx = "0";
      digitalWrite(65, LOW);}
    break;
  }//end of switch
}//end of slash
 
void LCDMODE(){
if (active != activeold)
    activeold = active;
  if (digitalRead(14)==0 and active != 14) { // Sets up the LCD when switching to Com1)
    lcd.setCursor(0, 0);
    delay (11);
    lcd.print("Com.1    "); 
    lcd.setCursor(9, 0);
    delay (11);
    lcd.print(com1); 
    lcd.setCursor(0, 1);
    delay (11);
    lcd.print("  s/b    ");
    lcd.setCursor(9, 1);
    delay (11);
    lcd.print(com1sb); 
    active = 14;
  }

  if (digitalRead(15)==0 and active != 15) {  // Sets up the LCD when switching to Com2)
    lcd.setCursor(0, 0);
    delay (11);
    lcd.print("Com.2    ");  
    lcd.setCursor(9, 0);
    delay (11);
    lcd.print(com2); 
    lcd.setCursor(0, 1);
    lcd.print("  s/b    ");
    lcd.setCursor(9, 1);
    delay (11);
    lcd.print(com2sb);
    active = 15;
  }

  if (digitalRead(16)==0 and active != 16) {  // Sets up the LCD when switching to Nav1)
    lcd.setCursor(0, 0);
    delay (11);
    lcd.print("Nav.1    ");  
    lcd.setCursor(9, 0);
    delay (11);
    lcd.print(nav1); 
    lcd.setCursor(0, 1);
    lcd.print("  s/b    ");
    lcd.setCursor(9, 1);
    delay (11);
    lcd.print(nav1sb);
    active = 16;
  }
  if (digitalRead(17)==0 and active != 17) {  // Sets up the LCD when switching to Nav2)
    lcd.setCursor(0, 0);
    delay (11);
    lcd.print("Nav.2    ");  
    lcd.setCursor(9, 0);
    delay (11);
    lcd.print(nav2); 
    lcd.setCursor(0, 1);
    lcd.print("  s/b    ");
    lcd.setCursor(9, 1);
    delay (11);
    lcd.print(nav2sb);
    active = 17;
  }    
  if (digitalRead(18)==0 and active != 18) {  // Sets up the LCD when switching to ADF)
    lcd.setCursor(0, 0);
    delay (11);
    lcd.print("ADF      ");  
    lcd.setCursor(9, 0);
    delay (11);
    lcd.print(adf); 
    lcd.print("  ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    if (mark == 9 ){mark = 10;}
    lcd.setCursor(mark, 1);
    lcd.print("-");
    active = 18;             
  }    
  if (digitalRead(19)==0 and active != 19) {  // Sets up the LCD when switching to DME)
    lcd.setCursor(0, 0);
    delay (11);
    lcd.print("DME1     ");  
    lcd.setCursor(9, 0);
    delay (11);
    lcd.print(dme1); 
    lcd.print("  ");
    lcd.setCursor(0, 1);
    lcd.print("DME2     ");
    lcd.setCursor(9, 1);
    delay (11);
    lcd.print(dme2);
    active = 19;
  }        
  if (digitalRead(20)==0 and active != 20) {  // Sets up the LCD when switching to XPDR)
    lcd.setCursor(0, 0);
    delay (11);
    lcd.print("XPonder  ");  
    lcd.setCursor(9, 0);
    delay (11);
    lcd.print(xpdr); 
    //lcd.print("    ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(mark, 1);
    lcd.print("-");
    active = 20;                   
   }         
  } // end of LCDmode

void EQUALS(){      // The first identifier was "="
       delay (11);
       CodeIn = getChar(); // Get another character
  switch(CodeIn) {// Now lets find what to do with it
    case 'A'://Found the reading "Com1"
        com1 = "";
        com1 += getChar();
        com1 += getChar();
        com1 += getChar();
        com1 += getChar();
        com1 += getChar();
        com1 += getChar();
        com1 += getChar();
        if (com1 != com1old && digitalRead(14) == LOW) {
          delay (11);
          lcd.setCursor(9, 0);
          delay (11);
          lcd.print(com1);
          com1old = com1;
          }
         break;
      case 'B': // Found the reading "Com1 s/b" 
        com1sb = "";
        com1sb += getChar();
        com1sb += getChar();
        com1sb += getChar();
        com1sb += getChar();
        com1sb += getChar();
        com1sb += getChar();
        com1sb += getChar();
        if (com1sb != com1sbold && digitalRead(14) == LOW ){
          delay (11);
          lcd.setCursor(9, 1);
          delay (11);
          lcd.print(com1sb);
          com1sbold = com1sb;
          }        
         break;
       case  'C': //  Found the reading "Com2"
        com2 = "";
        com2 += getChar();
        com2 += getChar();
        com2 += getChar();
        com2 += getChar();
        com2 += getChar();
        com2 += getChar();
        com2 += getChar();
        if (com2 != com2old && digitalRead(15) == LOW) {
          lcd.setCursor(9, 0);
          delay (11);
          lcd.print(com2);
          com2old = com2;
          }
         break;
       case  'D': //  Found the reading "Com2 s/b"
        com2sb = "";
        com2sb += getChar();
        com2sb += getChar();
        com2sb += getChar();
        com2sb += getChar();
        com2sb += getChar();
        com2sb += getChar();
        com2sb += getChar();
        if (com2sb != com2sbold && digitalRead(15) == LOW) {
          lcd.setCursor(9, 1);
          delay (11);
          lcd.print(com2sb);
          com2sbold = com2sb;
          }
         break;          
       case  'E': //  Found the reading "Nav1"
        delay (11);
        nav1 = "";
        nav1 += getChar();
        nav1 += getChar();
        nav1 += getChar();
        nav1 += getChar();
        nav1 += getChar();
        nav1 += getChar();
        nav1 += (" "); //pads it up to 8 caracters
        if (nav1 != nav1old && digitalRead(16) == LOW) {
          lcd.setCursor(9, 0);
          delay (11);
          lcd.print(nav1);
          nav1old = nav1;
          }
         break;
       case  'F': //  Found the reading "Nav1 s/b"
        nav1sb = "";
        nav1sb += getChar();
        nav1sb += getChar();
        nav1sb += getChar();
        nav1sb += getChar();
        nav1sb += getChar();
        nav1sb += getChar();
        nav1sb += (" "); //pads it up to 8 caracters
        if (nav1sb != nav1sbold && digitalRead(16) == LOW) {
          lcd.setCursor(9, 1);
          delay (11);
          lcd.print(nav1sb);
          nav1sbold = nav1sb;
          }              
         break;
       case  'G': //  Found the reading "Nav2"
        delay (11);
        nav2 = "";
        nav2 += getChar();
        nav2 += getChar();
        nav2 += getChar();
        nav2 += getChar();
        nav2 += getChar();
        nav2 += getChar();
        nav2 += (" "); //pads it up to 8 caracters
        if (nav2 != nav2old && digitalRead(17) == LOW) {
          lcd.setCursor(9, 0);
          delay (11);
          lcd.print(nav2);
          nav2old = nav2;
          }
         break;
       case  'H': //  Found the reading "Nav2 s/b"
        nav2sb = "";
        nav2sb += getChar();
        nav2sb += getChar();
        nav2sb += getChar();
        nav2sb += getChar();
        nav2sb += getChar();
        nav2sb += getChar();
        nav2sb += (" "); //pads it up to 8 caracters
        if (nav2sb != nav2sbold && digitalRead(17) == LOW) {
          lcd.setCursor(9, 1);
          delay (11);
          lcd.print(nav2sb);
          nav2sbold = nav2sb;
          }              
         break;
       case  'I': //  Found the reading "ADF"
        adf = "";
        adf += getChar();
        adf += getChar();
        adf += getChar();
        adf += getChar();
        adf += getChar();
        adf += getChar();
        adf += (" "); //pads it up to 8 caracters
        if (adf != adfold && digitalRead(18) == LOW) {
          lcd.setCursor(9, 0);
          delay (11);
          lcd.print(adf);
          adfold = adf;
          }
         break;   
       case  'J': //  Found the reading "XPDR"
        delay (11);
        xpdr = "";
        xpdr += getChar();
        xpdr += getChar();
        xpdr += getChar();
        xpdr += getChar();
        xpdr += ("    "); //pads it up to 8 caracters
        if (xpdr != xpdrold && digitalRead(20) == LOW) {
          lcd.setCursor(9, 0);
          delay (11);
          lcd.print(xpdr);
          xpdrold = xpdr;
          }               
         break;
       case  'K': //  Found the reading "DME1"
        delay (11);
        dme1 = "";
        dme1 += getChar();
        dme1 += getChar();
        dme1 += getChar();
        dme1 += getChar();
        dme1 += getChar();
        dme1 += ("   "); //pads it up to 8 caracters
        if (dme1 != dme1old && digitalRead(19) == LOW) {
          lcd.setCursor(9, 0);
          delay (11);
          lcd.print(dme1);
          dme1old = dme1;
          }
         break;          
       case  'L': //  Found the reading "DME2"
        delay (11);
        dme2 = "";
        dme2 += getChar();
        dme2 += getChar();
        dme2 += getChar();
        dme2 += getChar();
        dme2 += getChar();
        dme2 += ("   "); //pads it up to 8 caracters
        if (dme2 != dme2old && digitalRead(19) == LOW) {
          lcd.setCursor(9, 1);
          delay (11);
          lcd.print(dme2);
          dme2old = dme2;
        }
        break;
      }               
    }

void ROTARYS() {//now the bit for the rotary encoder input
  X = (quad1.position()/2);
  if (X != Xold) { // checks to see if it different
    (Xdif = (X-Xold));// finds out the difference
 
    if (active == 14){// Com1 rotary encoder output
      if (Xdif == 1) {
        if (digitalRead(10)==0 ) { 
          Serial.println("A01");
        } 
        else Serial.println("A03");
      }
      if (Xdif == -1) {
        if (digitalRead(10)==0 ) { 
          Serial.println("A02");
        } 
        else Serial.println("A04");
      }
    }
    if (active == 15){// Com2 rotary encoder output
      if (Xdif == 1) {
        if (digitalRead(10)==0 ) { 
          Serial.println("A07");
        } 
        else Serial.println("A09");
      }
      if (Xdif == -1) {
        if (digitalRead(10)==0 ) { 
          Serial.println("A08");
        } 
        else Serial.println("A10");
      }
    }
    if (active == 16){// Nav1 rotary encoder output
      if (Xdif == 1) {
        if (digitalRead(10)==0 ) { 
          Serial.println("A13");
        } 
        else Serial.println("A15");
      }
      if (Xdif == -1) {
        if (digitalRead(10)==0 ) { 
          Serial.println("A14");
        } 
        else Serial.println("A16");
      }
    }
    if (active == 17){// Nav2 rotary encoder output
      if (Xdif == 1) {
        if (digitalRead(10)==0 ) { 
          Serial.println("A19");
        } 
        else Serial.println("A21");
      }
      if (Xdif == -1) {
        if (digitalRead(10)==0 ) { 
          Serial.println("A20");
        } 
        else Serial.println("A22");
      }
    }
    if (active == 18){// ADF rotary encoder output
      if (digitalRead(10)==0 ) {
        if (Xdif == 1) { 
          mark = (mark - 1);
          active = 2; 
          if (mark == 13) mark =12;
        }
        if (Xdif == -1){ 
          mark = (mark + 1); 
          active = 2;
          if (mark == 13) mark =14;
        }
        if (mark > 14) mark = 14;
        if (mark < 10) mark = 10;
      }
      else{
        if (Xdif == 1) { 
          if (mark == 10) {
            Serial.println("A29"); 
          }
          if (mark == 11) {
            Serial.println("A30"); 
          }
          if (mark == 12) {
            Serial.println("A31"); 
          }
          if (mark == 14) {
            Serial.println("A32"); 
          }
        }    
        if (Xdif == -1){
          if (mark == 10){
            Serial.println("A25"); 
          }  
          if (mark == 11){
            Serial.println("A26"); 
          }  
          if (mark == 12){
            Serial.println("A27"); 
          }  
          if (mark == 14){
            Serial.println("A28"); 
          }  
        } 
      }
    }
    if (active == 20){// Xponder rotary encoder output
      if (digitalRead(10)==0 ) {
        if (Xdif == 1) { 
          mark = (mark - 1);
          active = 2; 
        }
        if (Xdif == -1){ 
          mark = (mark + 1); 
          active = 2;
        }
        if (mark > 12) mark = 12;
        if (mark < 9) mark = 9;
      }
      else{
        if (Xdif == 1) { 
          if (mark == 9) {
            Serial.println("A38"); 
          }
          if (mark == 10) {
            Serial.println("A39"); 
          }
          if (mark == 11) {
            Serial.println("A40"); 
          }
          if (mark == 12) {
            Serial.println("A41"); 
          }
        }    
        if (Xdif == -1){ 
          if (mark == 9){
            Serial.println("A34"); 
          }  
          if (mark == 10){
            Serial.println("A35"); 
          }  
          if (mark == 11){
            Serial.println("A36"); 
          }  
          if (mark == 12){
            Serial.println("A37"); 
          }  
        } 
      }    
    }

    if (quad1.position() > 1000){ // zero the rotary encoder count if too high or low
      quad1.position(0);
    }
    if (quad1.position() < -1000){
      quad1.position(0);
    }
    Xold = X; // overwrites the old reading with the new one.
    }
  }// end of rotarys
  
void INPUTPINS(){
  stringnewstate = "";
  for (int pinNo = 10; pinNo <= 53; pinNo++){// checks all the pins 10 to 53
    pinStateSTR = String(digitalRead(pinNo)); 
    oldpinStateSTR = "";
    oldpinStateSTR += String(stringoldstate.charAt(pinNo - 10));
    if (pinStateSTR != oldpinStateSTR) {// yes it's different
      if (pinNo == 11 and pinStateSTR == "0") { //Change-over button is pressed
        if (active == 14) Serial.println("A06");//com1
        if (active == 15) Serial.println("A12");//com2
        if (active == 16) Serial.println("A18");//nav1
        if (active == 17) Serial.println("A24"); //nav2
        if ( active == 18){ //adf
          mark = (mark + 1);
          if (mark == 13){ 
            mark = 14;
          }// sort out for piont in ADF
          active = 1;
          if (mark > 14)mark = 10;
        }
        if  (active == 20){ //xponder
          mark = (mark + 1);
          if (mark == 13){ 
            mark = 9;
          }// sort out for length Xponder
          active = 1;
          if (mark > 12)mark = 12;
        }
      }// end of pin 11 sortout
        if (pinNo == 21 && pinStateSTR == "0"){Serial.println ("C01");} // Gear UP 
        if (pinNo == 21 && pinStateSTR == "1"){Serial.println ("C02");} // Gear DOWN 
        if (pinNo == 22 && pinStateSTR == "0"){
           {digitalWrite(65, LOW);}
           if (AnunB == "1"){AnunBx = "3";}
           if (AnunC == "1"){AnunCx = "3";}
           if (AnunD == "1"){AnunDx = "3";}
           if (AnunE == "1"){AnunEx = "3";}
           if (AnunH == "1"){AnunHx = "3";}
           } // end of pin 22 sortout
        if (pinNo == 23 && pinStateSTR == "0"){Serial.println ("C19");} // Trim adjust DOWN
        if (pinNo == 24 && pinStateSTR == "0"){Serial.println ("C18");} // Trim adjust UP
        if (pinNo == 25 && pinStateSTR == "0"){Serial.println ("C15");} // Flaps DOWN a bit
        if (pinNo == 26 && pinStateSTR == "0"){Serial.println ("C14");} // Flaps UP a bit
        // Add more "Simconnect direct" codes here.
        // now the "keys" bit ,,, notice the "26" in line above and also in line below.
        if (pinNo > 26){ // start of "Keys" bit
      Serial.print ("D"); 
      if (pinNo < 10) Serial.print ("0");
      Serial.print (pinNo);
      Serial.println (pinStateSTR);
      } // end of "Keys" bit
    }// end of "yes it's different"
 stringnewstate += pinStateSTR;
  }
  stringoldstate = stringnewstate;
    delay(11);
}// end of inputpins

void PULSE_LEDs(){ // This void pulses the active LED's after pressing the 'Cancel' button
    TimeNow = millis();
if(TimeNow - TimeStart > TimeInterval) {
    TimeStart = TimeNow; 
    pulseOn++; 
     if (pulseOn > 1){pulseOn = 0;};
     if (AnunBx == "3"){if (pulseOn == 1){digitalWrite(60, HIGH);}else{digitalWrite(60, LOW);}}
     if (AnunCx == "3"){if (pulseOn == 1){digitalWrite(61, HIGH);}else{digitalWrite(61, LOW);}}
     if (AnunDx == "3"){if (pulseOn == 1){digitalWrite(62, HIGH);}else{digitalWrite(62, LOW);}}
     if (AnunEx == "3"){if (pulseOn == 1){digitalWrite(63, HIGH);}else{digitalWrite(63, LOW);}}
     if (AnunHx == "3"){if (pulseOn == 1){digitalWrite(64, HIGH);}else{digitalWrite(64, LOW);}}
  }//end of a time trigger
}// end of pulse_led's void


