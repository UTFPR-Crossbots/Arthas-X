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

Dependências (o PlatformIO baixa sozinho): **ESP32Servo** para o pulso do ESC de sucção e
**IRremoteESP8266** para o controle remoto.

---

## Controle: infravermelho na pista, serial na bancada

O robô aceita comandos por **dois canais ao mesmo tempo**, e os dois entram na mesma fila:
o IR traduz a tecla numérica para o mesmo comando de uma letra que a serial usa. Quem faz essa
junção é a classe [`Console`](lib/Arthas/include/Console.h) — o despacho de comandos não sabe
(nem precisa saber) de onde veio a instrução.

O Bluetooth saiu do firmware: o ESP32-S3 não tem Bluetooth Clássico, o app Android antigo (SPP) já
não conectava, e o controle de pista agora é o IR.

### 1. Controle remoto (LE-7009), receptor no GPIO 9

Como o controle não tem canal de volta, o **buzzer** (GPIO 5) é o único retorno na pista:

| Evento | Beep |
|---|---|
| Comando reconhecido | 1 curto |
| Parada (tecla 0) | 2 curtos |
| Código IR fora da tabela | 1 longo |
| Boot concluído / calibração terminada | 3 curtos |

### 2. Serial USB, para bancada

```
pio device monitor -e esp32s3
```

O menu sai sozinho no boot (~5 s, por causa da armação do ESC); `?` mostra de novo.

Nesta placa o **UART0 (GPIO 43/44) não está conectado** — só existe o USB nativo. Por isso o
`platformio.ini` compila com `-DARDUINO_USB_CDC_ON_BOOT=1`, que faz o `Serial` apontar para o USB
CDC. Sem essa flag o menu não sairia em lugar nenhum.

### Comandos

| Tecla IR | Serial | Ação |
|---|---|---|
| `1` | `a` | Calibrar a barra (7 s) — também detecta sensores mortos |
| `2` | `e` | `chase` — segue a linha até parar |
| `3` | `b` | Volta com contagem de marcas |
| `4` | `i` | Barra frontal — cru do ADC (0–4095) |
| `5` | `h` | Barra frontal — calibrado (0–1000) + posição |
| `6` | `d` | Sensores laterais |
| `7` | `j` | Motor esquerdo |
| `8` | `k` | Motor direito |
| `9` | `l` | Sucção (ESC) |
| `0` | `c` | **PARAR** — também sai de qualquer teste contínuo |

Só na serial: `f` (os dois motores), `g` (posição, uma leitura), `x` (capturar códigos IR),
`off <n>` / `on <n>` (tirar/devolver um sensor da barra), `s` (estado), `?` (menu).

**Não há mais comandos para mudar constantes em runtime** — ver `Tuning.h` abaixo.

---

## ⚠️ O controle IR não funciona antes de capturar os códigos

Não há como adivinhar os códigos de um LE-7009: eles têm de ser lidos do próprio controle. A tabela
em [`IrCodes.h`](lib/Arthas/include/IrCodes.h) **nasce zerada**, e enquanto estiver assim o robô não
responde ao controle (o menu avisa).

1. Grave, abra a serial e mande `x`.
2. Aperte 0–9 no controle, um de cada vez. Cada tecla imprime protocolo, bits e código em hex.
3. Copie os dez valores para `IrCodes.h`, na ordem dos dígitos.
4. Mande `c` para sair, recompile e grave.

Segurar a tecla gera frames de repetição do NEC; eles são descartados, e ainda há uma janela de
debounce de 250 ms sobre o mesmo código, para um toque um pouco mais longo não disparar o comando
várias vezes.

---

## Tuning.h — constantes no código

PID, velocidade, marcas por volta, ré da roda interna e throttle da sucção ficam em
[`Tuning.h`](lib/Arthas/include/Tuning.h). Ajustar lá e recompilar; o `s` continua imprimindo os
valores em uso.

### O erro do PID é normalizado

O erro entra no PID **normalizado para −1..+1**, não em unidades de posição. Com 14 sensores a
posição vai de 0 a 13000, então o erro cru chega a ±6500 — nessa escala qualquer `Kp` acima de
~0,015 satura a correção e o robô vira bang-bang. Normalizado, **`Kp` é lido em unidades de
velocidade por deflexão cheia**: `Kp` igual ao `maxSpeed` dá a diferença máxima entre as rodas no
extremo da barra.

**Constantes tunadas na escala antiga não valem.**

### Convenção de sinais

Valor alto = preto, valor baixo = branco, e `readLineWhite()` devolve o centróide do branco.
Com o **sensor 0 na esquerda** da barra:

| Linha | `pos` | `erro = setPoint - pos` | Rodas | Robô |
|---|---|---|---|---|
| Esquerda | baixo | positivo | esquerda desacelera, direita acelera | vira à **esquerda** |
| Centrada | 6500 | 0 | iguais | reto |
| Direita | alto | negativo | esquerda acelera, direita desacelera | vira à **direita** |

Correção positiva **subtrai da esquerda e soma na direita**. Se um dia a barra for montada
espelhada (canal 0 do mux na direita), a linha a trocar é a do `Arthas::applyCorrection()`.

A roda interna pode entrar em ré para fechar curva fechada, limitada por `reverseRatio`.
`reverseRatio = 0` trava as rodas em só para frente.

---

## Sensor morto na barra

Um canal queimado ou desconectado calibra com faixa nula. Sem tratamento ele sai como 0 no
`readCalibrated()`, e o `1000 - valor` do `readLineWhite()` o transforma no **ponto mais branco da
barra** — um sensor morto sozinho passa a mandar na posição da linha e joga o robô para o lado dele.

Por isso a calibração marca como desabilitado todo sensor cuja faixa branco–preto fique abaixo de
`minimumCalibrationRange` (100 contagens de ADC). Sensor desabilitado sai da média ponderada, é
reportado como 1000 (preto / "não vejo linha") e aparece como `x` no teste `h`.

`off <n>` e `on <n>` forçam na mão, para o caso de um sensor intermitente passar pela detecção
automática. Recalibrar refaz a detecção e sobrescreve o que foi forçado.

---

## Motor de sucção (ESC LittleBee Spring 20A)

Sinal no **GPIO 10** (net `CL`). Liga automaticamente no `chase` e no `driveLap`, e a parada corta
junto com os motores.

O LittleBee Spring é um BLHeli_S: fala **PWM de servo a 50 Hz**, não o PWM de ponte H dos motores de
tração. Por isso tem driver próprio, [`SuctionMotor`](lib/Arthas/include/SuctionMotor.h), construído
sobre a **ESP32Servo**.

### ⚠️ O neutro é 1488 µs, não 1000

Este ESC está em **modo bidirecional (3D)**: o neutro fica em ~1500 µs. Mandar 1000 µs **não liga o
motor** — a faixa útil para frente vai de **1488 µs (parado) a 2000 µs (cheio)**.

### Rampas de subida e de descida

Um ESC de 20 A puxando corrente num degrau derruba a tensão da placa. `setTarget()` só define o
alvo; `update()` caminha até ele, sem bloquear:

| Sentido | Taxa | Faixa toda |
|---|---|---|
| Subida | 100 µs/s | ~5 s |
| Descida | 300 µs/s | ~1,7 s |

A subida usa o ritmo da referência de bancada (10 µs a cada 100 ms). A descida é mais rápida porque
não tem o pico de partida, e uma parada lenta demais é ruim num comando que também serve de
segurança.

**`Arthas::update()` precisa rodar toda iteração** — no `loop()` principal e dentro de cada loop de
modo. É o `loop()` principal que permite a rampa de descida existir: `stopMotors()` retorna
imediatamente, e sem alguém continuando a chamar `update()` a rampa congelaria no ponto em que o
modo terminou, deixando a sucção girando depois do comando de parada.

`update()` também limita cada passo a `maxStepInterval_ms` (50 ms). Sem esse teto, a primeira
chamada depois de um intervalo parado veria o intervalo inteiro como `elapsed` e daria um passo que
atravessa os 512 µs de uma vez.

### Timers: quem usa o quê

| Periférico | Timer | Uso |
|---|---|---|
| LEDC | 0 (canais 0 e 1) | Motores de tração, 20 kHz |
| LEDC | 2 (canal 4) | Sucção / ESC, 50 Hz (ESP32Servo) |
| Timer de propósito geral | 1 | Amostragem do IR (IRremoteESP8266) |

A ESP32Servo, por padrão, procura timer LEDC livre **a partir do 0** — que é o dos motores — e o
reconfiguraria para 50 Hz, quebrando o PWM das rodas. Por isso `SuctionMotor::setup()` chama
`ESP32PWM::allocateTimer(pins::suctionPwmTimer)`. O IR usa um periférico diferente (timer group),
então não entra nessa disputa.

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
| Sucção | — | ESC LittleBee no `CL` 10 |
| Controle | Bluetooth Clássico (SPP) | Infravermelho no `CTRL` 9 + serial USB |
| Feedback | — | Buzzer ativo no `BUZZER` 5 |

A pinagem completa está em [`lib/Arthas/include/Pinout.h`](lib/Arthas/include/Pinout.h).

### Ressalvas de hardware

- **`R_IR` (GPIO 42) não tem ADC.** No ESP32-S3, ADC1 é GPIO 1–10 e ADC2 é GPIO 11–20; o 42 é `MTMS`.
  O lateral direito só pode ser lido digitalmente — o esquerdo (GPIO 4) é que tem limiar ajustável,
  e é ele que conta as marcas de volta.
- **`R_CS` (GPIO 14) cai no ADC2**, compartilhado com o rádio; leitura pouco confiável.
- **`nFAULT` dos DRV8874 não chega ao ESP32** — o firmware não tem como detectar falha do driver.
- **GPIO 39–42 são os pinos de JTAG**, então não há debug por JTAG nesta placa.
- **Pull-up do `CTRL` (GPIO 9):** esse pino não aparece em nenhuma das folhas de esquemático
  disponíveis, então não dá para saber se a placa traz pull-up. O `IrRemote::setup()` liga o pull-up
  interno, que é inofensivo caso já exista um externo.
- **IR na pista é frágil.** Iluminação forte de arena e luz solar saturam receptores de 38 kHz, e o
  alcance cai muito fora do eixo. Vale medir o alcance real na arena antes de depender só disso.

### Ainda sem driver

Encoders SPI (`CS_LE` 16, `CS_RE` 48), IMU (`CS_IMU` 15, `INT1` 6, `INT2` 7), leitura de bateria
(`VBAT` 1) e sense de corrente dos motores (`L_CS` 8, `R_CS` 14). Os pinos já estão mapeados em
`Pinout.h`.

---

## Ordem recomendada de bring-up

1. **Buzzer:** ouvir os 3 beeps no fim do boot. Se ficar mudo, inverter `buzzerActiveLow` no
   `Pinout.h` — alguns módulos ativos apitam com nível baixo.
2. **Códigos IR (`x`):** capturar as teclas 0–9 e preencher o `IrCodes.h`. Nada de IR funciona antes
   disso.
3. **IR:** cada tecla dispara a ação certa e sai 1 beep; segurar a tecla **não** repete; tecla fora
   do mapa dá o beep longo. Repetir a 2–3 m e fora do eixo.
4. **Barra (`4`):** passar uma linha branca devagar. Os valores devem **cair** um vizinho de cada
   vez. Se dois canais mudarem juntos, aumentar `channelSettleTime_us` em `FrontSensor.cpp`;
   se a ordem sair embaralhada, os bits S0–S3 estão trocados no `Pinout.h`.
5. **Calibrar (`1`) e conferir (`5`):** cada sensor chega a 0 e a 1000 nos extremos.
6. **Motores (`7`/`8`) com o robô suspenso:** confirmar o sentido de cada roda e ajustar
   `leftMotorInverted` / `rightMotorInverted` no `Pinout.h`.
7. **Sucção (`9` e depois `0`):** o throttle impresso sobe em ~5 s e desce em ~1,7 s. A descida tem
   de continuar **depois** do comando de parada retornar — é isso que prova que o `update()` do
   `loop()` está rodando.
8. **Laterais (`6`)** sobre a marca branca e fora dela, para fixar o limiar do lateral esquerdo.
9. **Pista:** `2` (chase) com `maxSpeed` baixo (~40) e Kd = 0, subindo Kp até oscilar. Só então `3`.
