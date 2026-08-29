#include <Arduino.h>
#include <Arthas.h>
#include <Pinout.h>

int16_t maxspeed = 100;

Arthas arthas = Arthas(
	pins::leftMotor,
	pins::rightMotor,
	pins::frontSensorCommon,
	pins::frontSensorSelect,
	pins::frontSensorCount,
	pins::lateralSensorLeft,
	pins::lateralSensorRight,
	pins::suctionEsc,
	pins::suctionPwmTimer,
	pins::suctionRaceThrottle,
	maxspeed,
	pins::leftMotorInverted,
	pins::rightMotorInverted
);

void setup() {
	/* Primeiro de tudo, e bloqueante: o ESC so arma depois de receber o pulso
	 * neutro sustentado por ~3 s. Ate isso terminar ele ignora qualquer
	 * comando, entao nao ha o que fazer em paralelo. */
	arthas.setupSuction();

	arthas.setupConsole();
	arthas.setupMotors();
	arthas.setupFrontSensor();
	arthas.setupLateralSensors();

	/* O host demora a enumerar o USB CDC; sem esta folga o menu sai antes de
	 * alguem estar escutando e a serial abre em branco. */
	delay(1500);
	arthas.printMenu();
}

void loop() {
	ArthasAction action = arthas.parseInput();
	if (action != ArthasAction::None) {
		switch(action) {
			case ArthasAction::LeftMotorTest:
				arthas.testLeftMotor();
				break;
			case ArthasAction::RightMotorTest:
				arthas.testRightMotor();
				break;
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
			case ArthasAction::LateralSensorTest:
				arthas.testLateralSensor();
				break;
			case ArthasAction::SuctionTest:
				arthas.testSuction();
				break;
			case ArthasAction::Calibrate:
				arthas.calibrateFrontSensor();
				break;
			case ArthasAction::UpdateConstants:
				arthas.println("Updated Constants!");
				arthas.printMaxSpeed();
				arthas.printPID();
				break;
			case ArthasAction::ShowStatus:
				arthas.printStatus();
				break;
			case ArthasAction::ShowMenu:
				arthas.printMenu();
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
				arthas.println("Cliente Conectado!");
				arthas.printMenu();
				break;
			case ArthasAction::BluetoothDisconnected:
				arthas.println("Cliente Desconectado!");
				break;
			default:
				break;
		}
	}
}
