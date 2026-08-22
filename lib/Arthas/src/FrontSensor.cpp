#include <FrontSensor.h>

namespace {
    /* Tempo para o mux comutar e o RC da entrada assentar antes da conversão.
     * Se dois canais vizinhos parecerem mudar juntos no teste, aumentar. */
    const uint8_t channelSettleTime_us = 10;

    /* Amostras por rodada de calibração. Guardar o menor dos máximos e o maior
     * dos mínimos rejeita picos isolados de ruído. */
    const uint8_t calibrationSamples = 10;

    const uint16_t adcMaxValue = 4095;      // 12 bits

    /* Faixa mínima entre o branco e o preto para o sensor ser considerado vivo.
     * Um canal queimado ou desconectado calibra com faixa ~0. */
    const uint16_t minimumCalibrationRange = 100;
}

FrontSensor::FrontSensor(const uint8_t commonPin, const uint8_t* selectPins, const uint8_t numberOfSensors):
    commonPin(commonPin),
    numberOfSensors(numberOfSensors > maxSensors ? maxSensors : numberOfSensors),
    calibrated(false),
    lastLinePosition(0),
    lineEverSeen(false)
{
    for (uint8_t i = 0; i < 4; i++) {
        this->selectPins[i] = selectPins[i];
    }

    for (uint8_t i = 0; i < maxSensors; i++) {
        calibrationMin[i] = adcMaxValue;
        calibrationMax[i] = 0;
        sensorEnabled[i] = true;
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

    /* Faixa nula ou minúscula = canal morto. Deixá-lo na conta seria pior que
     * ignorá-lo: ele viraria um "branco" permanente na posição da linha. */
    for (uint8_t i = 0; i < numberOfSensors; i++) {
        sensorEnabled[i] = calibrationMax[i] > calibrationMin[i] &&
                           (calibrationMax[i] - calibrationMin[i]) >= minimumCalibrationRange;
    }

    calibrated = true;
}

void FrontSensor::readCalibrated(uint16_t* sensorValues) {
    readRaw(sensorValues);

    for (uint8_t i = 0; i < numberOfSensors; i++) {
        /* 1000 = preto = "não vejo linha". Reportar 0 aqui faria o sensor
         * parecer o mais branco da barra. */
        if (!sensorEnabled[i]) {
            sensorValues[i] = 1000;
            continue;
        }

        const uint16_t minimum = calibrationMin[i];
        const uint16_t maximum = calibrationMax[i];

        if (maximum <= minimum) {
            sensorValues[i] = 1000;
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
        if (!sensorEnabled[i]) continue;

        /* Linha branca em fundo preto: o sensor sobre a linha lê MENOS. */
        const uint16_t value = 1000 - sensorValues[i];

        if (value > 200) onLine = true;

        if (value > 50) {
            weightedSum += (uint32_t)value * (i * 1000);
            sum += value;
        }
    }

    const uint16_t middle = (numberOfSensors - 1) * 1000 / 2;

    if (!onLine) {
        /* Nunca viu a linha (robô parado na bancada, barra fora da pista):
         * devolve o centro, para o PID não pedir uma curva fechada do nada.
         * lastLinePosition começa no meio, e "meio < meio" é falso — sem esta
         * guarda o primeiro readLineWhite() já apontaria para o extremo. */
        if (!lineEverSeen) return middle;

        /* Linha perdida depois de vista: trava no extremo por onde ela saiu,
         * senão o robô "esquece" a curva e sai reto. */
        return lastLinePosition < middle ? 0 : (numberOfSensors - 1) * 1000;
    }

    lineEverSeen = true;
    lastLinePosition = weightedSum / sum;
    return lastLinePosition;
}

uint8_t FrontSensor::getNumberOfSensors() const {
    return numberOfSensors;
}

bool FrontSensor::isCalibrated() const {
    return calibrated;
}

bool FrontSensor::isSensorEnabled(const uint8_t index) const {
    if (index >= numberOfSensors) return false;
    return sensorEnabled[index];
}

void FrontSensor::setSensorEnabled(const uint8_t index, const bool enabled) {
    if (index >= numberOfSensors) return;
    sensorEnabled[index] = enabled;
}

uint8_t FrontSensor::getEnabledCount() const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < numberOfSensors; i++) {
        if (sensorEnabled[i]) count++;
    }
    return count;
}
