#pragma once
#include <stdint.h>

/*
 * ============================================================================
 *  CÓDIGOS DO CONTROLE REMOTO — PRECISAM SER CAPTURADOS ANTES DE FUNCIONAR
 * ============================================================================
 *
 * A tabela abaixo nasce ZERADA. Enquanto estiver assim, o robô NÃO responde
 * ao controle: não existe jeito de adivinhar os códigos de um LE-7009, eles
 * têm de ser lidos do próprio controle.
 *
 * Como preencher:
 *
 *   1. Grave o firmware e abra a serial:  pio device monitor -e esp32s3
 *   2. Mande o comando  x  (captura de códigos IR).
 *   3. Aperte 0, 1, 2, ... 9 no controle, um de cada vez. Cada tecla imprime
 *      o protocolo e o código em hexadecimal.
 *   4. Copie os dez valores para a tabela abaixo, na ordem dos dígitos.
 *   5. Mande  c  para sair da captura, recompile e grave de novo.
 *
 * Códigos são característica fixa do hardware, então moram aqui no código —
 * mesma decisão das constantes de tuning em Tuning.h.
 */
namespace ir {

    /* Índice = dígito da tecla. Zero significa "ainda não capturado". */
    constexpr uint64_t keyCode[10] = {
        0x0,    // tecla 0  -> PARAR
        0x0,    // tecla 1  -> calibrar a barra
        0x0,    // tecla 2  -> chase
        0x0,    // tecla 3  -> volta (driveLap)
        0x0,    // tecla 4  -> barra frontal, cru do ADC
        0x0,    // tecla 5  -> barra frontal, calibrado
        0x0,    // tecla 6  -> sensores laterais
        0x0,    // tecla 7  -> motor esquerdo
        0x0,    // tecla 8  -> motor direito
        0x0,    // tecla 9  -> sucção (ESC)
    };

    /*
     * Cada dígito vira o MESMO comando de uma letra que a serial já entende,
     * então o despacho de comandos continua sendo um só — o IR não tem um
     * caminho paralelo.
     */
    constexpr char keyCommand[10] = {
        'c',    // 0 -> PARAR
        'a',    // 1 -> calibrar
        'e',    // 2 -> chase
        'b',    // 3 -> volta
        'i',    // 4 -> cru do ADC
        'h',    // 5 -> calibrado
        'd',    // 6 -> laterais
        'j',    // 7 -> motor esquerdo
        'k',    // 8 -> motor direito
        'l',    // 9 -> sucção
    };

    /* Segurar a tecla gera repetição; sem esta janela um toque um pouco mais
     * longo dispararia o comando várias vezes seguidas. */
    constexpr uint16_t debounce_ms = 250;

    inline bool isTableFilled() {
        for (uint8_t i = 0; i < 10; i++) {
            if (keyCode[i] != 0) return true;
        }
        return false;
    }

}
