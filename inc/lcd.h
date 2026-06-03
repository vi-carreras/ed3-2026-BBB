#ifndef LCD_H_
#define LCD_H_

#include "LPC17xx.h"

// Contador de ms del sistema, incrementado por SysTick_Handler en main.c
extern volatile uint32_t msTicks;

void retardo_ms(uint32_t ms);
void mensajeLCD(char* linea1, char* linea2);

#endif
