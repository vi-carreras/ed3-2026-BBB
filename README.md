# ed3-2026-BBB
Repositorio para trabajo práctico final de la materia.

**Juego de reacción:** 

2–4 jugadores presionan su botón cuando se enciende su LED. El micro mide tiempos de reacción con precisión de microsegundos (Timer Input Capture), calcula estadísticas y anuncia ganador por UART.

Timer Input Capture medición de microsegundos: Es el uso más limpio posible de este periférico. Se captura el flanco del botón con resolución de ciclos de reloj.

DAC con rol real: Genera el tono de cuenta regresiva y el sonido de 'ya' con frecuencia variable. No es decorativo.

Flujo de IRQs
Timer IRQ (genera secuencia aleatoria de LED) → instante de referencia
GPIO EINT × 4 → cada botón de jugador captura timestamp con Input Capture
DAC DMA IRQ → secuencia de beeps de cuenta regresiva
UART RX IRQ → START_GAME, GET_STATS, SET_PLAYERS, RESET_SCORES
Timer IRQ (timeout) → si nadie presionó en 2s, siguiente ronda

Libreria(Funciones)
Reaction_Start(players)
Reaction_GetTime(player) → uint32 us
Stats_GetWinner() → player_id
Audio_Beep(freq, duration)
