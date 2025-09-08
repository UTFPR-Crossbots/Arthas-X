#include <Arduino.h>
#include <Arthas.h>
#include <BluetoothSerial.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run make menuconfig to and enable it
#endif

// // TB6612FNG pinout
#define RIN1 5
#define LIN1 17
#define RIN2 18
#define LIN2 16
#define PWMR 19
#define PWML 4

const uint8_t numberOfFrontSensors = 6;
const uint8_t frontSensorPins[] = {32, 33, 25, 26, 27, 14};
const uint8_t rightMotorPins[] = {RIN1, RIN2, PWMR};
const uint8_t leftMotorPins[] = {LIN1, LIN2, PWML};
const uint8_t lateralSensorPin = 34;

int16_t maxspeed = 100;

Arthas arthas = Arthas(leftMotorPins, rightMotorPins, frontSensorPins, numberOfFrontSensors, lateralSensorPin, maxspeed);

void setup() {
	arthas.setupBluetooth();
	arthas.setupFrontSensor();
	arthas.setupLateralSensor();
}

void loop() {
	ArthasAction action = arthas.parseBluetoothInput();
	if (action != ArthasAction::None) {
		switch(action) {
			case ArthasAction::BothMotorsTest:
				arthas.testBothMotors();
				break;
			case ArthasAction::FrontSensorTest:
				arthas.testFrontSensor();
				break;
			case ArthasAction::FrontSensorTestAnalogRead:
				arthas.testFrontSensorAnalogRead();
				break;
			case ArthasAction::FrontSensorTestWhileLine:
				arthas.testFrontSensorWhiteLine();
				break;
			case ArthasAction::Calibrate:
				arthas.calibrateFrontSensor();
				break;
			case ArthasAction::UpdateConstants:
				arthas.printlnBT("Updated Constants!");
				arthas.printMaxSpeed();
				arthas.printPID();
				break;
			case ArthasAction::DriveLap:
				arthas.driveLap(5);
				break;
			case ArthasAction::Chase:
				arthas.chase();
				break;
			case ArthasAction::Stop:
				arthas.stopMotors();
				break;
			case ArthasAction::BluetoothConnected:
				arthas.printlnBT("Cliente Conectado!");
				break;
			case ArthasAction::BluetoothDisconnected:
				arthas.printlnBT("Cliente Desconectado!");
				break;
		}
	}
}
