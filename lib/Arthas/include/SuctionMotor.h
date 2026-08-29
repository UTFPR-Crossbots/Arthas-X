#pragma once
#include <Arduino.h>

/*
 * Motor de sucção acionado por ESC LittleBee Spring 20A (BLHeli_S).
 *
 * O ESC fala PWM de servo padrão: pulso de 1000 us = parado, 2000 us = cheio,
 * a 50 Hz. Não é o PWM direto de ponte H dos motores de tração — por isso este
 * driver não reaproveita MotorArthas.
 *
 * Armar: o BLHeli só aceita comando depois de receber throttle mínimo por
 * ~2 s. setup() já começa mandando 1000 us, então o ESC arma sozinho durante
 * o boot; isArmed() existe para não deixar ninguém acelerar antes disso.
 *
 * Rampa: um ESC de 20 A puxando corrente num degrau derruba a tensão da placa.
 * setTarget() só define o alvo; update() caminha até ele em passos limitados e
 * NÃO bloqueia, para poder ser chamado de dentro da malha de controle.
 */
class SuctionMotor {
    private:
        const uint8_t signalPin;
        const uint8_t ledcChannel;

        uint8_t targetThrottle;         // 0..100 %
        double currentThrottle;         // 0..100 %, o que está saindo agora
        unsigned long lastUpdate;
        unsigned long armStartTime;

        void writeThrottle(const double throttlePercent);

    public:
        static const uint16_t minPulseWidth_us = 1000;  // parado
        static const uint16_t maxPulseWidth_us = 2000;  // cheio
        static const uint32_t frequency_hz = 50;
        static const uint8_t  resolutionBits = 16;      // ~0,3 us de passo
        static const uint16_t armingTime_ms = 2000;
        static const uint16_t rampRate_percentPerSecond = 200;   // 0->100% em 0,5 s

        SuctionMotor(const uint8_t signalPin, const uint8_t ledcChannel);
        ~SuctionMotor();

        void setup();
        void setTarget(const uint8_t throttlePercent);
        void update();
        void stop();

        bool isArmed() const;
        uint8_t getTarget() const;
        uint8_t getCurrent() const;
};
