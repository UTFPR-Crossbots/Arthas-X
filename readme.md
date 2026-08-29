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
| `a` | Calibrar a barra frontal (7 s) — também detecta sensores mortos |
| `d` | Stream dos dois sensores laterais |
| `off <n>` / `on <n>` | Tirar/devolver um sensor da barra à conta da linha |
| **Motores** (robô suspenso!) | |
| `j` / `k` | Testar motor esquerdo / direito |
| `f` | Testar os dois motores |
| `l` | Testar a sucção (ESC) |
| `c` | **Parar** — também sai de qualquer teste contínuo |
| **Modos** | |
| `e` | `chase` — segue a linha até receber `c` |
| `b` | `driveLap` — volta com contagem de marcas |
| **Constantes** | |
| `kp <valor>` | ex: `kp 50` — ver escala do PID abaixo |
| `ki <valor>` / `kd <valor>` | |
| `vel <0-255>` | Velocidade máxima |
| `marcas <n>` | Marcas para fechar a volta |
| `rev <0-100>` | Quanto a roda interna pode ir de ré, em % do `maxspeed` (padrão 50) |
| `suc <0-100>` | Throttle da sucção durante a corrida, em % (padrão 80) |

Os comandos com valor (`kp`, `vel`, ...) existem porque o pacote de texto de 29 caracteres do app é
inviável de digitar à mão. Esse pacote continua funcionando, para o app não quebrar.

---

## PID: o erro é normalizado

O erro entra no PID **normalizado para −1..+1**, não em unidades de posição.

Com 14 sensores a posição vai de 0 a 13000, então o erro cru chega a ±6500. Nessa escala qualquer
`Kp` acima de ~0,015 satura a correção e o robô vira bang-bang, com uma roda no máximo e a outra em
ré. Normalizado, **`Kp` é lido em unidades de velocidade por deflexão cheia**: `Kp` igual ao
`maxspeed` dá a diferença máxima entre as rodas no extremo da barra. Comece com `Kp` na ordem do
`maxspeed` (padrão 50) e Kd = 0.

**Constantes tunadas na escala antiga não valem mais.**

### Convenção de sinais

Valor alto = preto, valor baixo = branco, e `readLineWhite()` devolve o centróide do branco.
Com o **sensor 0 na esquerda** da barra:

| Linha | `pos` | `erro = setPoint - pos` | Rodas | Robô |
|---|---|---|---|---|
| Esquerda | baixo | positivo | esquerda desacelera, direita acelera | vira à **esquerda** |
| Centrada | 6500 | 0 | iguais | reto |
| Direita | alto | negativo | esquerda acelera, direita desacelera | vira à **direita** |

Ou seja, correção positiva **subtrai da esquerda e soma na direita**. Se um dia a barra for montada
espelhada (canal 0 do mux na direita), a linha a trocar é a do `Arthas::applyCorrection()`.

A roda interna pode entrar em ré para fechar curva fechada — é o `constrain(..., -maxspeed/2, ...)`
de antes, agora centralizado em `Arthas::applyCorrection()` e ajustável por `rev <0-100>`.
`rev 0` trava as rodas em só para frente.

---

## Sensor morto na barra

Um canal queimado ou desconectado calibra com faixa nula. Sem tratamento ele sai como 0 no
`readCalibrated()`, e o `1000 - valor` do `readLineWhite()` o transforma no **ponto mais branco da
barra** — um sensor morto sozinho passa a mandar na posição da linha e joga o robô para o lado dele.

Por isso a calibração (`a`) marca como desabilitado todo sensor cuja faixa branco–preto fique abaixo
de `minimumCalibrationRange` (100 contagens de ADC). Sensor desabilitado:

- sai da média ponderada da posição;
- é reportado como 1000 (preto / "não vejo linha"), que é o valor seguro;
- aparece como `x` no teste `h` e é listado após a calibração e no `s`.

`off <n>` e `on <n>` forçam na mão, para o caso de um sensor intermitente passar pela detecção
automática. Recalibrar refaz a detecção e sobrescreve o que foi forçado.

---

## Motor de sucção (ESC LittleBee Spring 20A)

Sinal no **GPIO 10** (net `CL`), canal 2 do LEDC. Liga automaticamente no `chase` e no `driveLap`,
e o `c` (parar) corta junto com os motores — `stopMotors()` desliga os dois.

O LittleBee Spring é um BLHeli_S: fala **PWM de servo a 50 Hz**, não o PWM de ponte H dos motores de
tração. Por isso tem driver próprio, [`SuctionMotor`](lib/Arthas/include/SuctionMotor.h), construído
sobre a **ESP32Servo**, e não reaproveita o `MotorArthas`.

### ⚠️ O neutro é 1488 µs, não 1000

Este ESC está em **modo bidirecional (3D)**: o neutro fica em ~1500 µs, e não em 1000 µs como num
ESC unidirecional. Mandar 1000 µs **não liga o motor** — a faixa útil para frente vai de
**1488 µs (parado) a 2000 µs (cheio)**.

- **Armação.** O BLHeli arma depois de receber o pulso neutro sustentado por ~3 s. O
  `setupSuction()` é a **primeira** coisa do `setup()` e **bloqueia** esses 3 s: até terminar o ESC
  ignora qualquer comando, então não há o que fazer em paralelo. O boot inteiro leva ~5 s.
- **Rampa.** Um ESC de 20 A puxando corrente num degrau derruba a tensão da placa. `setTarget()` só
  define o alvo; `update()` caminha até ele a **100 µs/s** (faixa toda em ~5 s, o mesmo ritmo da
  referência de bancada: 10 µs a cada 100 ms) e **não bloqueia** — é chamado de dentro da malha de
  controle, sem atrapalhar o seguimento de linha. Para largar mais rápido, subir
  `rampRate_usPerSecond`.

  `update()` limita cada passo a `maxStepInterval_ms` (50 ms). Sem esse teto, a primeira chamada
  depois de um intervalo parado — entre o boot e o comando de largada, por exemplo — veria o
  intervalo inteiro como `elapsed` e daria um passo que atravessa os 512 µs de uma vez, ligando o
  motor em degrau justamente onde a rampa deveria agir.
- **Parada** é imediata, sem rampa: é o caminho de segurança.

O `CL` não aparecia com função definida em nenhuma das folhas de esquemático, então o barramento SPI
(`MOSI` 11, `SCLK` 12, `MISO` 13) fica intacto — IMU e encoders seguem viáveis.

### ⚠️ A ESP32Servo rouba o timer dos motores se deixar

O core mapeia `timer = (canal / 2) % 4`, então canais no mesmo timer compartilham frequência:

| Timer | Canais | Uso | Frequência |
|---|---|---|---|
| 0 | 0 e 1 | Motores de tração (`ledc` na mão) | 20 kHz |
| 2 | 4 | Sucção / ESC (ESP32Servo) | 50 Hz |

A ESP32Servo, por padrão, procura timer livre **a partir do 0** — que é justamente o dos motores — e
o reconfiguraria para 50 Hz, quebrando o PWM das rodas. Por isso `SuctionMotor::setup()` chama
`ESP32PWM::allocateTimer(pins::suctionPwmTimer)`, que marca todos os timers como ocupados e libera
só o 2 para a biblioteca.

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
- **`CTRL` (GPIO 9)** não aparece em nenhuma das folhas de esquemático disponíveis; está reservado
  no `Pinout.h`, sem uso.

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
