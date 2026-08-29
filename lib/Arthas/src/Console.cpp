#include <Console.h>

namespace {
    /* Trava o acumulador caso chegue lixo sem terminador de linha. */
    const uint16_t maxLineLength = 128;

    const size_t maxQueuedLines = 16;
}

Console::Console(const uint8_t irReceivePin) :
	ir(irReceivePin),
	serialEnabled(false),
	irEnabled(true),
	serialAccumulator("")
{}

Console::~Console() {}

void Console::setup(const unsigned long baudRate) {
	Serial.begin(baudRate);     // no USB CDC o baud rate é ignorado
	serialEnabled = true;

	ir.setup();
}

IrRemote* Console::getIr() {
	return &ir;
}

bool Console::consumeUnknownIrCode() {
	return ir.consumeUnknownCode();
}

/* --- Entrada --- */

void Console::pollSerial() {
	if (!serialEnabled) return;

	while (Serial.available() > 0) {
		const char character = (char)Serial.read();

		if (character == '\n' || character == '\r') {
			if (serialAccumulator.length() > 0) {
				if (lines.size() >= maxQueuedLines) lines.pop_front();
				lines.push_back(serialAccumulator);
				serialAccumulator = "";
			}
		}
		else if (serialAccumulator.length() < maxLineLength) {
			serialAccumulator += character;
		}
	}
}

void Console::setIrEnabled(const bool enabled) {
	irEnabled = enabled;
}

void Console::pollIr() {
	if (!irEnabled) return;

	const char command = ir.readCommand();
	if (command == 0) return;

	if (lines.size() >= maxQueuedLines) lines.pop_front();
	lines.push_back(String(command));
}

bool Console::isAvailable() {
	pollSerial();
	pollIr();
	return !lines.empty();
}

String Console::readInput() {
	pollSerial();
	pollIr();

	if (lines.empty()) return "";

	const String line = lines.front();
	lines.pop_front();
	return line;
}

/* --- Saída: só a serial; o controle remoto não tem canal de volta --- */

void Console::print(const String msg) {
	if (serialEnabled) Serial.print(msg);
}

void Console::print(const int msg) {
	if (serialEnabled) Serial.print(msg);
}

void Console::println(const String msg) {
	if (serialEnabled) Serial.println(msg);
}

void Console::println(const int msg) {
	if (serialEnabled) Serial.println(msg);
}

void Console::println(const uint16_t msg) {
	if (serialEnabled) Serial.println(msg);
}

void Console::printDoubleln(const double msg) {
	if (serialEnabled) Serial.println(msg, 4);
}
