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
 *
 * Sensor morto: um canal que não varia na calibração é DESABILITADO e sai da
 * conta da posição. Sem isso ele calibra com faixa nula, sai como 0, e o
 * 1000-0 de readLineWhite() o transforma no ponto mais branco da barra —
 * um sensor queimado sozinho passa a mandar na posição da linha.
 */
class FrontSensor {
    private:
        static const uint8_t maxSensors = 16;   // limite físico do CD74HC4067

        const uint8_t commonPin;
        uint8_t selectPins[4];                  // S0..S3, S0 é o LSB
        const uint8_t numberOfSensors;

        uint16_t calibrationMin[maxSensors];
        uint16_t calibrationMax[maxSensors];
        bool sensorEnabled[maxSensors];
        bool calibrated;
        uint16_t lastLinePosition;
        bool lineEverSeen;

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

        /* Um sensor desabilitado não entra na média ponderada e é reportado
         * como 1000 (preto / sem linha), que é o valor seguro. */
        bool isSensorEnabled(const uint8_t index) const;
        void setSensorEnabled(const uint8_t index, const bool enabled);
        uint8_t getEnabledCount() const;
};
