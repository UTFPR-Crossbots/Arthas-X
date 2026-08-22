#include <MotorArthas.h>

MotorArthas::MotorArthas(const uint8_t directionPin, const uint8_t pwmPin, const uint8_t ledcChannel, const bool inverted) :
    directionPin(directionPin),
    pwmPin(pwmPin),
    ledcChannel(ledcChannel),
    inverted(inverted)
{}

MotorArthas::~MotorArthas() {}

void MotorArthas::setup() {
    pinMode(directionPin, OUTPUT);
    digitalWrite(directionPin, LOW);

    ledcSetup(ledcChannel, pwmFrequency_hz, pwmResolutionBits);
    ledcAttachPin(pwmPin, ledcChannel);
    ledcWrite(ledcChannel, 0);
}

void MotorArthas::drive(int16_t speed) {
    speed = constrain(speed, -maxDutyCycle, maxDutyCycle);

    const bool reverse = (speed < 0) != inverted;
    digitalWrite(directionPin, reverse ? HIGH : LOW);

    ledcWrite(ledcChannel, abs(speed));
}
