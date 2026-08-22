#pragma once
#include <MotorArthas.h>

class Powertrain {
    private:
        MotorArthas leftMotor;
        MotorArthas rightMotor;

    public:
        /* pins = {direção, pwm} para cada motor. */
        Powertrain(const uint8_t* leftPins, const uint8_t* rightPins,
                   const bool invertLeft = false, const bool invertRight = false);
        ~Powertrain();

        void setup();
        void leftMotorDrive(const int16_t speed);
        void rightMotorDrive(const int16_t speed);
        void motorsDrive(const int16_t leftMotorSpeed, const int16_t rightMotorSpeed);
        void stopMotors();
};
