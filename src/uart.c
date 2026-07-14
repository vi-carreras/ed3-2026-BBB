/**
 * @file        uart.c
 * @brief       Transmisión UART2 básica para LPC1769.
 */

#include "uart.h"
#include "maquinaestados.h"
#include "LPC17xx.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_pinsel.h"

void configUART2(void) {
    LPC_SC->PCONP |= (1 << 24); // PCUART2

    PINSEL_CFG_T pinCfg = {PORT_0, PIN_10, PINSEL_FUNC_01, PINSEL_PULLUP, DISABLE};
    PINSEL_ConfigPin(&pinCfg);   // TXD2 (P0.10)

    pinCfg.pin = PIN_11;
    PINSEL_ConfigPin(&pinCfg);   // RXD2 (P0.11)

    UART_CFG_T uartCfg = {115200, UART_PARITY_NONE, UART_DBITS_8, UART_STOPBIT_1};
    UART_Init(UART2, &uartCfg);

    UART_FIFO_CFG_T fifoCfg = {ENABLE, ENABLE, DISABLE, UART_FIFO_TRGLEV0};
    UART_FIFOConfig(UART2, &fifoCfg);

    UART_TxEnable(UART2);

    UART_IntConfig(UART2, UART_INT_RBR, ENABLE);
    NVIC_EnableIRQ(UART2_IRQn);
}

void enviar_char(uint8_t c) {
    while (!(UART_GetLineStatus(UART2) & UART_LSR_THRE));
    UART_SendByte(UART2, c);
}

void enviar_str(const char *s) {
    while (*s) enviar_char((uint8_t)*s++);
}

void UART2_IRQHandler(void) {
    static char line_buf[16];
    static uint8_t line_idx = 0;

    uint8_t rx = UART_ReceiveByte(UART2);

    if (rx == '\r' || rx == '\n') {
        if (line_idx > 0) {
            line_buf[line_idx] = '\0';
            char cmd = line_buf[0];
            if (cmd >= 'a' && cmd <= 'z') cmd -= 32;

            switch (cmd) {
                case 'S':
                    flag_start_game = 1;
                    break;
                case 'K':
                    if (line_idx >= 2) {
                        tecla_objetivo = line_buf[1];
                    }
                    break;
                case 'R':
                    victorias_j1 = 0;
                    victorias_j2 = 0;
                    enviar_str("P0,0\r\n");
                    break;
                default:
                    break;
            }
        }
        line_idx = 0;
    } else if (line_idx < sizeof(line_buf) - 1) {
        line_buf[line_idx++] = (char)rx;
    }
}
