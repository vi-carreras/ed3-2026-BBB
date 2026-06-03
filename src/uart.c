#include "uart.h"
#include "lpc17xx_uart.h"

void enviar_char(uint8_t dato){
	while(!(UART_GetLineStatus(UART1) & UART_LSR_THRE));
	UART_SendByte(UART1, dato);
}

void enviar_str(const char* str){
	while(*str){
		enviar_char((uint8_t)*str++);
	}
}
