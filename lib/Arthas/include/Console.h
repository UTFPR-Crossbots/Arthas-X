#pragma once
#include <Arduino.h>
#include <deque>
#include "IrRemote.h"

/*
 * Une as duas fontes de comando do robô numa só:
 *
 *  - controle infravermelho, que é o modo de operação na pista;
 *  - porta serial USB, para bancada.
 *
 * As duas entram na MESMA fila de linhas: o IR traduz a tecla numérica para o
 * comando de uma letra que a serial já usa (ver IrCodes.h). Assim
 * Arthas::parseInput() não sabe — nem precisa saber — de onde veio a
 * instrução, e o despacho de comandos continua sendo um só.
 *
 * A saída vai só para a serial: o controle remoto não tem canal de volta, e é
 * por isso que existe o buzzer.
 *
 * Nesta placa o UART0 (GPIO 43/44) não está conectado, então "Serial" só
 * existe compilando com -DARDUINO_USB_CDC_ON_BOOT=1, que o aponta para o USB
 * nativo (GPIO 19/20). Ver platformio.ini.
 */
class Console {
    private:
        IrRemote ir;
        bool serialEnabled;
        bool irEnabled;
        String serialAccumulator;
        std::deque<String> lines;

        void pollSerial();
        void pollIr();

    public:
        Console(const uint8_t irReceivePin);
        ~Console();

        void setup(const unsigned long baudRate = 115200);

        IrRemote* getIr();

        /* Durante a captura de códigos os frames precisam chegar crus ao
         * captureRaw(), e não virar comando na fila. */
        void setIrEnabled(const bool enabled);

        /* True se a última leitura trouxe um código IR fora da tabela — o
         * chamador usa isso para o beep de "não entendi". */
        bool consumeUnknownIrCode();

        bool isAvailable();
        String readInput();

        void print(const String msg);
        void print(const int msg);
        void println(const String msg = "");
        void println(const int msg);
        void println(const uint16_t msg);
        void printDoubleln(const double msg);
};
