#ifndef TECLADO_H_
#define TECLADO_H_

#include <stdint.h>

uint8_t EscanearTecladoJ1(void);
uint8_t EscanearTecladoJ2(void);
void enviar(uint8_t dato);

#endif
