#pragma once
#include <Arduino.h>

/*
 * Buzzer ATIVO: já tem oscilador dentro, então digitalWrite(HIGH) apita.
 * Não precisa de PWM nem de canal LEDC — logo não disputa timer com os
 * motores de tração (timer 0) nem com o ESC de sucção (timer 2).
 *
 * Não bloqueante de propósito: um delay() de beep dentro do chase()
 * congelaria o seguimento de linha. beep() só enfileira o padrão e retorna;
 * update() avança a máquina de estados, chamado junto com o resto.
 */
class Buzzer {
    private:
        const uint8_t pin;
        const bool activeLow;

        uint8_t beepsRemaining;
        uint16_t onTime_ms;
        uint16_t offTime_ms;
        bool soundOn;
        unsigned long lastTransition;

        void write(const bool on);

    public:
        /* Padrões de feedback. Curto para "recebi", longo para "não entendi" —
         * dá para distinguir de longe sem olhar para o robô. */
        static const uint16_t shortBeep_ms = 80;
        static const uint16_t longBeep_ms = 400;
        static const uint16_t gap_ms = 80;

        Buzzer(const uint8_t pin, const bool activeLow = false);
        ~Buzzer();

        void setup();
        void update();

        void beep(const uint8_t count = 1, const uint16_t onTime = shortBeep_ms,
                  const uint16_t offTime = gap_ms);
        void beepLong();
        void silence();

        bool isBusy() const;
};
