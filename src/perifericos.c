/**
 * @file        perifericos.c
 * @brief       Inicialización de periféricos del LPC1769.
 */

#include "perifericos.h"
#include "maquinaestados.h"
#include "LPC17xx.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"

void I2C0_Init(void) {
    PINSEL_CFG_T p = {PORT_0, PIN_27, PINSEL_FUNC_01, PINSEL_TRISTATE, ENABLE};
    PINSEL_ConfigPin(&p);
    p.pin = PIN_28;
    PINSEL_ConfigPin(&p);
    PINSEL_SetI2CPins(PINSEL_I2C_FAST, DISABLE);
    I2C_Init(LPC_I2C0, 100000);
    I2C_Cmd(LPC_I2C0, ENABLE);
}

void TIM3_Capture_Init(void) {
    TIM_TIMERCFG_T tcfg;
    tcfg.prescaleOpt   = TIM_US;
    tcfg.prescaleValue = 1;
    TIM_InitTimer(LPC_TIM3, &tcfg);

    TIM_CAPTURECFG_T ccfg;
    ccfg.channel   = TIM_CAPTURE_0;
    ccfg.risingEn  = DISABLE;
    ccfg.fallingEn = ENABLE;
    ccfg.intEn     = ENABLE;
    TIM_ConfigCapture(LPC_TIM3, &ccfg);
    TIM_PinConfig(TIM_CAP3_0_P0_23);

    ccfg.channel = TIM_CAPTURE_1;
    TIM_ConfigCapture(LPC_TIM3, &ccfg);
    TIM_PinConfig(TIM_CAP3_1_P0_24);

    LPC_PINCON->PINMODE1 &= ~(0x3UL << 14); // P0.23 pull-up
    LPC_PINCON->PINMODE1 &= ~(0x3UL << 16); // P0.24 pull-up
}

void iniciar_medicion(void) {
    flag_capture = 0;
    quien_gano   = 0;
    tiempo_us    = 0;

    TIM_Disable(LPC_TIM3);
    TIM_ResetCounter(LPC_TIM3);
    TIM_ClearIntPending(LPC_TIM3, TIM_CR0_INT);
    TIM_ClearIntPending(LPC_TIM3, TIM_CR1_INT);
    NVIC_ClearPendingIRQ(TIMER3_IRQn);
    NVIC_EnableIRQ(TIMER3_IRQn);
    TIM_Enable(LPC_TIM3);
}
