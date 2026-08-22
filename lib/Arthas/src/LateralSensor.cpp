#include <LateralSensor.h>

LateralSensor::LateralSensor(const uint8_t pin, const bool isAnalog,
                             const uint16_t whiteLineLimitValue, const bool activeLow):
    pin(pin),
    isAnalog(isAnalog),
    whiteLineLimitValue(whiteLineLimitValue),   // leitura vai de 0 a 4095 (12 bits)
    activeLow(activeLow)
{}

LateralSensor::~LateralSensor() {}

void LateralSensor::setup() {
    pinMode(pin, INPUT);

    if (isAnalog) {
        analogReadResolution(12);
        analogSetPinAttenuation(pin, ADC_11db);
    }
}

const uint16_t LateralSensor::read() {
    if (isAnalog)
        return analogRead(pin);
    else
        return digitalRead(pin);
}

const bool LateralSensor::isWhite() {
    const uint16_t sensorValue = read();

    if (!isAnalog) {
        return activeLow ? (sensorValue == LOW) : (sensorValue == HIGH);
    }

    /* Com pull-up, mais reflexão (branco) puxa a tensão para baixo. */
    return activeLow ? (sensorValue < whiteLineLimitValue)
                     : (sensorValue > whiteLineLimitValue);
}
