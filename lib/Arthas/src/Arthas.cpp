#include <Arthas.h>
#include <Tuning.h>

Arthas::Arthas(const uint8_t* leftMotorPins,
               const uint8_t* rightMotorPins,
               const uint8_t frontSensorCommonPin,
               const uint8_t* frontSensorSelectPins,
               const uint8_t numberOfFrontSensors,
               const uint8_t leftLateralSensorPin,
               const uint8_t rightLateralSensorPin,
               const uint8_t suctionEscPin,
               const uint8_t suctionPwmTimer,
               const uint8_t irReceiverPin,
               const uint8_t buzzerPin,
               const bool buzzerActiveLow,
               const bool invertLeftMotor,
               const bool invertRightMotor):
	console(irReceiverPin),
	buzzer(buzzerPin, buzzerActiveLow),
	powertrain(leftMotorPins, rightMotorPins, invertLeftMotor, invertRightMotor),
	frontSensor(frontSensorCommonPin, frontSensorSelectPins, numberOfFrontSensors),
	lateralSensorLeft(leftLateralSensorPin, true),
	lateralSensorRight(rightLateralSensorPin, false),
	suction(suctionEscPin, suctionPwmTimer),
	pid(numberOfFrontSensors, tuning::kP, tuning::kI, tuning::kD),
	maxspeed(tuning::maxSpeed),
	marcas(tuning::markersPerLap),
	reverseRatio(tuning::reverseRatio),
	suctionThrottle(tuning::suctionThrottle)
{}

Arthas::~Arthas() {
}

void Arthas::update() {
	suction.update();
	buzzer.update();
}

/* Comunicação */
Console* Arthas::getConsole() {
	return &console;
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

bool Arthas::parseSensorMaskCommand(const String& input) {
	const bool disable = input.startsWith("off ");
	const bool enable = input.startsWith("on ");

	if (!disable && !enable) return false;

	const uint8_t index = (uint8_t)input.substring(disable ? 4 : 3).toInt();
	frontSensor.setSensorEnabled(index, enable);

	console.print("Sensor ");
	console.print((int)index);
	console.println(enable ? " habilitado." : " desabilitado.");
	return true;
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
	const bool hasInput = isInputAvailable();

	/* Código IR fora da tabela: avisa por som. O controle remoto não tem
	 * canal de volta, então sem o buzzer não haveria como saber na pista que
	 * a tecla chegou mas não significa nada. */
	if (console.consumeUnknownIrCode()) {
		buzzer.beepLong();
		console.println("Codigo IR desconhecido. Use 'x' para capturar.");
	}

	if (!hasInput) return ArthasAction::None;

	String input = readInput();
	input.trim();

	if (input.length() == 0) return ArthasAction::None;

	ArthasAction action = ArthasAction::None;

	if (input == "a")      action = ArthasAction::Calibrate;
	else if (input == "b") action = ArthasAction::DriveLap;
	else if (input == "c") action = ArthasAction::Stop;
	else if (input == "d") action = ArthasAction::LateralSensorTest;
	else if (input == "e") action = ArthasAction::Chase;
	else if (input == "f") action = ArthasAction::BothMotorsTest;
	else if (input == "g") action = ArthasAction::FrontSensorTestWhileLine;
	else if (input == "h") action = ArthasAction::FrontSensorTest;
	else if (input == "i") action = ArthasAction::FrontSensorTestAnalogRead;
	else if (input == "j") action = ArthasAction::LeftMotorTest;
	else if (input == "k") action = ArthasAction::RightMotorTest;
	else if (input == "l") action = ArthasAction::SuctionTest;
	else if (input == "x") action = ArthasAction::IrCapture;
	else if (input == "s") action = ArthasAction::ShowStatus;
	else if (input == "?" || input == "m") action = ArthasAction::ShowMenu;
	else if (parseSensorMaskCommand(input)) action = ArthasAction::SensorMaskChanged;
	else {
		console.println("Comando desconhecido: '" + input + "'. Digite ? para o menu.");
		return ArthasAction::None;
	}

	/* Confirmação sonora: dois beeps para parada, um para o resto. */
	buzzer.beep(action == ArthasAction::Stop ? 2 : 1);

	return action;
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
	buzzer.beep(3);
}

const uint16_t Arthas::readLineWhiteFrontSensor() {
	return frontSensor.readLineWhite();
}

void Arthas::testFrontSensor() {
	const uint8_t numberOfSensors = frontSensor.getNumberOfSensors();
	uint16_t sensorValues[numberOfSensors];
	unsigned long lastPrint = 0;

	if (!frontSensor.isCalibrated()) {
		console.println("AVISO: barra ainda nao calibrada (tecla 1 / comando 'a').");
	}
	console.println("Calibrado (0-1000) | posicao. Tecla 0 (ou 'c') para parar.");

	/* Sai no comando de parada — sem isso o bring-up trava aqui para sempre. */
	while (parseInput() != ArthasAction::Stop) {
		update();

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

	console.println("Cru do ADC (0-4095). Branco = valor MENOR. Tecla 0 para parar.");

	while (parseInput() != ArthasAction::Stop) {
		update();

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

	console.println("Laterais: esq analogico (0-4095), dir digital. Tecla 0 para parar.");

	while (parseInput() != ArthasAction::Stop) {
		update();

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
	console.print("Succao a ");
	console.print((int)suctionThrottle);
	console.println("%. Tecla 0 (ou 'c') para parar.");

	suction.setTarget(suctionThrottle);

	unsigned long lastPrint = 0;
	while (parseInput() != ArthasAction::Stop) {
		update();

		if (millis() - lastPrint > 500) {
			console.print("throttle: ");
			console.print((int)suction.getCurrent());
			console.print("% (");
			console.print((int)suction.getCurrentPulse());
			console.println(" us)");
			lastPrint = millis();
		}
	}

	suction.stop();
	console.println("Succao descendo em rampa...");
}

/* Buzzer */
void Arthas::setupBuzzer() {
	buzzer.setup();
}

void Arthas::beepReady() {
	buzzer.beep(3);
}

/* Infravermelho */
void Arthas::captureIrCodes() {
	IrRemote* remote = console.getIr();

	/* Enquanto captura, os frames não podem virar comando na fila: eles
	 * precisam chegar crus aqui. */
	console.setIrEnabled(false);

	console.println();
	console.println("=== Captura de codigos IR ===");
	console.println("Aperte as teclas 0-9 do controle, uma de cada vez.");
	console.println("Anote os codigos e preencha lib/Arthas/include/IrCodes.h.");
	console.println("Digite 'c' na serial para sair.");
	console.println();

	String protocol;
	uint64_t code = 0;
	uint16_t bits = 0;

	while (parseInput() != ArthasAction::Stop) {
		update();

		if (remote->captureRaw(protocol, code, bits)) {
			console.print("protocolo=");
			console.print(protocol);
			console.print("  bits=");
			console.print((int)bits);
			console.print("  codigo=0x");
			console.println(String((uint32_t)(code >> 32), HEX) +
			                String((uint32_t)code, HEX));
			buzzer.beep(1);
		}
	}

	console.setIrEnabled(true);
	console.println("=== Fim da captura ===");
}

/* Motors */
void Arthas::setupMotors() {
	powertrain.setup();
}

void Arthas::stopMotors() {
	powertrain.stopMotors();
	/* Parar e parar tudo. As rodas travam na hora; a succao desce em rampa,
	 * o que so acontece porque update() continua sendo chamado pelo loop()
	 * principal depois que este comando retorna. */
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
	console.println("Testando os dois motores! Tecla 0 (ou 'c') para parar.");
	testLeftMotor();
	testRightMotor();
}

/* Info */
void Arthas::printMenu() {
	console.println();
	console.println("========================================");
	console.println("   ARTHAS - menu de comandos");
	console.println("========================================");
	console.println(" CONTROLE IR (teclas numericas)  |  serial");
	console.println("  1  calibrar a barra (7 s)      |  a");
	console.println("  2  chase - segue a linha       |  e");
	console.println("  3  volta com contagem de marcas|  b");
	console.println("  4  barra frontal - cru do ADC  |  i");
	console.println("  5  barra frontal - calibrado   |  h");
	console.println("  6  sensores laterais           |  d");
	console.println("  7  motor esquerdo              |  j");
	console.println("  8  motor direito               |  k");
	console.println("  9  succao (ESC)                |  l");
	console.println("  0  >>> PARAR <<<               |  c");
	console.println();
	console.println(" SO NA SERIAL");
	console.println("  f   testar os dois motores");
	console.println("  g   barra frontal - posicao (1 leitura)");
	console.println("  x   capturar codigos do controle IR");
	console.println("  off <n> / on <n>   ignorar/voltar um sensor da barra");
	console.println("  s   mostrar estado atual");
	console.println("  ?   mostrar este menu");
	console.println();
	console.println(" Constantes (PID, velocidade, marcas) ficam em Tuning.h:");
	console.println(" ajustar no codigo e regravar.");
	console.println("========================================");

	if (!console.getIr()->isConfigured()) {
		console.println();
		console.println("!!! CONTROLE IR AINDA NAO CONFIGURADO !!!");
		console.println("A tabela em IrCodes.h esta zerada, entao o controle");
		console.println("nao responde. Rode 'x', anote os codigos das teclas");
		console.println("0-9, preencha IrCodes.h e regrave.");
		console.println();
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
	console.print("controle IR: ");
	console.println(console.getIr()->isConfigured() ? "configurado"
	                                                : "NAO configurado (ver 'x')");
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
void Arthas::driveLap() {
	println("//---Comecando primeira volta---//");

	/* A succao entra junto com a corrida; update() faz a rampa dentro da
	 * malha, sem bloquear o controle da linha. */
	suction.setTarget(suctionThrottle);

	bool lapFinishedFlag = false;
	bool isNewMarker = true;
	uint8_t markerCount = 0;
	unsigned long currentTime = 0;
	unsigned long previousTime = 0;
	uint8_t acelerationInterval = 2;	// 2 miliseconds
	unsigned long finishTime = 0;
	uint8_t speed = 0;

	bool aborted = false;

	while(!aborted && (!lapFinishedFlag || currentTime - finishTime < 200)) {
		currentTime = millis();
		if (speed < maxspeed) {
			if (currentTime - previousTime >= acelerationInterval) {
				speed++;
				previousTime = currentTime;
			}
		}

		/* A tecla 0 tem de abortar a volta. Sem isto o robô só pararia ao
		 * completar as marcas, e o comando de parada — que na pista é o botão
		 * de emergência — não valeria em um dos dois modos de corrida. */
		if (parseInput() == ArthasAction::Stop) {
			aborted = true;
			console.println("//---Volta abortada---//");
		}

		update();

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

		update();

		uint16_t linePosition = frontSensor.readLineWhite();

		double correction = pid.calculateCorrection(linePosition);

		applyCorrection(maxspeed, correction);

		if (action == ArthasAction::Stop) {
			stopMotors();
			stop = true;
		}
	}

	console.println("--- Stopped! ---");
}
