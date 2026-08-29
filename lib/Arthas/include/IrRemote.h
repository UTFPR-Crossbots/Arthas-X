#pragma once
#include <Arduino.h>
#include <IRrecv.h>
#include "IrCodes.h"

/*
 * Receptor infravermelho (controle de TV LE-7009) no pino CTRL.
 *
 * Traduz a tecla numérica para o mesmo comando de uma letra que a serial usa,
 * de modo que Arthas::parseInput() não precise saber de onde veio a instrução.
 *
 * IMPORTANTE: os códigos vivem em IrCodes.h e nascem zerados — ver as
 * instruções de captura lá. Enquanto não forem preenchidos, readCommand()
 * nunca devolve comando.
 */
class IrRemote {
    private:
        const uint8_t receivePin;
        IRrecv receiver;
        decode_results results;

        uint64_t lastCode;
        unsigned long lastCodeTime;
        bool unknownCodeSeen;

        bool decodeNext();

    public:
        IrRemote(const uint8_t receivePin);
        ~IrRemote();

        void setup();

        /* Devolve o comando de uma letra correspondente à tecla, ou 0 se nada
         * chegou. Trata repetição e códigos fora da tabela. */
        char readCommand();

        /* True quando chegou algo que não está na tabela — serve para o robô
         * responder com o beep de "não entendi". Limpa ao ser lido. */
        bool consumeUnknownCode();

        /* Modo de captura: se algo chegou, preenche protocolo/código/bits e
         * devolve true. Alimenta o comando "x". */
        bool captureRaw(String& protocol, uint64_t& code, uint16_t& bits);

        bool isConfigured() const;
};
