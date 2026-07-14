/**
 * @file        teclado.c
 * @brief       Identificación de tecla presionada en teclado matricial 4x4.
 */

#include "teclado.h"
#include "lcd.h"

static const char keymap[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

char identificar_tecla(LPC_GPIO_TypeDef *gpio) {
    uint8_t row, col;
    uint32_t portVal;

    for (row = 0; row < 4; row++) {
        gpio->FIOSET = ROW_MASK;
        gpio->FIOCLR = (1 << row);

        delay_us(50);

        portVal = gpio->FIOPIN;

        for (col = 0; col < 4; col++) {
            if (!(portVal & (1 << (col + 4)))) {
                gpio->FIOCLR = ROW_MASK;
                return keymap[row][col];
            }
        }
    }

    gpio->FIOCLR = ROW_MASK;
    return '?';
}
