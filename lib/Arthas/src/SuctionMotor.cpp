#include <SuctionMotor.h>

SuctionMotor::SuctionMotor(const uint8_t signalPin, const uint8_t ledcChannel) :
    signalPin(signalPin),
    ledcChannel(ledcChannel),
    targetThrottle(0),
    currentThrottle(0),
    lastUpdate(0),
    armStartTime(0)
{}

SuctionMotor::~SuctionMotor() {}

void SuctionMotor::setup() {
    ledcSetup(ledcChannel, frequency_hz, resolutionBits);
    ledcAttachPin(signalPin, ledcChannel);

    targetThrottle = 0;
    currentThrottle = 0;
    writeThrottle(0);   // throttle mínimo: é isso que arma o BLHeli

    armStartTime = millis();
    lastUpdate = armStartTime;
}

void SuctionMotor::writeThrottle(const double throttlePercent) {
    const double clamped = constrain(throttlePercent, 0.0, 100.0);

    const double pulseWidth_us = minPulseWidth_us +
        (maxPulseWidth_us - minPulseWidth_us) * clamped / 100.0;

    const uint32_t period_us = 1000000UL / frequency_hz;
    const uint32_t duty = (uint32_t)(pulseWidth_us * ((1UL << resolutionBits) - 1) / period_us);

    ledcWrite(ledcChannel, duty);
}

void SuctionMotor::setTarget(const uint8_t throttlePercent) {
    targetThrottle = throttlePercent > 100 ? 100 : throttlePercent;
}

void SuctionMotor::update() {
    const unsigned long now = millis();
    const unsigned long elapsed = now - lastUpdate;

    if (elapsed == 0) return;
    lastUpdate = now;

    /* Antes de armar o alvo é sempre 0, senão o ESC ignora e fica beepando. */
    const double target = isArmed() ? (double)targetThrottle : 0.0;
    const double maxStep = (rampRate_percentPerSecond * (double)elapsed) / 1000.0;

    if (currentThrottle < target) {
        currentThrottle += maxStep;
        if (currentThrottle > target) currentThrottle = target;
    }
    else if (currentThrottle > target) {
        currentThrottle -= maxStep;
        if (currentThrottle < target) currentThrottle = target;
    }
    else {
        return;
    }

    writeThrottle(currentThrottle);
}

void SuctionMotor::stop() {
    /* Parada é imediata, sem rampa: é o caminho de segurança. */
    targetThrottle = 0;
    currentThrottle = 0;
    lastUpdate = millis();
    writeThrottle(0);
}

bool SuctionMotor::isArmed() const {
    return (millis() - armStartTime) >= armingTime_ms;
}

uint8_t SuctionMotor::getTarget() const {
    return targetThrottle;
}

uint8_t SuctionMotor::getCurrent() const {
    return (uint8_t)(currentThrottle + 0.5);
}
