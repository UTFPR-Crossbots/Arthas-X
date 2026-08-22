#pragma once
#include <Arduino.h>
#include <deque>

/*
 * O ESP32-S3 não tem Bluetooth Clássico — BluetoothSerial (SPP) não existe
 * nele. Esta classe mantém a mesma API pública de antes, mas por baixo é BLE
 * usando o Nordic UART Service (NimBLE).
 *
 * Duas diferenças que o resto do código precisa saber:
 *
 * 1) Notify tem limite de MTU-3 bytes (20 no default). Linhas longas são
 *    fatiadas aqui dentro; o SPP não tinha esse limite.
 * 2) BLE entrega os dados fragmentados, sem o timeout de ~1 s que o
 *    readString() do SPP dava de graça. As mensagens são remontadas e
 *    delimitadas por '\n' (ou por uma pausa curta na recepção, para clientes
 *    que não mandam terminador), senão um fragmento parcial chegaria ao
 *    parser de constantes de PID e corromperia Kp/Kd em silêncio.
 */

class NimBLEServer;
class NimBLECharacteristic;

class Bluetooth {
    friend class ArthasServerCallbacks;
    friend class ArthasRxCallbacks;

    private:
        NimBLEServer* server;
        NimBLECharacteristic* txCharacteristic;

        std::deque<String> rxLines;
        SemaphoreHandle_t rxMutex;
        String rxAccumulator;
        unsigned long lastRxTime;

        volatile bool connected;
        volatile uint16_t connectionHandle;

        void pushIncoming(const uint8_t* data, const size_t length);
        void pushLine(const String& line);
        void flushAccumulatorIfIdle();
        void send(const String& msg);
        uint16_t maxNotifyLength();

    public:
        Bluetooth();
        ~Bluetooth();

        void setup();
        bool isAvailable();
        bool isConnected();
        String readSerialInput();
        void print(const String msg);
        void print(const int msg);
        void println(const String msg = "");
        void println(const int msg);
        void println(const uint16_t msg);
        void printDoubleln(const double msg);
        void turnOff();
};
