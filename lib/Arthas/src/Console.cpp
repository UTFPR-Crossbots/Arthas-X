#include <Console.h>

namespace {
    /* Trava o acumulador caso chegue lixo sem terminador de linha.
     * O maior comando válido é o pacote de constantes do app, com 29 bytes. */
    const uint16_t maxLineLength = 128;

    const size_t maxQueuedLines = 16;
}

Console::Console() :
	bluetooth(),
	serialEnabled(false),
	serialAccumulator("")
{}

Console::~Console() {}

void Console::setup(const unsigned long baudRate) {
	Serial.begin(baudRate);     // no USB CDC o baud rate é ignorado
	serialEnabled = true;

	bluetooth.setup();
}

Bluetooth* Console::getBluetooth() {
	return &bluetooth;
}

bool Console::isBluetoothConnected() {
	return bluetooth.isConnected();
}

void Console::turnOffBluetooth() {
	bluetooth.turnOff();
}

/* --- Entrada --- */

void Console::pollSerial() {
	if (!serialEnabled) return;

	while (Serial.available() > 0) {
		const char character = (char)Serial.read();

		if (character == '\n' || character == '\r') {
			if (serialAccumulator.length() > 0) {
				if (serialLines.size() >= maxQueuedLines) serialLines.pop_front();
				serialLines.push_back(serialAccumulator);
				serialAccumulator = "";
			}
		}
		else if (serialAccumulator.length() < maxLineLength) {
			serialAccumulator += character;
		}
	}
}

bool Console::isAvailable() {
	pollSerial();
	return !serialLines.empty() || bluetooth.isAvailable();
}

String Console::readInput() {
	pollSerial();

	if (!serialLines.empty()) {
		const String line = serialLines.front();
		serialLines.pop_front();
		return line;
	}

	return bluetooth.readSerialInput();
}

/* --- Saída: espelhada nos dois canais --- */

void Console::print(const String msg) {
	if (serialEnabled) Serial.print(msg);
	bluetooth.print(msg);
}

void Console::print(const int msg) {
	if (serialEnabled) Serial.print(msg);
	bluetooth.print(msg);
}

void Console::println(const String msg) {
	if (serialEnabled) Serial.println(msg);
	bluetooth.println(msg);
}

void Console::println(const int msg) {
	if (serialEnabled) Serial.println(msg);
	bluetooth.println(msg);
}

void Console::println(const uint16_t msg) {
	if (serialEnabled) Serial.println(msg);
	bluetooth.println(msg);
}

void Console::printDoubleln(const double msg) {
	if (serialEnabled) Serial.println(msg, 4);
	bluetooth.printDoubleln(msg);
}
