/**
 * @file        lcd.c
 * @brief       LCD 16x2 por I2C (PCF8574) para LPC1769.
 */

#include "lcd.h"
#include "LPC17xx.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_pinsel.h"

/* --- Delays busy-wait --- */
void delay_ms(uint32_t ms) {
    volatile uint32_t i;
    for (i = 0; i < ms * 8000; i++);
}

void delay_us(uint32_t us) {
    volatile uint32_t i;
    for (i = 0; i < us * 8; i++);
}

/* --- Escritura I2C de un byte al PCF8574 --- */
static void I2C_Write(uint8_t data) {
    I2C_M_SETUP_Type cfg;
    cfg.sl_addr7bit          = LCD_ADDR;
    cfg.tx_data              = &data;
    cfg.tx_length            = 1;
    cfg.tx_count             = 0;
    cfg.rx_data              = NULL;
    cfg.rx_length            = 0;
    cfg.rx_count             = 0;
    cfg.retransmissions_max  = 3;
    cfg.retransmissions_count= 0;
    cfg.status               = 0;
    cfg.callback             = NULL;
    I2C_MasterTransferData(LPC_I2C0, &cfg, I2C_TRANSFER_POLLING);
}

static void LCD_Pulse(uint8_t data) {
    I2C_Write(data | LCD_EN);
    delay_us(2);
    I2C_Write(data & ~LCD_EN);
    delay_us(100);
}

static void LCD_Send(uint8_t byte, uint8_t rs) {
    LCD_Pulse((byte & 0xF0)        | LCD_BL | rs);
    LCD_Pulse(((byte << 4) & 0xF0) | LCD_BL | rs);
}

static void LCD_Cmd(uint8_t cmd)  { LCD_Send(cmd, 0);          }
static void LCD_Char(char c)      { LCD_Send((uint8_t)c, LCD_RS); }

void LCD_Init(void) {
    delay_ms(100);
    LCD_Pulse(0x30 | LCD_BL); delay_ms(5);
    LCD_Pulse(0x30 | LCD_BL); delay_ms(1);
    LCD_Pulse(0x30 | LCD_BL); delay_ms(1);
    LCD_Pulse(0x20 | LCD_BL); delay_ms(1);
    LCD_Cmd(0x28); delay_ms(1);
    LCD_Cmd(0x08); delay_ms(1);
    LCD_Cmd(0x01); delay_ms(2);
    LCD_Cmd(0x06); delay_ms(1);
    LCD_Cmd(0x0C); delay_ms(1);
}

void LCD_Linea(uint8_t fila, const char *s) {
    LCD_Cmd(fila == 0 ? 0x80 : 0xC0);
    uint8_t i = 0;
    while (*s && i < 16) { LCD_Char(*s++); i++; }
    while (i++ < 16)     { LCD_Char(' '); }
}
