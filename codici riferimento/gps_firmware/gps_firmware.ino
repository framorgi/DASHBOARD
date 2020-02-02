/****************************************************************************************/
/*																						*/
/*									GPS  | �Kantooya									*/
/*																						*/
/*		Author:		Sam Farah															*/
/*		Email:		sam.farah1986@gmail.com												*/
/*		Website:	http://samfarah.net													*/
/*		Version:	1.0																	*/
/*		Description:																	*/
/*				The firmware for the ATMega microcontroller on board, it controls		*/
/*				the circuit, and handles reading inputs and sends commands to FSX.		*/
/*																						*/
/****************************************************************************************/

//-------------------------- Headers ----------------------------
#include <Quadrature.h> 
#include <AS1115.h>
#include <WSWire.h>
//---------------------------------------------------------------

//---------------------------- Pins -----------------------------
//			Name			Arduino Pin				Pin on ATMega
//		-----------			-----------				-------------
#define IRQ1					A0			//			 
#define Encoder1A				A3			//	
#define Encoder1B				A2			//	
#define Encoder2A				8			//	
#define Encoder2B				9			//	
//---------------------------------------------------------------

//------------------------ SETTINGS -----------------------------
#define HoldDelay				700
//---------------------------------------------------------------

//----------------------- Prototypes ----------------------------
class TSwitchCtrl;
uint8_t I2C_ClearBus();
//---------------------------------------------------------------

//------------------------- Objects -----------------------------
Quadrature Encoder1(Encoder1A, Encoder1B);
Quadrature Encoder2(Encoder2A, Encoder2B);
TSwitchCtrl *SwitchCtr;
AS1115 *SwitchCtr1;
//---------------------------------------------------------------

//---------------------- Public Variables -----------------------
int R1old;
int R2old;
int R1dif;
int R2dif;
unsigned long int HoldTime = 0;
bool ClearPressed = false;
//---------------------------------------------------------------

//-------------------------- Classes ----------------------------
class TSwitchCtrl
{
private:
	AS1115 * Chip;
	String Data;
	String OldData;
public:
	byte IRQ;
	byte LSB, MSB;
	TSwitchCtrl(uint8_t _IRQ, AS1115 *_Chip, uint8_t _LSB, uint8_t _MSB)
	{
		IRQ = _IRQ;
		Chip = _Chip;
		LSB = _LSB;
		MSB = _MSB;
		Data = "";
		Chip->begin(0x03,0x00);
	}
	void ReadSwitchs()
	{
		uint8_t ErrorCode;
		OldData = Data;
		Data = Chip->ReadKeysMul(&ErrorCode);// GetLeadingZeros(, 16);//potentially not needed (the Leading Zeros, already handled from library)
		if (ErrorCode >= 4)
		{
			I2C_ClearBus();
			Wire.begin();
		}
	}
	String GetData() { return Data; }
	String GetOldData() { return OldData; }
};
//---------------------------------------------------------------

//------------------------ Functions -----------------------------
String GetLeadingZeros(String Bin, short digits)
{
	String Padding = "";
	for (int i = 0; i < digits - Bin.length(); i++)Padding += "0";
	return Padding + Bin;
}
//---------------------------------------------------------------

//----------------------- Interrupt ----------------------------
ISR(PCINT1_vect) // handle pin change interrupt for D8 to D13 here
{
	interrupts();
	if (digitalRead(SwitchCtr->IRQ) == LOW)
	{

		SwitchCtr->ReadSwitchs();
		byte ChangedIndex = 0;
		char NewValue, InverseValue;
		for (byte i = 0; i < 16; i++)
			if (SwitchCtr->GetData()[i] != SwitchCtr->GetOldData()[i])
			{
				NewValue = SwitchCtr->GetData()[i];
				InverseValue = NewValue == '0' ? '1' : '0';
				ChangedIndex = i;
			}
		//Serial.println(ChangedIndex);
		//if (!MasterPower && ChangedIndex != 20) return;//if powered off, only listen to power swtich.
		if (NewValue == '1') //push buttons
		{
			switch (ChangedIndex)
			{
			case 1:  Serial.println("G11"); break;
			case 2:  Serial.println("G10"); break;
			case 3:  Serial.println("G12"); break;
			case 4:  Serial.println("G13"); break;
			case 5:	 HoldTime = millis(); ClearPressed = true; break;
			case 6:  Serial.println("G18"); break;
			case 7:  Serial.println("G19"); break;
			case 9:  Serial.println("G02"); break;
			case 10: Serial.println("G03"); break;
			case 11: Serial.println("G04"); break;
			case 12: Serial.println("G07"); break;
			case 13: Serial.println("G08"); break;
			case 14: Serial.println("G09"); break;
			}
		}
		else
		{
			if (ChangedIndex == 5)
			{
				ClearPressed = false;
				if (millis() - HoldTime < HoldDelay)Serial.println("G14");
			}
		}
	}
}

//******************************************************************************************************************************************
void setup() {
	pinMode(IRQ1, INPUT_PULLUP);
	Serial.begin(115200);
	SwitchCtr1 = new AS1115(0x00);
	SwitchCtr = new TSwitchCtrl(IRQ1, SwitchCtr1, 0, 15);
	delay(50);
	pciSetup(IRQ1);
	delay(50);
	SwitchCtr->ReadSwitchs();
	SwitchCtr->GetData();
}

void loop() {
	ENCODER();
	if (ClearPressed == true && millis() - HoldTime >= HoldDelay)
	{
		Serial.println("G15");
		ClearPressed = false;
	}
}
//******************************************************************************************************************************************

//------------------------------------------------------------------------------------------------------------------------------------------
void pciSetup(byte pin)
{
	*digitalPinToPCMSK(pin) |= bit(digitalPinToPCMSKbit(pin));  // enable pin
	PCIFR |= bit(digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
	PCICR |= bit(digitalPinToPCICRbit(pin)); // enable interrupt for the group
}
//------------------------------------------------------------------------------------------------------------------------------------------
void ENCODER() {
	int R1 = (Encoder1.position()); //The /2 is to suit the encoder
	if (R1 != R1old) { // checks to see if it different
		(R1dif = (R1 - R1old));// finds out the difference
							   //Serial.println(Encoder1.position());
		if (R1dif == 1) Serial.println("G22");
		if (R1dif == -1)Serial.println("G23");
		R1old = R1; // overwrites the old reading with the new one.
					//Disp->SetTempValChange(true);
	}
	int R2 = (Encoder2.position()); //The /2 is to suit the encoder
	if (R2 != R2old) { // checks to see if it different
		(R2dif = (R2 - R2old));// finds out the difference
		if (R2dif == 1) Serial.println("G20");
		if (R2dif == -1)Serial.println("G21");
		R2old = R2; // overwrites the old reading with the new one.
					//Disp->SetTempValChange(true);
	}

}
//------------------------------------------------------------------------------------------------------------------------------------------
uint8_t I2C_ClearBus() {
#if defined(TWCR) && defined(TWEN)
	TWCR &= ~(_BV(TWEN)); //Disable the Atmel 2-Wire interface so we can control the SDA and SCL pins directly
#endif

	pinMode(SDA, INPUT_PULLUP); // Make SDA (data) and SCL (clock) pins Inputs with pullup.
	pinMode(SCL, INPUT_PULLUP);
	delay(20);

	boolean SCL_LOW = (digitalRead(SCL) == LOW); // Check is SCL is Low.
	if (SCL_LOW) { //If it is held low Arduno cannot become the I2C master.
		return 1; //I2C bus error. Could not clear SCL clock line held low
	}

	boolean SDA_LOW = (digitalRead(SDA) == LOW);  // vi. Check SDA input.
	int clockCount = 20; // > 2x9 clock

	while (SDA_LOW && (clockCount > 0)) { //  vii. If SDA is Low,
		clockCount--;
		// Note: I2C bus is open collector so do NOT drive SCL or SDA high.
		pinMode(SCL, INPUT); // release SCL pullup so that when made output it will be LOW
		pinMode(SCL, OUTPUT); // then clock SCL Low
		delayMicroseconds(10); //  for >5uS
		pinMode(SCL, INPUT); // release SCL LOW
		pinMode(SCL, INPUT_PULLUP); // turn on pullup resistors again
									// do not force high as slave may be holding it low for clock stretching.
		delayMicroseconds(10); //  for >5uS
							   // The >5uS is so that even the slowest I2C devices are handled.
		SCL_LOW = (digitalRead(SCL) == LOW); // Check if SCL is Low.
		int counter = 20;
		while (SCL_LOW && (counter > 0)) {  //  loop waiting for SCL to become High only wait 2sec.
			counter--;
			delay(50);
			SCL_LOW = (digitalRead(SCL) == LOW);
		}
		if (SCL_LOW) { // still low after 2 sec error
			return 2; // I2C bus error. Could not clear. SCL clock line held low by slave clock stretch for >2sec
		}
		SDA_LOW = (digitalRead(SDA) == LOW); //   and check SDA input again and loop
	}
	if (SDA_LOW) { // still low
		return 3; // I2C bus error. Could not clear. SDA data line held low
	}

	// else pull SDA line low for Start or Repeated Start
	pinMode(SDA, INPUT); // remove pullup.
	pinMode(SDA, OUTPUT);  // and then make it LOW i.e. send an I2C Start or Repeated start control.
						   // When there is only one I2C master a Start or Repeat Start has the same function as a Stop and clears the bus.
						   // A Repeat Start is a Start occurring after a Start with no intervening Stop.
	delayMicroseconds(10); // wait >5uS
	pinMode(SDA, INPUT); // remove output low
	pinMode(SDA, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.
	delayMicroseconds(10); // x. wait >5uS
	pinMode(SDA, INPUT); // and reset pins as tri-state inputs which is the default state on reset
	pinMode(SCL, INPUT);
	//Serial.println("I2C Recovered");
	return 0; // all ok
}
//------------------------------------------------------------------------------------------------------------------------------------------
