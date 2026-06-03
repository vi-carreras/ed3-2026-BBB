#include "audio.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"
#include "LPC17xx.h"
#include <math.h>

#define DAC_MAX_VALUE	1023U
#define DAC_BIAS		512U
#define SAMPLE_RATE_HZ	10000U

static GPDMA_Channel_CFG_Type dmacfg;

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

void Audio_Play(uint32_t* buffer, uint32_t size){

	TIM_TIMERCFG_T tmcfg;
	tmcfg.prescaleOpt = TIM_US;
	tmcfg.prescaleValue = 1;

	Tim_InitTimer(&tmcfg);

	TIM_MATCHCFG_T mcfg;
	mcfg.channel = TIM_MATCH_0;
	mcfg.intEn = DISABLE;
	mcfg.stopEn = DISABLE;
	mcfg.resetEn = ENABLE;
	mcfg.extOpt = TIM_NOTHING;
	mcfg.matchValue = 1000000 / SAMPLE_RATE_HZ;
	TIM_ConfigMatch(LPC_TIM0, &mcfg);

	DAC_CONVERTER_CFG_T dacfg;
	dacfg.doubleBuffer = DISABLE;
	dacfg.dmaCounter = ENABLE;
	dacfg.dmaRequest = ENABLE;
	DAC_ConfigDAConverterControl(&dacfg);

	DAC_SetDMATimeOut((uint32_t)(25000000 / SAMPLE_RATE_HZ));

	GPDMA_Endpoint_T src;
	src.width = GPDMA_WORD;
	src.burst = GPDMA_BSIZE_1;
	src.increment = ENABLE;

	GPDMA_Endpoint_T dst;
	dst.width = GPDMA_WORD;
	dst.burst = GPDMA_BSIZE_1;
	dst.increment = DISABLE;

	dmacfg.channelNum = GPDMA_CH_0;
	dmacfg.transferSize = size;
	dmacfg.type = GPDMA_M2P;
	dmacfg.srcMemAddr = (uint32_t)buffer;
	dmacfg.dstMemAddr = (uint32_t)&(LPC_DAC->DACR);
	dmacfg.srcConn = GPDMA_MAT0_0;
	dmacfg.dstConn = GPDMA_DAC;
	dmacfg.src = &src;
	dmacfg.dst = &dst;
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
