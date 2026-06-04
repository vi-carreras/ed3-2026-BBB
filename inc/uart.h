/**
 * @file        uart.h
 * @brief       Transmisión UART básica para el LPC1769.
 *             Provee funciones para enviar caracteres y cadenas
 *             por UART1 en modo polling.
 * @version     1.0
 * @date        04. June. 2026
 * @author      Bombón, Burbuja y Bellota
 *
 * @par Refactor:
 * Last update: 04/06/2026, Author: David Trujillo Medina
 */

#ifndef UART_H_
#define UART_H_

#include <stdint.h>

/* --------------------------- Funciones públicas --------------------------- */
/**
 * @brief  Envía un carácter por UART1 (polling).
 * @param  dato  Carácter a transmitir.
 */
void enviar_char(uint8_t dato);

/**
 * @brief  Envía una cadena terminada en '\0' por UART1.
 * @param  str  Puntero a la cadena.
 */
void enviar_str(const char* str);

#endif /* UART_H_ */
