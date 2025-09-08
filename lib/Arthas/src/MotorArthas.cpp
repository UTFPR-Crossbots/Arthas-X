#include <MotorArthas.h>

MotorArthas::MotorArthas(const uint8_t* pins) :
    IN1(pins[0]),
    IN2(pins[1]),
    PWM(pins[2])
{
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(PWM, OUTPUT);
}

MotorArthas::~MotorArthas() {}

void MotorArthas::drive(int16_t speed) {
    if (speed >= 0) {
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        analogWrite(PWM, speed);
    }
    else {
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        analogWrite(PWM, -speed);
    }
}