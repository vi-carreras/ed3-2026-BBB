/**
 * @file        teclado.h
 * @brief       Teclado matricial 4x4 para ambos jugadores.
 *             Escanea las filas y columnas de los puertos P0 (J1) y
 *             P2 (J2) y retorna la tecla presionada.
 * @version     1.0
 * @date        04. June. 2026
 * @author      Bombón, Burbuja y Bellota
 *
 * @par Refactor:
 * Last update: 04/06/2026, Author: David Trujillo Medina
 */

#ifndef TECLADO_H_
#define TECLADO_H_

#include <stdint.h>

/* --------------------------- Funciones públicas --------------------------- */
/**
 * @brief  Escanea el teclado del Jugador 1 (P0) y retorna la tecla presionada.
 * @return Código de la tecla (0-15) o 0xFF si ninguna está presionada.
 */
uint8_t EscanearTecladoJ1(void);

/**
 * @brief  Escanea el teclado del Jugador 2 (P2) y retorna la tecla presionada.
 * @return Código de la tecla (0-15) o 0xFF si ninguna está presionada.
 */
uint8_t EscanearTecladoJ2(void);

#endif /* TECLADO_H_ */
