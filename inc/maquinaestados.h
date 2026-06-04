/**
 * @file        maquinaestados.h
 * @brief       Máquina de estados del juego de reacción competitivo.
 *             Define los estados del juego, las variables globales
 *             compartidas y la función que procesa la FSM.
 * @version     1.0
 * @date        04. June. 2026
 * @author      Bombón, Burbuja y Bellota
 *
 * @par Refactor:
 * Last update: 04/06/2026, Author: David Trujillo Medina
 */

#ifndef MAQUINAESTADOS_H_
#define MAQUINAESTADOS_H_

#include "LPC17xx.h"
#include <stdint.h>

/* --------------------------- Estados del juego --------------------------- */
/** @brief  Estados de la máquina de estados del juego. */
typedef enum {
    E_IDLE = 0,        /**< Reposo: muestra pantalla de inicio.           */
    E_CONFIG,          /**< Espera comandos de configuración por UART.    */
    E_COUNTDOWN,       /**< Cuenta regresiva 3… 2… 1… GO! con audio.     */
    E_WAIT_GO,         /**< Muestra tecla objetivo y habilita capture.    */
    E_WAIT_INPUT,      /**< Espera respuesta del jugador o timeout.       */
    E_ROUND_END,       /**< Muestra resultado de la ronda y scores.       */
    E_GAME_OVER        /**< Pantalla de ganador y tonos de celebración.   */
} estado_juego_t;

/* ---------------------- Variables globales compartidas -------------------- */
/** @brief  Estado actual del juego. */
extern volatile estado_juego_t estado_actual;

/** @brief  Tiempo de sistema en ms, incrementado por SysTick_Handler. */
extern volatile uint32_t msTicks;

/* --- Audio --- */
/** @brief  Tamaño del buffer de audio para reproducción por DMA. */
#define AUDIO_BUF_SIZE   2000
/** @brief  Buffer de muestras de audio (formato DAC). */
extern uint32_t audio_buf[AUDIO_BUF_SIZE];

/* --- Control de cuenta regresiva y ronda --- */
/** @brief  Fase de la cuenta regresiva (0-7). */
extern volatile uint8_t  countdown_phase;
/** @brief  Fase de fin de ronda: 0=pendiente, 1=mostrando resultado. */
extern volatile uint8_t  round_end_phase;
/** @brief  msTicks al iniciar la pausa de fin de ronda. */
extern volatile uint32_t round_end_start;

/* --- Configuración de partida (desde UART) --- */
/** @brief  Tecla que los jugadores deben presionar para ganar la ronda. */
extern volatile uint8_t  tecla_objetivo;
/** @brief  Frecuencia de los tonos de cuenta regresiva (Hz). */
extern volatile uint16_t tono_frecuencia;
/** @brief  Intervalo entre beeps de la cuenta regresiva (ms). */
extern volatile uint16_t velocidad_beeps;

/* --- Banderas de control --- */
/** @brief  Flag: se recibió comando 'S' para iniciar juego. */
extern volatile uint8_t  flag_start_game;
/** @brief  Flag: el DMA terminó de reproducir el buffer de audio. */
extern volatile uint8_t  flag_dma_audio_done;
/** @brief  Flag: ocurrió un evento de capture (tecla presionada). */
extern volatile uint8_t  flag_capture_event;
/** @brief  Flag: se recibió comando 'C', config completa. */
extern volatile uint8_t  config_hecho;

/* --- Resultados --- */
/** @brief  Tiempo de reacción del jugador que disparó el capture (µs). */
extern volatile uint32_t tiempo_reaccion_jugador;
/** @brief  Resultado de la ronda (0=timeout, 1=J1 ok, 2=J2 ok, 3=J1 fail, 4=J2 fail). */
extern volatile uint8_t  resultado_ronda;
/** @brief  Victorias acumuladas del Jugador 1. */
extern volatile uint8_t  victorias_j1;
/** @brief  Victorias acumuladas del Jugador 2. */
extern volatile uint8_t  victorias_j2;
/** @brief  Victorias necesarias para ganar la partida. */
#define MAX_VICTORIAS 10

/* --- Timeout --- */
/** @brief  msTicks al inicio de la espera de respuesta. */
extern volatile uint32_t tiempo_inicio_espera;
/** @brief  Tiempo máximo de espera de respuesta (ms). */
#define TIMEOUT_MS 10000

/* --------------------------- Función pública ----------------------------- */
/**
 * @brief  Procesa un ciclo de la máquina de estados.
 * @note   Debe llamarse en el bucle principal de main().
 */
void actualizarestado(void);

#endif /* MAQUINAESTADOS_H_ */
