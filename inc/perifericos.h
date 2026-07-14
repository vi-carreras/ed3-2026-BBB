/**
 * @file        perifericos.h
 * @brief       Inicialización de periféricos del LPC1769.
 * @version     1.0
 * @date        14. July. 2026
 * @author      Bombón, Burbuja y Bellota
 */

#ifndef PERIFERICOS_H_
#define PERIFERICOS_H_

/**
 * @brief  Inicializa I2C0 a 100 KHz para LCD vía PCF8574.
 */
void I2C0_Init(void);

/**
 * @brief  Inicializa TIM3 con capture en P0.23 (J1) y P0.24 (J2).
 */
void TIM3_Capture_Init(void);

/**
 * @brief  Reinicia TIM3 para una nueva medición de reacción.
 */
void iniciar_medicion(void);

#endif /* PERIFERICOS_H_ */
