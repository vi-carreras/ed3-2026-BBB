/**
 * @file        maquinaestados.h
 * @brief       Máquina de estados del juego de reacción competitivo.
 *             Define los estados del juego y la función que los procesa.
 */

#ifndef MAQUINAESTADOS_H_
#define MAQUINAESTADOS_H_

#include "LPC17xx.h"
#include <stdint.h>

/* --------------------------- Estados del juego --------------------------- */
typedef enum {
    E_IDLE = 0,
    E_CONFIG,
    E_COUNTDOWN,
    E_WAIT_GO,
    E_WAIT_INPUT,
    E_ROUND_END,
    E_GAME_OVER
} estado_juego_t;

/* ----------- Variables globales compartidas con main.c/handlers ---------- */

// Estado actual del juego
extern volatile estado_juego_t estado_actual;

// Tiempo de sistema (ms), incrementado por SysTick_Handler
extern volatile uint32_t msTicks;

// Audio
#define AUDIO_BUF_SIZE   2000
extern uint32_t audio_buf[AUDIO_BUF_SIZE];

// Control de cuenta regresiva y ronda
extern volatile uint8_t  countdown_phase;
extern volatile uint8_t  round_end_phase;
extern volatile uint32_t round_end_start;

// Configuración de partida (desde UART)
extern volatile uint8_t  tecla_objetivo;
extern volatile uint16_t tono_frecuencia;
extern volatile uint16_t velocidad_beeps;

// Banderas de control
extern volatile uint8_t  flag_start_game;
extern volatile uint8_t  flag_dma_audio_done;
extern volatile uint8_t  flag_capture_event;
extern volatile uint8_t  config_hecho;

// Resultados
extern volatile uint32_t tiempo_reaccion_jugador;
extern volatile uint8_t  resultado_ronda;
extern volatile uint8_t  victorias_j1;
extern volatile uint8_t  victorias_j2;
#define MAX_VICTORIAS 10

// Timeout de espera de respuesta (ms)
extern volatile uint32_t tiempo_inicio_espera;
#define TIMEOUT_MS 10000

/* --------------------------- Función pública ----------------------------- */
/** @brief  Procesa un ciclo de la máquina de estados. Debe llamarse
 *          en el bucle principal de main(). */
void actualizarestado(void);

#endif /* MAQUINAESTADOS_H_ */
