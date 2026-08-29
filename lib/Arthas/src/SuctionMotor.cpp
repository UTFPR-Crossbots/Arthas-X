#include <SuctionMotor.h>

SuctionMotor::SuctionMotor(const uint8_t signalPin, const uint8_t pwmTimer) :
    signalPin(signalPin),
    pwmTimer(pwmTimer),
    targetThrottle(0),
    currentPulse_us(neutralPulse_us),
    lastUpdate(0)
{}

SuctionMotor::~SuctionMotor() {}

void SuctionMotor::setup() {
    /* Sem isto a ESP32Servo começa procurando timer a partir do 0 — que é o
     * dos motores de tração, a 20 kHz — e o reconfiguraria para 50 Hz,
     * quebrando o PWM das rodas. allocateTimer() marca todos como ocupados e
     * libera só este. */
    ESP32PWM::allocateTimer(pwmTimer);

    esc.setPeriodHertz(frequency_hz);
    esc.attach(signalPin, minPulse_us, maxPulse_us);

    /* Neutro sustentado é o que arma o BLHeli. */
    esc.writeMicroseconds(neutralPulse_us);
    delay(armingTime_ms);

    targetThrottle = 0;
    currentPulse_us = neutralPulse_us;
    lastUpdate = millis();
}

double SuctionMotor::throttleToPulse(const uint8_t throttlePercent) const {
    const uint8_t clamped = throttlePercent > 100 ? 100 : throttlePercent;
    return neutralPulse_us + (double)(maxPulse_us - neutralPulse_us) * clamped / 100.0;
}

void SuctionMotor::setTarget(const uint8_t throttlePercent) {
    targetThrottle = throttlePercent > 100 ? 100 : throttlePercent;
}

void SuctionMotor::update() {
    const unsigned long now = millis();
    const unsigned long elapsed = now - lastUpdate;

    if (elapsed == 0) return;
    lastUpdate = now;

    const double target = throttleToPulse(targetThrottle);
    const double maxStep = (rampRate_usPerSecond * (double)elapsed) / 1000.0;

    if (currentPulse_us < target) {
        currentPulse_us += maxStep;
        if (currentPulse_us > target) currentPulse_us = target;
    }
    else if (currentPulse_us > target) {
        currentPulse_us -= maxStep;
        if (currentPulse_us < target) currentPulse_us = target;
    }
    else {
        return;
    }

    esc.writeMicroseconds((int)currentPulse_us);
}

void SuctionMotor::stop() {
    /* Parada é imediata, sem rampa: é o caminho de segurança. */
    targetThrottle = 0;
    currentPulse_us = neutralPulse_us;
    lastUpdate = millis();
    esc.writeMicroseconds(neutralPulse_us);
}

uint8_t SuctionMotor::getTarget() const {
    return targetThrottle;
}

uint8_t SuctionMotor::getCurrent() const {
    const double span = maxPulse_us - neutralPulse_us;
    const double percent = (currentPulse_us - neutralPulse_us) * 100.0 / span;
    return (uint8_t)(percent < 0 ? 0 : percent + 0.5);
}

uint16_t SuctionMotor::getCurrentPulse() const {
    return (uint16_t)currentPulse_us;
}
