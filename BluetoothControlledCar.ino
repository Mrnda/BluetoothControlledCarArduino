
#include <SoftwareSerial.h>


typedef struct{
	String		attributeName;
	String		attributeValue;

} Attribute;

typedef struct
{
	String		elementName;
	Attribute	attributes[2];
	int			attributeCount;

} Command;



int				statesTable[6][5] = { { 2, 1, 1, 1, 1 },	//WAITING_STATE
{ 6, 2, 3, 6, 5 },		//READ_ELEMENT_NAME_STATE
{ 6, 3, 6, 4, 6 },		//READ_ATTRIBUTE_NAME_STATE
{ 6, 4, 3, 6, 5 },		//READ_ATTRIBUTE_VALUE_STATE
{ 1, 1, 1, 1, 1 },		//HANDLE_DATA_STATE
{ 6, 6, 6, 6, 6 } };	//ERROR_STATE



SoftwareSerial	Bluetooth(3, 4); //RX, TX
Command			lastCommand;
int				currentState = 1;
char			lastReadChar;

#define Debug						Serial
#define DEBUG_MODE					true


#define WAITING_STATE				1
#define READ_ELEMENT_NAME_STATE		2
#define READ_ATTRIBUTE_NAME_STATE	3
#define READ_ATTRIBUTE_VALUE_STATE	4
#define HANDLE_DATA_STATE			5
#define ERROR_STATE					6


#define MOTOR_PIN_RIGHT_CLOCKWISE		10 // has to be PWM pins
#define MOTOR_PIN_RIGHT_ANTICLOCKWISE	9  // has to be PWM pins
#define MOTOR_PIN_LEFT_CLOCKWISE		11 // hat to be PWM pins
#define MOTOR_PIN_LEFT_ANTICLOCKWISE	9  // has to be PWM pins
#define MOTOR_SIDE_RIGHT_VALUE			'R'
#define MOTOR_SIDE_LEFT_VALUE			'L'


#define MOTOR_COMMAND_SIDE				String("side")
#define MOTOR_COMMAND_SIDE_RIGHT		String("right")
#define MOTOR_COMMAND_SIDE_LEFT			String("left")
#define MOTOR_COMMAND_POWER				String("power")


#define MOVE_COMMAND_MODE				String("mode")
#define MOVE_COMMAND_MODE_FORWARD		String("forward")
#define MOVE_COMMAND_MODE_BACKWARD		String("backward")

#define COMMAND_STRING_MOTOR			String("motor")
#define COMMAND_STRING_MOVE				String("move")
#define COMMAND_STRING_STOP				String("stop")

void debugCommandLog(){
	Serial.println("Debugging last command");
	Serial.print("elementName = ");
	Serial.println(lastCommand.elementName);
	Serial.print("attributesCount = ");
	Serial.println(lastCommand.attributeCount);
	if (lastCommand.attributeCount > 0){
		int i = 0;
		for (i = 0; i < lastCommand.attributeCount; i++){
			Serial.print("attribute position = ");
			Serial.println(i);
			Serial.print("attributeName = ");
			Serial.println(lastCommand.attributes[i].attributeName);
			Serial.print("attributeValue = ");
			Serial.println(lastCommand.attributes[i].attributeValue);
		}
	}
}

void setBothMotors(int valueRight, int valueLeft){
	setMotor(valueRight, MOTOR_SIDE_RIGHT_VALUE);
	setMotor(valueLeft, MOTOR_SIDE_LEFT_VALUE);
}



void setMotor(int pwmValue, int side){
	int motorPin;
	if (pwmValue >= 0){
		if (MOTOR_SIDE_LEFT_VALUE == side){
			digitalWrite(MOTOR_PIN_LEFT_ANTICLOCKWISE, LOW);
			motorPin = MOTOR_PIN_LEFT_CLOCKWISE;
		}
		else if (MOTOR_SIDE_RIGHT_VALUE == side){
			digitalWrite(MOTOR_PIN_RIGHT_ANTICLOCKWISE, LOW);
			motorPin = MOTOR_PIN_RIGHT_CLOCKWISE;
		}
	}
	else{
		pwmValue *= -1;
		if (MOTOR_SIDE_LEFT_VALUE == side){
			digitalWrite(MOTOR_PIN_LEFT_CLOCKWISE, LOW);
			motorPin = MOTOR_PIN_LEFT_ANTICLOCKWISE;
		}
		else if (MOTOR_SIDE_RIGHT_VALUE == side){
			digitalWrite(MOTOR_PIN_RIGHT_CLOCKWISE, LOW);
			motorPin = MOTOR_PIN_RIGHT_ANTICLOCKWISE;
		}
	}

	if (DEBUG_MODE){
		Serial.println("Motor command");
		Serial.print("side = ");
		Serial.println(side);
		Serial.print("pwmValue = ");
		Serial.println(pwmValue);
		Serial.print("motorPin = ");
		Serial.println(motorPin);
	}


	analogWrite(motorPin, pwmValue);
}

void handleData(){
	debugCommandLog();
	//Handle data from your command here.
	//if(lastCommand == "test"){
	//	doSomething();
	//}
	if (COMMAND_STRING_MOTOR == lastCommand.elementName){
		if (lastCommand.attributeCount > 0){
			int i,  pwmValue = 255, side;
			
			for (i = 0; i < lastCommand.attributeCount; i++){
				Attribute currentAttribute =  lastCommand.attributes[i];
				if (MOTOR_COMMAND_SIDE == currentAttribute.attributeName){
					if (MOTOR_COMMAND_SIDE_LEFT == currentAttribute.attributeValue){
						side = MOTOR_SIDE_LEFT_VALUE;
					}
					else if (MOTOR_COMMAND_SIDE_RIGHT == currentAttribute.attributeValue){
						side = MOTOR_SIDE_RIGHT_VALUE;
					}
					
				}
				if (MOTOR_COMMAND_POWER == currentAttribute.attributeName){
					pwmValue = currentAttribute.attributeValue.toInt();
				}
			}

			setMotor(pwmValue, side);			
		}
	}
	else if (COMMAND_STRING_MOVE == lastCommand.elementName){
		if (lastCommand.attributeCount > 0){
			int i = 0;
			for (i = 0; i < lastCommand.attributeCount; i++){
				Attribute currentAttribute = lastCommand.attributes[i];

				if (MOVE_COMMAND_MODE = currentAttribute.attributeName){
					if (MOVE_COMMAND_MODE_FORWARD == currentAttribute.attributeValue){
						setBothMotors(255, 255);
					}
					else if (MOVE_COMMAND_MODE_BACKWARD == currentAttribute.attributeValue){
						setBothMotors(-255, -255);
					}
				}
				
			}
		}
	}
	else if (COMMAND_STRING_STOP == lastCommand.elementName){
		setBothMotors(0, 0);
	}
	

	currentState = WAITING_STATE; // Handling done now waiting again

}

int getNextState(){
	int tableColumn;
	int tableRow = currentState - 1;

	if (lastReadChar == '<'){
		tableColumn = 0;
	}
	else if ((lastReadChar >= '0') && (lastReadChar <= '9') ||
		((lastReadChar >= 'A') && (lastReadChar <= 'Z')) ||
		((lastReadChar >= 'a') && (lastReadChar <= 'z'))){
		tableColumn = 1;
	}
	else if (lastReadChar == ' '){
		tableColumn = 2;
	}
	else if (lastReadChar == '='){
		tableColumn = 3;
	}
	else if (lastReadChar == '>'){
		tableColumn = 4;
	}
	else {
		return WAITING_STATE;
	}

	if (DEBUG_MODE){
		Serial.print("Getting state frm state table ");
		Serial.print(tableRow);
		Serial.print(' ');
		Serial.println(tableColumn);
	}

	return statesTable[tableRow][tableColumn];
}


void act(){
	Serial.println("Acting");
	if (READ_ELEMENT_NAME_STATE == currentState){
		if (lastReadChar == '<'){
			lastCommand.elementName = "";
		}
		else {
			lastCommand.elementName += lastReadChar;
		}
		if(DEBUG_MODE) Serial.println("currentState = READ_ELEMENT_NAME_STATE");
	}
	else if (READ_ATTRIBUTE_NAME_STATE == currentState) {

		if (lastReadChar == ' '){
			lastCommand.attributeCount++;
			lastCommand.attributes[lastCommand.attributeCount - 1].attributeName = "";
		}
		else {
			lastCommand.attributes[lastCommand.attributeCount - 1].attributeName += lastReadChar;
		}
		if (DEBUG_MODE) Serial.println("currentState = READ_ATTRIBUTE_NAME_STATE");
	}
	else if (READ_ATTRIBUTE_VALUE_STATE == currentState){
		if (lastCommand.attributes[lastCommand.attributeCount - 1].attributeName.length()> 0){
			if (lastReadChar == '='){
				lastCommand.attributes[lastCommand.attributeCount - 1].attributeValue = "";
			}
			else {
				lastCommand.attributes[lastCommand.attributeCount - 1].attributeValue += lastReadChar;
			}
			if (DEBUG_MODE) Serial.println("currentState = READ_ATTRIBUTE_VALUE_STATE");
		}
		else{
			currentState = ERROR_STATE;
		}

	}
	else if (HANDLE_DATA_STATE == currentState){

		if (lastCommand.elementName.length() > 0){
			handleData();
			if (DEBUG_MODE) Serial.println("currentState = HANDLE_DATA_STATE");
		}
		else {
			currentState = ERROR_STATE;
		}

	}

}

void setup()
{
	Bluetooth.begin(9600);

	Serial.begin(19200);

	if (DEBUG_MODE) Serial.println("The board is ready.");

	pinMode(MOTOR_PIN_LEFT_CLOCKWISE, OUTPUT);
	pinMode(MOTOR_PIN_LEFT_ANTICLOCKWISE, OUTPUT);
	pinMode(MOTOR_PIN_RIGHT_CLOCKWISE, OUTPUT);
	pinMode(MOTOR_PIN_RIGHT_ANTICLOCKWISE, OUTPUT);
}


void loop()
{


	if (Bluetooth.available()){
		if (DEBUG_MODE) Serial.println("Bluetooth available");

		lastReadChar = Bluetooth.read();	//gets character

		if (DEBUG_MODE) {
			Serial.print("lastReadChar = ");
			Serial.println(lastReadChar);
		}

		currentState = getNextState();		//checks character and get state of it

		if (DEBUG_MODE){
			Serial.print("currentStatedID = ");
			Serial.println(currentState);
		}
		
		act();								//acts according to current state
	}

}
