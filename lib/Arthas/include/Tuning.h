#pragma once
#include <stdint.h>

/*
 * Constantes de ajuste do robô.
 *
 * Não há mais comandos para mudar isto em runtime: ajustar aqui e recompilar.
 * A escala de cada valor está anotada porque nenhuma delas é óbvia sozinha.
 */
namespace tuning {

    /* --- PID ---
     * O erro entra NORMALIZADO para -1..+1, não em unidades de posição. Logo
     * Kp é lido em "unidades de velocidade por deflexão cheia": Kp igual ao
     * maxSpeed dá a diferença máxima entre as rodas no extremo da barra.
     *
     * Constantes tunadas na escala antiga (erro cru, 0..13000) NÃO valem. */
    constexpr double kP = 50.0;
    constexpr double kI = 0.0;
    constexpr double kD = 0.0;

    /* Velocidade base das rodas, na escala do PWM de 8 bits (0..255). */
    constexpr int16_t maxSpeed = 100;

    /* Marcas do sensor lateral esquerdo para dar a volta por encerrada. */
    constexpr uint8_t markersPerLap = 5;

    /* Quanto a roda interna pode girar em RÉ para fechar curva, como fração
     * do maxSpeed. 0 trava as rodas em só para frente. */
    constexpr double reverseRatio = 0.5;

    /* Throttle da sucção durante a corrida, em %. */
    constexpr uint8_t suctionThrottle = 80;

}
