#include <Arthas.h>

Arthas::Arthas(const uint8_t* leftMotorPins, const uint8_t* rightMotorPins, const uint8_t* frontSensorPins, uint8_t numberOfPins, const uint8_t lateralSensorPin, const int16_t maxspeed):
	bluetooth(),
	powertrain(leftMotorPins, rightMotorPins),
	frontSensor(numberOfPins, frontSensorPins, true),
	lateralSensor(lateralSensorPin, false),
	pid(numberOfPins),
	maxspeed(maxspeed),
	tempo(0),
	marcas(0)
{
	for (uint8_t i = 0; i < numberOfPins; i++) {
		this->frontSensorPins[i] = frontSensorPins[i];
	}
}

Arthas::~Arthas() {
}

/* Bluetooth */
Bluetooth* Arthas::getBluetooth() {
	return &bluetooth;
}

void Arthas::setupBluetooth() {
	bluetooth.setup();
}

const bool Arthas::isBluetoothAvailable() {
	return bluetooth.isAvailable();
}

String Arthas::readBluetoothInput() {
	return bluetooth.readSerialInput();
}

ArthasAction Arthas::parseBluetoothInput() {
	if (!isBluetoothAvailable()) return ArthasAction::None;

	String serialInput = readBluetoothInput();

	bluetooth.println(serialInput);

	if (serialInput == "a") {
		return ArthasAction::Calibrate;
	}
	else if (serialInput == "b") {
		return ArthasAction::DriveLap;
	}
	else if (serialInput == "c") {
		return ArthasAction::Stop;
	}
	else if (serialInput == "e") {
		return ArthasAction::Chase;
	}
	else if (serialInput == "f") {
		return ArthasAction::BothMotorsTest;
	}
	else if (serialInput == "g") {
		return ArthasAction::FrontSensorTestWhileLine;
	}
	else if(serialInput == "Cliente Conectado!"){
		return ArthasAction::BluetoothConnected;
	}
	else if (serialInput ==  "Cliente Desconectado!") {
		return ArthasAction::BluetoothDisconnected;
	}
	// This code is ugly, it should not do the functions calls here but the input from the app is trash so this is the best way
	// TODO: Change the app to a better way of sending PIDs Constants
	else {
		String rawKp = serialInput.substring(2,9);
		double kP = rawKp.toDouble();

		String rawKi = serialInput.substring(10,17);
		marcas = (uint8_t)rawKi.toInt();

		String rawKd = serialInput.substring(18,25);
		double kD = rawKd.toDouble();

		pid.updatePIDConstants(kP, 0, kD);

		String rawSpeed = serialInput.substring(26,29);
		uint8_t speed = (uint8_t)rawSpeed.toInt();
		maxspeed = speed;

		return ArthasAction::UpdateConstants;
	}
}

void Arthas::printBT(const String msg) {
	bluetooth.print(msg);
}

void Arthas::printBT(const int msg) {
	bluetooth.print(msg);
}

void Arthas::printlnBT(const String msg) {
	bluetooth.println(msg);
}

void Arthas::printlnBT(const int msg) {
	bluetooth.println(msg);
}

void Arthas::turnOffBluetooth() {
	bluetooth.turnOff();
}

/* Line Sensor */
void Arthas::setupFrontSensor() {
	frontSensor.setup();
}

void Arthas::calibrateFrontSensor() {
	bluetooth.print("//---Calibration started---//");
	frontSensor.calibrate();
	bluetooth.print("//---Calibration finished---//");
}

const uint16_t Arthas::readLineWhiteFrontSensor() {
	return frontSensor.readLineWhite();
}

void Arthas::testFrontSensor() {
	const uint8_t numberOfSensors = frontSensor.getNumberOfSensors();
	uint16_t sensorValues[numberOfSensors];
	unsigned long lastPrint = 0;
	while(true) {
		if (millis() - lastPrint > 1000) {
			frontSensor.readCalibrated(sensorValues);
			for(uint8_t i = 0; i < numberOfSensors; i++) {
				bluetooth.print(sensorValues[i]);
				bluetooth.print("|");
			}
			bluetooth.println();
			lastPrint = millis();
		}
	}
}

void Arthas::testFrontSensorAnalogRead() {
	const uint8_t numberOfSensors = frontSensor.getNumberOfSensors();
	uint16_t sensorValues[numberOfSensors];
	unsigned long lastPrint = 0;
	unsigned long currentTime = 0;
	while(true) {
		currentTime = millis();
		if (currentTime - lastPrint > 1000) {
			for(uint8_t i = 0; i < numberOfSensors; i++) {
				bluetooth.print(analogRead(frontSensorPins[i]));
				bluetooth.print("|");
			}
			bluetooth.println();
			lastPrint = currentTime;
		}
	}
}

void Arthas::testFrontSensorWhiteLine() {
	uint16_t whiteLineValue = frontSensor.readLineWhite();
	bluetooth.print("White Line Value: ");
	bluetooth.println(whiteLineValue);
}

/* Lateral Sensor */
void Arthas::setupLateralSensor() {
	lateralSensor.setup();
}

void Arthas::testLateralSensor() {
	bluetooth.print("Leitura: ");
	bluetooth.println(lateralSensor.read());
}

/* Motors */
void Arthas::stopMotors() {
	powertrain.leftMotorDrive(0);
	powertrain.rightMotorDrive(0);
}

void Arthas::testLeftMotor() {
	powertrain.leftMotorDrive(maxspeed);
}

void Arthas::testRightMotor() {
    powertrain.rightMotorDrive(maxspeed);
}

void Arthas::testBothMotors() {
	bluetooth.println("Testing both motors!");
	bluetooth.println("Testing left motor!");
    testLeftMotor();
	bluetooth.println("Testing right motor!");
    testRightMotor();
}

/* Constants */
void Arthas::printMaxSpeed() {
	bluetooth.print("maxSpeed: ");
	bluetooth.println(maxspeed);
}

void Arthas::printPID() {
	bluetooth.print("P: ");
	bluetooth.printDoubleln(pid.getP());
	bluetooth.print("I: ");
	bluetooth.printDoubleln(pid.getI());
	bluetooth.print("D: ");
	bluetooth.printDoubleln(pid.getD());
}

/* Modes */
void Arthas::driveLap(const uint8_t markers) {
	printlnBT("//---Começando primeira volta---//");
	// printlnBT("//---Turning off the bluetooth---//");
	// turnOffBluetooth();
	
	bool lapFinishedFlag = false;
	bool isNewMarker = true;
	uint8_t markerCount = 0;
	unsigned long currentTime = 0;
	unsigned long previousTime = 0;
	uint8_t acelerationInterval = 2;	// 2 miliseconds
	unsigned long finishTime = 0;
	uint8_t speed = 0;

	while(!lapFinishedFlag || currentTime - finishTime < 200) {
		currentTime = millis();
		if (speed < maxspeed) {
			if (currentTime - previousTime >= acelerationInterval) {
				speed++;
				previousTime = currentTime;
			}
		}

		uint16_t linePosition = frontSensor.readLineWhite();

		double correction = pid.calculateCorrection(linePosition);

		int16_t leftMotorSpeed = constrain(speed + correction, -maxspeed/2, maxspeed);
		int16_t rightMotorSpeed = constrain(speed - correction, -maxspeed/2, maxspeed);

		powertrain.motorsDrive(leftMotorSpeed, rightMotorSpeed);

		//Case lateral sensor is analog read
		if (lateralSensor.isWhite()) {
			if (isNewMarker) {
				markerCount++;
				bluetooth.println(markerCount);
				isNewMarker = false;
			}
		} else {
			isNewMarker = true;
		}

		if (markerCount >= marcas) {
			if (!lapFinishedFlag) {
				finishTime = millis();
			}
			lapFinishedFlag = true;
		}
	}
	stopMotors();
	pid.clearTimeAndError();
}

void Arthas::chase() {
	bluetooth.println("--- Starting! ---");
	bool stop = false;

	while (!stop) {
		ArthasAction action = parseBluetoothInput();
		uint16_t linePosition = frontSensor.readLineWhite();

		double correction = pid.calculateCorrection(linePosition);

		powertrain.leftMotorDrive(constrain(maxspeed + correction, -maxspeed/2, maxspeed));
		powertrain.rightMotorDrive(constrain(maxspeed - correction, -maxspeed/2, maxspeed));

		if (action == ArthasAction::Stop) {
			stopMotors();
			stop = true;
		} 
		else if (action == ArthasAction::UpdateConstants) {
			bluetooth.println("Updated Constants!");
			this->printPID();
		}
	}

	bluetooth.println("--- Stopped! ---");
}
