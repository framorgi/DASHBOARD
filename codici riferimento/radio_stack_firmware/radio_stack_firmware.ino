/****************************************************************************************/
/*																						*/
/*							Radiostack 125-PB | ©Kantooya 								*/
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
#include <AS1115.h>
#include "WSWire.h"
#include "math.h"
#include "Quadrature.h" //for rotary encoders
#include <MemoryFree.h>
#include <avr/pgmspace.h>
//---------------------------------------------------------------

//---------------------------- Pins -----------------------------
//			Name			Arduino Pin				Pin on ATMega
//		-----------			-----------				-------------
#define LED1_DS					2			//			PD2
#define LED1_CLK				3			//			PD3
#define LED1_LTCH				4			//			PD4
#define LED2_DS					5			//			PD5
#define LED2_CLK				6			//			PD6
#define LED2_LTCH				7			//			PD7
#define FrqEnc_A				8			//
#define FrqEnc_B				9			//
#define IRQ1					11			//			 
#define IRQ2					12			//			 
#define IRQ3					13			//			 
#define SDA						A4			//
#define SCL						A5			//
//---------------------------------------------------------------

//------------------------- Settings ----------------------------
#define SerialTimeout			1000
#define TempDelay				3000
#define TWI_FREQ 				50L
//---------------------------------------------------------------

//-------------------------- Enums ------------------------------
enum EDispID{ EActiveDisp = 0, EStdByDisp = 1, EATCDisp = 2 };
enum EChannel{ COM1 = 0, COM2 = 1, NAV1 = 2, NAV2 = 3, ADF = 4 };
//---------------------------------------------------------------

//----------------------- Prototypes ----------------------------
class SDisplayData;
class TDisplay;
class TEncoderData;
class TSwitchCtrl;
class TChannel;
//---------------------------------------------------------------

//------------------------- Objects -----------------------------
AS1115 DispCtrl1 = AS1115(0x01), DispCtrl2 = AS1115(0x02);
AS1115 *SwitchCtr1 = new AS1115(0x03), *SwitchCtr2 = &DispCtrl2, *SwitchCtr3 = new AS1115(0x00);
TEncoderData *FreqCtrl;
TChannel *Channel[5];
//---------------------------------------------------------------

//---------------------- Public Variables -----------------------
String LEDString = "";
TDisplay *Displays[3];
TSwitchCtrl *SwitchCtrs[3];
byte tempDigitIndex, CodeIn;
unsigned long TimeOutCheck;
bool MHz = true, MasterPower = false;
uint8_t SelectedMode;
bool ATCMode;
char temp;
short mul;
//---------------------------------------------------------------

//-------------------------- Classes ----------------------------
class TChannel
{
public:
	char MHzIncCommand[4];
	char MHzDecCommand[4];
	char KHzIncCommand[4];
	char KHzDecCommand[4];
	char SwapCommand[4];
	char CallCommand[4];
	String Active;
	String  Standby;
	uint8_t Status;
	uint8_t LEDIndex;

	TChannel(char MHzUp[], char MHzDown[], char KHzUp[], char KHzDown[], char Swap[], char call[], uint8_t LED, uint8_t StatLED)
	{
		strcpy(MHzIncCommand, strcat(MHzUp, "\0"));
		strcpy(MHzDecCommand, strcat(MHzDown, "\0"));
		strcpy(KHzIncCommand, strcat(KHzUp, "\0"));
		strcpy(KHzDecCommand, strcat(KHzDown, "\0"));
		strcpy(SwapCommand, strcat(Swap, "\0"));
		strcpy(CallCommand, strcat(call, "\0"));

		Status = StatLED;
		LEDIndex = LED;
	}
};
//---------------------------------------------------------------
class TDisplay
{
private:
	byte *Index;
	byte DigitCount;
	EDispID ID;
	bool TempValChange;
	unsigned long  LastTempChange;

	void SendValues(byte index, char value, bool ShowDP)
	{
		AS1115 *CurrentDriver = index > 7 ? &DispCtrl2 : &DispCtrl1;
		index %= 8;
		if (CurrentDriver->digitWrite(index, value, ShowDP))
		{
			//	Serial.println("I2C Froze (Displays)");
			I2C_ClearBus();
			Wire.begin();
		}
	}
public:
	String Data, TempData;
	TDisplay(byte *_Index, EDispID _ID, byte _DigitCount)
	{
		Index = _Index;
		DigitCount = _DigitCount;
		ID = _ID;
		UpdateDisplay("     ");
		TempValChange = false;
		TempData = "";
	}
	void CheckTempDisplay()
	{
		if (TempValChange && millis() - LastTempChange > TempDelay)
		{
			TempValChange = false;
			UpdateDisplay();
			tempDigitIndex = 0;
		}
	}
	void UpdateDisplay(){ UpdateDisplay(TempValChange ? TempData : Data); }
	void UpdateDisplay(String Val)
	{
		uint8_t DPFound = 0;
		for (byte i = 0; i < DigitCount + DPFound; i++)
		{
			if (Val[i] == '.')
			{
				SendValues(Index[i - 1], Val[i - 1], true);
				DPFound++;
			}
			else SendValues(Index[i - DPFound], Val[i], false);
		}
	}
	void ClearDisplay(){ UpdateDisplay("      "); }
	EDispID GetID(){ return ID; }
	void SetTempValChange(bool val)
	{
		TempValChange = val;
		LastTempChange = millis();
	}
};
//--------------------------------------------
class TEncoderData
{
private:
	int R;// a variable
	int Rold;// the old reading
	int Rdif;// the difference since last loop
	Quadrature *Encoder;
	TDisplay *Disp;

public:
	TEncoderData(uint8_t _PinA, uint8_t _PinB, TDisplay *_Disp)
	{
		pinMode(_PinA, INPUT);
		pinMode(_PinB, INPUT);
		Encoder = new Quadrature(_PinA, _PinB);
		Disp = _Disp;
	}
	void ReadEncoder(String IncCommand, String DecCommand)
	{
		R = (Encoder->position() / 2); //The /2 is to suit the encoder
		if (R != Rold) { // checks to see if it different
			(Rdif = (R - Rold));// finds out the difference
			if (Rdif == 1) Serial.println(IncCommand);
			if (Rdif == -1)Serial.println(DecCommand);
			Rold = R; // overwrites the old reading with the new one.
		}
	}
	void ReadEncoder()
	{
		R = (Encoder->position() / 2); //The /2 is to suit the encoder
		if (R != Rold) { // checks to see if it different
			(Rdif = (R - Rold));// finds out the difference
			if (Rdif == 1)
			{
				switch (mul)
				{
				case 0:Serial.println("A28"); break;
				case 1:Serial.println("A27"); break;
				case 2:Serial.println("A26"); break;
				case 3:Serial.println("A25"); break;
				}
			}
			if (Rdif == -1)
			{
				switch (mul)
				{
				case 0:Serial.println("A32"); break;
				case 1:Serial.println("A31"); break;
				case 2:Serial.println("A30"); break;
				case 3:Serial.println("A29"); break;
				}
			}
			Rold = R; // overwrites the old reading with the new one.
		}
	}
};
//--------------------------------------------
class TSwitchCtrl
{
private:
	AS1115 *Chip;
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
		Chip->begin();
	}
	void ReadSwitchs()
	{
		uint8_t ErrorCode;
		OldData = Data;
		Data = GetLeadingZeros(Chip->ReadKeysMul(&ErrorCode), 16);//potentially not needed (the Leading Zeros, already handled from library)
		if (ErrorCode >= 4)
		{
			I2C_ClearBus();
			Wire.begin();
		}
	}
	String GetData(){ return Data; }
	String GetOldData(){ return OldData; }
};
//--------------------------------------------
void pciSetup(byte pin)
{
	*digitalPinToPCMSK(pin) |= bit(digitalPinToPCMSKbit(pin));  // enable pin
	PCIFR |= bit(digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
	PCICR |= bit(digitalPinToPCICRbit(pin)); // enable interrupt for the group
}
//----------------------- Interrupt ----------------------------
ISR(PCINT0_vect) // handle pin change interrupt for D8 to D13 here
{
	interrupts();
	for (byte SwtchCtrlIndex = 0; SwtchCtrlIndex < 3; SwtchCtrlIndex++)
		if (digitalRead(SwitchCtrs[SwtchCtrlIndex]->IRQ) == LOW)
		{
			SwitchCtrs[SwtchCtrlIndex]->ReadSwitchs();
			byte ChangedIndex = 0;
			char NewValue, InverseValue;
			for (byte i = 0; i < 16; i++)
				if (SwitchCtrs[SwtchCtrlIndex]->GetData()[i] != SwitchCtrs[SwtchCtrlIndex]->GetOldData()[i])
				{
					NewValue = SwitchCtrs[SwtchCtrlIndex]->GetData()[i];
					InverseValue = NewValue == '0' ? '1' : '0';
					//Serial.println(String(48 - i - SwitchCtrs[SwtchCtrlIndex]->LSB) + " to " + NewValue);
					ChangedIndex = 48 - i - SwitchCtrs[SwtchCtrlIndex]->LSB;
					if (ChangedIndex == 33 || ChangedIndex == 34)break;//priortize Pitot heat, due to HW design error
				}
			if (!MasterPower && ChangedIndex != 20) return;//if powered off, only listen to power swtich.
			if (NewValue == '1') //push buttons
			{
				switch (ChangedIndex)
				{
				case 33://Pitot Heat
				case 34:
					if (LEDString[28] == '0')Serial.println("C05");
					else Serial.println("C06");
					break;
				case 8:ChangeMode(COM1);			break;														//COM1 Mode
				case 16:ChangeMode(COM2);			break;														//COM2 Mode
				case 18:ChangeMode(NAV1);			break;														//NAV1 Mode
				case 23:ChangeMode(NAV2);			break;														//NAV2 Mode
				case 22:ChangeMode(ADF); mul = 0;	break;														//ADF Mode
				case 19:																						//Range Change
					if (SelectedMode == ADF)
					{
						mul += 1;
						mul %= 4;
					}
					else
					{
						MHz = !MHz;
						LEDString[9] = LEDString[9] == '1' ? '0' : '1';
						LEDString[8] = LEDString[9] == '1' ? '0' : '1';

					}
					SendLEDData(LEDString);
					break;
				case 40:Serial.println(Channel[SelectedMode]->CallCommand); break;						//Call
				case 1:																					//ATC Mode
					ATCMode = !ATCMode;
					if (ATCMode)Displays[EATCDisp]->UpdateDisplay("----");
					else Displays[EATCDisp]->UpdateDisplay();
					break;
				case 7: KeypadPress("Y131", '1'); break;													//Keypad 1
				case 15:KeypadPress("Y141", '2'); break;													//Keypad 2
				case 6: KeypadPress("Y151", '3'); break;													//Keypad 3
				case 14:KeypadPress("Y181", '4'); break;													//Keypad 4
				case 5: KeypadPress("Y191", '5'); break;													//Keypad 5
				case 13:KeypadPress("Y201", '6'); break;													//Keypad 6
				case 4: KeypadPress("Y211", '7'); break;													//Keypad 7
				case 12:KeypadPress("Y241", '0'); break;													//Keypad 0
				case 3:UpdateTempDigits('8'); break;	//potenial problem									//Keypad CLR
				case 11:																				//Keypad [8]
					if (ATCMode)Serial.println("Y221");
					else
					{
						char c[5];
						sprintf(c, "%04d", freeMemory());
						Displays[EATCDisp]->TempData = String(c);
						Displays[EATCDisp]->SetTempValChange(true);
						Displays[EATCDisp]->UpdateDisplay();
					};
					break;
				case 2:	if (ATCMode)Serial.println("Y231");												//Keypad [9]	
						else Serial.println("Y131");
						break;
				}
			}
			switch (ChangedIndex) //toggle switches
			{
			case 20:																					//Master Power

				if (NewValue == '1')PowerUp();
				else Shutdown();

				break;
			case 36:Serial.print("C43"); Serial.println(InverseValue); break;
			case 44:Serial.print("C44"); Serial.println(InverseValue); break;
			case 35:
				Serial.print("C49"); Serial.println(InverseValue); 
				Serial.print("C48"); Serial.println(InverseValue);
				break;
			case 43:Serial.print("C45"); Serial.println(InverseValue); break;
			case 37:
				Serial.print("C47"); Serial.println(InverseValue); 
				Serial.print("C41");  Serial.println(InverseValue);
				break;
			case 45:Serial.print("C42"); Serial.println(InverseValue); break;
			case 38:Serial.print("C46"); Serial.println(InverseValue); break;
			case 46:
				Serial.print("C50");  Serial.println(InverseValue);
				
				break;
			case 39:Serial.println("Y171"); break;
			case 47:Serial.println("Y161"); break;
			case 48:Serial.println("F12"); break;
			case 21:Serial.println(Channel[SelectedMode]->SwapCommand);
				MHz = true;
				LEDString[9] = '1';
				LEDString[8] = '0';
				SendLEDData(LEDString);
				break;
			}
			break;//potenial problem
		}
}

//******************************************************************************************************************************************
void setup() {
	Serial.begin(115200);
	for (int i = 0; i < 32; i++)LEDString += "0";
	ATCMode = false;
	//PinModes
	pinMode(IRQ1, INPUT);
	pinMode(IRQ2, INPUT);
	pinMode(IRQ3, INPUT);
	pinMode(LED1_DS, OUTPUT);
	pinMode(LED1_CLK, OUTPUT);
	pinMode(LED1_LTCH, OUTPUT);
	pinMode(LED2_DS, OUTPUT);
	pinMode(LED2_CLK, OUTPUT);
	pinMode(LED2_LTCH, OUTPUT);
	//Display Controlers Settings
	DispCtrl1.setFont(FONT_CODEB);
	DispCtrl2.setFont(FONT_CODEB);

	DispCtrl1.begin();

	//Switch Controlers
	SwitchCtrs[0] = new TSwitchCtrl(IRQ1, SwitchCtr1, 32, 47);
	SwitchCtrs[1] = new TSwitchCtrl(IRQ2, SwitchCtr2, 16, 31);
	SwitchCtrs[2] = new TSwitchCtrl(IRQ3, SwitchCtr3, 0, 15);

	DispCtrl1.setIntensity(0x06);
	DispCtrl2.setIntensity(0x06);
	//Display definitions
	Displays[EActiveDisp] = new TDisplay(new byte[5]{0, 1, 2, 3, 4}, EActiveDisp, 5);
	Displays[EStdByDisp] = new TDisplay(new byte[5]{5, 6, 7, 8, 9}, EStdByDisp, 5);
	Displays[EATCDisp] = new TDisplay(new byte[4]{13, 12, 11, 10}, EATCDisp, 4);
	//Rotary Encoder Definition
	FreqCtrl = new TEncoderData(FrqEnc_A, FrqEnc_B, Displays[EStdByDisp]);
	//Modes / Channels
	Channel[0] = new TChannel("A02", "A01", "A04", "A03", "A06", "A45", 7, 3);
	Channel[1] = new TChannel("A08", "A07", "A10", "A09", "A12", "A46", 6, 2);
	Channel[2] = new TChannel("A14", "A13", "A16", "A15", "A18", "A48", 5, 1);
	Channel[3] = new TChannel("A20", "A19", "A22", "A21", "A24", "A49", 4, 0);
	Channel[4] = new TChannel("A26", "A30", "A27", "A31", "", "A52", 12, 13);

	delay(50);

	for (byte i = 0; i < 3; i++)SwitchCtrs[i]->ReadSwitchs();//initialize switches - TODO: apply changes to sim accordingly
	SendLEDData(LEDString);//turn off all LEDS
	//Setup interrupt pins
	pciSetup(IRQ1);
	pciSetup(IRQ2);
	pciSetup(IRQ3);

	ChangeMode(SelectedMode);
	if (SwitchCtrs[1]->GetData()[12] == '0')Shutdown();
	else PowerUp();


}
//------------------------------------------------------------------------------------------------------------------------------------------
void loop() {
	ENCODER();  //Check the Rotary Encoders
	Displays[EATCDisp]->CheckTempDisplay();//Check if ATC display showing tmep data.
	if (!Serial.available())return;//Check if anything there
	switch (getChar())  //Get a serial read if there is.
	{
	case '=':EQUALS(); break;
	case '<':LESSTHAN(); break;
	default:while (Serial.available())Serial.read(); break;
	}
	UpdateAllDisplays();	//only if data was available.
}
//******************************************************************************************************************************************

//------------------------------------------------------------------------------------------------------------------------------------------
String GetLeadingZeros(String Bin, short digits)
{
	String Padding = "";
	for (int i = 0; i < digits - Bin.length(); i++)Padding += "0";
	return Padding + Bin;
}
//------------------------------------------------------------------------------------------------------------------------------------------
void UpdateAllDisplays()
{
	Displays[EActiveDisp]->Data = Channel[SelectedMode]->Active;
	Displays[EStdByDisp]->Data = Channel[SelectedMode]->Standby;
	if (MasterPower){
		for (int i = 0; i < 3; i++){
			if (i == 2 && ATCMode)continue;//to skip updating SQWAK display if in ATC Mode
			Displays[i]->UpdateDisplay();
		}
		SendLEDData(LEDString);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------
void ChangeMode(uint8_t  Mode)
{
	SelectedMode = Mode;
	for (uint8_t i = 0; i < 5; i++)LEDString[Channel[i]->LEDIndex] = i == Mode ? '1' : '0';
	LEDString[11] = LEDString[Channel[SelectedMode]->Status];
	UpdateAllDisplays();
}
//------------------------------------------------------------------------------------------------------------------------------------------
void UpdateTempDigits(char val)
{
	if (val == '8')Displays[EATCDisp]->TempData = "1200";
	else
	{
		if (tempDigitIndex == 0)Displays[EATCDisp]->TempData = "    ";
		Displays[EATCDisp]->TempData[tempDigitIndex] = val;
		tempDigitIndex++;
		Displays[EATCDisp]->SetTempValChange(true);
		Displays[EATCDisp]->UpdateDisplay();
	}
	if (tempDigitIndex > 3 || val == '8')
	{
		Serial.println("A42" + Displays[EATCDisp]->TempData);
		Displays[EATCDisp]->SetTempValChange(false);
		if (tempDigitIndex > 0 && tempDigitIndex < 4)
		{
			Displays[EATCDisp]->Data = "1200";
			Displays[EATCDisp]->UpdateDisplay();
		}
		tempDigitIndex = 0;
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------
void KeypadPress(String ATCCommand, char KeypadValue)
{
	if (ATCMode)Serial.println(ATCCommand);
	else UpdateTempDigits(KeypadValue);
}
//------------------------------------------------------------------------------------------------------------------------------------------
void Shutdown()
{
	LEDString[10] = '0';
	SendLEDData("0000000000000000");
	for (byte i = 0; i < 3; i++)Displays[i]->ClearDisplay();
	MasterPower = false;
	Serial.println("A430");
}
void PowerUp()
{
	LEDString[10] = '1';
	LEDString[9] = MHz ? '1' : '0';
	LEDString[8] = LEDString[9] == '1' ? '0' : '1';
	SendLEDData(LEDString);
	for (byte i = 0; i < 3; i++)Displays[i]->UpdateDisplay();
	MasterPower = true;
	Serial.println("A431");
}
void TestLEDs(int D)
{
	String temp = "";
	for (int j = 0; j < 32; j++)temp += (random(0, 2) == 1 ? "1" : "0");
	SendLEDData(temp);
	delay(D);
}
//------------------------------------------------------------------------------------------------------------------------------------------
int ReadSwitches()
{
	uint8_t ErrorCode;
	int val = SwitchCtr1->ReadKeys(&ErrorCode);
	if (ErrorCode >= 4)
	{
		I2C_ClearBus();
		Wire.begin();
	}
	if (val > 0 && val < 255)return val;
	return 0;
}
//------------------------------------------------------------------------------------------------------------------------------------------
char getChar()// Get a character from the serial buffer
{
	TimeOutCheck = millis();
	while (Serial.available() == 0)if (millis() - TimeOutCheck > SerialTimeout)return '0';//time out command - prevent freezing
	return((char)Serial.read());
}
//------------------------------------------------------------------------------------------------------------------------------------------
void EQUALS(){
	CodeIn = getChar();
	switch (CodeIn) {
	case 'M':LEDString[3] = getChar();												break;	//COM1 Active
	case 'N':LEDString[2] = getChar();												break;	//COM2 Active
	case 'P':LEDString[1] = getChar();												break;	//NAV1 Active
	case 'Q':LEDString[0] = getChar();												break;	//NAV2 Active
	case 'S':LEDString[13] = getChar();												break;	//ADF Active
	case 'A':Channel[COM1]->Active = GetBufferValue(7);								break;	//COM1 Active
	case 'B':Channel[COM1]->Standby = GetBufferValue(7);								break;	//COM1 Stby	
	case 'C':Channel[COM2]->Active = GetBufferValue(7);								break;	//COM2 Active
	case 'D':Channel[COM2]->Standby = GetBufferValue(7);								break;	//COM2 Stby
	case 'E':Channel[NAV1]->Active = GetBufferValue(6);								break;	//NAV1 Active
	case 'F':Channel[NAV1]->Standby = GetBufferValue(6);								break;	//NAV1 Stby
	case 'G':Channel[NAV2]->Active = GetBufferValue(6);								break;	//NAV2 Active
	case 'H':Channel[NAV2]->Standby = GetBufferValue(6);								break;	//NAV2 Stby
	case 'I':Channel[ADF]->Standby = GetBufferValue(6); Channel[4]->Active = "     ";	break;	//ADF
	case 'J':Displays[EATCDisp]->Data = GetBufferValue(4);							break;	//SQUAK Set	
	}
	LEDString[11] = LEDString[Channel[SelectedMode]->Status];//update Call Stats LED
}
//------------------------------------------------------------------------------------------------------------------------------------------
void LESSTHAN()
{
	CodeIn = getChar();
	switch (CodeIn)
	{
	case 'd':	LEDString[26] = getChar();		break;
	case 'e':	LEDString[27] = getChar();		break;
	case 'b':	LEDString[28] = getChar();		break;
	case 'c':	LEDString[29] = getChar();		break;
	case 'g':											//Avionics Power	
		temp = getChar();
		if (temp == '0' && MasterPower)Serial.println("A431");
		else if (temp == '1' && !MasterPower)Serial.println("A430");//force sim to follow phyical switch
		break;
	case 'f'://LED Status
		String LightState = GetBufferValue(10);
		LEDString[18] = LightState[2];
		LEDString[19] = LightState[3];
		LEDString[20] = LightState[8];
		LEDString[21] = LightState[4];
		LEDString[22] = LightState[6];
		LEDString[23] = LightState[1];
		LEDString[17] = LightState[5];
		LEDString[16] = LightState[9];
		break;
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------
void ENCODER(){

	if (SelectedMode == ADF)
	{
		FreqCtrl->ReadEncoder();

	}
	else
	{
		if (MHz)FreqCtrl->ReadEncoder(Channel[SelectedMode]->MHzIncCommand, Channel[SelectedMode]->MHzDecCommand);
		else FreqCtrl->ReadEncoder(Channel[SelectedMode]->KHzIncCommand, Channel[SelectedMode]->KHzDecCommand);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------
String GetBufferValue(int byteCount)
{
	String buff = "";
	for (int i = 0; i < byteCount; i++)
	{
		temp = getChar();
		if ((temp <= '9' && temp >= '0') || temp == '.')buff += temp;
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
void SendLEDData(String data)
{
	PulseLEN(LED1_LTCH, false);
	for (int i = 15; i >= 0; i--)PulseData(LED1_DS, LED1_CLK, data[i]);
	PulseLEN(LED1_LTCH, true);

	PulseLEN(LED2_LTCH, false);
	for (int i = 31; i >= 16; i--)PulseData(LED2_DS, LED2_CLK, data[i]);
	PulseLEN(LED2_LTCH, true);
}
//------------------------------------------------------------------------------------------------------------------------------------------