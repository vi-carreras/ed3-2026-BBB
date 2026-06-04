/**
 * @file        handlers.h
 * @brief       Manejadores de interrupción (IRQ) del LPC1769 para el
 *             juego de reacción competitivo.
 *
 *             Los handlers son referenciados por nombre desde la vector
 *             table en startup_LPC17xx.s; no se llaman desde el código
 *             de aplicación directamente.
 */

#ifndef HANDLERS_H_
#define HANDLERS_H_

#include "LPC17xx.h"

void TIMER0_IRQHandler(void);
void TIMER1_IRQHandler(void);
void UART1_IRQHandler(void);
void DMA_IRQHandler(void);

#endif /* HANDLERS_H_ */
