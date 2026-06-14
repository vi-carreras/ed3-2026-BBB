/**
 * @file        perifericos.c
 * @brief       Implementación de la inicialización de periféricos del
 *             LPC1769 para el juego de reacción competitivo.
 */

#include "perifericos.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpdma.h"

/*----------------------------------------------------------------------------
  configGPIO: Pines para teclado matricial 4x4 de J1 (P0) y J2 (P2)
 *----------------------------------------------------------------------------*/
void configGPIO(void)
{
	PINSEL_CFG_T pinCfg = {PORT_0, 0, PINSEL_FUNC_00, PINSEL_TRISTATE, DISABLE};

    // Puerto 0[7:0] para teclado J1
	PINSEL_ConfigMultiplePins(&pinCfg, 0x0F); // -> [3:0]
	pinCfg.mode = PINSEL_PULLUP;
	PINSEL_ConfigMultiplePins(&pinCfg, 0xF0); // -> [7:4]
	GPIO_SetDir(PORT_0, 0x0F, GPIO_OUTPUT);	  // [3:0] salidas
	GPIO_SetDir(PORT_0, 0xF0, GPIO_INPUT);    // [7:4] entradas
	GPIO_ClearPins(PORT_0, 0xFF);
	GPIO_SetMask(PORT_0, 0xFFFFFF00, ENABLE); // enmascarar pines no usados

    // Puerto 2[7:0] para teclado J2
	pinCfg.port = PORT_2;
	pinCfg.mode = PINSEL_TRISTATE;
	PINSEL_ConfigMultiplePins(&pinCfg, 0x0F); // -> [3:0]
	pinCfg.mode = PINSEL_PULLUP;
	PINSEL_ConfigMultiplePins(&pinCfg, 0xF0); // -> [7:4]
	GPIO_SetDir(PORT_2, 0x0F, GPIO_OUTPUT);	  // [3:0] salidas
	GPIO_SetDir(PORT_2, 0xF0, GPIO_INPUT);    // [7:4] entradas
    GPIO_ClearPins(PORT_2, 0xFF);
    GPIO_SetMask(PORT_2, 0xFFFFFF00, ENABLE); // enmascarar pines no usados
}

/*----------------------------------------------------------------------------
  configUART1: UART1 a 115200 8N1 con FIFO e IRQ de recepción
 *----------------------------------------------------------------------------*/
void configUART1(void)
{
    PINSEL_CFG_T pinCfg = {PORT_0, PIN_15, PINSEL_FUNC_01, PINSEL_PULLUP, DISABLE};
    PINSEL_ConfigPin(&pinCfg);	  // TXD1
    pinCfg.pin = PIN_16;
    PINSEL_ConfigPin(&pinCfg);	  // RXD1

    UART_CFG_T uartCfg = {115200, UART_PARITY_NONE, UART_DBITS_8, UART_STOPBIT_1};
    UART_Init(UART1, &uartCfg);   // 115200 8N1

    UART_FIFO_CFG_T fifoCfg = {ENABLE, ENABLE, DISABLE, UART_FIFO_TRGLEV0};
    UART_FIFOConfig(UART1, &fifoCfg);

    UART_TxEnable(UART1);

    UART_IntConfig(UART1, UART_INT_RBR, ENABLE);
    NVIC_EnableIRQ(UART1_IRQn);
}

/*----------------------------------------------------------------------------
  configI2C0: I2C0 a 100 KHz para LCD (PCF8574)
 *----------------------------------------------------------------------------*/
void configI2C0(void)
{
	PINSEL_CFG_T pinCfg = {PORT_0, PIN_27, PINSEL_FUNC_01, PINSEL_PULLUP, ENABLE};
    PINSEL_ConfigPin(&pinCfg);	  // SDA0

    pinCfg.pin = PIN_28;
    PINSEL_ConfigPin(&pinCfg);	  // SCL0

    I2C_Init(LPC_I2C0, 100000);	  // 100 KHz
    I2C_Cmd(LPC_I2C0, ENABLE);
}

/*----------------------------------------------------------------------------
  configDAC: DAC con DMA, bias 700 µA, salida en 0
 *----------------------------------------------------------------------------*/
void configDAC(void)
{
	PINSEL_CFG_T pinCfg = {PORT_0, PIN_26, PINSEL_FUNC_10, PINSEL_TRISTATE, DISABLE};
	PINSEL_ConfigPin(&pinCfg);	  // AOUT por P0.26

	DAC_Init();
	DAC_SetBias(DAC_700uA);
	DAC_UpdateValue(0);

	DAC_CONVERTER_CFG_T dacfg;
	dacfg.doubleBuffer = DISABLE;
	dacfg.dmaCounter   = ENABLE;
	dacfg.dmaRequest   = ENABLE;
	DAC_ConfigDAConverterControl(&dacfg);

	DAC_SetDMATimeOut(2500);	  // 2500 µs timeout
}

/*----------------------------------------------------------------------------
  configDMA: Inicializa GPDMA. Los canales los configura Audio_Play().
 *----------------------------------------------------------------------------*/
void configDMA(void)
{
	GPDMA_Init();
}

/*----------------------------------------------------------------------------
  configTIMER0: TIM0 como scheduler DAC — MR0 dispara DMA cada 100 µs
 *----------------------------------------------------------------------------*/
void configTIMER0(void)
{
	TIM_TIMERCFG_T timerCfg;
	timerCfg.prescaleOpt  = TIM_US;
	timerCfg.prescaleValue = 1;		  // 1 µs / tick

	TIM_InitTimer(LPC_TIM0, &timerCfg);

	TIM_MATCHCFG_T matchCfg;
	matchCfg.channel    = 0;
	matchCfg.intEn      = DISABLE;	  // no IRQ, solo trigger DMA
	matchCfg.resetEn    = ENABLE;
	matchCfg.stopEn     = DISABLE;
	matchCfg.extOpt     = TIM_NOTHING;
	matchCfg.matchValue = 100;		  // 100 µs → 10 KHz

	TIM_ConfigMatch(LPC_TIM0, &matchCfg);

	TIM_Disable(LPC_TIM0);			  // apagado; Audio_Play lo enciende
}

/*----------------------------------------------------------------------------
  configTIMER3: TIM3 con capture para medir tiempo de reacción (1 µs res.)
 *----------------------------------------------------------------------------*/
void configTIMER3(void)
{
	TIM_TIMERCFG_T timerCfg;
	timerCfg.prescaleOpt  = TIM_US;
	timerCfg.prescaleValue = 1;		  // 1 µs / tick
	TIM_InitTimer(LPC_TIM3, &timerCfg);

	PINSEL_CFG_T pinCfg = {PORT_0, PIN_23, PINSEL_FUNC_11, PINSEL_PULLDOWN, DISABLE};
	PINSEL_ConfigPin(&pinCfg);

	pinCfg.pin = PIN_24;
	PINSEL_ConfigPin(&pinCfg);

	TIM_CAPTURECFG_T capCfg;
	capCfg.channel   = TIM_CAPTURE_0; // J1 -> CAP3.0 (P0.23)
	capCfg.risingEn  = ENABLE;
	capCfg.fallingEn = ENABLE;
	capCfg.intEn     = ENABLE;
	TIM_ConfigCapture(LPC_TIM3, &capCfg);

	capCfg.channel 	= TIM_CAPTURE_1; // J2 -> CAP3.1 (P0.24)
	TIM_ConfigCapture(LPC_TIM3, &capCfg);

	NVIC_EnableIRQ(TIMER3_IRQn);
}
