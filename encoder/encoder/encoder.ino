
#include "ClickEncoder.h"
#include <TimerOne.h>


ClickEncoder *encoder;
int16_t last, value;

void timerIsr() {
  encoder->service();
}


void setup() {
  Serial.begin(9600);
  encoder = new ClickEncoder(5, 6, A2);

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); 
  
  last = 0;
  
}

void loop() {  
  value += encoder->getValue();
  
  if (value != last) {
    Serial.print(value);
   Serial.print(" ");
   Serial.print(last);
    if (value>last)  Serial.println("++");
    else Serial.println("--");
    }
    last = value;
   

  
  

 
}
