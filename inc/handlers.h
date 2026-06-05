/**
 * @file        handlers.h
 * @brief       Manejadores de interrupción (IRQ) del LPC1769 para el
 *             juego de reacción competitivo.
 *
 *             Los handlers son referenciados por nombre desde la vector
 *             table en startup_LPC17xx.s; no se llaman desde el código
 *             de aplicación directamente.
 * @version     1.0
 * @date        04. June. 2026
 * @author      Bombón, Burbuja y Bellota
 *
 * @par Refactor:
 * Last update: 04/06/2026, Author: David Trujillo Medina
 */

#ifndef HANDLERS_H_
#define HANDLERS_H_

#include "LPC17xx.h"

/* ------------------------- Manejadores de IRQ ---------------------------- */
/**
 * @brief  Handler de TIM0. Limpia el flag de interrupción MR0.
 * @note   MR0 tiene intEn = DISABLE (solo trigger DMA), por lo que
 *         esta IRQ no se dispara en operación normal.
 */
void TIMER0_IRQHandler(void);

/**
 * @brief  Handler de TIM3. Captura tiempo de reacción para J1 (CAP3.0)
 *         y J2 (CAP3.1), lee la tecla presionada y actualiza el resultado.
 */
void TIMER3_IRQHandler(void);

/**
 * @brief  Handler de UART1. Acumula caracteres recibidos y procesa
 *         comandos al recibir \r o \n.
 */
void UART1_IRQHandler(void);

/**
 * @brief  Handler de DMA. Limpia el flag de interrupción del canal 0
 *         y notifica a la FSM que terminó la reproducción de audio.
 */
void DMA_IRQHandler(void);

#endif /* HANDLERS_H_ */
