#include <LateralSensor.h>

LateralSensor::LateralSensor(const uint8_t pin, const bool isAnalog):
    pin(pin),
    isAnalog(isAnalog),
    whiteLineLimitValue(2000)   //threshold for analog reading, the sensor goes to 4096
{}

LateralSensor::~LateralSensor() {}

void LateralSensor::setup() {
    pinMode(pin, INPUT);
}

const uint16_t LateralSensor::read() {
    if (isAnalog)
        return analogRead(pin);
    else 
        return digitalRead(pin);
}

const bool LateralSensor::isWhite() {
    if (!isAnalog) {
        return read();
    } else {
        uint16_t sensorValue = read();
        if (sensorValue > whiteLineLimitValue)
            return true;
        else 
            return false;
    }
}