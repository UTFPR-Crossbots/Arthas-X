#include <Arthas.h>

namespace {
	/* Formato do pacote de constantes vindo do app: os índices abaixo vão até
	 * 29, então qualquer coisa mais curta é fragmento de BLE, não pacote. */
	const uint16_t constantsPacketLength = 29;
}

Arthas::Arthas(const uint8_t* leftMotorPins,
               const uint8_t* rightMotorPins,
               const uint8_t frontSensorCommonPin,
               const uint8_t* frontSensorSelectPins,
               const uint8_t numberOfFrontSensors,
               const uint8_t leftLateralSensorPin,
               const uint8_t rightLateralSensorPin,
               const uint8_t suctionEscPin,
               const uint8_t suctionLedcChannel,
               const uint8_t suctionRaceThrottle,
               const int16_t maxspeed,
               const bool invertLeftMotor,
               const bool invertRightMotor):
	console(),
	powertrain(leftMotorPins, rightMotorPins, invertLeftMotor, invertRightMotor),
	frontSensor(frontSensorCommonPin, frontSensorSelectPins, numberOfFrontSensors),
	lateralSensorLeft(leftLateralSensorPin, true),
	lateralSensorRight(rightLateralSensorPin, false),
	suction(suctionEscPin, suctionLedcChannel),
	pid(numberOfFrontSensors),
	maxspeed(maxspeed),
	tempo(0),
	marcas(0),
	reverseRatio(0.5),
	suctionThrottle(suctionRaceThrottle)
{}

Arthas::~Arthas() {
}

/* Comunicação */
Console* Arthas::getConsole() {
	return &console;
}

Bluetooth* Arthas::getBluetooth() {
	return console.getBluetooth();
}

void Arthas::setupConsole() {
	console.setup();
}

const bool Arthas::isInputAvailable() {
	return console.isAvailable();
}

String Arthas::readInput() {
	return console.readInput();
}

bool Arthas::parseValueCommand(const String& input) {
	if (input.startsWith("kp ")) {
		pid.updatePIDConstants(input.substring(3).toDouble(), pid.getI(), pid.getD());
		return true;
	}
	if (input.startsWith("ki ")) {
		pid.updatePIDConstants(pid.getP(), input.substring(3).toDouble(), pid.getD());
		return true;
	}
	if (input.startsWith("kd ")) {
		pid.updatePIDConstants(pid.getP(), pid.getI(), input.substring(3).toDouble());
		return true;
	}
	if (input.startsWith("vel ")) {
		maxspeed = constrain(input.substring(4).toInt(), 0, MotorArthas::maxDutyCycle);
		return true;
	}
	if (input.startsWith("marcas ")) {
		marcas = (uint8_t)input.substring(7).toInt();
		return true;
	}
	if (input.startsWith("suc ")) {
		suctionThrottle = (uint8_t)constrain(input.substring(4).toInt(), 0, 100);
		return true;
	}
	if (input.startsWith("rev ")) {
		reverseRatio = constrain(input.substring(4).toInt(), 0, 100) / 100.0;
		return true;
	}
	if (input.startsWith("off ")) {
		const uint8_t index = (uint8_t)input.substring(4).toInt();
		frontSensor.setSensorEnabled(index, false);
		console.print("Sensor ");
		console.print((int)index);
		console.println(" desabilitado.");
		return true;
	}
	if (input.startsWith("on ")) {
		const uint8_t index = (uint8_t)input.substring(3).toInt();
		frontSensor.setSensorEnabled(index, true);
		console.print("Sensor ");
		console.print((int)index);
		console.println(" habilitado.");
		return true;
	}

	return false;
}

void Arthas::applyCorrection(const int16_t baseSpeed, const double correction) {
	const int16_t minSpeed = -(int16_t)(maxspeed * reverseRatio);

	/* Convenção de sinais, derivada do sensor 0 estar na ESQUERDA da barra:
	 *
	 *   valor alto = preto, valor baixo = branco, e readLineWhite() devolve o
	 *   centroide do branco. Linha à esquerda => posicao baixa => o erro
	 *   (setPoint - posicao) fica POSITIVO.
	 *
	 *   Para virar à esquerda num diferencial, a roda de DENTRO (esquerda)
	 *   desacelera e a de FORA (direita) acelera. Logo correção positiva tem
	 *   de SUBTRAIR da esquerda e SOMAR na direita.
	 *
	 * Se algum dia a barra for montada espelhada (canal 0 do mux na direita),
	 * é esta a linha para trocar. */
	powertrain.motorsDrive(
		constrain(baseSpeed - correction, minSpeed, maxspeed),
		constrain(baseSpeed + correction, minSpeed, maxspeed));
}

ArthasAction Arthas::parseInput() {
	if (!isInputAvailable()) return ArthasAction::None;

	String input = readInput();
	input.trim();

	if (input.length() == 0) return ArthasAction::None;

	if (input == "a") {
		return ArthasAction::Calibrate;
	}
	else if (input == "b") {
		return ArthasAction::DriveLap;
	}
	else if (input == "c") {
		return ArthasAction::Stop;
	}
	else if (input == "d") {
		return ArthasAction::LateralSensorTest;
	}
	else if (input == "e") {
		return ArthasAction::Chase;
	}
	else if (input == "f") {
		return ArthasAction::BothMotorsTest;
	}
	else if (input == "g") {
		return ArthasAction::FrontSensorTestWhileLine;
	}
	else if (input == "h") {
		return ArthasAction::FrontSensorTest;
	}
	else if (input == "i") {
		return ArthasAction::FrontSensorTestAnalogRead;
	}
	else if (input == "j") {
		return ArthasAction::LeftMotorTest;
	}
	else if (input == "k") {
		return ArthasAction::RightMotorTest;
	}
	else if (input == "l") {
		return ArthasAction::SuctionTest;
	}
	else if (input == "s") {
		return ArthasAction::ShowStatus;
	}
	else if (input == "?" || input == "m") {
		return ArthasAction::ShowMenu;
	}
	else if(input == "Cliente Conectado!"){
		return ArthasAction::BluetoothConnected;
	}
	else if (input ==  "Cliente Desconectado!") {
		return ArthasAction::BluetoothDisconnected;
	}
	/* Comandos com valor ("kp 0.35", "vel 120"), pensados para a serial. */
	else if (parseValueCommand(input)) {
		return ArthasAction::UpdateConstants;
	}
	// This code is ugly, it should not do the functions calls here but the input from the app is trash so this is the best way
	// TODO: Change the app to a better way of sending PIDs Constants
	else {
		/* Sem esta guarda, um fragmento de BLE cairia aqui e sobrescreveria
		 * Kp/Kd com lixo sem ninguém perceber. O SPP não tinha esse problema
		 * porque readString() esperava o pacote inteiro. */
		if (input.length() < constantsPacketLength) {
			console.println("Comando desconhecido: '" + input + "'. Digite ? para o menu.");
			return ArthasAction::None;
		}

		String rawKp = input.substring(2,9);
		double kP = rawKp.toDouble();

		String rawKi = input.substring(10,17);
		marcas = (uint8_t)rawKi.toInt();

		String rawKd = input.substring(18,25);
		double kD = rawKd.toDouble();

		pid.updatePIDConstants(kP, 0, kD);

		String rawSpeed = input.substring(26,29);
		uint8_t speed = (uint8_t)rawSpeed.toInt();
		maxspeed = speed;

		return ArthasAction::UpdateConstants;
	}
}

void Arthas::print(const String msg) {
	console.print(msg);
}

void Arthas::print(const int msg) {
	console.print(msg);
}

void Arthas::println(const String msg) {
	console.println(msg);
}

void Arthas::println(const int msg) {
	console.println(msg);
}

void Arthas::turnOffBluetooth() {
	console.turnOffBluetooth();
}

/* Line Sensor */
void Arthas::setupFrontSensor() {
	frontSensor.setup();
}

void Arthas::calibrateFrontSensor() {
	console.println("//---Calibration started---//");
	console.println("Passe a barra sobre a linha e o fundo por 7 s...");
	frontSensor.calibrate();
	console.println("//---Calibration finished---//");
	reportDisabledSensors();
}

const uint16_t Arthas::readLineWhiteFrontSensor() {
	return frontSensor.readLineWhite();
}

void Arthas::testFrontSensor() {
	const uint8_t numberOfSensors = frontSensor.getNumberOfSensors();
	uint16_t sensorValues[numberOfSensors];
	unsigned long lastPrint = 0;

	if (!frontSensor.isCalibrated()) {
		console.println("AVISO: barra ainda nao calibrada (comando 'a').");
	}
	console.println("Calibrado (0-1000) | posicao. 'c' para parar.");

	/* Sai no comando de parada — sem isso o bring-up trava aqui para sempre. */
	while (parseInput() != ArthasAction::Stop) {
		if (millis() - lastPrint > 300) {
			frontSensor.readCalibrated(sensorValues);
			for(uint8_t i = 0; i < numberOfSensors; i++) {
				/* Desabilitado aparece como x: ele nao entra na posicao. */
				if (frontSensor.isSensorEnabled(i)) console.print(sensorValues[i]);
				else console.print("x");
				console.print("|");
			}
			console.print(" pos=");
			console.println(frontSensor.readLineWhite());
			lastPrint = millis();
		}
	}
}

void Arthas::testFrontSensorAnalogRead() {
	const uint8_t numberOfSensors = frontSensor.getNumberOfSensors();
	uint16_t sensorValues[numberOfSensors];
	unsigned long lastPrint = 0;

	console.println("Cru do ADC (0-4095). Branco = valor MENOR. 'c' para parar.");

	while (parseInput() != ArthasAction::Stop) {
		if (millis() - lastPrint > 300) {
			/* Passa pelo mux: não existe mais um GPIO por sensor. */
			frontSensor.readRaw(sensorValues);
			for(uint8_t i = 0; i < numberOfSensors; i++) {
				console.print(sensorValues[i]);
				console.print("|");
			}
			console.println();
			lastPrint = millis();
		}
	}
}

void Arthas::testFrontSensorWhiteLine() {
	uint16_t whiteLineValue = frontSensor.readLineWhite();
	console.print("White Line Value: ");
	console.println(whiteLineValue);
}

/* Lateral Sensors */
void Arthas::setupLateralSensors() {
	lateralSensorLeft.setup();
	lateralSensorRight.setup();
}

void Arthas::testLateralSensor() {
	unsigned long lastPrint = 0;

	console.println("Laterais: esq analogico (0-4095), dir digital. 'c' para parar.");

	while (parseInput() != ArthasAction::Stop) {
		if (millis() - lastPrint > 300) {
			console.print("Esq: ");
			console.print((int)lateralSensorLeft.read());
			console.print(lateralSensorLeft.isWhite() ? " [BRANCO] " : " [PRETO]  ");
			console.print("| Dir: ");
			console.print((int)lateralSensorRight.read());
			console.println(lateralSensorRight.isWhite() ? " [BRANCO]" : " [PRETO]");
			lastPrint = millis();
		}
	}
}

/* Suction */
void Arthas::setupSuction() {
	suction.setup();
}

void Arthas::testSuction() {
	if (!suction.isArmed()) {
		console.println("ESC ainda armando, aguarde...");
		/* delay() cede o processador: um spin vazio aqui starva a idle task
		 * e derruba o watchdog. */
		while (!suction.isArmed()) delay(1);
	}

	console.print("Succao a ");
	console.print((int)suctionThrottle);
	console.println("%. 'c' para parar.");

	suction.setTarget(suctionThrottle);

	unsigned long lastPrint = 0;
	while (parseInput() != ArthasAction::Stop) {
		suction.update();

		if (millis() - lastPrint > 500) {
			console.print("throttle: ");
			console.print((int)suction.getCurrent());
			console.println("%");
			lastPrint = millis();
		}
	}

	suction.stop();
	console.println("Succao parada.");
}

/* Motors */
void Arthas::setupMotors() {
	powertrain.setup();
}

void Arthas::stopMotors() {
	powertrain.stopMotors();
	/* Parar e parar tudo: a succao nao pode continuar girando apos o 'c'. */
	suction.stop();
}

void Arthas::testLeftMotor() {
	console.print("Motor esquerdo a ");
	console.println(maxspeed);
	powertrain.leftMotorDrive(maxspeed);
}

void Arthas::testRightMotor() {
	console.print("Motor direito a ");
	console.println(maxspeed);
	powertrain.rightMotorDrive(maxspeed);
}

void Arthas::testBothMotors() {
	console.println("Testando os dois motores! 'c' para parar.");
	testLeftMotor();
	testRightMotor();
}

/* Constants */
void Arthas::printMenu() {
	console.println();
	console.println("========================================");
	console.println("   ARTHAS - menu de testes");
	console.println("========================================");
	console.println(" SENSORES");
	console.println("  i   barra frontal - cru do ADC (0-4095)");
	console.println("  h   barra frontal - calibrado (0-1000)");
	console.println("  g   barra frontal - posicao (1 leitura)");
	console.println("  a   calibrar a barra frontal (7 s)");
	console.println("  d   sensores laterais (esq + dir)");
	console.println("  off <n> / on <n>   ignorar/voltar um sensor da barra");
	console.println();
	console.println(" MOTORES  (robo suspenso!)");
	console.println("  j   testar motor esquerdo");
	console.println("  k   testar motor direito");
	console.println("  f   testar os dois motores");
	console.println("  l   testar succao (ESC)");
	console.println("  c   PARAR (sai dos testes continuos)");
	console.println();
	console.println(" MODOS");
	console.println("  e   chase - segue a linha ate receber 'c'");
	console.println("  b   volta - com contagem de marcas");
	console.println();
	console.println(" CONSTANTES");
	console.println("  kp <valor>       ex: kp 50   (erro normalizado -1..1)");
	console.println("  ki <valor>");
	console.println("  kd <valor>");
	console.println("  vel <0-255>      velocidade maxima");
	console.println("  marcas <n>       marcas para fechar a volta");
	console.println("  rev <0-100>      re da roda interna, % do maxspeed");
	console.println("  suc <0-100>      throttle da succao na corrida");
	console.println("  s                mostrar estado atual");
	console.println();
	console.println("  ?   mostrar este menu");
	console.println("========================================");
}

void Arthas::reportDisabledSensors() {
	const uint8_t numberOfSensors = frontSensor.getNumberOfSensors();
	bool anyDisabled = false;

	for (uint8_t i = 0; i < numberOfSensors; i++) {
		if (frontSensor.isSensorEnabled(i)) continue;

		if (!anyDisabled) {
			console.print("Sensores DESABILITADOS (fora da conta da linha): ");
			anyDisabled = true;
		} else {
			console.print(", ");
		}
		console.print((int)i);
	}

	if (anyDisabled) {
		console.print(" | ativos: ");
		console.print((int)frontSensor.getEnabledCount());
		console.print("/");
		console.println((int)numberOfSensors);
	}
}

void Arthas::printStatus() {
	console.println("--- Estado ---");
	printPID();
	printMaxSpeed();
	console.print("marcas: ");
	console.println((int)marcas);
	console.print("barra calibrada: ");
	console.println(frontSensor.isCalibrated() ? "sim" : "nao");
	reportDisabledSensors();
	console.print("succao (corrida): ");
	console.print((int)suctionThrottle);
	console.print("% | saindo agora: ");
	console.print((int)suction.getCurrent());
	console.println("%");
	console.print("re da roda interna: ");
	console.print((int)(reverseRatio * 100));
	console.println("% do maxspeed");
	console.print("BLE conectado: ");
	console.println(console.isBluetoothConnected() ? "sim" : "nao");
}

void Arthas::printMaxSpeed() {
	console.print("maxSpeed: ");
	console.println(maxspeed);
}

void Arthas::printPID() {
	console.println("PID sobre erro normalizado (-1..+1); Kp na ordem do maxspeed.");
	console.print("P: ");
	console.printDoubleln(pid.getP());
	console.print("I: ");
	console.printDoubleln(pid.getI());
	console.print("D: ");
	console.printDoubleln(pid.getD());
}

/* Modes */
void Arthas::driveLap(const uint8_t markers) {
	println("//---Comecando primeira volta---//");

	/* A succao entra junto com a corrida; update() faz a rampa dentro da
	 * malha, sem bloquear o controle da linha. */
	suction.setTarget(suctionThrottle);
	// println("//---Turning off the bluetooth---//");
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

		suction.update();

		uint16_t linePosition = frontSensor.readLineWhite();

		double correction = pid.calculateCorrection(linePosition);

		applyCorrection(speed, correction);

		/* Marcas contadas pelo lateral esquerdo: é o analógico, com limiar
		 * ajustável. O direito está num pino sem ADC e só tem limiar fixo. */
		if (lateralSensorLeft.isWhite()) {
			if (isNewMarker) {
				markerCount++;
				console.println(markerCount);
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
	console.println("--- Starting! ---");
	bool stop = false;

	suction.setTarget(suctionThrottle);

	while (!stop) {
		ArthasAction action = parseInput();

		suction.update();

		uint16_t linePosition = frontSensor.readLineWhite();

		double correction = pid.calculateCorrection(linePosition);

		applyCorrection(maxspeed, correction);

		if (action == ArthasAction::Stop) {
			stopMotors();
			stop = true;
		}
		else if (action == ArthasAction::UpdateConstants) {
			console.println("Updated Constants!");
			this->printPID();
		}
	}

	console.println("--- Stopped! ---");
}
