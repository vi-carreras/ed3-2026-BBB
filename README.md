# ed3-2026-BBB

Repositorio para trabajo práctico final de la materia.

Juego de reacción competitivo para 2 jugadores sobre **LPC1769** — medición de tiempos de reacción con precisión de microsegundos usando Timer Input Capture, generación de audio por DAC en modo polling, LCD 16x2 por I2C, y comunicación serial con PC.

- [Especificación del sistema](docs/Juego_de_reaccion.md)
- [Protocolo UART](docs/protocolo-uart.md)
- `MainTpF/` — proyecto funcional presentado para MCUXpresso.
- `src/` — versión modularizada del firmware (`main.c`, `lcd.c`, `teclado.c`, `audio.c`, `uart.c`, `handlers.c`, `perifericos.c`, `maquinaestados.c`).
- `inc/` — headers correspondientes (`lcd.h`, `teclado.h`, `audio.h`, `uart.h`, `handlers.h`, `perifericos.h`, `maquinaestados.h`).
- `gui/` — interfaz gráfica Python y mock del LPC1769.
- `docs/` — documentación del proyecto.
