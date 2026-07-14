/**
 * @file        maquinaestados.h
 * @brief       Variables globales y estado del juego de reacción.
 * @version     1.0
 * @date        14. July. 2026
 * @author      Bombón, Burbuja y Bellota
 */

#ifndef MAQUINAESTADOS_H_
#define MAQUINAESTADOS_H_

#include "LPC17xx.h"
#include <stdint.h>

/* --- Tiempo de sistema --- */
extern volatile uint32_t msTicks;

/* --- Configuración de partida --- */
extern volatile char    tecla_objetivo;
extern volatile uint8_t flag_start_game;

/* --- Marcador --- */
extern volatile uint8_t victorias_j1;
extern volatile uint8_t victorias_j2;
#define MAX_VICTORIAS 3

/* --- Resultado del capture --- */
extern volatile uint8_t  flag_capture;
extern volatile uint8_t  quien_gano;
extern volatile uint32_t tiempo_us;

/* --- Timeout de espera --- */
#define TIMEOUT_MS 10000

#endif /* MAQUINAESTADOS_H_ */
