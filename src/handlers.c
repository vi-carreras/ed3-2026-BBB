/**
 * @file        handlers.c
 * @brief       Manejadores de interrupción del LPC1769.
 */

#include "handlers.h"
#include "maquinaestados.h"
#include "LPC17xx.h"
#include "lpc17xx_timer.h"

void SysTick_Handler(void) {
    msTicks++;
}

void TIMER3_IRQHandler(void) {
    uint8_t  j1 = 0, j2 = 0;
    uint32_t t1 = 0, t2 = 0;

    if (TIM_GetIntStatus(LPC_TIM3, TIM_CR0_INT)) {
        TIM_ClearIntPending(LPC_TIM3, TIM_CR0_INT);
        t1 = TIM_GetCaptureValue(LPC_TIM3, TIM_CAPTURE_0);
        j1 = 1;
    }
    if (TIM_GetIntStatus(LPC_TIM3, TIM_CR1_INT)) {
        TIM_ClearIntPending(LPC_TIM3, TIM_CR1_INT);
        t2 = TIM_GetCaptureValue(LPC_TIM3, TIM_CAPTURE_1);
        j2 = 1;
    }

    if (j1 && j2) {
        if (t1 <= t2) { quien_gano = 1; tiempo_us = t1; }
        else          { quien_gano = 2; tiempo_us = t2; }
    } else if (j1) {
        quien_gano = 1; tiempo_us = t1;
    } else if (j2) {
        quien_gano = 2; tiempo_us = t2;
    }

    if (quien_gano) {
        TIM_Disable(LPC_TIM3);
        NVIC_DisableIRQ(TIMER3_IRQn);
        flag_capture = 1;
    }
}
