/**
 * @file        lcd.h
 * @brief       LCD 16x2 por I2C (PCF8574) para LPC1769.
 * @version     1.0
 * @date        14. July. 2026
 * @author      Bombón, Burbuja y Bellota
 */

#ifndef LCD_H_
#define LCD_H_

#include <stdint.h>

/* --- Dirección y máscaras del PCF8574 --- */
#define LCD_ADDR 0x27
#define LCD_BL   (1 << 3)
#define LCD_EN   (1 << 2)
#define LCD_RS   (1 << 0)

/**
 * @brief  Bloquea la ejecución durante la cantidad de milisegundos indicada.
 * @param  ms  Milisegundos a esperar.
 */
void delay_ms(uint32_t ms);

/**
 * @brief  Bloquea la ejecución durante la cantidad de microsegundos indicada.
 * @param  us  Microsegundos a esperar.
 */
void delay_us(uint32_t us);

/**
 * @brief  Inicializa el LCD en modo 4 bits.
 */
void LCD_Init(void);

/**
 * @brief  Escribe una línea de texto en el LCD.
 * @param  fila  0 para línea superior, 1 para línea inferior.
 * @param  s     Cadena de hasta 16 caracteres.
 */
void LCD_Linea(uint8_t fila, const char *s);

#endif /* LCD_H_ */
