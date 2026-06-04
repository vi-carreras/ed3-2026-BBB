# Juego de reacción competitivo

Sistema interactivo para 2 jugadores utilizando el microcontrolador LPC1769.

Cada jugador debe presionar la tecla asignada cuando esta aparece indicada en la pantalla LCD luego de la secuencia de cuenta regresiva auditiva. El micro mide tiempos de reacción con precisión de microsegundos (Timer Input Capture) y envía resultados por UART.

---

## Características principales

- Medición de tiempos de reacción con precisión de microsegundos mediante Timer Input Capture.
- Generación de audio mediante DAC con DMA disparado por Timer0.
- Interfaz visual mediante LCD 16x2 por I2C (PCF8574).
- Ingreso mediante teclado matricial 4x4 (uno por jugador).
- Comunicación serial UART a 115200 baud, protocolo ASCII line-based.
- Arquitectura basada en interrupciones y máquina de estados (FSM).
- Gestión de rondas, puntajes (máximo 10 victorias) y penalización por tecla incorrecta.

---

## Arquitectura de hardware

### LPC1769

Microcontrolador principal encargado de:

- control del juego,
- temporización (SysTick cada 1 ms, Timers 0 y 1),
- captura temporal por hardware (Timer1 Capture),
- generación de audio (DAC + DMA + Timer0),
- comunicación serial (UART1),
- manejo de interrupciones.

### Teclado matricial 4x4

Dos teclados independientes:

- **Jugador 1** conectado a **Puerto 0** (P0[3:0] salidas, P0[7:4] entradas).
- **Jugador 2** conectado a **Puerto 2** (P2[3:0] salidas, P2[7:4] entradas).

Cada tecla presionada además activa una entrada Capture dedicada (CAP1.0 para J1, CAP1.1 para J2) para medición temporal por hardware, independiente del scanning del teclado.

### LCD 16x2 por I2C

Controlador PCF8574, dirección `0x27`. Protocolo 4-bit mode estándar HD44780.

Utilizado para mostrar:

- estado del juego,
- tecla objetivo,
- mensajes de ronda (3...2...1...GO!),
- puntajes acumulados,
- ganador de la partida.

### DAC + DMA

El DAC genera:

- cuenta regresiva auditiva (tonos de 150 ms por número),
- silencios entre tonos (duración configurable),
- tono de inicio ("GO!") al doble de frecuencia,
- tonos de victoria (3 tonos ascendentes: 440, 660, 880 Hz).

La reproducción se realiza con DMA disparado por Timer0 Match0 a 10 KHz, transfiriendo samples desde RAM al DAC sin intervención de CPU.

### Comunicación serial (UART1)

Configuración: **115200 baud, 8N1**, sin control de flujo.

Protocolo ASCII line-based con terminador `\r\n`. Ver [protocolo-uart.md](protocolo-uart.md) para detalle completo de comandos.

---

## Funcionamiento general

### Flujo del juego

1. La PC envía `S` para iniciar una partida.
2. La PC configura tecla objetivo (`K`), frecuencia de tono (`F`) y velocidad de beeps (`B`).
3. La PC confirma con `C` y comienza la cuenta regresiva auditiva (3...2...1...GO!).
4. Al finalizar "GO!":
   - el LCD muestra la tecla objetivo a presionar,
   - se habilitan los módulos Capture,
   - comienza la medición temporal con resolución de 1 µs.
5. El primer jugador que presiona su tecla genera un evento Capture que almacena el timestamp por hardware.
6. La interrupción Capture determina:
   - qué jugador presionó,
   - si la tecla es correcta o no,
   - el tiempo de reacción en microsegundos.
7. Los resultados se envían por UART y se muestran en LCD.
8. La partida continúa hasta que un jugador alcanza 10 victorias.

---

## Máquina de estados (FSM)

| Estado | Descripción |
|--------|-------------|
| `E_IDLE` | Pantalla inicial. Espera comando `S` para comenzar. |
| `E_CONFIG` | Espera comandos de configuración (`K`, `F`, `B`) hasta recibir `C`. |
| `E_COUNTDOWN` | Reproduce 3...2...1...GO! con audio por DMA. Transiciones manejadas por `countdown_phase` + `flag_dma_audio_done`. |
| `E_WAIT_GO` | Muestra tecla objetivo en LCD, habilita Timer1 Capture. |
| `E_WAIT_INPUT` | Espera capture event o timeout de 5 segundos. |
| `E_ROUND_END` | Procesa resultado (acierto/error/timeout), actualiza scores, envía datos por UART. Pausa no bloqueante de 2 segundos. |
| `E_GAME_OVER` | Muestra ganador en LCD, reproduce tonos de victoria, vuelve a `E_IDLE`. |

---

## Periféricos

### SysTick

- Interrupción cada 1 ms para el contador `msTicks`.
- Usado para timeouts de ronda (5 s), pausa no bloqueante de round end (2 s), y retardos de audio en game over.

### Timer0

- Configurado en modo Match, sin interrupción.
- MR0 a 100 µs → 10 KHz, dispara solicitudes DMA para el DAC.
- Habilitado/deshabilitado por `Audio_Play()` / `Audio_Stop()`.

### Timer1

- Dos canales Capture: CAP1.0 (P1.18, J1) y CAP1.1 (P1.19, J2).
- Prescaler a 1 µs, flanco de subida, interrupción habilitada.
- En la IRQ se lee el timestamp, se escanea el teclado correspondiente, y se determina `resultado_ronda`.

### DMA (GPDMA)

- Canal 0, modo memoria-a-periférico (M2P).
- Fuente: buffer de audio en RAM. Destino: `DACR`.
- Trigger: Timer0 Match0. Transferencia completa dispara `DMA_IRQHandler`.

### UART1

- RX por interrupción (`UART1_IRQHandler`): buffer circular de línea, parsea comandos al recibir `\r\n`.
- TX por polling (`enviar_char` / `enviar_str`): espera THRE antes de enviar cada byte.

### I2C0

- 100 KHz, maestro. Comunicación con LCD a través de PCF8574 en modo polling.

---

## Flujo de interrupciones

| IRQ | Disparador | Acción |
|-----|------------|--------|
| SysTick | Cada 1 ms | Incrementa `msTicks` |
| Timer1 Capture | CAP1.0 o CAP1.1 | Guarda timestamp, escanea teclado, determina resultado |
| DMA | Fin de transferencia | Setea `flag_dma_audio_done` |
| UART1 RX | Byte recibido | Acumula en buffer de línea, procesa comando al recibir `\r\n` |

---

## Módulos de software

### `lcd.h` / `lcd.c`

Driver LCD 16x2 por I2C (PCF8574), modo 4-bit.

| Función | Descripción |
|---------|-------------|
| `mensajeLCD(linea1, linea2)` | Escribe dos líneas de 16 caracteres en LCD |
| `retardo_ms(ms)` | Espera bloqueante en milisegundos (basada en `msTicks`) |

### `teclado.h` / `teclado.c`

Escaneo de teclado matricial 4×4. Cada función recorre filas, pone una a 0, y lee columnas para detectar la tecla presionada.

| Función | Descripción |
|---------|-------------|
| `EscanearTecladoJ1()` | Escanea teclado de J1 en Puerto 0. Devuelve caracter ASCII o 0. |
| `EscanearTecladoJ2()` | Escanea teclado de J2 en Puerto 2. Devuelve caracter ASCII o 0. |

### `uart.h` / `uart.c`

Funciones de transmisión UART.

| Función | Descripción |
|---------|-------------|
| `enviar_char(dato)` | Envía un byte por UART1 (espera THRE) |
| `enviar_str(str)` | Envía un string null-terminated llamando a `enviar_char` por cada caracter |

### `audio.h` / `audio.c`

Generación y reproducción de tonos por DAC + DMA + Timer0.

| Función | Descripción |
|---------|-------------|
| `Audio_GenerateTone(buffer, samples, frecuencia)` | Llena el buffer con una onda senoidal de la frecuencia indicada |
| `Audio_GenerateSilence(buffer, samples)` | Llena el buffer con el valor de bias (silencio) |
| `Audio_Play(buffer, size)` | Configura DMA y arranca reproducción |
| `Audio_Stop()` | Detiene DMA y Timer0, limpia DAC |

---

## Protocolo UART

Ver [protocolo-uart.md](protocolo-uart.md) para especificación completa de comandos RX y mensajes TX.

---

## Historial de revisiones

| Fecha | Versión | Cambios |
|-------|---------|---------|
| 2026-06-03 | 2.0 | Actualizada con implementación real: FSM completa, módulos, protocolo line-based |
| Original | 1.0 | Especificación inicial del proyecto |
