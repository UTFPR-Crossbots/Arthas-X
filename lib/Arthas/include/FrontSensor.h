#pragma once
#include <QTRSensors.h>
#include <SparkFun_TB6612.h>

class FrontSensor {
    private:
        QTRSensors frontSensor;
        const uint8_t numberOfSensors;
        const uint8_t* pins;
        bool isAnalog;

    public:
        FrontSensor(const uint8_t numberOfSensors, const uint8_t* pinsArray, const bool isAnalog);
        ~FrontSensor();

        void setup();
        void calibrate();
        uint16_t readLineWhite();
        void readCalibrated(uint16_t* sensorValues);

        uint8_t getNumberOfSensors() const;
};