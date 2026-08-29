#pragma once

/* In order to recognize uint16_t */
#include <stdint.h>

/*
 * O erro é NORMALIZADO para -1..+1 antes de entrar nos termos.
 *
 * Sem isso o erro vale a posição crua: com 14 sensores a faixa é 0..13000 e o
 * erro chega a +-6500, então qualquer Kp acima de ~0,015 satura a correção e o
 * robô vira bang-bang, com uma roda no máximo e a outra em ré. Normalizado, o
 * Kp passa a ser lido em "unidades de velocidade por deflexão cheia": Kp igual
 * ao maxspeed dá a diferença máxima entre as rodas no extremo da barra.
 *
 * ATENÇÃO: constantes tunadas na escala antiga não valem mais aqui.
 */
class PIDController {
    private:
        double Kp, Ki, Kd;
        double setPoint;
        double lastError;       // já normalizado
        double integral;
        const double deadZone;  // em unidades de posição, antes de normalizar
        unsigned long lastTime;

    public:
        PIDController(const uint8_t numberOfSensors,
                      const double Kp, const double Ki, const double Kd);
        ~PIDController();

        void clearTimeAndError();
        double calculateCorrection(uint16_t linePosition);
        const double getP();
        const double getI();
        const double getD();
        double getSetPoint() const;
};
