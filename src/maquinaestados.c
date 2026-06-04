/**
 * @file        maquinaestados.c
 * @brief       Implementación de la máquina de estados del juego de
 *             reacción competitivo para LPC1769.
 */

#include "maquinaestados.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpdma.h"
#include "lcd.h"
#include "uart.h"
#include "audio.h"

/*----------------------------------------------------------------------------
  actualizarestado: procesa un ciclo de la FSM.
  Debe llamarse en el bucle principal de main().
 *----------------------------------------------------------------------------*/
void actualizarestado(void)
{
	char l1[16], l2[16];

	switch(estado_actual){
		case E_IDLE:
			TIM_Disable(LPC_TIM1);
			TIM_ResetCounter(LPC_TIM1);
			TIM_Disable(LPC_TIM0);
			TIM_ResetCounter(LPC_TIM0);
			GPDMA_ChannelStop(GPDMA_CH_0);

			mensajeLCD("Juego Reacción ", "Configurando...");

			if (flag_start_game) {
				flag_start_game = 0;
				config_hecho = 0;
				estado_actual = E_CONFIG;
			}
			break;

		case E_CONFIG:
			mensajeLCD("Configurando   ", " partida...    ");

			if(config_hecho){
				config_hecho = 0;
				victorias_j1 = 0;
				victorias_j2 = 0;
				resultado_ronda = 0;
				countdown_phase = 0;
				estado_actual = E_COUNTDOWN;
			}
			break;

		case E_COUNTDOWN:
			if(countdown_phase == 0){
				Audio_GenerateTone(audio_buf, 150 * SAMPLE_RATE_HZ / 1000, tono_frecuencia);
				mensajeLCD("3...           ", "               ");
				flag_dma_audio_done = 0;
				Audio_Play(audio_buf, 150 * SAMPLE_RATE_HZ / 1000);
				countdown_phase = 1;

			} else if(countdown_phase == 1 && flag_dma_audio_done){
				flag_dma_audio_done = 0;
				Audio_GenerateSilence(audio_buf, velocidad_beeps * SAMPLE_RATE_HZ / 1000);
				Audio_Play(audio_buf, velocidad_beeps * SAMPLE_RATE_HZ / 1000);
				countdown_phase = 2;

			} else if(countdown_phase == 2 && flag_dma_audio_done){
				flag_dma_audio_done = 0;
				Audio_GenerateTone(audio_buf, 150 * SAMPLE_RATE_HZ / 1000, tono_frecuencia);
				mensajeLCD("2...           ", "               ");
				Audio_Play(audio_buf, 150 * SAMPLE_RATE_HZ / 1000);
				countdown_phase = 3;

			} else if(countdown_phase == 3 && flag_dma_audio_done){
				flag_dma_audio_done = 0;
				Audio_GenerateSilence(audio_buf, velocidad_beeps * SAMPLE_RATE_HZ / 1000);
				Audio_Play(audio_buf, velocidad_beeps * SAMPLE_RATE_HZ / 1000);
				countdown_phase = 4;

			} else if(countdown_phase == 4 && flag_dma_audio_done){
				flag_dma_audio_done = 0;
				Audio_GenerateTone(audio_buf, 150 * SAMPLE_RATE_HZ / 1000, tono_frecuencia);
				mensajeLCD("1...           ", "               ");
				Audio_Play(audio_buf, 150 * SAMPLE_RATE_HZ / 1000);
				countdown_phase = 5;

			} else if(countdown_phase == 5 && flag_dma_audio_done){
				flag_dma_audio_done = 0;
				Audio_GenerateSilence(audio_buf, velocidad_beeps * SAMPLE_RATE_HZ / 1000);
				Audio_Play(audio_buf, velocidad_beeps * SAMPLE_RATE_HZ / 1000);
				countdown_phase = 6;

			} else if(countdown_phase == 6 && flag_dma_audio_done){
				flag_dma_audio_done = 0;
				Audio_GenerateTone(audio_buf, 200 * SAMPLE_RATE_HZ / 1000, tono_frecuencia * 2);
				mensajeLCD("GO!            ", "               ");
				Audio_Play(audio_buf, 200 * SAMPLE_RATE_HZ / 1000);
				countdown_phase = 7;

			} else if(countdown_phase == 7 && flag_dma_audio_done){
				flag_dma_audio_done = 0;
				countdown_phase = 0;
				estado_actual = E_WAIT_GO;
			}
			break;

		case E_WAIT_GO:
			l1[0] = 'P'; l1[1] = 'r'; l1[2] = 'e'; l1[3] = 's'; l1[4] = 'i'; l1[5] = 'o'; l1[6] = 'n'; l1[7] = 'e'; l1[8] = ':'; l1[9] = ' ';
			l1[10] = tecla_objetivo;
			for(int i = 11; i < 16; i++) l1[i] = ' ';
			mensajeLCD(l1, "               ");

			NVIC_ClearPendingIRQ(TIMER1_IRQn);
			NVIC_EnableIRQ(TIMER1_IRQn);
			TIM_Enable(LPC_TIM1);

			flag_capture_event = 0;
			resultado_ronda = 0;
			tiempo_inicio_espera = msTicks;
			estado_actual = E_WAIT_INPUT;
			break;

		case E_WAIT_INPUT:
			if (flag_capture_event) {
				estado_actual = E_ROUND_END;
			}
			else if ((msTicks - tiempo_inicio_espera) >= TIMEOUT_MS) {
				TIM_Disable(LPC_TIM1);
				NVIC_DisableIRQ(TIMER1_IRQn);
				resultado_ronda = 0;
				estado_actual = E_ROUND_END;
			}
			break;

		case E_ROUND_END:
			if(round_end_phase == 0) {
				round_end_phase = 1;

				switch(resultado_ronda) {
					case 0: // Timeout
						mensajeLCD("TIEMPO AGOTADO ", "               ");
						enviar_str("T\r\n");
						break;
					case 1: { // J1 Acertó
						victorias_j1++;
						enviar_str("W1\r\n");
						uint32_t val = tiempo_reaccion_jugador;
						char rbuf[8];
						uint8_t rlen = 0;
						do {
							rbuf[rlen++] = '0' + (val % 10);
							val /= 10;
						} while(val > 0);
						enviar_char('R');
						for(int8_t i = (int8_t)rlen - 1; i >= 0; i--)
							enviar_char(rbuf[i]);
						enviar_str("\r\n");
						break;
					}
					case 2: { // J2 Acertó
						victorias_j2++;
						enviar_str("W2\r\n");
						uint32_t val = tiempo_reaccion_jugador;
						char rbuf[8];
						uint8_t rlen = 0;
						do {
							rbuf[rlen++] = '0' + (val % 10);
							val /= 10;
						} while(val > 0);
						enviar_char('R');
						for(int8_t i = (int8_t)rlen - 1; i >= 0; i--)
							enviar_char(rbuf[i]);
						enviar_str("\r\n");
						break;
					}
					case 3: // J1 Incorrecto
						if(victorias_j1 > 0) victorias_j1--;
						enviar_str("E1\r\n");
						break;
					case 4: // J2 Incorrecto
						if(victorias_j2 > 0) victorias_j2--;
						enviar_str("E2\r\n");
						break;
					default:
						break;
				}

				enviar_str("S");
				if(victorias_j1 >= 10) enviar_char('0' + (victorias_j1 / 10));
				enviar_char('0' + (victorias_j1 % 10));
				enviar_char(',');
				if(victorias_j2 >= 10) enviar_char('0' + (victorias_j2 / 10));
				enviar_char('0' + (victorias_j2 % 10));
				enviar_str("\r\n");

				l1[0] = 'J'; l1[1] = '1'; l1[2] = ':'; l1[3] = ' ';
				l1[4] = '0' + (victorias_j1 % 10); l1[5] = ' ';
				for(int i = 6; i < 16; i++) l1[i] = ' ';
				l2[0] = 'J'; l2[1] = '2'; l2[2] = ':'; l2[3] = ' ';
				l2[4] = '0' + (victorias_j2 % 10); l2[5] = ' ';
				for(int i = 6; i < 16; i++) l2[i] = ' ';
				mensajeLCD(l1, l2);

				round_end_start = msTicks;
			}

			if((msTicks - round_end_start) >= 2000) {
				round_end_phase = 0;
				if(victorias_j1 >= MAX_VICTORIAS || victorias_j2 >= MAX_VICTORIAS){
					enviar_str(victorias_j1 >= MAX_VICTORIAS ? "G1\r\n" : "G2\r\n");
					estado_actual = E_GAME_OVER;
				} else {
					countdown_phase = 0;
					estado_actual = E_COUNTDOWN;
				}
			}
			break;

		case E_GAME_OVER:
			mensajeLCD("GANADOR:       ",
				victorias_j1 >= MAX_VICTORIAS ? "JUGADOR 1      " : "JUGADOR 2      ");

			{
				uint16_t freqs[] = {440, 660, 880};
				for(int i = 0; i < 3; i++){
					Audio_GenerateTone(audio_buf, 20, freqs[i]);
					Audio_Play(audio_buf, 20);
					retardo_ms(50);
					Audio_Stop();
				}
			}

			retardo_ms(500);
			estado_actual = E_IDLE;
			break;

		default:
			break;
	}
}
