/**
 * @file        uart.h
 * @brief       Transmisión UART2 básica para LPC1769.
 * @version     1.0
 * @date        14. July. 2026
 * @author      Bombón, Burbuja y Bellota
 */

#ifndef UART_H_
#define UART_H_

#include <stdint.h>

/**
 * @brief  Configura UART2 a 115200 8N1 con FIFO e IRQ de recepción.
 */
void configUART2(void);

/**
 * @brief  Envía un carácter por UART2 en modo polling.
 * @param  c  Carácter a transmitir.
 */
void enviar_char(uint8_t c);

/**
 * @brief  Envía una cadena terminada en '\0' por UART2.
 * @param  s  Puntero a la cadena.
 */
void enviar_str(const char *s);

#endif /* UART_H_ */
