/**
 * @file        perifericos.h
 * @brief       Inicialización de periféricos del LPC1769 para el juego
 *             de reacción competitivo. Agrupa la config de pines, UART,
 *             I2C, DAC, DMA y timers usados por el sistema.
 */

#ifndef PERIFERICOS_H_
#define PERIFERICOS_H_

#include "LPC17xx.h"

/* ----------- Funciones de configuración de periféricos ----------- */

/** @brief  Configura pines GPIO para teclado matricial de ambos jugadores.
 *          P0[7:0] -> J1, P2[7:0] -> J2.
 *          [3:0] salidas (barrido), [7:4] entradas con pull-up. */
void configGPIO(void);

/** @brief  Configura UART1 a 115200 baudios, 8N1, con FIFO e IRQ de recepción.
 *          P0.15 = TXD1, P0.16 = RXD1. */
void configUART1(void);

/** @brief  Configura I2C0 a 100 KHz para LCD 16x2 vía PCF8574.
 *          P0.27 = SDA0, P0.28 = SCL0. */
void configI2C0(void);

/** @brief  Configura DAC con DMA, bias de 700 µA, y actualiza a 0.
 *          P0.26 = AOUT. */
void configDAC(void);

/** @brief  Inicializa el controlador GPDMA. Los canales se configuran
 *          dinámicamente en Audio_Play(). */
void configDMA(void);

/** @brief  Configura TIM0 como scheduler del DAC: MR0 dispara DMA
 *          cada 100 µs (10 KHz). Inicia apagado; Audio_Play lo enciende. */
void configTIMER0(void);

/** @brief  Configura TIM1 con capture en CAP1.0 (P1.18, J1) y CAP1.1
 *          (P1.19, J2) para medir tiempo de reacción con resolución de 1 µs. */
void configTIMER1(void);

#endif /* PERIFERICOS_H_ */
