#include <PID.h>

PIDController::PIDController(const uint8_t numberOfSensors):
    Kp(1),
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

// double PIDController::calculateCorrection(uint16_t linePosition) {
//     double currentCorrection;

//     unsigned long currentTime = millis();
//     unsigned long deltaTime = currentTime - this->lastTime;

//     this->error = this->setPoint - linePosition;

//     double derivativeTerm = (this->error - this->lastError) / static_cast<double>(deltaTime);


//     currentCorrection = this->Kp * this->error + this->Kd * derivativeTerm;


//     this->lastError = this->setPoint - linePosition;
//     this->lastTime = currentTime;

//     return currentCorrection;
// }

double PIDController::calculateCorrection(uint16_t linePosition) {
    unsigned long currentTime = millis();
    // verifica se é a primeira execução
    double deltaTime = (currentTime - lastTime) / 1000.0; // Convert to seconds
    
    if (deltaTime == 0) {
        deltaTime = 0.001;
    }

    double error = setPoint - linePosition;

    if (abs(error) < deadZone) {
        error = 0;
        integral = 0;
    }
    
    // Proportional term
    double proportionalTerm = Kp * error;
    
    // Integral term
    integral += error * deltaTime;
    double integralTerm = Ki * integral;
    
    // Derivative term (avoid on first cycle)
    double derivativeTerm = 0;
    if (lastTime != 0) {
        derivativeTerm = Kd * (error - lastError) / deltaTime;
    }

    // Calculate total correction
    double correction = proportionalTerm + integralTerm + derivativeTerm;

    // Update states
    lastError = error;
    lastTime = currentTime;

    return correction;
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
