/**
 * @file        main.c
 * @brief       Juego de reacción competitivo para LPC1769.
 *             Versión funcional modularizada (audio por DAC en polling).
 */

#include "LPC17xx.h"
#include "lpc17xx_dac.h"
#include "lcd.h"
#include "uart.h"
#include "audio.h"
#include "teclado.h"
#include "handlers.h"
#include "perifericos.h"
#include "maquinaestados.h"

volatile uint32_t msTicks = 0;
volatile char     tecla_objetivo = '5';
volatile uint8_t  flag_start_game = 0;
volatile uint8_t  victorias_j1 = 0;
volatile uint8_t  victorias_j2 = 0;
volatile uint8_t  flag_capture = 0;
volatile uint8_t  quien_gano = 0;
volatile uint32_t tiempo_us = 0;

static void uint_to_str(uint32_t val, char *buf) {
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char tmp[12];
    uint8_t len = 0;
    while (val > 0) { tmp[len++] = '0' + (uint8_t)(val % 10); val /= 10; }
    uint8_t i;
    for (i = 0; i < len; i++) buf[i] = tmp[len - 1 - i];
    buf[len] = '\0';
}

int main(void) {
    SysTick_Config(SystemCoreClock / 1000);

    I2C0_Init();
    LCD_Init();
    configUART2();
    DAC_Init();
    DAC_UpdateValue(0);
    TIM3_Capture_Init();

    LCD_Linea(0, "TP FINAL DIG 3");
    LCD_Linea(1, "JUEGO REACCION");
    delay_ms(2000);

    while (1) {
        LCD_Linea(0, "Esperando");
        LCD_Linea(1, "inicio (S)...");

        flag_start_game = 0;
        while (!flag_start_game) {
            // UART2_IRQHandler setea flag_start_game al recibir "S"
        }

        Audio_Countdown();

        char linea_obj[17] = "Presiona: ?    ";
        linea_obj[10] = tecla_objetivo;
        LCD_Linea(0, linea_obj);
        LCD_Linea(1, "Esperando...");

        LPC_GPIO0->FIODIR |= 0x0F;
        LPC_GPIO0->FIOCLR  = 0x0F;
        LPC_GPIO2->FIODIR |= 0x0F;
        LPC_GPIO2->FIOCLR  = 0x0F;

        iniciar_medicion();

        uint32_t inicio_espera = msTicks;
        while (!flag_capture && (msTicks - inicio_espera) < TIMEOUT_MS) {
            // loop vacío
        }

        TIM_Disable(LPC_TIM3);
        NVIC_DisableIRQ(TIMER3_IRQn);

        if (flag_capture) {
            Audio_BeepReaccion();
        }

        if (!flag_capture) {
            LCD_Linea(0, "TIMEOUT");
            LCD_Linea(1, "Sin entrada");
            enviar_str("T\r\n");
        } else {
            char tecla;
            if (quien_gano == 1) {
                tecla = identificar_tecla(LPC_GPIO0);
            } else {
                tecla = identificar_tecla(LPC_GPIO2);
            }

            char linea1[17];
            char linea2[17];
            char numstr[12];

            if (tecla == tecla_objetivo) {
                const char *base = "CORRECTO J?    ";
                for (uint8_t k = 0; k < 16; k++) linea1[k] = base[k];
                linea1[16] = '\0';
                linea1[10] = (quien_gano == 1) ? '1' : '2';

                linea2[0] = 'T';
                uint_to_str(tiempo_us, numstr);
                uint8_t ii = 1, jj = 0;
                while (numstr[jj] && ii < 11) linea2[ii++] = numstr[jj++];
                linea2[ii++] = ' '; linea2[ii++] = 'u'; linea2[ii++] = 's';
                while (ii < 16) linea2[ii++] = ' ';
                linea2[16] = '\0';

                enviar_str(quien_gano == 1 ? "W1\r\n" : "W2\r\n");
                enviar_char('R');
                enviar_str(numstr);
                enviar_str("\r\n");

                if (quien_gano == 1) victorias_j1++;
                else                 victorias_j2++;
            } else {
                const char *base = "INCORRECTO J?  ";
                for (uint8_t k = 0; k < 16; k++) linea1[k] = base[k];
                linea1[16] = '\0';
                linea1[12] = (quien_gano == 1) ? '1' : '2';

                const char *base2 = "Tecla: ?       ";
                for (uint8_t k = 0; k < 16; k++) linea2[k] = base2[k];
                linea2[16] = '\0';
                linea2[7] = tecla;

                enviar_str(quien_gano == 1 ? "E1\r\n" : "E2\r\n");

                if (quien_gano == 1) {
                    if (victorias_j1 > 0) victorias_j1--;
                } else {
                    if (victorias_j2 > 0) victorias_j2--;
                }
            }

            LCD_Linea(0, linea1);
            LCD_Linea(1, linea2);
        }

        delay_ms(2000);

        {
            char numj1[4], numj2[4];
            uint_to_str(victorias_j1, numj1);
            uint_to_str(victorias_j2, numj2);
            enviar_char('P');
            enviar_str(numj1);
            enviar_char(',');
            enviar_str(numj2);
            enviar_str("\r\n");
        }

        if (victorias_j1 >= MAX_VICTORIAS || victorias_j2 >= MAX_VICTORIAS) {
            char linea_win[17] = "GANADOR: J?     ";
            linea_win[10] = (victorias_j1 >= MAX_VICTORIAS) ? '1' : '2';
            LCD_Linea(0, linea_win);
            LCD_Linea(1, "Reiniciando...");

            enviar_str((victorias_j1 >= MAX_VICTORIAS) ? "G1\r\n" : "G2\r\n");
            Audio_Victoria();

            victorias_j1 = 0;
            victorias_j2 = 0;
            enviar_str("P0,0\r\n");

            delay_ms(1500);
        } else {
            char linea_score[17] = "J1: ?   J2: ?   ";
            linea_score[4]  = '0' + victorias_j1;
            linea_score[12] = '0' + victorias_j2;
            LCD_Linea(0, "Marcador:");
            LCD_Linea(1, linea_score);
            delay_ms(1500);
        }
    }
}
