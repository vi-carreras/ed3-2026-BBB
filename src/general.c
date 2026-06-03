#include "general.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_i2c.h"
#include "lpc_types.h"
#include <stddef.h>

// --- LCD 16x2 por I2C (PCF8574) ---
#define LCD_I2C_ADDR		0x27
#define LCD_BACKLIGHT		0x08
#define LCD_EN				0x04
#define LCD_RW				0x02
#define LCD_RS				0x01

static void lcd_send_nibble(uint8_t data){
	I2C_M_SETUP_Type cfg;
	cfg.sl_addr7bit = LCD_I2C_ADDR;
	cfg.tx_data = &data;
	cfg.tx_length = 1;
	cfg.tx_count = 0;
	cfg.rx_data = NULL;
	cfg.rx_length = 0;
	cfg.rx_count = 0;
	cfg.retransmissions_max = 3;
	cfg.retransmissions_count = 0;
	cfg.status = 0;
	cfg.callback = NULL;

	I2C_MasterTransferData(LPC_I2C0, &cfg, I2C_TRANSFER_POLLING);
	retardo_ms(2);
}

static void lcd_write(uint8_t data, uint8_t mode){
	uint8_t high = (data & 0xF0) | LCD_BACKLIGHT | mode | LCD_EN;
	uint8_t low  = ((data << 4) & 0xF0) | LCD_BACKLIGHT | mode;

	lcd_send_nibble(high);
	lcd_send_nibble(high & ~LCD_EN);	// pulso E baja
	lcd_send_nibble(low);
	lcd_send_nibble(low & ~LCD_EN);
}

static void lcd_cmd(uint8_t cmd){
	lcd_write(cmd, 0);
}

static void lcd_data(uint8_t data){
	lcd_write(data, LCD_RS);
}

static void lcd_init(void){
	// Inicialización en 4-bit mode (secuencia estándar HD44780)
	lcd_send_nibble(0x30 | LCD_BACKLIGHT | LCD_EN);
	lcd_send_nibble(0x30 | LCD_BACKLIGHT);
	lcd_send_nibble(0x30 | LCD_BACKLIGHT | LCD_EN);
	lcd_send_nibble(0x30 | LCD_BACKLIGHT);
	lcd_send_nibble(0x20 | LCD_BACKLIGHT | LCD_EN);	// 4-bit mode
	lcd_send_nibble(0x20 | LCD_BACKLIGHT);

	lcd_cmd(0x28);	// 2 líneas, 5x8
	lcd_cmd(0x0C);	// display on, cursor off
	lcd_cmd(0x01);	// clear display
	lcd_cmd(0x06);	// entry mode: increment, no shift
}

static uint8_t lcd_inicializado = 0;

void mensajeLCD(char* linea1, char* linea2){
	uint8_t i;
	if(!lcd_inicializado){
		lcd_init();
		lcd_inicializado = 1;
	}
	lcd_cmd(0x80);	// cursor inicio línea 1 (DDRAM 0x00)
	for(i=0; i<16; i++){
		lcd_data(linea1[i] ? (uint8_t)linea1[i] : ' ');
	}

	lcd_cmd(0xC0);	// cursor inicio línea 2 (DDRAM 0x40)
	for(i=0; i<16; i++){
		lcd_data(linea2[i] ? (uint8_t)linea2[i] : ' ');
	}
}

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

void retardo_ms(uint32_t ms){
	uint32_t inicio = msTicks;
	while((msTicks - inicio) < ms);
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
