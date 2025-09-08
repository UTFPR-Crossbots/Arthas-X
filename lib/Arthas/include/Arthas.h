#pragma once
#include "PID.h"
#include "Bluetooth.h"
#include "FrontSensor.h"
#include "LateralSensor.h"
#include "Powertrain.h"
#include <QTRSensors.h>
#include <SparkFun_TB6612.h>

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
    DriveLap,
    Chase,
    Stop,
    BluetoothConnected,
    BluetoothDisconnected
};

class Arthas {
    private:
        Bluetooth bluetooth;
        Powertrain powertrain;
        FrontSensor frontSensor;
        LateralSensor lateralSensor;
        PIDController pid;
        int16_t maxspeed;
        uint8_t frontSensorPins[8];
        double tempo;
        uint8_t marcas;

    public:
        Arthas(const uint8_t* leftMotorPins, const uint8_t* rightMotorPins, const uint8_t* frontSensorPins, const uint8_t numberOfPins, const uint8_t lateralSensorPin, const int16_t maxspeed);
        ~Arthas();

        /* Bluetooth */
        Bluetooth* getBluetooth();
        void setupBluetooth();
        const bool isBluetoothAvailable();
        String readBluetoothInput();
        ArthasAction parseBluetoothInput();
        void printBT(const String msg);
        void printBT(const int msg);
        void printlnBT(const String msg = "");
        void printlnBT(const int msg);
        void turnOffBluetooth();

        /* Line Sensors */
        void setupFrontSensor();
        void calibrateFrontSensor();
        const uint16_t readLineWhiteFrontSensor();
        void testFrontSensor();
        void testFrontSensorAnalogRead();
        void testFrontSensorWhiteLine();

        /* Lateral Sensor */
        void setupLateralSensor();
        void testLateralSensor();

        /* Motors */
        void stopMotors();
        void testLeftMotor();
        void testRightMotor();
        void testBothMotors();

        /* Constants */
        void printMaxSpeed();
        void printPID();

        /* Modes */
        void driveLap(const uint8_t markers);
        void driveLapTest();
        void chase();
};