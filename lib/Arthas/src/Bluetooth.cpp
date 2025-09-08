#include <Bluetooth.h>

Bluetooth::Bluetooth() : SerialBT() {}

Bluetooth::~Bluetooth() {}

void Bluetooth::setup() {
	SerialBT.begin("Bluetooth-BT3");
}

bool Bluetooth::isAvailable() {
	return SerialBT.available();
}

String Bluetooth::readSerialInput() {
	return SerialBT.readString();
}

void Bluetooth::print(const String msg) {
    SerialBT.print(msg);
}

void Bluetooth::print(const int msg) {
    SerialBT.print(msg);
}

void Bluetooth::println(const String msg) {
	SerialBT.println(msg);
}

void Bluetooth::println(const int msg) {
    SerialBT.println(msg);
}

void Bluetooth::println(const uint16_t msg) {
	SerialBT.println(msg);
}

void Bluetooth::printDoubleln(const double msg) {
	SerialBT.println(msg);
}

void Bluetooth::turnOff() {
	SerialBT.flush();
	SerialBT.disconnect();
	SerialBT.end();
}