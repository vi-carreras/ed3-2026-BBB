/*
 * general.h
 *
 *  Created on: 31 may. 2026
 *      Author:
 */

#ifndef GENERAL_H_
#define GENERAL_H_

#include "LPC17xx.h"
#include "lpc17xx_i2c.h"


uint8_t EscanearTecladoJ1(void);

uint8_t EscanearTecladoJ2(void);

void mensajeLCD(char* linea1, char* linea2);

//void reproducirAudio(uint32_t* buffer, uint32_t size);

//void detenerAudio(void);

//void enviarStatsUART(int8_t vict_j1, int8_t vict_j2, uint32_t tiempo_reaccion, uint8_t resultado);

//uint8_t generarTeclaAleatoria(void);

#endif /* GENERAL_H_ */
