# Protocolo UART — Juego de Reacción Competitivo

Comunicación entre LPC1769 y PC por UART1 (115200 baud, 8N1).

**Formato general:** ASCII, line-based, terminador `\r\n` (CR+LF). Esto hace que sea legible y usable desde terminales serie como PuTTY, además de fácil de parsear desde cualquier aplicación.

---

## RX: PC → LPC1769

La PC envía comandos. El LPC1769 acumula bytes en un buffer hasta recibir `\r\n`, y recién ahí procesa el comando. Esto aísla ruido y evita que bytes sueltos rompan el parser.

| Comando | Extensión | Qué hace | Formato |
|---------|-----------|----------|---------|
| `S` | `S` | **START_GAME**: sale de `E_IDLE` a `E_CONFIG` | `S\r\n` |
| `K` | `K<tecla>` | **SET_KEY**: define la tecla objetivo para la ronda | `KA\r\n` |
| `F` | `F<frecuencia>` | **SET_FREQ**: frecuencia del tono de cuenta regresiva en Hz | `F440\r\n` |
| `B` | `B<ms>` | **SET_BPM**: velocidad entre beeps en ms (máx 200) | `B200\r\n` |
| `C` | `C` | **CONFIG_DONE**: confirma que la config está completa; la FSM salta a `E_COUNTDOWN` | `C\r\n` |
| `?` | `?` | **QUERY**: pide estado actual del juego (scores, estado FSM) | `?\r\n` |
| `R` | `R` | **RESET**: reinicia los scores de ambos jugadores a cero | `R\r\n` |

### Notas sobre RX

- `F` acepta frecuencia en ASCII decimal (1 a 65535 Hz). Se parsea con `atoi()` o similar.
- `B` acepta velocidad en ASCII decimal (0 a 200). Si se supera 200, se limita a 200.
- Los comandos pueden ir en mayúscula o minúscula (sensibilidad _case-insensitive_).
- Si un comando es inválido (ej: `Z\r\n`), se ignora y no se envía respuesta de error — mantenerlo simple.

---

## TX: LPC1769 → PC

El LPC1769 envía mensajes de forma _unilateral_ (sin polling de la PC) a medida que ocurren eventos de juego. Cada mensaje es una línea independiente con `\r\n`.

| Código | Extensión | Qué significa | Cuándo se envía | Formato |
|--------|-----------|---------------|-----------------|---------|
| `W` | `W<jugador>` | **WINNER**: el jugador acertó la tecla objetivo | Al finalizar ronda con acierto | `W1\r\n` |
| `E` | `E<jugador>` | **ERROR**: el jugador presionó una tecla incorrecta | Al finalizar ronda con error | `E1\r\n` |
| `T` | `T` | **TIMEOUT**: ningún jugador presionó a tiempo | Al finalizar ronda sin actividad | `T\r\n` |
| `S` | `S<J1>,<J2>` | **SCORE**: scores acumulados de ambos jugadores | Después de cada ronda | `S5,3\r\n` |
| `R` | `R<µs>` | **REACTION**: tiempo de reacción del ganador en microsegundos | Solo si hay ganador (W1/W2), después del resultado | `R2346000\r\n` |
| `G` | `G<jugador>` | **GAME_OVER**: un jugador alcanzó `MAX_VICTORIAS` (10) | Al finalizar la partida | `G1\r\n` |

### Secuencia típica TX por ronda

```
W1             ← ganó J1
R1234000       ← tiempo de reacción 1,234 s
S5,3           ← scores actualizados
```

```
E2             ← J2 presionó tecla incorrecta
S5,2           ← scores actualizados (J2 perdió un punto)
```

```
T              ← timeout
S5,3           ← scores (sin cambios)
```

Para game over, se envía después de la última ronda:

```
W1
R2346000
S10,3
G1             ← J1 ganó la partida
```

---

## Cómo usar cada comando (ejemplos genéricos)

### PuTTY / terminal serie

Conectarse a 115200 baud, 8N1, sin control de flujo. Cada comando se escribe y se presiona Enter (`\r\n`).

```
S              → el juego sale de IDLE y pasa a CONFIG
KA             → setea tecla 'A' como objetivo
F440           → setea tono a 440 Hz
B150           → setea beeps cada 150 ms
C              → confirma configuración, arranca cuenta regresiva
```

### Aplicación externa (Java, Python, etc.)

Se puede abrir el puerto serie y enviar/recibir líneas de texto estándar. El `\r\n` al final de cada línea simplifica el parseo con `readLine()` o equivalente.

```python
# Ejemplo Python (pyserial)
import serial

ser = serial.Serial('COM3', 115200, timeout=1)

# Configurar partida
ser.write(b'S\r\n')
ser.write(b'KA\r\n')
ser.write(b'F440\r\n')
ser.write(b'B200\r\n')
ser.write(b'C\r\n')

# Leer resultados
while True:
    linea = ser.readline().decode().strip()
    if linea:
        print(f"Recibido: {linea}")
```

---

## Recomendaciones para PuTTY

1. **Configuración:**
   - _Speed_: 115200
   - _Data bits_: 8
   - _Parity_: None
   - _Stop bits_: 1
   - _Flow control_: None
   - _Terminal → Implicit CR in every LF_: desmarcado
   - _Terminal → Local echo_: **Forced on** (para ver lo que escribís)

2. **Uso:**
   - Escribís cada comando y presionás Enter.
   - Las respuestas del LPC1769 aparecen como líneas entrantes.
   - No hay eco de comandos desde el micro (es más simple así) — por eso el _Local echo_ en On.

3. **Limitaciones:**
   - No podés enviar `\r\n` desde PuTTY sin apretar Enter.
   - Con el formato ASCII + terminador `\r\n` no hay problema: cada Enter envía el comando completo.
   - No hay ACK ni NACK; si un comando es inválido se ignora silenciosamente. Para verificar, mandá `?` a ver si recibís los scores.

---

## Cambios necesarios en el firmware

Si se aprueba este protocolo, los cambios respecto al código actual son:

- **Parser RX:** cambiar de posicional (`K` espera 1 byte, `F` espera 2 bytes) a line-based: acumular en buffer hasta `\r\n`, después parsear el comando completo.
- **TX:** agregar `\r\n` al final de cada mensaje; enviar scores (`S`) y game over (`G`) además de los resultados de ronda.
- **Nuevos comandos:** implementar `?` (QUERY) y `R` (RESET).
- **Frecuencia `F`:** cambiar de 2 bytes raw a ASCII decimal.

---

*Versión: 2026-06-03 — Propuesta inicial*
