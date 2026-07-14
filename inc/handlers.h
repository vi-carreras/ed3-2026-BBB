/**
 * @file        handlers.h
 * @brief       Manejadores de interrupción del LPC1769.
 * @version     1.0
 * @date        14. July. 2026
 * @author      Bombón, Burbuja y Bellota
 */

#ifndef HANDLERS_H_
#define HANDLERS_H_

/**
 * @brief  Handler de SysTick: incrementa contador de milisegundos.
 */
void SysTick_Handler(void);

/**
 * @brief  Handler de TIMER3: captura tiempos de reacción para J1/J2.
 */
void TIMER3_IRQHandler(void);

#endif /* HANDLERS_H_ */
