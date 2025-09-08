#pragma once

/* In order to recognize uint16_t */
#include <stdint.h>
#include "Bluetooth.h"


class PIDController {
    private:
        double Kp, Ki, Kd;
        double setPoint;
        double lastError;
        double integral;
        const double deadZone;
        unsigned long lastTime;
        
    public:
        PIDController(const uint8_t numberOfSensors);
        ~PIDController();

        void clearTimeAndError();
        void updatePIDConstants(const double Kp, const double Ki, const double Kd);
        double calculateCorrection(uint16_t linePosition);
        const double getP();
        const double getI();
        const double getD();
};