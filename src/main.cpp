#include <Arduino.h>
#include <Arthas.h>
#include <Pinout.h>

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
	pins::irReceiver,
	pins::buzzer,
	pins::buzzerActiveLow,
	pins::leftMotorInverted,
	pins::rightMotorInverted
);

void setup() {
	/* Buzzer primeiro: e o unico jeito de saber que o boot comecou, ja que os
	 * 3 s de armacao do ESC vem logo em seguida e a serial ainda nem existe. */
	arthas.setupBuzzer();

	/* Bloqueante: o ESC so arma depois de receber o pulso neutro sustentado
	 * por ~3 s. Ate isso terminar ele ignora qualquer comando, entao nao ha o
	 * que fazer em paralelo. */
	arthas.setupSuction();

	arthas.setupConsole();
	arthas.setupMotors();
	arthas.setupFrontSensor();
	arthas.setupLateralSensors();

	/* O host demora a enumerar o USB CDC; sem esta folga o menu sai antes de
	 * alguem estar escutando e a serial abre em branco. */
	delay(1500);
	arthas.printMenu();
	arthas.beepReady();
}

void loop() {
	/* Avanca a rampa da succao e o buzzer. Precisa rodar TODA iteracao: sem
	 * isso a rampa de desaceleracao congelaria no ponto em que o modo
	 * terminou, e a succao ficaria girando depois do comando de parada. */
	arthas.update();

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
			case ArthasAction::IrCapture:
				arthas.captureIrCodes();
				break;
			case ArthasAction::Calibrate:
				arthas.calibrateFrontSensor();
				break;
			case ArthasAction::ShowStatus:
				arthas.printStatus();
				break;
			case ArthasAction::ShowMenu:
				arthas.printMenu();
				break;
			case ArthasAction::DriveLap:
				arthas.driveLap();
				break;
			case ArthasAction::Chase:
				arthas.chase();
				break;
			case ArthasAction::Stop:
				arthas.stopMotors();
				break;
			case ArthasAction::SensorMaskChanged:
				break;
			default:
				break;
		}
	}
}
