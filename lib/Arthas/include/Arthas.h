#pragma once
#include "PID.h"
#include "Console.h"
#include "Buzzer.h"
#include "FrontSensor.h"
#include "LateralSensor.h"
#include "Powertrain.h"
#include "SuctionMotor.h"

enum class ArthasAction {
    None = 0,
    RightMotorTest,
    LeftMotorTest,
    BothMotorsTest,
    FrontSensorTest,
    FrontSensorTestAnalogRead,
    FrontSensorTestWhileLine,
    LateralSensorTest,
    SuctionTest,
    IrCapture,
    Calibrate,
    SensorMaskChanged,
    ShowStatus,
    ShowMenu,
    DriveLap,
    Chase,
    Stop
};

class Arthas {
    private:
        Console console;
        Buzzer buzzer;
        Powertrain powertrain;
        FrontSensor frontSensor;
        LateralSensor lateralSensorLeft;
        LateralSensor lateralSensorRight;
        SuctionMotor suction;
        PIDController pid;

        const int16_t maxspeed;
        const uint8_t marcas;
        const double reverseRatio;
        const uint8_t suctionThrottle;

        /* Único ponto onde a correção do PID vira velocidade de roda —
         * inclusive a decisão de deixar (ou não) a roda interna entrar em ré. */
        void applyCorrection(const int16_t baseSpeed, const double correction);

        /* Habilita/desabilita um sensor da barra ("off 11" / "on 11").
         * Devolve true se a linha era um desses. */
        bool parseSensorMaskCommand(const String& input);

    public:
        /* leftMotorPins/rightMotorPins = {direção, pwm} (DRV8874 em PH/EN).
         * frontSensorSelectPins = {S0, S1, S2, S3} do CD74HC4067, S0 é o LSB.
         * Constantes de ajuste vêm de Tuning.h, via main.cpp. */
        Arthas(const uint8_t* leftMotorPins,
               const uint8_t* rightMotorPins,
               const uint8_t frontSensorCommonPin,
               const uint8_t* frontSensorSelectPins,
               const uint8_t numberOfFrontSensors,
               const uint8_t leftLateralSensorPin,
               const uint8_t rightLateralSensorPin,
               const uint8_t suctionEscPin,
               const uint8_t suctionPwmTimer,
               const uint8_t irReceiverPin,
               const uint8_t buzzerPin,
               const bool buzzerActiveLow,
               const bool invertLeftMotor = false,
               const bool invertRightMotor = false);
        ~Arthas();

        /* Avança tudo que é não bloqueante: rampa da sucção e buzzer.
         * PRECISA ser chamado toda iteração, tanto pelo loop() principal
         * quanto pelos loops de modo — senão a rampa de desaceleração
         * congela no ponto em que o modo terminou. */
        void update();

        /* Comunicação (IR + serial USB) */
        Console* getConsole();
        void setupConsole();
        const bool isInputAvailable();
        String readInput();
        ArthasAction parseInput();
        void print(const String msg);
        void print(const int msg);
        void println(const String msg = "");
        void println(const int msg);

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

        /* Suction */
        void setupSuction();
        void testSuction();

        /* Buzzer */
        void setupBuzzer();
        void beepReady();

        /* Infravermelho */
        void captureIrCodes();

        /* Motors */
        void setupMotors();
        void stopMotors();
        void testLeftMotor();
        void testRightMotor();
        void testBothMotors();

        /* Info */
        void printMenu();
        void printStatus();
        void reportDisabledSensors();
        void printMaxSpeed();
        void printPID();

        /* Modes */
        void driveLap();
        void chase();
};
