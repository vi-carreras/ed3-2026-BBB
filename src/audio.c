#include "audio.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_timer.h"
#include "LPC17xx.h"
#include <math.h>

#define DAC_MAX_VALUE	1023
#define DAC_BIAS		512

static GPDMA_Channel_CFG_T dmacfg;

void Audio_GenerateTone(uint32_t* buffer, uint32_t samples, uint32_t frecuencia){
	for(uint32_t i = 0; i < samples; i++) {
		float angulo 	= 2.0f * 3.14159f * frecuencia * i / SAMPLE_RATE_HZ;
		int32_t valor 	= (int32_t) (DAC_BIAS + DAC_BIAS * sinf(angulo));

		if(valor < 0) {
			valor = 0;
		}
		if(valor > DAC_MAX_VALUE) {
			valor = DAC_MAX_VALUE;
		}

		buffer[i] = (uint32_t)(valor << 6);
	}
}

void Audio_GenerateSilence(uint32_t* buffer, uint32_t samples){
	for(uint32_t i = 0; i < samples; i++){
		buffer[i] = (uint32_t)(DAC_BIAS << 6);
	}
}

void Audio_Play(uint32_t* buffer, uint32_t size){

	// TIM0, DAC y DMA engine ya fueron configurados por main.
	// Acá solo se configura el canal DMA con el buffer actual y se arranca.

	dmacfg.channelNum = GPDMA_CH_0;
	dmacfg.transferSize = size;
	dmacfg.type = GPDMA_M2P;
	dmacfg.srcMemAddr = (uint32_t)buffer;
	dmacfg.dstMemAddr = (uint32_t)&(LPC_DAC->DACR);
	dmacfg.srcConn = GPDMA_MAT0_0;
	dmacfg.dstConn = GPDMA_DAC;
	dmacfg.src.width = GPDMA_WORD;
	dmacfg.src.burst = GPDMA_BSIZE_1;
	dmacfg.src.increment = ENABLE;
	dmacfg.dst.width = GPDMA_WORD;
	dmacfg.dst.burst = GPDMA_BSIZE_1;
	dmacfg.dst.increment = DISABLE;
	dmacfg.intTC = ENABLE;
	dmacfg.intErr = ENABLE;
	dmacfg.linkedList = 0;

	GPDMA_SetupChannel(&dmacfg);

	NVIC_ClearPendingIRQ(DMA_IRQn);
	NVIC_EnableIRQ(DMA_IRQn);

	GPDMA_ChannelStart(GPDMA_CH_0);
	TIM_Enable(LPC_TIM0);
}

void Audio_Stop(void){
	GPDMA_ChannelStop(GPDMA_CH_0);
	TIM_Disable(LPC_TIM0);
	TIM_ResetCounter(LPC_TIM0);
	DAC_UpdateValue(0);
}
