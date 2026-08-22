#include <Powertrain.h>

Powertrain::Powertrain(const uint8_t* leftMotorPins, const uint8_t* rightMotorPins,
                       const bool invertLeft, const bool invertRight):
    leftMotor(leftMotorPins[0], leftMotorPins[1], 0, invertLeft),
    rightMotor(rightMotorPins[0], rightMotorPins[1], 1, invertRight)
{}

Powertrain::~Powertrain() {}

void Powertrain::setup() {
    leftMotor.setup();
    rightMotor.setup();
}

void Powertrain::leftMotorDrive(const int16_t speed) {
    leftMotor.drive(speed);
}

void Powertrain::rightMotorDrive(const int16_t speed) {
    rightMotor.drive(speed);
}

void Powertrain::motorsDrive(const int16_t leftMotorSpeed, const int16_t rightMotorSpeed) {
    leftMotor.drive(leftMotorSpeed);
    rightMotor.drive(rightMotorSpeed);
}

void Powertrain::stopMotors() {
	leftMotor.drive(0);
	rightMotor.drive(0);
}
