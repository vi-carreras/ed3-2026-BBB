/**
 * @file        lcd.h
 * @brief       LCD 16x2 driver via I2C (PCF8574) y función de retardo
 *             basada en SysTick para el LPC1769.
 * @version     1.0
 * @date        04. June. 2026
 * @author      Victoria P.
 *
 * @par Refactor:
 * Last update: 04/06/2026, Author: David Trujillo Medina
 */

#ifndef LCD_H_
#define LCD_H_

#include "LPC17xx.h"

/* -------------------------- Variables globales --------------------------- */
/** @brief  Contador de ms del sistema, incrementado por SysTick_Handler. */
extern volatile uint32_t msTicks;

/* --------------------------- Funciones públicas --------------------------- */
/**
 * @brief  Bloquea la ejecución durante la cantidad de milisegundos indicada.
 * @param  ms  Milisegundos a esperar.
 */
void retardo_ms(uint32_t ms);

/**
 * @brief  Muestra dos líneas de texto en el LCD 16x2.
 * @param  linea1  Cadena para la línea superior (hasta 16 caracteres).
 * @param  linea2  Cadena para la línea inferior (hasta 16 caracteres).
 */
void mensajeLCD(char* linea1, char* linea2);

#endif /* LCD_H_ */
