#include <FrontSensor.h>

namespace {
    /* Tempo para o mux comutar e o RC da entrada assentar antes da conversão.
     * Se dois canais vizinhos parecerem mudar juntos no teste, aumentar. */
    const uint8_t channelSettleTime_us = 10;

    /* Amostras por rodada de calibração. Guardar o menor dos máximos e o maior
     * dos mínimos rejeita picos isolados de ruído. */
    const uint8_t calibrationSamples = 10;

    const uint16_t adcMaxValue = 4095;      // 12 bits
}

FrontSensor::FrontSensor(const uint8_t commonPin, const uint8_t* selectPins, const uint8_t numberOfSensors):
    commonPin(commonPin),
    numberOfSensors(numberOfSensors > maxSensors ? maxSensors : numberOfSensors),
    calibrated(false),
    lastLinePosition(0)
{
    for (uint8_t i = 0; i < 4; i++) {
        this->selectPins[i] = selectPins[i];
    }

    for (uint8_t i = 0; i < maxSensors; i++) {
        calibrationMin[i] = adcMaxValue;
        calibrationMax[i] = 0;
    }

    lastLinePosition = (this->numberOfSensors - 1) * 500;
}

FrontSensor::~FrontSensor() {}

void FrontSensor::setup() {
    for (uint8_t i = 0; i < 4; i++) {
        pinMode(selectPins[i], OUTPUT);
        digitalWrite(selectPins[i], LOW);
    }

    pinMode(commonPin, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(commonPin, ADC_11db);   // faixa cheia de 0 a ~3,3 V
}

void FrontSensor::selectChannel(const uint8_t channel) {
    for (uint8_t bit = 0; bit < 4; bit++) {
        digitalWrite(selectPins[bit], (channel >> bit) & 0x01);
    }
    delayMicroseconds(channelSettleTime_us);
}

void FrontSensor::readRaw(uint16_t* sensorValues) {
    for (uint8_t i = 0; i < numberOfSensors; i++) {
        selectChannel(i);
        sensorValues[i] = analogRead(commonPin);
    }
}

void FrontSensor::calibrate() {
    const uint16_t calibrationTime_ms = 7000;   // mesma janela do firmware antigo
    const unsigned long timeStart = millis();

    uint16_t sensorValues[maxSensors];
    uint16_t roundMin[maxSensors];
    uint16_t roundMax[maxSensors];

    while (millis() - timeStart < calibrationTime_ms) {
        for (uint8_t sample = 0; sample < calibrationSamples; sample++) {
            readRaw(sensorValues);

            for (uint8_t i = 0; i < numberOfSensors; i++) {
                if (sample == 0 || sensorValues[i] < roundMin[i]) roundMin[i] = sensorValues[i];
                if (sample == 0 || sensorValues[i] > roundMax[i]) roundMax[i] = sensorValues[i];
            }
        }

        for (uint8_t i = 0; i < numberOfSensors; i++) {
            /* Só sobe o máximo se até o menor das amostras ficou acima dele. */
            if (roundMin[i] > calibrationMax[i]) calibrationMax[i] = roundMin[i];
            /* Só desce o mínimo se até o maior das amostras ficou abaixo dele. */
            if (roundMax[i] < calibrationMin[i]) calibrationMin[i] = roundMax[i];
        }
    }

    calibrated = true;
}

void FrontSensor::readCalibrated(uint16_t* sensorValues) {
    readRaw(sensorValues);

    for (uint8_t i = 0; i < numberOfSensors; i++) {
        const uint16_t minimum = calibrationMin[i];
        const uint16_t maximum = calibrationMax[i];

        if (maximum <= minimum) {
            sensorValues[i] = 0;
            continue;
        }

        const int32_t scaled = ((int32_t)sensorValues[i] - minimum) * 1000 / (maximum - minimum);
        sensorValues[i] = constrain(scaled, 0, 1000);
    }
}

uint16_t FrontSensor::readLineWhite() {
    uint16_t sensorValues[maxSensors];
    readCalibrated(sensorValues);

    bool onLine = false;
    uint32_t weightedSum = 0;
    uint32_t sum = 0;

    for (uint8_t i = 0; i < numberOfSensors; i++) {
        /* Linha branca em fundo preto: o sensor sobre a linha lê MENOS. */
        const uint16_t value = 1000 - sensorValues[i];

        if (value > 200) onLine = true;

        if (value > 50) {
            weightedSum += (uint32_t)value * (i * 1000);
            sum += value;
        }
    }

    if (!onLine) {
        /* Linha perdida: trava no extremo por onde ela saiu, senão o robô
         * "esquece" a curva e sai reto. */
        const uint16_t middle = (numberOfSensors - 1) * 1000 / 2;
        return lastLinePosition < middle ? 0 : (numberOfSensors - 1) * 1000;
    }

    lastLinePosition = weightedSum / sum;
    return lastLinePosition;
}

uint8_t FrontSensor::getNumberOfSensors() const {
    return numberOfSensors;
}

bool FrontSensor::isCalibrated() const {
    return calibrated;
}
