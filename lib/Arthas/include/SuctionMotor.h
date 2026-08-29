#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>

/*
 * Motor de sucção acionado por ESC LittleBee Spring 20A (BLHeli_S).
 *
 * O ESC está em modo BIDIRECIONAL: o neutro fica em ~1500 us, não em 1000.
 * Mandar 1000 us não liga o motor — foi o que travou o primeiro teste.
 * O valor que arma e mantém parado nesta montagem é 1488 us, e a faixa útil
 * para frente vai de 1488 a 2000 us.
 *
 * Usa a ESP32Servo em vez de LEDC na mão porque ela já cuida do pulso de
 * servo. Ver o cuidado com o timer em setup().
 */
class SuctionMotor {
    private:
        Servo esc;
        const uint8_t signalPin;
        const uint8_t pwmTimer;

        uint8_t targetThrottle;     // 0..100 %
        double currentPulse_us;     // o que está saindo agora
        unsigned long lastUpdate;

        double throttleToPulse(const uint8_t throttlePercent) const;

    public:
        static const uint16_t neutralPulse_us = 1488;   // parado / arma o ESC
        static const uint16_t maxPulse_us = 2000;       // cheio
        static const uint16_t minPulse_us = 1000;       // limite inferior do attach()
        static const uint16_t frequency_hz = 50;
        static const uint16_t armingTime_ms = 3000;

        /* Rampa de subida. A referência de bancada subia 10 us a cada 100 ms
         * (100 us/s); 250 us/s leva a faixa toda em ~2 s, que é mais razoável
         * para largada sem dar degrau de corrente no ESC de 20 A. */
        static const uint16_t rampRate_usPerSecond = 250;

        SuctionMotor(const uint8_t signalPin, const uint8_t pwmTimer);
        ~SuctionMotor();

        void setup();
        void setTarget(const uint8_t throttlePercent);
        void update();
        void stop();

        uint8_t getTarget() const;
        uint8_t getCurrent() const;
        uint16_t getCurrentPulse() const;
};
