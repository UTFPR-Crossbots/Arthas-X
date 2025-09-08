#include <FrontSensor.h>

FrontSensor::FrontSensor(const uint8_t numberOfSensors, const uint8_t* pinsArray, const bool isAnalog):
    numberOfSensors(numberOfSensors),
    pins(pinsArray),
    isAnalog(isAnalog),
    frontSensor()
{}

FrontSensor::~FrontSensor() {
    pins = nullptr;
}

void FrontSensor::setup() {
    if (isAnalog)
        frontSensor.setTypeAnalog();
    else
	    frontSensor.setTypeRC();

	frontSensor.setSensorPins(pins, numberOfSensors);
	frontSensor.setTimeout(1000);
}

void FrontSensor::calibrate() {
	unsigned long timeStart = millis();
	const uint16_t calibrationTime_ms = 7000; // 7 seconds for calibration
	while(millis() - timeStart < calibrationTime_ms) {
		frontSensor.calibrate();
	}
}

uint16_t FrontSensor::readLineWhite() {
    uint16_t sensorValues[numberOfSensors];
    return frontSensor.readLineWhite(sensorValues);
}

void FrontSensor::readCalibrated(uint16_t* sensorValues) {
    frontSensor.readCalibrated(sensorValues);
}

uint8_t FrontSensor::getNumberOfSensors() const {
    return numberOfSensors;
}
