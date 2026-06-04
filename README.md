# ed3-2026-BBB

Repositorio para trabajo práctico final de la materia.

Juego de reacción competitivo para 2 jugadores sobre **LPC1769** — medición de tiempos de reacción con precisión de microsegundos usando Timer Input Capture, generación de audio por DAC + DMA, LCD 16x2 por I2C, y comunicación serial con PC.

- [Especificación del sistema](docs/Juego_de_reaccion.md)
- [Protocolo UART](docs/protocolo-uart.md)
- `src/` — firmware (`main.c`, `lcd.c`, `teclado.c`, `audio.c`)
- `inc/` — headers (`lcd.h`, `teclado.h`, `audio.h`)
