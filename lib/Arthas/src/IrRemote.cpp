#include <IrRemote.h>
#include <IRutils.h>

namespace {
    /* Buffer do decodificador. O default da biblioteca cobre NEC com folga. */
    const uint16_t captureBufferSize = 1024;
    const uint8_t captureTimeout_ms = 50;
}

IrRemote::IrRemote(const uint8_t receivePin) :
    receivePin(receivePin),
    receiver(receivePin, captureBufferSize, captureTimeout_ms, true),
    lastCode(0),
    lastCodeTime(0),
    unknownCodeSeen(false)
{}

IrRemote::~IrRemote() {}

void IrRemote::setup() {
    /* O módulo TSOP tem saída em coletor aberto. Se a placa não trouxer
     * pull-up no CTRL, a leitura fica travada em nível baixo — daí o pull-up
     * interno. Ele é inofensivo caso já exista um externo. */
    pinMode(receivePin, INPUT_PULLUP);

    receiver.enableIRIn();
}

bool IrRemote::decodeNext() {
    return receiver.decode(&results);
}

char IrRemote::readCommand() {
    if (!decodeNext()) return 0;

    const uint64_t code = results.value;
    receiver.resume();

    /* Frames de repetição do NEC (tecla segurada) chegam como 0xFFFFFFFF...
     * e não devem virar comando. */
    if (results.repeat || code == 0xFFFFFFFFFFFFFFFFULL) return 0;

    const unsigned long now = millis();

    /* Mesmo código dentro da janela = ainda é o mesmo toque. */
    if (code == lastCode && (now - lastCodeTime) < ir::debounce_ms) {
        lastCodeTime = now;
        return 0;
    }

    lastCode = code;
    lastCodeTime = now;

    for (uint8_t digit = 0; digit < 10; digit++) {
        if (ir::keyCode[digit] != 0 && ir::keyCode[digit] == code) {
            return ir::keyCommand[digit];
        }
    }

    unknownCodeSeen = true;
    return 0;
}

bool IrRemote::consumeUnknownCode() {
    const bool seen = unknownCodeSeen;
    unknownCodeSeen = false;
    return seen;
}

bool IrRemote::captureRaw(String& protocol, uint64_t& code, uint16_t& bits) {
    if (!decodeNext()) return false;

    protocol = typeToString(results.decode_type, results.repeat);
    code = results.value;
    bits = results.bits;

    receiver.resume();
    return true;
}

bool IrRemote::isConfigured() const {
    return ir::isTableFilled();
}
