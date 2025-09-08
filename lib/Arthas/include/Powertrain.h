#pragma once
#include <SparkFun_TB6612.h>
#include <MotorArthas.h>

class Powertrain {
    private:
        MotorArthas leftMotor;
        MotorArthas rightMotor;

    public:
        Powertrain(const uint8_t* leftPins, const uint8_t* rightPins);
        ~Powertrain();

        void leftMotorDrive(const int16_t speed);
        void rightMotorDrive(const int16_t speed);
        void motorsDrive(const int16_t leftMotorSpeed, const int16_t rightMotorSpeed);
        void stopMotors();
};