#pragma once
#include <stdint.h>

/*
 * Pinagem da placa nova do Arthas (ESP32-S3-WROOM-1).
 * Fonte: esquematicos_arthas/ (EasyEDA).
 *
 * Esta é a única fonte da pinagem — nada de #define solto no main.cpp.
 */
namespace pins {

    /* --- Barra frontal: 14x QRE1113GR atrás do mux CD74HC4067 (U6) --- */
    /* Todos os sensores chegam ao ESP32 por este único pino (ADC1_CH0..CH9). */
    constexpr uint8_t frontSensorCommon = 2;        // net COM
    /* S0 é o bit menos significativo do canal. */
    constexpr uint8_t frontSensorSelect[4] = {40, 41, 39, 38};  // S0, S1, S2, S3
    constexpr uint8_t frontSensorCount = 14;        // canais L0..L13 (L14/L15 sem uso)

    /* --- Sensores laterais --- */
    constexpr uint8_t lateralSensorLeft = 4;        // net L_IR, ADC1_CH3
    /* Atenção: o GPIO 42 não tem ADC no ESP32-S3 (ADC1 = 1..10, ADC2 = 11..20).
     * O lateral direito só pode ser lido digitalmente. */
    constexpr uint8_t lateralSensorRight = 42;      // net R_IR, sem ADC

    /* --- Motores: 2x DRV8874 em modo PH/EN (PMODE no GND) ---
     * PH/IN2 = direção, EN/IN1 = PWM. nSLEEP puxado para 3V3 (sempre ativo).
     * nFAULT (L_FT/R_FT) não chega ao ESP32 — falha do driver não é detectável. */
    constexpr uint8_t leftMotor[2]  = {17, 18};     // {L_DR, L_PWM}
    constexpr uint8_t rightMotor[2] = {21, 47};     // {R_DR, R_PWM}

    /* Sentido efetivo de PH depende de como OUT1/OUT2 foram para o conector.
     * Confirmar na bancada com o robô suspenso e inverter aqui se necessário. */
    constexpr bool leftMotorInverted  = false;
    constexpr bool rightMotorInverted = false;

    /* --- Motor de sucção: ESC LittleBee Spring 20A (BLHeli_S) ---
     *
     * Fala PWM de servo (1000-2000 us a 50 Hz), não o PWM de ponte H dos
     * motores de tração — por isso tem driver próprio, SuctionMotor.
     *
     * Sai na net CL, que não aparecia com função definida em nenhuma das
     * folhas de esquemático. O barramento SPI (MOSI 11, SCLK 12, MISO 13)
     * fica intacto, então IMU e encoders seguem viáveis. */
    constexpr uint8_t suctionEsc = 10;              // net CL

    /* --- Controle remoto infravermelho (LE-7009) ---
     * Receptor de 38 kHz na net CTRL. Os códigos das teclas ficam em
     * IrCodes.h e precisam ser capturados antes de o controle funcionar. */
    constexpr uint8_t irReceiver = 9;               // net CTRL

    /* --- Buzzer ativo (já tem oscilador, basta nível alto) ---
     * Alguns módulos apitam com nível BAIXO; se ficar mudo, inverter. */
    constexpr uint8_t buzzer = 5;                   // net BUZZER
    constexpr bool buzzerActiveLow = false;

    /* --- Alocação de timers do LEDC ---
     * O core mapeia timer = (canal / 2) % 4, então canais no mesmo timer
     * compartilham frequência:
     *
     *   timer 0 -> canais 0 e 1: motores de tração, 20 kHz (ledc na mão)
     *   timer 2 -> canal 4:      sucção / ESC, 50 Hz (ESP32Servo)
     *
     * A ESP32Servo, por padrão, procura timer livre a partir do 0 e roubaria
     * o dos motores. Por isso SuctionMotor::setup() chama allocateTimer() com
     * este número, que reserva a biblioteca a um timer só. */
    constexpr uint8_t suctionPwmTimer = 2;

    /* --- Reservado: fora do escopo do bring-up, sem driver ainda --- */
    namespace reserved {
        constexpr uint8_t batteryVoltage    = 1;    // VBAT,   ADC1_CH0
        /* BUZZER (5) e CTRL (9) agora têm uso — ver buzzer e irReceiver acima. */
        constexpr uint8_t imuInterrupt1     = 6;    // INT1
        constexpr uint8_t imuInterrupt2     = 7;    // INT2
        constexpr uint8_t leftCurrentSense  = 8;    // L_CS,   ADC1_CH7 (IPROPI)
        /* CL (10) é o sinal do ESC de sucção — ver suctionEsc acima. */
        constexpr uint8_t spiMosi           = 11;   // MOSI
        constexpr uint8_t spiSclk           = 12;   // SCLK
        constexpr uint8_t spiMiso           = 13;   // MISO
        /* R_CS cai no ADC2, compartilhado com o rádio — leitura pouco confiável. */
        constexpr uint8_t rightCurrentSense = 14;   // R_CS,   ADC2_CH3 (IPROPI)
        constexpr uint8_t imuChipSelect     = 15;   // CS_IMU
        constexpr uint8_t leftEncoderCs     = 16;   // CS_LE
        constexpr uint8_t rightEncoderCs    = 48;   // CS_RE
    }

    /* Não conectados no esquemático: 3, 35, 36, 37, 43 (TXD0), 44 (RXD0), 45, 46.
     * 19/20 são o USB nativo (gravação e monitor).
     * 0 é o botão SW3/BOOT.
     * 39..42 são os pinos de JTAG — usá-los aqui elimina debug por JTAG. */

}
