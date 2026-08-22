#pragma once
#include <Arduino.h>

/*
 * Barra frontal da placa nova: 14 QRE1113GR ligados aos canais L0..L13 de um
 * mux CD74HC4067, todos saindo pelo mesmo pino de ADC (net COM).
 *
 * A QTRSensors não serve aqui: setSensorPins() espera GPIOs reais e não há
 * gancho para injetar leituras multiplexadas. A calibração e o cálculo de
 * posição abaixo replicam o comportamento dela (faixa 0..1000 por sensor,
 * média ponderada, latch da última posição quando a linha some).
 *
 * Polaridade: com pull-up de 10 kOhm, mais reflexão (branco) = tensão MENOR,
 * igual ao QTR analógico — daí readLineWhite() inverter os valores.
 */
class FrontSensor {
    private:
        static const uint8_t maxSensors = 16;   // limite físico do CD74HC4067

        const uint8_t commonPin;
        uint8_t selectPins[4];                  // S0..S3, S0 é o LSB
        const uint8_t numberOfSensors;

        uint16_t calibrationMin[maxSensors];
        uint16_t calibrationMax[maxSensors];
        bool calibrated;
        uint16_t lastLinePosition;

        void selectChannel(const uint8_t channel);

    public:
        FrontSensor(const uint8_t commonPin, const uint8_t* selectPins, const uint8_t numberOfSensors);
        ~FrontSensor();

        void setup();
        void calibrate();
        uint16_t readLineWhite();
        void readCalibrated(uint16_t* sensorValues);
        void readRaw(uint16_t* sensorValues);

        uint8_t getNumberOfSensors() const;
        bool isCalibrated() const;
};
