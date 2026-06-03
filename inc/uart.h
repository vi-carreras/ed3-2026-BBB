#ifndef UART_H_
#define UART_H_

#include <stdint.h>

void enviar_char(uint8_t dato);
void enviar_str(const char* str);

#endif
