#pragma once
#include "PID.h"
#include "Console.h"
#include "FrontSensor.h"
#include "LateralSensor.h"
#include "Powertrain.h"

enum class ArthasAction {
    None = 0,
    RightMotorTest,
    LeftMotorTest,
    BothMotorsTest,
    FrontSensorTest,
    FrontSensorTestAnalogRead,
    FrontSensorTestWhileLine,
    LateralSensorTest,
    Calibrate,
    UpdateConstants,
    ShowStatus,
    ShowMenu,
    DriveLap,
    Chase,
    Stop,
    BluetoothConnected,
    BluetoothDisconnected
};

class Arthas {
    private:
        Console console;
        Powertrain powertrain;
        FrontSensor frontSensor;
        LateralSensor lateralSensorLeft;
        LateralSensor lateralSensorRight;
        PIDController pid;
        int16_t maxspeed;
        double tempo;
        uint8_t marcas;

        /* Comandos "kp 0.35", "vel 120" etc., pensados para digitar na serial.
         * Devolve true se a linha era um desses e já foi aplicada. */
        bool parseValueCommand(const String& input);

    public:
        /* leftMotorPins/rightMotorPins = {direção, pwm} (DRV8874 em PH/EN).
         * frontSensorSelectPins = {S0, S1, S2, S3} do CD74HC4067, S0 é o LSB. */
        Arthas(const uint8_t* leftMotorPins,
               const uint8_t* rightMotorPins,
               const uint8_t frontSensorCommonPin,
               const uint8_t* frontSensorSelectPins,
               const uint8_t numberOfFrontSensors,
               const uint8_t leftLateralSensorPin,
               const uint8_t rightLateralSensorPin,
               const int16_t maxspeed,
               const bool invertLeftMotor = false,
               const bool invertRightMotor = false);
        ~Arthas();

        /* Comunicação (BLE + serial USB) */
        Console* getConsole();
        Bluetooth* getBluetooth();
        void setupConsole();
        const bool isInputAvailable();
        String readInput();
        ArthasAction parseInput();
        void print(const String msg);
        void print(const int msg);
        void println(const String msg = "");
        void println(const int msg);
        void turnOffBluetooth();

        /* Line Sensors */
        void setupFrontSensor();
        void calibrateFrontSensor();
        const uint16_t readLineWhiteFrontSensor();
        void testFrontSensor();
        void testFrontSensorAnalogRead();
        void testFrontSensorWhiteLine();

        /* Lateral Sensors */
        void setupLateralSensors();
        void testLateralSensor();

        /* Motors */
        void setupMotors();
        void stopMotors();
        void testLeftMotor();
        void testRightMotor();
        void testBothMotors();

        /* Constants */
        void printMenu();
        void printStatus();
        void printMaxSpeed();
        void printPID();

        /* Modes */
        void driveLap(const uint8_t markers);
        void driveLapTest();
        void chase();
};
