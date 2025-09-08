#pragma once
#include <BluetoothSerial.h>

class Bluetooth {
    BluetoothSerial SerialBT;

    public:
        Bluetooth();
        ~Bluetooth();

        void setup();
        bool isAvailable();
        String readSerialInput();
        void print(const String msg);
        void print(const int msg);
        void println(const String msg = "");
        void println(const int msg);
        void println(const uint16_t msg);
        void printDoubleln(const double msg);
        void turnOff();

};