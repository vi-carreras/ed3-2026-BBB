/**
 * @file        teclado.h
 * @brief       Identificación de tecla presionada en teclado matricial 4x4.
 * @version     1.0
 * @date        14. July. 2026
 * @author      Bombón, Burbuja y Bellota
 */

#ifndef TECLADO_H_
#define TECLADO_H_

#include "LPC17xx.h"

#define ROW_MASK (0x0F)
#define COL_MASK (0xF0)

/**
 * @brief  Identifica la tecla presionada escaneando fila por fila.
 * @param  gpio  Puerto GPIO donde está conectado el teclado (LPC_GPIO0 o LPC_GPIO2).
 * @return Carácter de la tecla presionada o '?' si ninguna.
 */
char identificar_tecla(LPC_GPIO_TypeDef *gpio);

#endif /* TECLADO_H_ */
