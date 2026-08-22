#pragma once
#include <stdint.h>
#include <Arduino.h>

/*
 * A placa nova tem dois: L_IR (GPIO 4, ADC1) e R_IR (GPIO 42).
 *
 * O GPIO 42 não tem ADC no ESP32-S3 (ADC1 = 1..10, ADC2 = 11..20), então o
 * lateral direito só pode ser lido digitalmente, com o limiar fixo do buffer
 * de entrada. Daí o flag isAnalog existir.
 *
 * activeLow reflete o pull-up de 10 kOhm dos QRE1113GR: branco = tensão baixa.
 * Como a folha dos L_IR/R_IR não veio nos esquemáticos, o flag fica exposto
 * para conferir na bancada.
 */
class LateralSensor {
    private:
        const uint8_t pin;
        const bool isAnalog;
        const uint16_t whiteLineLimitValue;
        const bool activeLow;

    public:
        LateralSensor(const uint8_t pin, const bool isAnalog,
                      const uint16_t whiteLineLimitValue = 2000, const bool activeLow = true);
        ~LateralSensor();

        void setup();
        const uint16_t read();
        const bool isWhite();
};
