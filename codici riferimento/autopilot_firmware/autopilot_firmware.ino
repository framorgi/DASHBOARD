/****************************************************************************************/
/*																						*/
/*							Autopilot 500-TS | ©Kantooya Inc							*/
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
//---------------------------------------------------------------

//---------------------------- Pins -----------------------------
//			Name			Arduino Pin				Pin on ATMega
//		-----------			-----------				-------------
#define LED_DS					2			//			32
#define LED_CLK					3			//			1
#define LED_LTCH				4			//			2 
#define ALTEnc_B				6			//
#define ALTEnc_A				7			//
#define VSEnc_B					8			//
#define VSEnc_A					9			//
#define IRQ						13			//			17 
#define SpdEnc_B				10			//
#define SpdEnc_A				11			//
#define HdgEnc_A				A0			//
#define HdgEnc_B				A1			//
#define CrsEnc_A				A2			//
#define CrsEnc_B				A3			//
#define SDA						A4			//
#define SCL						A5			//
//---------------------------------------------------------------

//------------------------- Settings ----------------------------
#define SerialTimeout			1000
#define TempDelay				2000
//---------------------------------------------------------------

//-------------------------- Enums ------------------------------
enum EDispID{ ESpeedDisp = 0, EALTDisp = 1, EVSDisp = 2, ECrsDsip = 3, EHdgDisp = 4 };
//---------------------------------------------------------------

//----------------------- Prototypes ----------------------------
int ReadSwitches();
void pciSetup(byte);
void SendLEDData(String);
void(*resetFunc) (void) = 0; //declare reset function @ address 0
void TestLEDs(int);
void InitLEDs(int);
uint8_t I2C_ClearBus();
class SDisplayData;
class TTestDisplayData;
class TDisplay;
class TEncoderData;
class TBlinkLED;
//---------------------------------------------------------------

//------------------------- Objects -----------------------------
AS1115 DispCtrl1 = AS1115(0x01);
AS1115 DispCtrl2 = AS1115(0x02);
AS1115 SwitchCtrl = AS1115(0x03);
TEncoderData *SpeedCtrl, *ALTCtrl, *VSCtrl, *HdgCtrl, *CrsCtrl;
TBlinkLED *RealDataBlink, *SpeedBlink, *AltBlink, *HdgBlink;
//---------------------------------------------------------------
class SDisplayData
{
public:
	String SetMem;
	String ReadMem;
};
//---------------------- Public Variables -----------------------
int SwLEDList[] = { 3, 1, 2, 0, 9, 10, 11, 4, 0, 0, 14, 5, 0, 12, 8, 13 };
bool BlinIndexArray[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
String FSXCommands[] = { "B09", "B04", "B10", "", "A54", "A54", "B30", "B08", "", "", "Y080", "B05", "", "B34", "B01", "B26" };
String LEDString = "0000000000000000";
String OldLEDs = "0000000000000000";
String  BlinkLEDs = "0000000000000000";
int tempVal, CodeIn;
bool TestProgram = false, ShowDP = false, ShowRealValues = false, OldShowRealValues = false;
bool ShowZeros, Connected = false, MasterBattery;
char navMeM, SpeedMeM, ATHRMeM;
SDisplayData ALTData, SpdData, VSData, HdgData, CrsData;
TDisplay *Displays[5];
unsigned long int HoldTime = 0, TimeOutCheck;
//---------------------------------------------------------------

//-------------------------- Classes ----------------------------
class TDisplay
{
private:
	byte *Index;
	int DigitCount;
	EDispID ID;
	bool TempValChange;
	unsigned long int LastTempChange;
	SDisplayData *Data;
	void SendValues(byte index, char value)
	{
		if (index > 7)
		{
			if (DispCtrl2.digitWrite(index - 8, value, (TestProgram ? ShowZeros : MasterBattery)))
			{
				Serial.println("I2C Froze (Displays)");
				I2C_ClearBus();
				Wire.begin();
				DispCtrl1.begin();
				DispCtrl2.begin();
				SwitchCtrl.begin();
			}
		}
		else
		{
			if (DispCtrl1.digitWrite(index, value, (index == 6 || index == 1 ? ShowDP : 0)))
			{
				Serial.println("I2C Froze (Displays)");
				I2C_ClearBus();
				Wire.begin();
				DispCtrl1.begin();
				DispCtrl2.begin();
				SwitchCtrl.begin();
			}
		}
	}
public:
	TDisplay(byte *_Index, EDispID _ID, SDisplayData *_Data)
	{
		Index = _Index;
		DigitCount = 3;
		ID = _ID;
		UpdateDisplay("   ");
		TempValChange = false;
		Data = _Data;
	}
	void CheckTempDisplay()
	{
		if (TempValChange && millis() - LastTempChange > TempDelay)
		{
			TempValChange = false;
			UpdateDisplay();
		}
	}

	void UpdateDisplay()
	{
		UpdateDisplay(!ShowRealValues || TempValChange ? Data->SetMem : Data->ReadMem);
	}
	void UpdateDisplay(String Val)
	{


		bool LeadZeroFound = false;
		bool Negative = false;
		for (int i = 0; i < DigitCount; i++)
		{
			if (!MasterBattery && !TestProgram)Val[i] = ' ';
			else
			{
				if (Val[i] == '-')
				{
					Negative = true;
					Val[i] = ' ';
				}
				else if (Val[i] == '+')Val[i] = ' ';
				else
					if (Val[i] == '0' && !LeadZeroFound && i != DigitCount - 1 && (!TestProgram || ((Index[i] != 6 && Index[i] != 1) || LEDString[11] == '0')))Val[i] = ' ';
					else
					{
						LeadZeroFound = true;
						if (Negative)
						{
							if (TestProgram && (LEDString[11] == '1' && (Index[i] == 7 || Index[i] == 2)))
							{
								SendValues(Index[i - 1], '0');
								SendValues(Index[i - 2], '-');
							}
							else SendValues(Index[i - 1], '-');

							Negative = false;
						}
					}
			}

			SendValues(Index[i], Val[i]);
		}
	}
	void ClearDisplay()
	{
		UpdateDisplay("   ");
	}
	EDispID GetID(){ return ID; }
	void SetTempValChange(bool val)
	{
		TempValChange = val;
		LastTempChange = millis();
	}

};
//--------------------------------------------

//--------------------------------------------
class TTestDisplayData
{
public:
	int Value;
	TTestDisplayData()
	{
		Value = 0;
	}
	String toString()
	{
		String RetVal = String(Value);
		String Buff = "";
		for (int i = 0; i < 3 - RetVal.length(); i++)
			Buff += '0';
		RetVal = Buff + RetVal;

		return RetVal;
	}
	void Inc()
	{
		Value++;
		if (Value>999)Value = 0;
	}
	void Dec(bool AllowNegative)
	{
		Value--;
		if (AllowNegative){ if (Value < -99)Value = 0; }
		else{ if (Value < 0)Value = 0; }
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
	TTestDisplayData *TestData;
	TDisplay *Disp;

public:
	TEncoderData(int _PinA, int _PinB, TDisplay *_Disp)
	{
		Encoder = new Quadrature(_PinA, _PinB);
		TestData = new TTestDisplayData();
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
			Disp->SetTempValChange(true);
		}
	}
	void ReadEncoder(TDisplay *Disp)//used for test program.
	{
		R = (Encoder->position() / 2); //The /2 is to suit the encoder
		if (R != Rold) { // checks to see if it different
			(Rdif = (R - Rold));// finds out the difference
			if (Rdif == 1)TestData->Inc();
			if (Rdif == -1)TestData->Dec(true);
			Rold = R; // overwrites the old reading with the new one.
			Disp->UpdateDisplay(TestData->toString());
		}
	}
	void RefreshDisplay(TDisplay *Disp)
	{
		Disp->UpdateDisplay(TestData->toString());
	}
};
//--------------------------------------------
class TBlinkLED
{
private:
	unsigned long  BlinkDelayCounter;
	int LEDIndex;
	int NormalDelay;
	int ActiveDelay;
	bool BlinkTemp;
public:
	void Activate()
	{
		BlinkTemp = true;
		BlinkDelayCounter = 0;

	}
	void deactivate()
	{
		BlinkTemp = false;
	}
	TBlinkLED(int _LEDIndex, int _NormalDelay, int _ActiveDelay)
	{
		LEDIndex = _LEDIndex;
		NormalDelay = _NormalDelay;
		ActiveDelay = _ActiveDelay;
		BlinIndexArray[LEDIndex] = true;
	}
	void Blink(bool pressed)
	{
		if (millis() - BlinkDelayCounter > (pressed ? ActiveDelay : NormalDelay))
		{
			BlinkLEDs = GetBlinkString();
			BlinkLEDs[LEDIndex] = BlinkTemp ? '1' : '0';
			SendLEDData(BlinkLEDs);
			BlinkTemp = !BlinkTemp;
			BlinkDelayCounter = millis();
		}
	}
	void DontBlink()
	{
		BlinkLEDs = GetBlinkString();
		BlinkLEDs[LEDIndex] = LEDString[LEDIndex];
		if (BlinkLEDs != OldLEDs)
		{
			SendLEDData(BlinkLEDs);
			OldLEDs = BlinkLEDs;
		}
	}
};
//--------------------------------------------
//----------------------- End of Classes ------------------------
String GetBlinkString()
{
	String RetVal = "0000000000000000";
	for (int i = 0; i < 16; i++)
	{
		if (!BlinIndexArray[i])RetVal[i] = LEDString[i];
		else RetVal[i] = BlinkLEDs[i];
	}
	return RetVal;
}
//----------------------- Interrupts ----------------------------
ISR(PCINT0_vect) // handle pin change interrupt for D8 to D13 here
{
	interrupts();
	if (digitalRead(IRQ) == LOW)
	{

		tempVal = ReadSwitches();
		if (tempVal > 0)
		{
			if (TestProgram) //Execute Test Program when buttons are pressed.
			{
				if ((LEDString[4] != '1' || tempVal == 8) && tempVal != 1)
				{
					LEDString[SwLEDList[tempVal - 1]] = (LEDString[SwLEDList[tempVal - 1]] == '0' ? '1' : '0');
					SendLEDData(ShowRealValues ? BlinkLEDs : LEDString);
				}
				switch (tempVal)
				{
				case 1:
					HoldTime = millis();
					break;
				case 13:
					resetFunc();  //exit test program
					break;
				case 5:
				case 6:
				case 7:
				case 14:
				case 15:
					ShowZeros = (LEDString[12] == '1');
					ShowDP = (LEDString[11] == '1'&& LEDString[8] == '1' && LEDString[5] == '0');
					if (LEDString[9] == '0' && LEDString[10] == '1' && LEDString[8] == '1')
					{
						DispCtrl1.setFont(FONT_CODEB);
						DispCtrl2.setFont(FONT_CODEB);
						SpeedCtrl->RefreshDisplay(Displays[ESpeedDisp]);
						ALTCtrl->RefreshDisplay(Displays[EALTDisp]);
						VSCtrl->RefreshDisplay(Displays[EVSDisp]);
						CrsCtrl->RefreshDisplay(Displays[ECrsDsip]);
						HdgCtrl->RefreshDisplay(Displays[EHdgDisp]);
					}
					break;
				case 8:
					if (LEDString[4] == '1')
					{
						ShowRealValues = false;
						LEDString = "1111111111111111";
						for (int i = 0; i < 8; i++)DispCtrl1.digitWrite(i, 8, (i == 6 || i == 1));
						for (int i = 0; i < 7; i++)DispCtrl2.digitWrite(i, 8, 1);
					}
					else
					{
						ShowDP = false;
						LEDString = "0000000000000000";
					}
					SendLEDData(LEDString);
					break;
				}
			}
			else if (Connected) //send commands to FSX 
			{
				switch (tempVal)
				{
				case 1: //B/C Hold Button
					HoldTime = millis();
					break;
				case 2: //heading pressed, fixed bug where it captures real data.
					Serial.println(LEDString[1] == '0' ? "Y111" : "Y121");
					break;
				case 12:// alt pressed, fixed bug where it captures real data
					Serial.println(LEDString[5] == '0' ? "Y091" : "Y101");
					break;
				case 13://Level Button
					Serial.println("B05");
					Serial.println("B05");
					Serial.println("B210000");
					break;
				default:
					Serial.println(FSXCommands[tempVal - 1]);
				}
			}
		}
	}
	else if (Connected || TestProgram)
	{
		if (tempVal == 1)
		{
			if (millis() - HoldTime > 300)
			{
				BlinkLEDs = LEDString;
				RealDataBlink->Activate();
				SpeedBlink->Activate();
				AltBlink->Activate();
				HdgBlink->deactivate();

				for (int i = 0; i < 5; i++)Displays[i]->SetTempValChange(false);
				
				ShowRealValues = !ShowRealValues;
				if (TestProgram){
					if (!ShowRealValues)
					{
						LEDString[3] == '0';
						SendLEDData(LEDString);
					}
				}
			}
			else
			{
				if (TestProgram)
				{
					LEDString[3] = LEDString[3] == '0' ? '1' : '0';
					SendLEDData(LEDString);
				}
				else
					Serial.println("B09");
			}
		}
	}
}
//---------------------------------------------------------------
void setup() {

	Serial.begin(115200);
	pinMode(IRQ, INPUT);
	pinMode(LED_DS, OUTPUT);
	pinMode(LED_CLK, OUTPUT);
	pinMode(LED_LTCH, OUTPUT);

	DispCtrl1.setFont(FONT_CODEB);
	DispCtrl2.setFont(FONT_CODEB);
	DispCtrl1.setDecode(DECODE_ALL_FONT);
	DispCtrl2.setDecode(DECODE_ALL_FONT);

	MasterBattery = false;

	Displays[ESpeedDisp] = new TDisplay(new byte[3]{3, 4, 5}, ESpeedDisp, &SpdData);;
	Displays[EALTDisp] = new TDisplay(new byte[3]{0, 1, 2}, EALTDisp, &ALTData);;
	Displays[EVSDisp] = new TDisplay(new byte[3]{14, 6, 7}, EVSDisp, &VSData);;
	Displays[ECrsDsip] = new TDisplay(new byte[3]{8, 9, 10}, ECrsDsip, &CrsData);;
	Displays[EHdgDisp] = new TDisplay(new byte[3]{11, 12, 13}, EHdgDisp, &HdgData);;

	SpeedCtrl = new TEncoderData(SpdEnc_A, SpdEnc_B, Displays[ESpeedDisp]);
	ALTCtrl = new TEncoderData(ALTEnc_A, ALTEnc_B, Displays[EALTDisp]);
	VSCtrl = new TEncoderData(VSEnc_A, VSEnc_B, Displays[EVSDisp]);
	HdgCtrl = new TEncoderData(HdgEnc_A, HdgEnc_B, Displays[EHdgDisp]);
	CrsCtrl = new TEncoderData(CrsEnc_A, CrsEnc_B, Displays[ECrsDsip]);

	RealDataBlink = new TBlinkLED(3, 250, 125);
	SpeedBlink = new TBlinkLED(13, 125, 250);
	AltBlink = new TBlinkLED(5, 130, 250);
	HdgBlink = new TBlinkLED(1, 135, 250);

	SwitchCtrl.begin();
	DispCtrl1.begin();
	DispCtrl2.begin();
	delay(500);

	DispCtrl1.setIntensity(0x0F);
	DispCtrl2.setIntensity(0x0F);

	InitLEDs(1);

	if (SwitchCtrl.ReadKeys(NULL) == 13)//go to test program
	{
		TestProgram = true;
		TestLEDs(50);
		SendLEDData("0000000000000000");
	}
	pciSetup(IRQ);

	if (TestProgram)InitLEDs(2);
}
//------------------------------------------------------------------------------------------------------------------------------------------
void loop() {
	if (TestProgram)
	{
		if (LEDString[4] == '0')
		{
			if (LEDString[8] == '1')
			{
				if (LEDString[9] == '1')
				{
					for (int i = 0; i < 5; i++)Displays[i]->UpdateDisplay(String(random(0, 1000)));
					delay(200);
				}
				else if (LEDString[10] == '1')
				{
					SpeedCtrl->ReadEncoder(Displays[ESpeedDisp]);
					ALTCtrl->ReadEncoder(Displays[EALTDisp]);
					VSCtrl->ReadEncoder(Displays[EVSDisp]);
					CrsCtrl->ReadEncoder(Displays[ECrsDsip]);
					HdgCtrl->ReadEncoder(Displays[EHdgDisp]);
				}
				else {
					DispCtrl1.setFont(FONT_HEX);
					DispCtrl2.setFont(FONT_HEX);

					for (int i = 0; i < 8; i++)
						if (DispCtrl1.digitWrite(i, i, 0) >= 4)
						{
							Serial.println("I2C Froze");
							I2C_ClearBus();
							Wire.begin();
							DispCtrl1.begin();
							DispCtrl2.begin();
						}
					for (int i = 0; i < 7; i++)
						if (DispCtrl2.digitWrite(i, i + 8, ShowZeros) >= 4)
						{
							Serial.println("I2C Froze");
							I2C_ClearBus();
							Wire.begin();
							DispCtrl1.begin();
							DispCtrl2.begin();
						}
					delay(200);
				}
			}
			else
			{
				DispCtrl1.setFont(FONT_CODEB);
				DispCtrl2.setFont(FONT_CODEB);
				ShowZeros = false;
				for (int i = 0; i < 5; i++)Displays[i]->ClearDisplay();
				delay(200);
			}
		}
		if (ShowRealValues)
		{
			RealDataBlink->Blink(LEDString[3] == '1');
			if (LEDString[13] == '1')SpeedBlink->Blink(false);
			if (LEDString[5] == '1')AltBlink->Blink(false);
			if (LEDString[1] == '1')HdgBlink->Blink(false);
		}
	}
	else //real program
	{
		{ENCODER(); } //Check the Rotary Encoders
		if (Serial.available())
		{  //Check if anything there
			CodeIn = getChar();      //Get a serial read if there is.
			if (CodeIn == '=') { EQUALS();   Connected = true; } // The first identifier is "=" ,, goto void EQUALS
			else if (CodeIn == '?') { QUESTION(); Connected = true; }// The first identifier is "?" ,, goto void QUESTION
			else if (CodeIn == '<') { LESSTHAN(); Connected = true; }// The first identifier is "?" ,, goto void QUESTION
			else
			{
				String ReadBuffer = "";
				while (Serial.available())ReadBuffer += char(Serial.read());// Empties the buffer use to show ReadBuffer += char(Serial.read());
				if (ReadBuffer.length() > 0)
				{
					Serial.println("--------------------------");
					Serial.println(ReadBuffer);
					Serial.println("--------------------------");
				}
			}
			for (int i = 0; i < 5; i++)Displays[i]->UpdateDisplay();
		}

		if (ShowRealValues != OldShowRealValues)
		{
			OldShowRealValues = ShowRealValues;
			for (int i = 0; i < 5; i++)Displays[i]->UpdateDisplay();
		}

		if (ShowRealValues && MasterBattery)
		{
			RealDataBlink->Blink(LEDString[3] == '1');

			if (LEDString[13] == '1' && SpdData.ReadMem != SpdData.SetMem)SpeedBlink->Blink(false);
			else SpeedBlink->DontBlink();

			if (LEDString[5] == '1' &&   VSData.ReadMem.substring(1, 3) != "00")AltBlink->Blink(false);
			else AltBlink->DontBlink();

			if (LEDString[1] == '1' && HdgData.ReadMem != HdgData.SetMem && !(HdgData.ReadMem == "360" && HdgData.SetMem == "000"))HdgBlink->Blink(false);
			else HdgBlink->DontBlink();
		}
		else
			if (LEDString != OldLEDs)
			{
				SendLEDData(LEDString);
				OldLEDs = LEDString;
			}

		for (int i = 0; i < 5; i++)Displays[i]->CheckTempDisplay();
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------
char getChar()// Get a character from the serial buffer
{
	TimeOutCheck = millis();
	while (Serial.available() == 0)
		if (millis() - TimeOutCheck > SerialTimeout)return '0';//time out command
	return((char)Serial.read());
}
//------------------------------------------------------------------------------------------------------------------------------------------
void SendLEDData(String data)
{
	digitalWrite(LED_LTCH, HIGH);
	digitalWrite(LED_LTCH, LOW);
	digitalWrite(LED_LTCH, HIGH);
	for (int i = 15; i >= 0; i--)
	{
		digitalWrite(LED_DS, (data[i] == '1' ? HIGH : LOW));
		digitalWrite(LED_CLK, LOW);
		digitalWrite(LED_CLK, HIGH);
	}
	digitalWrite(LED_LTCH, LOW);
	digitalWrite(LED_LTCH, HIGH); //fix for double send
	digitalWrite(LED_LTCH, LOW);

}
//------------------------------------------------------------------------------------------------------------------------------------------
void pciSetup(byte pin)
{
	*digitalPinToPCMSK(pin) |= bit(digitalPinToPCMSKbit(pin));  // enable pin
	PCIFR |= bit(digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
	PCICR |= bit(digitalPinToPCICRbit(pin)); // enable interrupt for the group
}
//------------------------------------------------------------------------------------------------------------------------------------------
int ReadSwitches()
{
	uint8_t ErrorCode;
	int val = SwitchCtrl.ReadKeys(&ErrorCode);
	if (ErrorCode >= 4)
	{
		Serial.println("I2C Froze (Switches)");
		I2C_ClearBus();
		Wire.begin();
		DispCtrl1.begin();
		DispCtrl2.begin();
		SwitchCtrl.begin();
	}
	if (val > 0 && val < 255)
		return val;
	return 0;
}
//------------------------------------------------------------------------------------------------------------------------------------------
void TestLEDs(int D)
{
	for (int i = 1; i < 15; i++)
	{
		if (i == 6 || i == 7 || i == 15 || i == 0)continue;
		String temp = "";
		for (int j = 0; j < 16; j++) temp += (j == i ? "1" : "0");
		SendLEDData(temp);
		delay(D);
	}
	for (int i = 13; i >= 2; i--)
	{
		if (i == 6 || i == 7 || i == 15 || i == 0)continue;
		String temp = "";
		for (int j = 0; j < 15; j++) temp += (j == i ? "1" : "0");
		SendLEDData(temp);
		delay(D);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------
void InitLEDs(int count)
{
	int _Delay = 300;
	for (int i = 1; i <= count; i++)
	{
		SendLEDData("1111111111111111");
		delay(_Delay);
		SendLEDData("0000000000000000");
		delay(_Delay);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------
void EQUALS(){
	CodeIn = getChar();
	switch (CodeIn) {
	case 'a':	LEDString[8] = getChar();		break;	//Master AP
	case 'k':	LEDString[5] = getChar();		break;	//Altitude Hold
	case 'm':	LEDString[4] = getChar();		break;	//App hold
	case 'o':	LEDString[2] = getChar();		break;	//Loc (Nav) Hold
	case 'n': 	LEDString[3] = getChar();		break;	//B/C hold
	case 'j':	LEDString[1] = getChar();		break;	//Heading Hold
	case 'q':	LEDString[11] = getChar();		break;	//Flight Director
	case 'v':	LEDString[14] = getChar();		break;	//Take-Off Power
	case 'b':	ALTData.SetMem = GetBufferValue(5);	break;	//ALT Set	
	case 'f':	SpdData.SetMem = GetBufferValue(3);	break;	//Speed Set		
	case 'c':	VSData.SetMem = GetBufferValue(5);	break;	//Vertical Speed Set
	case 'd':	HdgData.SetMem = GetBufferValue(3);	break;	//Heading Set
	case 'e':	CrsData.SetMem = GetBufferValue(3); CrsData.ReadMem = CrsData.SetMem; break;	//Course Set
	case 't':											//ATHR
		ATHRMeM = getChar();
		if (MasterBattery)LEDString[12] = ATHRMeM;
		break;
	case 'l':											//GPS-NAV
		navMeM = getChar();
		if (MasterBattery)
		{
			LEDString[9] = navMeM;
			LEDString[10] = (LEDString[9] == '0' ? '1' : '0');
		}
		break;
	case 's':											//Speed Hold
		SpeedMeM = getChar();
		if (MasterBattery)LEDString[13] = SpeedMeM;
		break;
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------
void QUESTION(){    // The first identifier was "?"
	CodeIn = getChar();
	switch (CodeIn) {
	case 'U': // Bus Voltage
		String buff = GetBufferValue(2);
		getChar();
		getChar();
		if (buff.toInt() == 0)//plain's turned off
		{
			MasterBattery = false;
			LEDString[9] = '0';
			LEDString[10] = '0';
			LEDString[12] = '0';
			LEDString[13] = '0';
			for (int i = 0; i < 5; i++)Displays[i]->UpdateDisplay("000");
		}
		else
		{
			MasterBattery = true;
			LEDString[9] = navMeM;
			LEDString[10] = (LEDString[9] == '0' ? '1' : '0');
			LEDString[12] = ATHRMeM;
			LEDString[13] = SpeedMeM;
			for (int i = 0; i < 5; i++)Displays[i]->UpdateDisplay();
		}
	}
}
void LESSTHAN()
{
	CodeIn = getChar();
	switch (CodeIn)
	{
	case 'D':	ALTData.ReadMem = GetBufferValue(5);		break;		//ALT Read	
	case 'P':	SpdData.ReadMem = GetBufferValue(3);		break;		//Speed Read
	case 'J':	HdgData.ReadMem = GetBufferValue(3);		break;		//Heading Read		
	case 'L':													//Vertical Speed Read
		VSData.ReadMem = GetBufferValue(6);
		VSData.ReadMem = VSData.ReadMem[0] + VSData.ReadMem.substring(2, 6);
		break;
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------
void ENCODER(){

	SpeedCtrl->ReadEncoder("B15", "B16");
	ALTCtrl->ReadEncoder("B11", "B12");
	VSCtrl->ReadEncoder("B13", "B14");
	HdgCtrl->ReadEncoder("A57", "A58");
	CrsCtrl->ReadEncoder("A56", "A55");
}
//------------------------------------------------------------------------------------------------------------------------------------------
String GetBufferValue(int byteCount)
{
	String buff = "";
	char temp;
	for (int i = 0; i < byteCount; i++)
	{
		temp = getChar();
		if ((temp <= '9' && temp >= '0') || temp == '-' || temp == '+' || temp == '.')
			buff += temp;
		else
			buff += '0';
	}

	//Serial.println(buff);

	return buff;
}
//------------------------------------------------------------------------------------------------------------------------------------------
uint8_t I2C_ClearBus() {
#if defined(TWCR) && defined(TWEN)
	TWCR &= ~(_BV(TWEN)); //Disable the Atmel 2-Wire interface so we can control the SDA and SCL pins directly
#endif

	pinMode(SDA, INPUT_PULLUP); // Make SDA (data) and SCL (clock) pins Inputs with pullup.
	pinMode(SCL, INPUT_PULLUP);
	delay(200);

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
			delay(100);
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
	/// A Repeat Start is a Start occurring after a Start with no intervening Stop.
	delayMicroseconds(10); // wait >5uS
	pinMode(SDA, INPUT); // remove output low
	pinMode(SDA, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.
	delayMicroseconds(10); // x. wait >5uS
	pinMode(SDA, INPUT); // and reset pins as tri-state inputs which is the default state on reset
	pinMode(SCL, INPUT);
	Serial.println("I2C Recovered");
	return 0; // all ok
}
//------------------------------------------------------------------------------------------------------------------------------------------