  #define VOLUME_PIN A0

#define DIR_PIN 10
#define STEP_PIN 11
#define EN_PIN 12

#define LED_PIN 13

#define COMPARE_VALUE_TIMER OCR1A

#define TurnOnTimer1 (TIMSK1 |= (1 << OCIE1A))
#define TurnOffTimer1 (TIMSK1 &= ~(1 << OCIE1A))

#define READ_VOLUME_TIME_MS 10

#define SERIAL_MODE true
#define VOLUME_MODE false

#define COMMAND_PORT Serial

#define MAX_SPEED 200
#define DEFAULT_SPEED 30

#define STEP_PER_MM 84.0

#define SPEED_TO_CYCLE(x) (1000000.0 / (STEP_PER_MM * x))

#include "MultiThread.h"

String inputString;
bool stringComplete;
bool Mode;
float DesireSpeed;
float DesirePosition;
long DesireStepPosition;
long CurrentStepPosition;
float OldSpeed;

byte VolumeValuaCouter;
float VolumeValue;

float DefaultSpeed;
bool IsConveyorRun;

MultiThread ReadVolumeScheduler;
MultiThread LedBlinkScheduler;

void setup()
{
  Serial.begin(115200);
  IOInit();
  setValue();
  TimerInit();
}

void loop()
{
	readVolume();
	SerialExecute();
  LedBlink();
}

void IOInit()
{
  pinMode(VOLUME_PIN, INPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(EN_PIN, 1);
}

void setValue()
{
	Mode = VOLUME_MODE;
  Serial.println("Begin:");
  OldSpeed = DEFAULT_SPEED;
}

void TimerInit()
{
  noInterrupts();

  // Reset register relate to Timer 1
  // Reset register relate
  TCCR1A = TCCR1B = TCNT1 = 0;
  // Set CTC mode to Timer 1
  TCCR1B |= (1 << WGM12);
  // Set prescaler 1 to Timer 1
  TCCR1B |= (1 << CS10);
  //Normal port operation, OCxA disconnected
  TCCR1A &= ~((1 << COM1A1) | (1 << COM1A0) | (1 << COM1B1) | (1 << COM1B0));

  interrupts();
}

void LedBlink()
{
  if (DesireSpeed == 0 && DesireStepPosition == 0)
  {
    digitalWrite(LED_BUILTIN, 0);
    return;
  }

  RUN_EVERY(LedBlinkScheduler, COMPARE_VALUE_TIMER / 16);
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void readVolume()
{
	if (Mode != VOLUME_MODE) return;

	RUN_EVERY(ReadVolumeScheduler, READ_VOLUME_TIME_MS);

	VolumeValuaCouter++;
	VolumeValue += map(analogRead(VOLUME_PIN), 0, 1023, 0, MAX_SPEED);

	if (VolumeValuaCouter == 10)
	{
		DesireSpeed = VolumeValue / 10.0;
		if (DesireSpeed < 0.05 && DesireSpeed > -0.05)
		{
			DesireSpeed = 0;
		}

    if  (DesireSpeed > 0)
    {
      setIntCycle(SPEED_TO_CYCLE(DesireSpeed));
      TurnOnTimer1;
      digitalWrite(EN_PIN, 0);
      digitalWrite(DIR_PIN, 0);
    }
    else
    {
      TurnOffTimer1;
      digitalWrite(EN_PIN, 1);
    }
     
		VolumeValuaCouter = 0;
		VolumeValue = 0;
	}
}

void ConveyorExecute()
{
	if (Mode != SERIAL_MODE)
	{
		return;
	}

	if (DesireSpeed < 0.01 && DesireSpeed > -0.01)
	{
		DesireSpeed = 0;
	}

	if (DesireSpeed > 0)
	{
		digitalWrite(DIR_PIN, 0);
	}
	else if (DesireSpeed < 0)
	{
		digitalWrite(DIR_PIN, 1);
	}

  DesireSpeed = abs(DesireSpeed);
  if  (DesireSpeed > MAX_SPEED)
  {
    DesireSpeed = MAX_SPEED;
  }
  setIntCycle(SPEED_TO_CYCLE(DesireSpeed));

	DesireStepPosition += roundf(DesirePosition * STEP_PER_MM);
  DesirePosition = 0;
 
	if (DesireStepPosition > 0)
	{
		digitalWrite(DIR_PIN, 1);
	}
	else if (DesireStepPosition < 0)
	{
		digitalWrite(DIR_PIN, 0);
    DesireStepPosition = -DesireStepPosition;
	}

	if (DesireStepPosition != 0)
	{
		setIntCycle(SPEED_TO_CYCLE(OldSpeed));
	}

	if (DesireSpeed == 0 && DesireStepPosition == 0)
	{
		TurnOffTimer1;
    digitalWrite(EN_PIN, 1);
		return;
	}

	TurnOnTimer1;
  digitalWrite(EN_PIN, 0);
}

//intCycle us
void setIntCycle(float intCycle)
{
  int prescaler;

  if (intCycle > 4000)
  {
    TCCR1B |= (1 << CS11);
    TCCR1B &= ~(1 << CS10);
    prescaler = 8;
  }
  else
  {
    TCCR1B &= ~(1 << CS11);
    TCCR1B |= (1 << CS10);
    prescaler = 1;
  }

  COMPARE_VALUE_TIMER = roundf(intCycle * F_CPU / (1000000.0 * prescaler)) - 1;
}

ISR(TIMER1_COMPA_vect)
{
	digitalWrite(STEP_PIN, 0);
	delayMicroseconds(2);
	digitalWrite(STEP_PIN, 1);
  
	if (DesireStepPosition != 0)
	{
		CurrentStepPosition++;
		if (DesireStepPosition == CurrentStepPosition)
		{
			Serial.println("Ok");
			TurnOffTimer1;
			DesireStepPosition = 0;
			CurrentStepPosition = 0;
      digitalWrite(EN_PIN, 1);
		}
	}
}

float ConvertSpeedToIntCycle(float speed)
{
	return 1000000.0 / (STEP_PER_MM * speed);
}

void SerialExecute()
{
  while (COMMAND_PORT.available())
  {
    char inChar = (char)COMMAND_PORT.read();

    if (inChar == '\n')
    {
      stringComplete = true;
      break;
    }

    inputString += inChar;
  }

  if (!stringComplete)
    return;

  String messageBuffer = inputString.substring(0, 4);

  if (messageBuffer == "M310")
  {
	  if (inputString.substring(5).toInt() == SERIAL_MODE)
	  {
	    Mode = SERIAL_MODE;
      DesireSpeed = 0;
	  }
	  if (inputString.substring(5).toInt() == VOLUME_MODE)
	  {
	    Mode = VOLUME_MODE;
      DesirePosition = 0;
	  }
    Serial.println("Ok");
  }

  if (messageBuffer == "M311")
  {
	  DesireSpeed = inputString.substring(5).toFloat();
	  DesirePosition = 0;
    Serial.println("Ok");
  }

  if (messageBuffer == "M312")
  {
	  DesirePosition = inputString.substring(5).toFloat();
	  DesireSpeed = 0; 
    if(DesirePosition == 0) Serial.println("Ok");
  }

  if (messageBuffer == "M313")
  {
    OldSpeed = inputString.substring(5).toFloat();
    Serial.println("Ok");
  }

  inputString = "";
  stringComplete = false;

  ConveyorExecute();
}
