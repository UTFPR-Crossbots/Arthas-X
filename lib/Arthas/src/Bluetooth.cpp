#include <Bluetooth.h>
#include <NimBLEDevice.h>

namespace {
    /* Nordic UART Service — é o que os apps de terminal BLE (nRF Connect,
     * Serial Bluetooth Terminal em modo BLE) já falam. */
    const char* nusServiceUuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
    const char* nusRxUuid      = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";  // celular escreve
    const char* nusTxUuid      = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";  // robô notifica

    const char* deviceName = "Arthas-S3";

    /* Cliente que não manda '\n': fecha a mensagem depois desta pausa. */
    const unsigned long rxIdleFlush_ms = 30;

    /* Trava o consumo de RAM se ninguém estiver lendo a fila. */
    const size_t maxQueuedLines = 16;

    const uint16_t minimumMtu = 23;
}

class ArthasServerCallbacks : public NimBLEServerCallbacks {
    public:
        ArthasServerCallbacks(Bluetooth* owner) : owner(owner) {}

        void onConnect(NimBLEServer* server, ble_gap_conn_desc* desc) override {
            owner->connected = true;
            owner->connectionHandle = desc->conn_handle;
            owner->pushLine("Cliente Conectado!");
        }

        void onDisconnect(NimBLEServer* server) override {
            owner->connected = false;
            owner->pushLine("Cliente Desconectado!");
            NimBLEDevice::startAdvertising();
        }

    private:
        Bluetooth* owner;
};

class ArthasRxCallbacks : public NimBLECharacteristicCallbacks {
    public:
        ArthasRxCallbacks(Bluetooth* owner) : owner(owner) {}

        void onWrite(NimBLECharacteristic* characteristic) override {
            const std::string value = characteristic->getValue();
            if (value.empty()) return;

            owner->pushIncoming((const uint8_t*)value.data(), value.length());
        }

    private:
        Bluetooth* owner;
};

Bluetooth::Bluetooth() :
	server(nullptr),
	txCharacteristic(nullptr),
	rxMutex(nullptr),
	rxAccumulator(""),
	lastRxTime(0),
	connected(false),
	connectionHandle(0)
{}

Bluetooth::~Bluetooth() {}

void Bluetooth::setup() {
	rxMutex = xSemaphoreCreateMutex();

	NimBLEDevice::init(deviceName);
	NimBLEDevice::setMTU(247);      // pede MTU maior; o cliente pode recusar

	server = NimBLEDevice::createServer();
	server->setCallbacks(new ArthasServerCallbacks(this));

	NimBLEService* service = server->createService(nusServiceUuid);

	txCharacteristic = service->createCharacteristic(nusTxUuid, NIMBLE_PROPERTY::NOTIFY);

	NimBLECharacteristic* rxCharacteristic = service->createCharacteristic(
		nusRxUuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
	rxCharacteristic->setCallbacks(new ArthasRxCallbacks(this));

	service->start();

	NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
	advertising->addServiceUUID(nusServiceUuid);
	advertising->setScanResponse(true);
	advertising->start();
}

/* --- Recepção --- */

void Bluetooth::pushIncoming(const uint8_t* data, const size_t length) {
	for (size_t i = 0; i < length; i++) {
		const char character = (char)data[i];

		if (character == '\n' || character == '\r') {
			if (rxAccumulator.length() > 0) {
				pushLine(rxAccumulator);
				rxAccumulator = "";
			}
		} else {
			rxAccumulator += character;
		}
	}

	lastRxTime = millis();
}

void Bluetooth::pushLine(const String& line) {
	if (rxMutex == nullptr) return;

	xSemaphoreTake(rxMutex, portMAX_DELAY);
	if (rxLines.size() >= maxQueuedLines) rxLines.pop_front();
	rxLines.push_back(line);
	xSemaphoreGive(rxMutex);
}

void Bluetooth::flushAccumulatorIfIdle() {
	if (rxAccumulator.length() == 0) return;
	if (millis() - lastRxTime < rxIdleFlush_ms) return;

	pushLine(rxAccumulator);
	rxAccumulator = "";
}

bool Bluetooth::isAvailable() {
	flushAccumulatorIfIdle();

	if (rxMutex == nullptr) return false;

	xSemaphoreTake(rxMutex, portMAX_DELAY);
	const bool hasData = !rxLines.empty();
	xSemaphoreGive(rxMutex);

	return hasData;
}

String Bluetooth::readSerialInput() {
	if (rxMutex == nullptr) return "";

	String line = "";

	xSemaphoreTake(rxMutex, portMAX_DELAY);
	if (!rxLines.empty()) {
		line = rxLines.front();
		rxLines.pop_front();
	}
	xSemaphoreGive(rxMutex);

	return line;
}

bool Bluetooth::isConnected() {
	return connected;
}

/* --- Transmissão --- */

uint16_t Bluetooth::maxNotifyLength() {
	uint16_t mtu = minimumMtu;

	if (server != nullptr && connected) {
		const uint16_t peerMtu = server->getPeerMTU(connectionHandle);
		if (peerMtu > mtu) mtu = peerMtu;
	}

	return mtu - 3;     // 3 bytes de cabeçalho ATT
}

void Bluetooth::send(const String& msg) {
	if (txCharacteristic == nullptr || !connected || msg.length() == 0) return;

	const uint16_t chunkSize = maxNotifyLength();

	for (uint16_t offset = 0; offset < msg.length(); offset += chunkSize) {
		const String chunk = msg.substring(offset, offset + chunkSize);
		txCharacteristic->setValue((uint8_t*)chunk.c_str(), chunk.length());
		txCharacteristic->notify();
	}
}

void Bluetooth::print(const String msg) {
	send(msg);
}

void Bluetooth::print(const int msg) {
	send(String(msg));
}

void Bluetooth::println(const String msg) {
	send(msg + "\n");
}

void Bluetooth::println(const int msg) {
	send(String(msg) + "\n");
}

void Bluetooth::println(const uint16_t msg) {
	send(String(msg) + "\n");
}

void Bluetooth::printDoubleln(const double msg) {
	send(String(msg, 4) + "\n");
}

void Bluetooth::turnOff() {
	NimBLEDevice::stopAdvertising();

	if (server != nullptr && connected) {
		server->disconnect(connectionHandle);
	}

	connected = false;
}
