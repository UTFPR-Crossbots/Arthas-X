#pragma once
#include <cstdint>
#include <Arduino.h>

class LateralSensor {
    private:
        const uint8_t pin;
        const bool isAnalog;
        const uint16_t whiteLineLimitValue;

    public:
        LateralSensor(const uint8_t pin, const bool isAnalog);
        ~LateralSensor();

        void setup();
        const uint16_t read();
        const bool isWhite();
};