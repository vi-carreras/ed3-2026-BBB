/**
 * @file        handlers.c
 * @brief       Implementación de los manejadores de interrupción del
 *             juego de reacción competitivo para LPC1769.
 *
 *             TIMER0_IRQHandler  — Limpia pendiente de MR0 (solo trigger DMA)
 *             TIMER3_IRQHandler  — Capture de tiempo de reacción (J1 / J2)
 *             UART1_IRQHandler   — Receptor de comandos serie
 *             DMA_IRQHandler     — Notifica fin de reproducción de audio
 */

#include "handlers.h"
#include "maquinaestados.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_gpdma.h"
#include "teclado.h"
#include "uart.h"

/*----------------------------------------------------------------------------
  TIMER0_IRQHandler: MR0 tiene intEn = DISABLE (solo trigger DMA),
  esta IRQ no se dispara en operación normal.
 *----------------------------------------------------------------------------*/
void TIMER0_IRQHandler(void)
{
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}

/*----------------------------------------------------------------------------
  TIMER3_IRQHandler: Capture de tiempo de reacción para J1 (CAP3.0) y
  J2 (CAP3.1). Lee la tecla presionada, compara con la objetivo y
  guarda el resultado para que la FSM lo procese.
 *----------------------------------------------------------------------------*/
void TIMER3_IRQHandler(void)
{
    uint8_t tecla_presionada = 0;

    // --- JUGADOR 1 (CAP3.0 - P0.23) ---
    if (TIM_GetIntStatus(LPC_TIM3, TIM_CR0_INT)) {
        TIM_ClearIntPending(LPC_TIM3, TIM_CR0_INT);
        tiempo_reaccion_jugador = TIM_GetCaptureValue(LPC_TIM3, TIM_CAPTURE_0);

        tecla_presionada = EscanearTecladoJ1();

        if (tecla_presionada == tecla_objetivo) {
            resultado_ronda = 1; // J1 Correcto
        } else {
            resultado_ronda = 3; // J1 Incorrecto (Penalización)
        }

        flag_capture_event = 1;
        TIM_Disable(LPC_TIM3);
        NVIC_DisableIRQ(TIMER3_IRQn);
    }

    // --- JUGADOR 2 (CAP3.1 - P0.24) ---
    else if (TIM_GetIntStatus(LPC_TIM3, TIM_CR1_INT)) {
        TIM_ClearIntPending(LPC_TIM3, TIM_CR1_INT);
        tiempo_reaccion_jugador = TIM_GetCaptureValue(LPC_TIM3, TIM_CAPTURE_1);

        tecla_presionada = EscanearTecladoJ2();

        if (tecla_presionada == tecla_objetivo) {
            resultado_ronda = 2; // J2 Correcto
        } else {
            resultado_ronda = 4; // J2 Incorrecto (Penalización)
        }

        flag_capture_event = 1;
        TIM_Disable(LPC_TIM3);
        NVIC_DisableIRQ(TIMER3_IRQn);
    }
}

/*----------------------------------------------------------------------------
  UART1_IRQHandler: Receptor de comandos serie.
  Acumula caracteres hasta \r o \n, luego interpreta el comando.
 *----------------------------------------------------------------------------*/
void UART1_IRQHandler(void)
{
    static char line_buf[16];
    static uint8_t line_idx = 0;

    uint8_t rx = UART_ReceiveByte(UART1);

    if(rx == '\r' || rx == '\n') {
        if(line_idx > 0) {
            line_buf[line_idx] = '\0';
            char cmd = line_buf[0];
            if(cmd >= 'a' && cmd <= 'z') cmd -= 32;

            switch(cmd) {
                case 'S':
                    flag_start_game = 1;
                    break;

                case 'K':
                    if(line_idx >= 2)
                        tecla_objetivo = (uint8_t)line_buf[1];
                    break;

                case 'F': {
                    uint16_t val = 0;
                    for(uint8_t i = 1; i < line_idx; i++){
                        char c = line_buf[i];
                        if(c >= '0' && c <= '9')
                            val = val * 10 + (uint16_t)(c - '0');
                        else
                            break;
                    }
                    if(val > 0) tono_frecuencia = val;
                    break;
                }

                case 'B': {
                    uint16_t val = 0;
                    for(uint8_t i = 1; i < line_idx; i++){
                        char c = line_buf[i];
                        if(c >= '0' && c <= '9')
                            val = val * 10 + (uint16_t)(c - '0');
                        else
                            break;
                    }
                    velocidad_beeps = (val > 200) ? 200 : (uint8_t)val;
                    break;
                }

                case 'C':
                    config_hecho = 1;
                    break;

                case '?': {
                    enviar_char('S');
                    if(victorias_j1 >= 10) enviar_char('0' + (victorias_j1 / 10));
                    enviar_char('0' + (victorias_j1 % 10));
                    enviar_char(',');
                    if(victorias_j2 >= 10) enviar_char('0' + (victorias_j2 / 10));
                    enviar_char('0' + (victorias_j2 % 10));
                    enviar_char('\r');
                    enviar_char('\n');
                    break;
                }

                case 'R':
                    victorias_j1 = 0;
                    victorias_j2 = 0;
                    break;
            }
        }
        line_idx = 0;
    }
    else if(line_idx < sizeof(line_buf) - 1) {
        line_buf[line_idx++] = (char)rx;
    }
}

/*----------------------------------------------------------------------------
  DMA_IRQHandler: Limpia flag de interrupción del DMA y notifica a la
  FSM que terminó la reproducción de audio.
 *----------------------------------------------------------------------------*/
void DMA_IRQHandler(void)
{
    GPDMA_ClearIntPending(GPDMA_CLR_INTTC, GPDMA_CH_0);
    flag_dma_audio_done = 1;
}
