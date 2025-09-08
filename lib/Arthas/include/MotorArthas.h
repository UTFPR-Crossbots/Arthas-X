#pragma once
#include <Arduino.h>

class MotorArthas {
    private:
        uint8_t IN1;
        uint8_t IN2;
        uint8_t PWM;

    public:
        MotorArthas(const uint8_t* pins);
        ~MotorArthas();

        void drive(int16_t speed);
};