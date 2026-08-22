## Arthas-X

Firmware do seguidor de linha Arthas para a **placa nova (ESP32-S3-WROOM-1)**.
Os esquemáticos (EasyEDA) estão em `esquematicos_arthas/`.

Este repositório foi criado a partir do repositório Arthas-II; se quiser ver o histórico de commits
antigo, vá para tal repositório.

---

## Compilar e gravar

```
pio run -e esp32s3
pio run -e esp32s3 -t upload
```

A gravação é pelo USB nativo (GPIO 19/20). Se a placa não entrar sozinha no bootloader,
segure o botão SW3/BOOT enquanto dá reset.

Não há mais submódulos: o driver do TB6612FNG saiu do projeto. A única dependência externa é a
NimBLE-Arduino, que o PlatformIO baixa sozinho a partir do `platformio.ini`.

---

## Comunicação: BLE + menu de testes na serial

O robô aceita comandos por **dois canais ao mesmo tempo**, e a saída é espelhada nos dois.
Quem faz essa junção é a classe [`Console`](lib/Arthas/include/Console.h) — o despacho de comandos
não sabe (nem precisa saber) de onde a linha veio.

### 1. BLE — o modo de operação

O ESP32-S3 **não tem Bluetooth Clássico** — `BluetoothSerial` (SPP) não existe nele. O robô anuncia
por BLE com o **Nordic UART Service**, sob o nome `Arthas-S3`:

| | UUID |
|---|---|
| Serviço | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` |
| RX (o celular escreve) | `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` |
| TX (o robô notifica) | `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` |

Dá para testar com o nRF Connect ou com o "Serial Bluetooth Terminal" em modo BLE.
**O app Android antigo, que falava SPP, não conecta mais** e precisa ser portado para BLE.

### 2. Serial USB — para testar na bancada

```
pio device monitor -e esp32s3
```

O menu sai sozinho no boot; digite `?` a qualquer momento para vê-lo de novo.

Nesta placa o **UART0 (GPIO 43/44) não está conectado** — só existe o USB nativo (GPIO 19/20).
Por isso o `platformio.ini` compila com `-DARDUINO_USB_CDC_ON_BOOT=1`, que faz o `Serial` apontar
para o USB CDC. Sem essa flag o menu não sairia em lugar nenhum.

O CDC some do host durante o reset da placa; o monitor reconecta sozinho.

### Comandos

Valem nos dois canais.

| Comando | Ação |
|---|---|
| `?` ou `m` | Mostrar o menu |
| `s` | Estado atual (PID, velocidade, marcas, calibração, BLE) |
| **Sensores** | |
| `i` | Stream dos 14 sensores crus do ADC (0–4095) |
| `h` | Stream dos 14 sensores calibrados (0–1000) + posição da linha |
| `g` | Posição da linha, uma leitura |
| `a` | Calibrar a barra frontal (7 s) |
| `d` | Stream dos dois sensores laterais |
| **Motores** (robô suspenso!) | |
| `j` / `k` | Testar motor esquerdo / direito |
| `f` | Testar os dois motores |
| `c` | **Parar** — também sai de qualquer teste contínuo |
| **Modos** | |
| `e` | `chase` — segue a linha até receber `c` |
| `b` | `driveLap` — volta com contagem de marcas |
| **Constantes** | |
| `kp <valor>` | ex: `kp 0.35` |
| `ki <valor>` / `kd <valor>` | |
| `vel <0-255>` | Velocidade máxima |
| `marcas <n>` | Marcas para fechar a volta |

Os comandos com valor (`kp`, `vel`, ...) existem porque o pacote de texto de 29 caracteres do app é
inviável de digitar à mão. Esse pacote continua funcionando, para o app não quebrar.

---

## Hardware — o que mudou em relação à placa antiga

| | Antiga | Nova |
|---|---|---|
| MCU | ESP32 clássico | ESP32-S3-WROOM-1 |
| Barra frontal | 6 QTR em GPIOs 32, 33, 25, 26, 27, 14 | 14 QRE1113GR no mux CD74HC4067, tudo pelo GPIO 2 |
| Driver de motor | TB6612FNG (IN1+IN2+PWM) | 2x DRV8874 em modo PH/EN (direção + PWM) |
| Motor esquerdo | 16 / 17 / 4 | `L_DR` 17, `L_PWM` 18 |
| Motor direito | 5 / 18 / 19 | `R_DR` 21, `R_PWM` 47 |
| Laterais | 1 sensor no GPIO 34 | `L_IR` 4 (analógico), `R_IR` 42 (digital) |
| Rádio | Bluetooth Clássico (SPP) | BLE (NimBLE) |

A pinagem completa, incluindo os pinos ainda sem driver, está em
[`lib/Arthas/include/Pinout.h`](lib/Arthas/include/Pinout.h).

### Ressalvas de hardware

- **`R_IR` (GPIO 42) não tem ADC.** No ESP32-S3, ADC1 é GPIO 1–10 e ADC2 é GPIO 11–20; o 42 é `MTMS`.
  O lateral direito só pode ser lido digitalmente, com o limiar fixo do buffer de entrada — o
  esquerdo (GPIO 4) é que tem limiar ajustável, e é ele que conta as marcas de volta.
- **`R_CS` (GPIO 14) cai no ADC2**, compartilhado com o rádio; leitura pouco confiável.
- **`nFAULT` dos DRV8874 não chega ao ESP32** — o firmware não tem como detectar falha do driver.
- **GPIO 39–42 são os pinos de JTAG**, então não há debug por JTAG nesta placa.
- **`CTRL` (GPIO 9) e `CL` (GPIO 10)** não aparecem em nenhuma das folhas de esquemático
  disponíveis; estão reservados no `Pinout.h`, sem uso.

### Ainda sem driver

Encoders SPI (`CS_LE` 16, `CS_RE` 48), IMU (`CS_IMU` 15, `INT1` 6, `INT2` 7), buzzer (5),
leitura de bateria (`VBAT` 1) e sense de corrente dos motores (`L_CS` 8, `R_CS` 14).
Os pinos já estão mapeados em `Pinout.h`.

---

## Ordem recomendada de bring-up

Tudo pela serial USB (`pio device monitor -e esp32s3`), sem precisar do celular.

1. `i` — passar uma linha branca devagar sob a barra. Os valores devem **cair** um vizinho de cada
   vez. Se dois canais mudarem juntos, aumentar `channelSettleTime_us` em `FrontSensor.cpp`;
   se a ordem sair embaralhada, os bits S0–S3 estão trocados no `Pinout.h`.
2. `a` e depois `h` — conferir que cada sensor chega a 0 e a 1000 nos extremos.
3. `j` e `k` com o robô suspenso — confirmar o sentido de cada roda e ajustar
   `leftMotorInverted` / `rightMotorInverted` no `Pinout.h` se estiver invertido.
4. `d` sobre a marca branca e fora dela — fixar o limiar do lateral esquerdo a partir dos valores
   medidos (parâmetro do construtor de `LateralSensor`).
5. `e` com `maxspeed` baixo (~40) e Kd = 0, subindo Kp até oscilar. Só então `b`.
