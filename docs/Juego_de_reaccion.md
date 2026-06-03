# Juego de reacción competitivo

Sistema interactivo para 2 jugadores utilizando el microcontrolador LPC1769.

Cada jugador debe presionar la tecla asignada cuando esta aparece indicada en la pantalla LCD luego de la secuencia de cuenta regresiva auditiva. El micro mide tiempos de reacción con precisión de microsegundos (Timer Input Capture), calcula estadísticas y anuncia ganador por UART.

---

## Características principales

- Medición de tiempos de reacción con precisión de microsegundos.
- Generación de audio mediante DAC utilizando DMA y temporizadores.
- Interfaz visual mediante LCD 16x2.
- Ingreso mediante teclado matricial 4x4.
- Comunicación serial con aplicación Java en PC.
- Arquitectura basada en interrupciones y máquina de estados (FSM).
- Estadísticas y gestión de partidas en tiempo real.

---

## Arquitectura de hardware

### LPC1769

Microcontrolador principal encargado de:

- control del juego,
- temporización,
- captura temporal,
- generación de audio,
- comunicación serial,
- manejo de interrupciones.

### Teclado matricial 4x4

Utilizado para:

- ingreso de teclas de jugadores,
- validación de respuestas,
- detección de eventos de juego.

Las teclas utilizadas por los jugadores estarán asociadas además a entradas Capture dedicadas para permitir medición temporal por hardware independiente del scanning del teclado.

### LCD 16x2 por I2C

Utilizado para mostrar:

- tecla objetivo,
- estado del juego,
- mensajes de error,
- ganador,
- tiempos de reacción.

### DAC + DMA

El DAC genera:

- cuenta regresiva auditiva,
- tono de inicio ("GO!"),
- sonidos de error,
- sonidos de victoria.

La reproducción de audio se realiza utilizando DMA disparado por temporizadores, permitiendo streaming de samples desde memoria hacia el DAC sin intervención continua de CPU.

### Aplicación Java en PC

Permite:

- iniciar partidas,
- configurar teclas,
- seleccionar dificultad,
- visualizar estadísticas,
- recibir resultados en tiempo real.

La comunicación con el LPC1769 se realiza mediante UART.

---

## Funcionamiento general

### Flujo del juego

1. La aplicación Java configura la partida mediante UART.
2. El LPC1769 entra en estado de espera.
3. Se inicia la cuenta regresiva auditiva mediante DAC.
4. Al finalizar el último tono:
   - el LCD muestra la tecla objetivo,
   - se habilitan los módulos Capture,
   - comienza la medición temporal.
5. El primer jugador que presione correctamente su tecla genera un evento Capture.
6. El hardware almacena automáticamente el timestamp.
7. El firmware calcula:
   - tiempo de reacción,
   - ganador,
   - estadísticas.
8. Los resultados son enviados a la PC.

---

## Uso de periféricos

### Timer Input Capture

Medición precisa de tiempos de reacción mediante captura por hardware del instante de activación de cada jugador.

La captura temporal se realiza con resolución de ciclos de reloj, evitando errores producidos por polling o latencia de software.

### DAC con DMA

El DAC posee un rol funcional dentro del sistema y no decorativo.

Los sonidos son generados mediante buffers de samples almacenados en memoria RAM y transferidos automáticamente al DAC utilizando DMA sincronizado por Timer Match.

Esto permite:

- reproducción de audio sin bloquear CPU,
- temporización precisa,
- desacoplamiento entre lógica del juego y generación de audio.

### Timer Match

Utilizado para:

- scheduler del sistema,
- scanning periódico del teclado,
- temporización de audio,
- timeout de ronda,
- eventos de juego.

### UART

Comunicación bidireccional con la aplicación Java.

Comandos soportados:

- `START_GAME`
- `GET_STATS`
- `SET_KEYS`
- `RESET_SCORES`

### I2C

Comunicación con pantalla LCD 16x2.

---

## Flujo de interrupciones

### Timer IRQ

- scheduler del sistema,
- scanning del teclado,
- timeout de ronda,
- sincronización de audio.

### Capture IRQ

- almacenamiento automático de timestamps de reacción.

### DMA IRQ

- control de finalización de reproducción de audio.

### UART RX IRQ

- recepción de comandos desde PC.

### I2C Event IRQ

- actualización de LCD.

---

## Máquina de estados (FSM)

Estados principales:

- `IDLE`
- `CONFIG`
- `COUNTDOWN`
- `WAIT_GO`
- `WAIT_INPUT`
- `ROUND_END`
- `GAME_OVER`

---

## Biblioteca de funciones

| Función | Descripción |
|---|---|
| `Reaction_Start()` | Inicia una nueva ronda y habilita los módulos de captura. |
| `Reaction_GetTime(player) → uint32_t us` | Devuelve el tiempo de reacción del jugador en microsegundos. |
| `Stats_GetWinner() → player_id` | Obtiene el jugador ganador de la ronda. |
| `Audio_Play(buffer, size)` | Reproduce un buffer de audio utilizando DAC + DMA. |
| `Audio_Stop()` | Detiene la reproducción de audio. |
| `Display_ShowKey(player, key)` | Actualiza la pantalla LCD con la tecla objetivo del jugador. |
| `UART_SendStats()` | Envía estadísticas y resultados hacia la aplicación Java. |
