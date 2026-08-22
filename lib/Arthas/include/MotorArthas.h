#pragma once
#include <Arduino.h>

/*
 * DRV8874 em modo PH/EN (PMODE amarrado ao GND na placa nova):
 * um pino de direção (PH/IN2) e um de PWM (EN/IN1), no lugar do
 * IN1+IN2+PWM que o TB6612FNG usava.
 */
class MotorArthas {
    private:
        const uint8_t directionPin;
        const uint8_t pwmPin;
        const uint8_t ledcChannel;
        const bool inverted;

    public:
        /* 8 bits de resolução de propósito: mantém a escala de velocidade
         * (-255..255) que o analogWrite antigo tinha, para as constantes de
         * PID e o maxspeed já ajustados continuarem significando o mesmo. */
        static const uint8_t pwmResolutionBits = 8;
        static const uint32_t pwmFrequency_hz = 20000;   // acima do audível, dentro do limite do DRV8874
        static const int16_t maxDutyCycle = 255;

        MotorArthas(const uint8_t directionPin, const uint8_t pwmPin, const uint8_t ledcChannel, const bool inverted);
        ~MotorArthas();

        void setup();
        void drive(int16_t speed);
};
