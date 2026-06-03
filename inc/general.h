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



#endif /* GENERAL_H_ */
