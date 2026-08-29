#include <Buzzer.h>

Buzzer::Buzzer(const uint8_t pin, const bool activeLow) :
    pin(pin),
    activeLow(activeLow),
    beepsRemaining(0),
    onTime_ms(shortBeep_ms),
    offTime_ms(gap_ms),
    soundOn(false),
    lastTransition(0)
{}

Buzzer::~Buzzer() {}

void Buzzer::setup() {
    pinMode(pin, OUTPUT);
    write(false);
}

void Buzzer::write(const bool on) {
    soundOn = on;
    digitalWrite(pin, (on != activeLow) ? HIGH : LOW);
}

void Buzzer::beep(const uint8_t count, const uint16_t onTime, const uint16_t offTime) {
    if (count == 0) return;

    beepsRemaining = count;
    onTime_ms = onTime;
    offTime_ms = offTime;

    /* Começa a apitar já, para o feedback não parecer atrasado. */
    write(true);
    lastTransition = millis();
}

void Buzzer::beepLong() {
    beep(1, longBeep_ms);
}

void Buzzer::silence() {
    beepsRemaining = 0;
    write(false);
    lastTransition = millis();
}

void Buzzer::update() {
    if (beepsRemaining == 0 && !soundOn) return;

    const unsigned long now = millis();
    const uint16_t interval = soundOn ? onTime_ms : offTime_ms;

    if (now - lastTransition < interval) return;
    lastTransition = now;

    if (soundOn) {
        write(false);
        /* O beep só é dado por encerrado quando o silêncio dele começa, senão
         * o último apito do padrão não teria duração garantida. */
        if (beepsRemaining > 0) beepsRemaining--;
    }
    else if (beepsRemaining > 0) {
        write(true);
    }
}

bool Buzzer::isBusy() const {
    return beepsRemaining > 0 || soundOn;
}
