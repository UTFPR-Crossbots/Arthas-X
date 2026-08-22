#pragma once
#include <Arduino.h>
#include <deque>
#include "Bluetooth.h"

/*
 * Une os dois canais de comando do robô num só:
 *
 *  - BLE (Nordic UART Service), que continua sendo o modo de operação;
 *  - porta serial USB, para testar o robô na bancada sem depender do celular.
 *
 * A saída é espelhada nos dois; a entrada é lida de quem tiver dado. Assim o
 * despacho de comandos em Arthas::parseInput() não precisa saber de onde veio
 * a linha, e os dois links funcionam ao mesmo tempo.
 *
 * Atenção: nesta placa o UART0 (GPIO 43/44) não está conectado, então "Serial"
 * só existe se o firmware for compilado com -DARDUINO_USB_CDC_ON_BOOT=1, que
 * faz o Serial apontar para o USB nativo (GPIO 19/20). Ver platformio.ini.
 */
class Console {
    private:
        Bluetooth bluetooth;
        bool serialEnabled;
        String serialAccumulator;
        std::deque<String> serialLines;

        void pollSerial();

    public:
        Console();
        ~Console();

        void setup(const unsigned long baudRate = 115200);

        Bluetooth* getBluetooth();
        bool isBluetoothConnected();
        void turnOffBluetooth();

        bool isAvailable();
        String readInput();

        void print(const String msg);
        void print(const int msg);
        void println(const String msg = "");
        void println(const int msg);
        void println(const uint16_t msg);
        void printDoubleln(const double msg);
};
