#include <PID.h>
#include <Arduino.h>
#include <math.h>

namespace {
    /* Anti-windup: o erro normalizado vale no máximo 1, então este limite
     * equivale a ~1 segundo de erro cheio acumulado. */
    const double integralLimit = 1.0;
}

PIDController::PIDController(const uint8_t numberOfSensors):
    Kp(50),     // erro normalizado: Kp na ordem do maxspeed é o ponto de partida
    Ki(0),
    Kd(0),
    setPoint((numberOfSensors - 1) * 500),   //This is the middle of the sensors, if it was 10 sensors it would be 9500, because its from 0 to 9
    lastError(0),
    lastTime(0),
    integral(0),
    deadZone(250)
{}

PIDController::~PIDController() {}

void PIDController::clearTimeAndError() {
    lastError = 0;
    lastTime = 0;
    integral = 0;
}

void PIDController::updatePIDConstants(const double Kp, const double Ki, const double Kd) {
    this->Kp = Kp;
    this->Ki = Ki;
    this->Kd = Kd;
}

double PIDController::calculateCorrection(uint16_t linePosition) {
    const unsigned long currentTime = millis();

    double error = setPoint - (double)linePosition;

    if (fabs(error) < deadZone) {
        error = 0;
        integral = 0;
    }

    /* Normaliza para -1..+1. O setPoint é o meio da barra, logo o erro máximo
     * possível é o próprio setPoint. */
    error /= setPoint;

    /* Primeiro ciclo: sem lastTime válido, deltaTime seria o tempo desde o
     * boot. Integrar isso estouraria a integral logo de cara, e a derivada
     * daria um pico do nada. */
    if (lastTime == 0) {
        lastError = error;
        lastTime = currentTime;
        return Kp * error;
    }

    double deltaTime = (currentTime - lastTime) / 1000.0; // Convert to seconds
    if (deltaTime <= 0) {
        deltaTime = 0.001;
    }

    const double proportionalTerm = Kp * error;

    integral += error * deltaTime;
    integral = constrain(integral, -integralLimit, integralLimit);
    const double integralTerm = Ki * integral;

    const double derivativeTerm = Kd * (error - lastError) / deltaTime;

    lastError = error;
    lastTime = currentTime;

    return proportionalTerm + integralTerm + derivativeTerm;
}

const double PIDController::getP() {
    return Kp;
}

const double PIDController::getI() {
    return Ki;
}

const double PIDController::getD() {
    return Kd;
}

double PIDController::getSetPoint() const {
    return setPoint;
}
