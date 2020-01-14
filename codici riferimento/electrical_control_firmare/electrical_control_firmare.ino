/****************************************************************************************/
/*																						*/
/*							Electrical Control  | ©Kantooya								*/
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
#include <U8g2lib.h>
#include <AS1115.h>
//#include <Arduino.h>
#include <Quadrature.h> //for rotary encoders
//#ifdef U8X8_HAVE_HW_SPI
//#include <SPI.h>
//#endif
#ifdef U8X8_HAVE_HW_I2C
#include <WSWire.h>
#endif
#include <MemoryFree.h>
#include <avr/pgmspace.h>
//---------------------------------------------------------------

//---------------------------- Pins -----------------------------
//			Name			Arduino Pin				Pin on ATMega
//		-----------			-----------				-------------
#define LED2_CLK				PD5
#define LED2_DS					PD7
#define LED2_LTCH				PD6
#define LED1_CLK				PD3
#define LED1_DS					PD2
#define LED1_LTCH			    PD4
#define	IRQ1					A0
#define	IRQ2					A1
//---------------------------------------------------------------


//------------------------- Settings ----------------------------
#define TCAADDR					0x70
#define SerialTimeout			1000
#define TempDelay				3000
#define EBaro					0
#define ENSet					1
//---------------------------------------------------------------

//----------------------- Prototypes ----------------------------
class TDisplay;
class TEncoderData;
class TSwitchCtrl;
class TChannel;
class TPowerData;
class TInstrumentData;
//---------------------------------------------------------------

//------------------------- Objects -----------------------------
AS1115 disp = AS1115(0x00);
AS1115 *SwitchCtr1 = new AS1115(0x00), *SwitchCtr2 = new AS1115(0x00);
TSwitchCtrl *SwitchCtrs[2];
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

Quadrature Encoder1(8, 9);
Quadrature Encoder2(A2, A3);
//---------------------------------------------------------------

//-------------------------- Enums ------------------------------
//enum EDispID { EBaro = 0, ENSet = 1 };
//---------------------------------------------------------------

//---------------------- Public Variables -----------------------
//unsigned long TimeOutCheck;
//byte counter = 0;
byte tempDigitIndex, CodeIn;
String LED1String = "";
String LED2String = "";

char temp;
TDisplay *Displays[2];
TPowerData *PowerData;
TInstrumentData *InstrumentData;
bool MasterPower = true;
bool UpdatePowerDisp = false;
bool UpdateInstrumentDisp = false;
static const byte TickPositions[] PROGMEM = { 39,41,43,50,60,71,92,	102, 124 };
static const byte GraphOrder[] PROGMEM = { 8,9,10,11,12,13,14,15,0,1 };
//static const byte BaroDigits[]  = { 3,4,5,6 };
//static const byte NSetDigits[]  = { 0, 1, 2 };
bool SpeedSet = false;
bool N1Set = false;
bool Strtr1 = false;
bool Strtr2 = false;
uint8_t Autobreak = 0;
uint8_t LastAddress = 7;
int8_t R1old, R2old, R1dif, R2dif;
//---------------------------------------------------------------
class TDisplay
{
private:
	byte *Index;
	byte DigitCount;
	//byte ID;
	//bool TempValChange;
	bool ShowADP;
	//unsigned long  LastTempChange;

	void SendValues(byte index, char value, bool ShowDP)
	{
		//return;
		tcaselect(3, true);
		AS1115 *CurrentDriver = &disp;// index > 7 ? &DispCtrl2 : &DispCtrl1;
		index %= 8;
		if (CurrentDriver->digitWrite(index, value, ShowDP))
		{
			//	Serial.println("I2C Froze (Displays)");
			I2C_ClearBus();
			Wire.begin();
		}
	}
public:
	String Data;// , TempData;
	TDisplay(byte *_Index, byte _DigitCount, bool _showADP)
	{
		Data.reserve(4);
		//TempData.reserve(4);
		ShowADP = _showADP;
		Index = _Index;
		DigitCount = _DigitCount;
		//ID = _ID;
		UpdateDisplay("     ");
		//TempValChange = false;
		//TempData = "";
	}
	void UpdateDisplay() { UpdateDisplay(Data); }
	void UpdateDisplay(String Val)
	{
		uint8_t DPFound = 0;
		for (byte i = 0; i < DigitCount + DPFound; i++)
		{
			if (Val[i] == '.')
			{
				SendValues(Index[i - 1], Val[i - 1], ShowADP);
				DPFound++;
			}
			else SendValues(Index[i - DPFound], Val[i], false);
		}
	}
	void ClearDisplay() { UpdateDisplay("      "); }
	//	byte GetID() { return ID; }
		//void SetTempValChange(bool val)
		//{
		//	TempValChange = val;
		//	//	LastTempChange = millis();
		//}
	byte GetDigitCount() { return DigitCount; }
};
//--------------------------------------------
class TPowerData
{
public:
	String BatteryVoltage;
	String BatteryCurrent;
	String MainBusVoltage;
	String MainBusCurrent;
	String APUVoltage;
	bool APUGenActive;
	TPowerData()
	{
		BatteryVoltage.reserve(4);
		BatteryCurrent.reserve(4);
		MainBusVoltage.reserve(4);
		MainBusCurrent.reserve(3);
		APUVoltage.reserve(2);
	}
};
//--------------------------------------------
class TInstrumentData
{
public:
	uint8_t Flaps;
	uint8_t FlapsHandle;
	uint8_t FlapsPoisitonCount;
	uint8_t FuelLeft;
	uint8_t FuelCentre;
	uint8_t FuelRight;
	String Autobreak;
	TInstrumentData() { Autobreak.reserve(3); }
	void SetFlaps(String StrFlaps) { Flaps = atoi(StrFlaps.c_str()); }
	void SetFlapsHandle(String strFlapsHandle) { FlapsHandle = atoi(strFlapsHandle.c_str()); }
	void SetFlapsPoisitonCount(String strFlapsPoisitonCount) { FlapsPoisitonCount = atoi(strFlapsPoisitonCount.c_str()); }
	void SetFuelLeft(String strFuel) { FuelLeft = atoi(strFuel.c_str()); }
	void SetFuelCentre(String strFuel) { FuelCentre = atoi(strFuel.c_str()); }
	void SetFuelRight(String strFuel) { FuelRight = atoi(strFuel.c_str()); }
	void SetAutoBreak(char Val)
	{
		switch (Val)
		{
		case '0':Autobreak = "RTO"; break;
		case '1':Autobreak = "OFF"; break;
		case '2':Autobreak = " 1 "; break;
		case '3':Autobreak = " 2 "; break;
		case '4':Autobreak = " 3 "; break;
		case '5':Autobreak = "MAX"; break;
		}
	}
};

class TSwitchCtrl
{
private:
	AS1115 *Chip;
	String Data;
	String OldData;
public:
	byte MuxAddress;
	byte IRQ;
	byte LSB, MSB;
	TSwitchCtrl(uint8_t _IRQ, AS1115 *_Chip, uint8_t _LSB, uint8_t _MSB, uint8_t _MuxAddress)
	{
		Data.reserve(16);
		OldData.reserve(16);
		IRQ = _IRQ;
		Chip = _Chip;
		LSB = _LSB;
		MSB = _MSB;
		MuxAddress = _MuxAddress;
		Data = "";
		tcaselect(MuxAddress, false);
		Chip->begin(0x00, 0x00);
		Chip->begin(0x01, 0x00);
		Chip->begin(0x02, 0x00);
		Chip->begin(0x03, 0x00);
	}
	void ReadSwitchs()
	{
		tcaselect(MuxAddress, false);
		uint8_t ErrorCode;
		//Serial.print("Saving ");
		//Serial.print(Data);
		//Serial.println(" as old");
		OldData = Data;
		Data = Chip->ReadKeysMul(&ErrorCode);//potentially not needed (the Leading Zeros, already handled from library)
		//Serial.print("new val is: ");
		//Serial.println (Data);
		if (ErrorCode >= 4)
		{
			I2C_ClearBus();
			Wire.begin();
		}
		tcaselect(LastAddress, false);
	}
	String GetData() { return Data; }
	String GetOldData() { return OldData; }
	void EmptyBuffer()
	{
		tcaselect(MuxAddress, false);
		uint8_t ErrorCode;

		Chip->ReadKeysMul(&ErrorCode);//potentially not needed (the Leading Zeros, already handled from library)
		if (ErrorCode >= 4)
		{
			I2C_ClearBus();
			Wire.begin();
		}
		tcaselect(LastAddress, false);
	}
};
/*
Frame Buffer Examples: clearBuffer/sendBuffer. Fast, but may not work with all Arduino boards because of RAM consumption
Page Buffer Examples: firstPage/nextPage. Less RAM usage, should work with all Arduino boards.
U8x8 Text Only Example: No RAM usage, direct communication with display controller. No graphics, 8x8 Text only.
*/


void tcaselect(uint8_t i, bool SaveAddress) {
	if (i > 7) return;
	if (SaveAddress == true)		LastAddress = i;
	Wire.beginTransmission(TCAADDR);
	Wire.write(1 << i);
	Wire.endTransmission();
	//delay(10);
}

void u8g2_prepare(void) {
	u8g2.setFont(u8g2_font_6x10_tf);
	u8g2.setFontRefHeightExtendedText();
	u8g2.setDrawColor(1);
	u8g2.setFontPosTop();
	u8g2.setFontDirection(0);
}
void PowerDevice()
{
	if (MasterPower == 0)
	{
		SendLED2Data("0000000000000000");
		SendLED1Data("0000000000000000");
		Displays[EBaro]->UpdateDisplay("    ");
		Displays[ENSet]->UpdateDisplay("   ");
		tcaselect(6, true);
		u8g2.clearDisplay();
		tcaselect(7, true);
		u8g2.clearDisplay();
		
	}
	else
	{
		UpdateAllDisplays();
		UpdateInstrumentDisplay();
		UpdatePowerDisplay();
	}
}
ISR(PCINT1_vect) // handle pin change interrupt for D8 to D13 here
{
	interrupts();
	for (byte SwtchCtrlIndex = 0; SwtchCtrlIndex < 2; SwtchCtrlIndex++)
	{
		if (digitalRead(SwitchCtrs[SwtchCtrlIndex]->IRQ) == LOW)
		{
			SwitchCtrs[SwtchCtrlIndex]->ReadSwitchs();
			byte ChangedIndex = 0;
			char NewValue, InverseValue;
			for (byte i = 0; i < 16; i++)
			{

				if (SwitchCtrs[SwtchCtrlIndex]->GetData()[i] != SwitchCtrs[SwtchCtrlIndex]->GetOldData()[i])
				{
					NewValue = SwitchCtrs[SwtchCtrlIndex]->GetData()[i];
					InverseValue = NewValue == '0' ? '1' : '0';
					ChangedIndex = 32 - i - SwitchCtrs[SwtchCtrlIndex]->LSB;
					break;
				}
			}
			if (!MasterPower && ChangedIndex != 31) return;//if powered off, only listen to power swtich.
			if (NewValue == '1') //push buttons
			{
				switch (ChangedIndex)
				{
				case 1:Serial.println(F("C15"));										break;
				case 2:Serial.println(F("C14"));										break;
				case 8:Serial.println(F("E43")); Serial.println(F("E46"));				break;
				case 14:Serial.println(F("C02"));										break;
				case 15:Serial.println(F("C01"));										break;
				case 22:Serial.println(F("F31"));										break;
				case 23:Serial.println(F("B31"));										break;
				case 25:Serial.println(F("B35"));										break;
				case 27:Serial.println(F("C13"));										break;
				case 28:Serial.println(LED1String[4] == '1' ? F("C21") : F("C20"));		break;
				case 29:Serial.println(F("C04"));										break;
				case 30:Serial.println(F("C16"));										break;
					//case 26:Serial.println("F34");										break; AUTO BARO??
				case 32:Serial.println(F("F34"));										break;
				}
			}
			switch (ChangedIndex) //toggle switches
			{
			case 4:Serial.println(NewValue == '1' ? F("E17") : F("E18"));	break;
			case 5:Serial.println(NewValue == '1' ? F("E32") : F("E31"));	break;
			case 6:Serial.println(NewValue == '1' ? F("E30") : F("C11"));	break;
			case 7:Serial.println(NewValue == '0' ? F("E30") : F("C11"));	break;
			case 12:Serial.println(NewValue == '1' ? F("E23") : F("E24"));	break;
			case 13:Serial.println(NewValue == '1' ? F("E20") : F("E21"));	break;
				//case 13:Serial.println(F("E21")); break;
			case 17:Autobreak = 4; SetAB();									break;
			case 18:Autobreak = 3; SetAB();									break;
			case 19:Autobreak = 2; SetAB();									break;
			case 20:Autobreak = 1; SetAB();									break;
			case 21:Autobreak = 0; SetAB();									break;
			case 24:Autobreak = 5; SetAB();									break;
			case 31:MasterPower = NewValue == '1';
				PowerDevice();
				break;
			}
		}
	}
}
void setup(void) {

	LED1String.reserve(16);
	LED2String.reserve(16);
	//BargraphStirng.reserve(10);

	Serial.begin(115200);
	pinMode(IRQ1, INPUT_PULLUP);
	pinMode(IRQ2, INPUT_PULLUP);
	pinMode(LED1_CLK, OUTPUT);
	pinMode(LED1_DS, OUTPUT);
	pinMode(LED1_LTCH, OUTPUT);
	pinMode(LED2_CLK, OUTPUT);
	pinMode(LED2_DS, OUTPUT);
	pinMode(LED2_LTCH, OUTPUT);


	SwitchCtrs[0] = new TSwitchCtrl(IRQ1, SwitchCtr1, 0, 15, 5);
	SwitchCtrs[1] = new TSwitchCtrl(IRQ2, SwitchCtr2, 16, 31, 4);

	tcaselect(3, false);
	disp.setFont(FONT_CODEB);
	disp.begin(0x03, 0x00);
	disp.setIntensity(0x06);


	Displays[EBaro] = new TDisplay(new byte[5]{ 3,4,5,6 }, 4, true);
	Displays[ENSet] = new TDisplay(new byte[5]{ 0, 1, 2 }, 3, false);

	PowerData = new TPowerData();
	InstrumentData = new TInstrumentData;

	SwitchCtrs[0]->ReadSwitchs();
	SwitchCtrs[1]->ReadSwitchs();

	MasterPower = SwitchCtrs[0]->GetData()[1] == '1';
	PowerDevice();

	delay(50);
	pciSetup(IRQ1);
	pciSetup(IRQ2);


	tcaselect(6, false);
	u8g2.begin();
	tcaselect(7, false);
	u8g2.begin();

	UpdatePowerDisp = false;
	UpdateInstrumentDisp = false;

	LED1String = "000000000000000000000000";
	LED2String = "000000000000000000000000";
	//u8g2.setFlipMode(0);
}
void loop(void) {
	ENCODER();  //Check the Rotary Encoders	
	if (!Serial.available()) //Check if anything there
	{
		if (UpdatePowerDisp == true && MasterPower == true) UpdatePowerDisplay();
		if (UpdateInstrumentDisp == true && MasterPower == true) UpdateInstrumentDisplay();
		return;
	}

	while (Serial.available())
	{
		switch (getChar())  //Get a serial read if there is.
		{
		case '=':EQUALS(); break;
		case '<':LESSTHAN();  break;
		case '?':QUESTION();  break;
		case '#':HASHTAG();  break;
		case '$':DOLLAR();  break;
		default:break; // ignore floating values //while (Serial.available())  Serial.read(); break; // empty buffer	 
		}
	}
	while (Serial.available())  Serial.read(); // empty buffer	 
	UpdateAllDisplays();
}
void pciSetup(byte pin)
{
	*digitalPinToPCMSK(pin) |= bit(digitalPinToPCMSKbit(pin));  // enable pin
	PCIFR |= bit(digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
	PCICR |= bit(digitalPinToPCICRbit(pin)); // enable interrupt for the group
}
void SetAB()
{
	for (byte i = 0; i < 6; i++)Serial.println(F("Y031"));
	for (byte i = 0; i < Autobreak; i++)Serial.println(F("Y021"));
}
//------------------------------------------------------------------------------------------------------------------------------------------
void UpdateAllDisplays()
{
	if (MasterPower) {
		//for (byte i = 0; i < 2; i++) {
		Displays[EBaro]->UpdateDisplay();
		Displays[ENSet]->UpdateDisplay();
		//}
		SendLED2Data(LED2String);
		LED1String[7] = !SpeedSet && N1Set ? '1' : '0';
		LED1String[6] = MasterPower ? '1' : '0';
		LED1String[15] = Displays[EBaro]->Data == "29.92" ? '1' : '0';
		SendLED1Data(LED1String);

	}
}
//------------------------------------------------------------------------------------------------------------------------------------------
char getChar()// Get a character from the serial buffer
{
	//TimeOutCheck = millis();
	while (Serial.available() == 0);// if (millis() - TimeOutCheck > SerialTimeout)return '0';//time out command - prevent freezing
	return((char)Serial.read());
}
//------------------------------------------------------------------------------------------------------------------------------------------
void EQUALS() {

	CodeIn = getChar();
	switch (CodeIn) {
	case 't':	LED1String[12] = getChar();		break;					//auto throttle
	case 's':	SpeedSet = getChar() == '1' ? true : false;		break;//airspeed
	case 'u':	N1Set = getChar() == '1' ? true : false;		break;//n1set
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------
void QUESTION() {
	CodeIn = getChar();
	String GenStats, GearStats;
	switch (CodeIn) {
	case 'k': Displays[EBaro]->Data = GetBufferValue(5);				break;	//Kohlsman setting Hg
	case 'I': PowerData->BatteryVoltage = GetBufferValue(4); UpdatePowerDisp = true;				break;	//Battery Voltage
	case 'J': PowerData->BatteryCurrent = GetBufferValue(4); UpdatePowerDisp = true;				break;	//Battery Current
	case 'K': PowerData->MainBusVoltage = GetBufferValue(4); UpdatePowerDisp = true;				break;	//Main Bus Voltage
	case 's':
		GenStats = GetBufferValue(2);
		LED1String[10] = GenStats[0] == '1' ? '0' : '1';
		LED1String[11] = GenStats[1] == '1' ? '0' : '1';
		break;
	case 'Y'://gear
		GearStats = GetBufferValue(3);
		LED2String[2] = GearStats[2] == '1' ? '1' : '0';//right red
		LED2String[4] = GearStats[1] == '1' ? '1' : '0';//left red
		LED2String[6] = GearStats[0] == '1' ? '1' : '0';//nose red
		LED2String[3] = GearStats[2] == '2' ? '1' : '0';//right grn
		LED2String[5] = GearStats[1] == '2' ? '1' : '0';//left grn
		LED2String[7] = GearStats[0] == '2' ? '1' : '0';//nose grn
		break;
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------
void LESSTHAN()
{
	CodeIn = getChar();
	switch (CodeIn)
	{
	case 'G':	InstrumentData->SetFlaps(GetBufferValue(3)); UpdateInstrumentDisp = true;				break;	 //Flaps
	case 'X':	InstrumentData->SetFuelLeft(GetBufferValue(3)); UpdateInstrumentDisp = true;			break;	 //Flaps
	case 'Y':	InstrumentData->SetFuelCentre(GetBufferValue(3)); UpdateInstrumentDisp = true;			break;	 //Flaps
	case 'Z':	InstrumentData->SetFuelRight(GetBufferValue(3)); UpdateInstrumentDisp = true;			break;	 //Flaps
	case 'I':	LED1String[1] = getChar();		break;	//Plane on Ground
	case 'q':	LED1String[5] = getChar();		break;	//Parking Break
	case 'i':	LED1String[4] = getChar();		break;	//Spoiler Arked
	case 'r':	LED1String[2] = getChar();		break;	//Eng 1 Fuel Vales
	case 's':	LED1String[0] = getChar();		break;	//Eng 2 Fuel Vales
	case 'k':	Strtr1 = (getChar() == '1' ? true : false); LED1String[3] = (Strtr1 || Strtr2) ? '1' : '0';	break;	//eng starter
	case 'l':	Strtr2 = (getChar() == '1' ? true : false); LED1String[3] = (Strtr1 || Strtr2) ? '1' : '0';	break;	//eng starter

	}
}
//------------------------------------------------------------------------------------------------------------------------------------------
void HASHTAG() {
	CodeIn = getChar();
	switch (CodeIn)
	{
	case 'H':	Displays[ENSet]->Data = GetBufferValue(4);													break;
	case 'I':	PowerData->MainBusCurrent = GetBufferValue(3); UpdatePowerDisp = true;						break;	//Main Bus Current
	case 'L':	InstrumentData->SetFlapsHandle(GetBufferValue(2)); UpdateInstrumentDisp = true;				break;	//Flaps Index
	case 'K':	InstrumentData->SetFlapsPoisitonCount(GetBufferValue(2)); UpdateInstrumentDisp = true;		break;	//Flaps Index
	case 'J':	InstrumentData->SetAutoBreak(getChar()); UpdateInstrumentDisp = true;		break;	//Flaps Index
	case 'N':	LED1String[14] = getChar(); break;	//HAS G/S
	case 'M':	LED1String[13] = getChar(); break;	//HAS LOC
	}

}
//------------------------------------------------------------------------------------------------------------------------------------------

void ENCODER() {

	int8_t R1 = (Encoder1.position() / 2); //The /2 is to suit the encoder
	if (R1 != R1old) { // checks to see if it different
		(R1dif = (R1 - R1old));// finds out the difference
		//Serial.println(Encoder1.position());
		if (R1dif == 1) Serial.println(F("B43"));
		if (R1dif == -1)Serial.println(F("B44"));
		R1old = R1; // overwrites the old reading with the new one.
		//Disp->SetTempValChange(true);
	}

	int8_t R2 = (Encoder2.position() / 2); //The /2 is to suit the encoder
	if (R2 != R2old) { // checks to see if it different
		(R2dif = (R2 - R2old));// finds out the difference
		if (R2dif == 1) Serial.println(F("C25"));
		if (R2dif == -1)Serial.println(F("C26"));
		R2old = R2; // overwrites the old reading with the new one.
					//Disp->SetTempValChange(true);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------
void DOLLAR() {
	CodeIn = getChar();
	switch (CodeIn)
	{
	case 'g':	GETBarLED(GetBufferValue(3));								break;
	case 'i':	PowerData->APUVoltage = GetBufferValue(2);	UpdatePowerDisp = true;				break;	//APU Voltage
	case 'k':	PowerData->APUGenActive = getChar() == '1' ? true : false; 		break;	//APU Gen Active (considers APU RPM)
	case 'j':	LED1String[9] = getChar();		break; //APU Gen
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------
String GetBufferValue(int byteCount)
{
	String buff = "";
	for (byte i = 0; i < byteCount; i++)
	{
		temp = getChar();
		if ((temp <= '9' && temp >= '0') || temp == '.' || temp == '-' || temp == '+')buff += temp;
		else buff += '0';
	}
	return buff;
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
	byte clockCount = 20; // > 2x9 clock

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
		byte counter = 20;
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


void PulseLEN(uint8_t pin, bool StartLow)
{
	if (StartLow)
	{
		digitalWrite(pin, LOW);
		digitalWrite(pin, HIGH); //fix for double send
		digitalWrite(pin, LOW);
	}
	else
	{
		digitalWrite(pin, HIGH);
		digitalWrite(pin, LOW);
		digitalWrite(pin, HIGH);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------
void PulseData(uint8_t DSpin, uint8_t CLKPin, char Data)
{
	digitalWrite(DSpin, (Data == '1' ? HIGH : LOW));
	digitalWrite(CLKPin, LOW);
	digitalWrite(CLKPin, HIGH);
}
//------------------------------------------------------------------------------------------------------------------------------------------
void SendLED1Data(String data)
{
	PulseLEN(LED1_LTCH, false);
	for (byte i = 0; i < 16; i++)PulseData(LED1_DS, LED1_CLK, data[i]);
	PulseLEN(LED1_LTCH, true);

	/*  PulseLEN(LED2_LTCH, false);
	for (int i = 31; i >= 16; i--)PulseData(LED2_DS, LED2_CLK, data[i]);
	PulseLEN(LED2_LTCH, true);*/
}
//------------------------------------------------------------------------------------------------------------------------------------------
void SendLED2Data(String data)
{
	PulseLEN(LED2_LTCH, false);
	for (byte i = 0; i < 16; i++) {
		PulseData(LED2_DS, LED2_CLK, data[i]); //delay(10);
	}
	PulseLEN(LED2_LTCH, true);

	/*  PulseLEN(LED2_LTCH, false);
	for (int i = 31; i >= 16; i--)PulseData(LED2_DS, LED2_CLK, data[i]);
	PulseLEN(LED2_LTCH, true);*/
}
//------------------------------------------------------------------------------------------------------------------------------------------
void GETBarLED(String APURPMVal)
{
	String BargraphStirng = "";
	if (APURPMVal[0] == '1')
	{
		BargraphStirng = "1111111111";
	}
	else
	{
		byte val = APURPMVal[1] - '0';
		BargraphStirng = "";
		for (byte i = 0; i < val; i++)
			BargraphStirng += '1';
		for (byte i = 0; i < 10 - val; i++)
			BargraphStirng += '0';
	}

	for (byte i = 0; i < 10; i++)
		LED2String[pgm_read_byte(&(GraphOrder[i]))] = BargraphStirng[i];
}
//------------------------------------------------------------------------------------------------------------------------------------------
void UpdatePowerDisplay()
{
	//return;
	tcaselect(6, true);
	u8g2.firstPage();
	do
	{

		u8g2.drawHLine(0, 17, 128);
		u8g2.drawHLine(0, 42, 128);
		u8g2.drawVLine(64, 0, 17);
		u8g2.drawVLine(43, 42, 22);
		u8g2.drawVLine(86, 42, 22);



		u8g2_prepare();
		u8g2.setFont(u8g2_font_freedoomr10_tu);
		u8g2.drawStr(10, 0, (PowerData->BatteryVoltage + "V").c_str());
		u8g2.drawStr(80, 0, (PowerData->BatteryCurrent + "A").c_str());
		u8g2.drawStr(3, 50, (PowerData->MainBusVoltage + "V").c_str());
		u8g2.drawStr(51, 50, (PowerData->MainBusCurrent + "A").c_str());
		u8g2.drawStr(97, 50, (PowerData->APUVoltage + "V").c_str());

		if (PowerData->BatteryCurrent[0] == '-')
		{
			u8g2.setFont(u8g2_font_crox3hb_tf);
			u8g2.drawStr(2, 23, "LOW BATTERY");
		}

		u8g2.setFont(u8g2_font_6x10_tf);

	} while (u8g2.nextPage());


	UpdatePowerDisp = false;

}
//------------------------------------------------------------------------------------------------------------------------------------------
void UpdateInstrumentDisplay()
{
	//return;
	uint8_t i;
	uint8_t CentrePos;
	uint8_t actualpos;
	tcaselect(7, true);
	u8g2.firstPage();
	u8g2_prepare();
	do
	{
		u8g2.drawVLine(34, 0, 44);
		u8g2.drawHLine(0, 44, 128);
		u8g2.drawVLine(42, 45, 21);
		u8g2.drawVLine(86, 45, 21);


		// Fuel 
		u8g2.setFont(u8g2_font_4x6_tf);
		u8g2.drawStr(13, 47, "LEFT");
		u8g2.drawStr(55, 47, "CENTRE");
		u8g2.drawStr(100, 47, "RIGHT");
		u8g2.drawStr(40, 2, "FLAPS:");
		u8g2.setFont(u8g2_font_6x10_tf);

		u8g2.drawStr(13, 56, (String(InstrumentData->FuelLeft) + "%").c_str());
		u8g2.drawStr(55, 56, (String(InstrumentData->FuelCentre) + "%").c_str());
		u8g2.drawStr(100, 56, (String(InstrumentData->FuelRight) + "%").c_str());

		//	//Autobreak
		u8g2.drawStr(8, 15, InstrumentData->Autobreak.c_str());

		//	//Flaps Demo
		u8g2.setFont(u8g2_font_micro_mn);


		u8g2.drawStr(42, 35, "0");//save ram
		u8g2.drawStr(40, 35, "1");
		//u8g2.drawStr(44, 35, "2");
		u8g2.drawStr(48, 35, "5");
		u8g2.drawStr(56, 35, "10");
		u8g2.drawStr(67, 35, "15");
		u8g2.drawStr(88, 35, "25");
		u8g2.drawStr(98, 35, "30");
		u8g2.drawStr(120, 35, "40");//save ram ends

			//u8g2.setFont(u8g2_font_4x6_tf);

		//u8g2.setFont(u8g2_font_6x10_tf);
		u8g2.drawHLine(44, 30, 81);

		for (i = 0; i < 9; i++)u8g2.drawVLine(pgm_read_byte(&(TickPositions[i])), 29, 3); //SAVE RAM


		actualpos = map(InstrumentData->Flaps, 0, 100, 0, 85);
		//u8g2.drawStr(50, 5, String(39 + actualpos).c_str());



		u8g2.drawVLine(39, 20, 7);
		u8g2.drawBox(39, 20, actualpos, 7);
		if (InstrumentData->FlapsPoisitonCount == 8)
		{
			CentrePos = pgm_read_byte(&(TickPositions[InstrumentData->FlapsHandle]));
			u8g2.drawVLine(CentrePos, 10, 4);
			u8g2.drawTriangle(CentrePos - 2, 14, CentrePos, 18, CentrePos + 3, 14); //tales waaayyy tooo much RAM
		}
		//	//u8g2.drawVLine(i , 20, 8);

	} while (u8g2.nextPage());
	UpdateInstrumentDisp = false;
}
//------------------------------------------------------------------------------------------------------------------------------------------