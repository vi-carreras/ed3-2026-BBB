#include "teclado.h"
#include "lpc17xx_uart.h"

static const uint8_t keyboard[4][4] = {
	{'1','2','3','A'},
	{'4','5','6','B'},
	{'7','8','9','C'},
	{'*','0','#','D'}
};

static uint32_t fila;
static int8_t columna;
static uint8_t scan_data;
static uint8_t dato_uart;

static void enviar(uint8_t dato){
	while(!(UART_GetStatus(UART1, UART_LSR_THRE)));
	UART_SendByte(UART1, dato);
}

uint8_t EscanearTecladoJ1(void){
	for(fila=0;fila<4;fila++){
		// Pongo la fila actual a 0 (las demás quedan como estaban)
		LPC_GPIO0->FIOCLR = (1<<fila);

		retardo_ms(5);  // estabilización de teclado (~5ms)

		scan_data = (LPC_GPIO0->FIOPIN >> 4) & 0x0F;

		// Restauro la fila a 1 (input tristate)
		LPC_GPIO0->FIOSET = (1<<fila);

		columna = -1;
		switch(scan_data){
			case 0x0E: columna = 0; break;
			case 0x0D: columna = 1; break;
			case 0x0B: columna = 2; break;
			case 0x07: columna = 3; break;
			default:   columna = -1; break;
		}

		if(columna != -1){
			break;
		}
	}

	if(columna == -1){
		return 0;
	}

	return keyboard[fila][columna];
}

uint8_t EscanearTecladoJ2(void){
	for(fila=0;fila<4;fila++){
		LPC_GPIO2->FIOCLR = (1<<fila);

		retardo_ms(5);

		scan_data = (LPC_GPIO2->FIOPIN >> 4) & 0x0F;

		LPC_GPIO2->FIOSET = (1<<fila);

		columna = -1;
		switch(scan_data){
			case 0x0E: columna = 0; break;
			case 0x0D: columna = 1; break;
			case 0x0B: columna = 2; break;
			case 0x07: columna = 3; break;
			default:   columna = -1; break;
		}

		if(columna != -1){
			break;
		}
	}

	if(columna == -1){
		return 0;
	}

	return keyboard[fila][columna];
}
