//---LIBRERIAS--------------------------------------------------------------------------
#include "LPC17xx.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpdma.h"
#include "lcd.h"
#include "teclado.h"
#include "uart.h"
#include "audio.h"
#include "lpc17xx_systick.h"
#include "perifericos.h"

//---Variables de SysTick para msTicks (reloj del sistema)---
volatile uint32_t msTicks = 0;

void SysTick_Handler(void){
	msTicks++;
}

//---VARIABLES-------------------------------------------------------------------------
typedef enum{
    E_IDLE = 0,
	E_CONFIG,
	E_COUNTDOWN,
	E_WAIT_GO,
	E_WAIT_INPUT,
	E_ROUND_END,
	E_GAME_OVER
} estado_juego_t;
volatile estado_juego_t estado_actual = E_IDLE;

// --- Audio ---
#define AUDIO_BUF_SIZE   2000  // suficientes samples para cualquier beep
uint32_t audio_buf[AUDIO_BUF_SIZE];

volatile uint8_t countdown_phase = 0;  // 0=3, 1=2, 2=1, 3=GO, 4=hecho
volatile uint8_t  round_end_phase = 0;  // 0=pendiente, 1=mostrando resultado
volatile uint32_t round_end_start = 0;  // msTicks al iniciar pausa

// --- Config de partida (desde UART) ---
volatile uint8_t  tecla_objetivo = 'A';
volatile uint16_t tono_frecuencia = 440;   // Hz para beeps
volatile uint16_t velocidad_beeps = 200;   // ms entre beeps

volatile uint8_t flag_start_game = 0;			//	flag para notificar que se recibe comando de empezar juego
volatile uint8_t flag_dma_audio_done = 0;		//
volatile uint8_t flag_capture_event = 0;		//
volatile uint32_t tiempo_reaccion_jugador = 0;	//	tiempo de reacción que se envía en caso de ganador de ronda
// 0: Ninguno, 1: J1 Correcto, 2: J2 Correcto, 3: J1 Incorrecto, 4: J2 Incorrecto
volatile uint8_t resultado_ronda = 0;

//puntaje de ambos jugadores
volatile uint8_t victorias_j1 = 0;
volatile uint8_t victorias_j2 = 0;
#define MAX_VICTORIAS 10

volatile uint32_t tiempo_inicio_espera = 0;	//timestamp de inicio de espera para timeout
volatile uint8_t config_hecho = 0;	//flag: config completa desde UART, ir a COUNTDOWN

#define TIMEOUT_MS 10000		//tiempo máximo de ingreso de respuesta

//---FUNCIONES-------------------------------------------------------------------------
void actualizarestado(); //función actualizar máquina de estados


int main(void) {
	configGPIO();
	configUART1();
	configI2C0();
	configDAC();
	configDMA();
	configTIMER0();
	configTIMER1();

	// SysTick cada 1ms para msTicks (reemplaza TIM0 MR1 que no puede convivir con MR0)
	SYSTICK_InternalInit(1);
	SYSTICK_IntCmd(ENABLE);
	SYSTICK_Cmd(ENABLE);

	while(1){
		actualizarestado();
	}
	// return 0; — no se llega nunca
}

//---FUNCIONES-------------------------------------------------------------------------
void actualizarestado(){
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

			// Esperar configuración por UART
			// UART1_IRQHandler llena tecla_objetivo, tono_frecuencia, velocidad_beeps
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
				// 3...
				Audio_GenerateTone(audio_buf, 150 * SAMPLE_RATE_HZ / 1000, tono_frecuencia);
				mensajeLCD("3...           ", "               ");
				flag_dma_audio_done = 0;
				Audio_Play(audio_buf, 150 * SAMPLE_RATE_HZ / 1000);
				countdown_phase = 1;

			} else if(countdown_phase == 1 && flag_dma_audio_done){
				flag_dma_audio_done = 0;
				// Silencio entre beeps
				Audio_GenerateSilence(audio_buf, velocidad_beeps * SAMPLE_RATE_HZ / 1000);
				Audio_Play(audio_buf, velocidad_beeps * SAMPLE_RATE_HZ / 1000);
				countdown_phase = 2;

			} else if(countdown_phase == 2 && flag_dma_audio_done){
				// 2...
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
				// 1...
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
				// GO! — más agudo
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
			// Mostrar tecla objetivo en LCD
			l1[0] = 'P'; l1[1] = 'r'; l1[2] = 'e'; l1[3] = 's'; l1[4] = 'i'; l1[5] = 'o'; l1[6] = 'n'; l1[7] = 'e'; l1[8] = ':'; l1[9] = ' ';
			l1[10] = tecla_objetivo;
			for(int i = 11; i < 16; i++) l1[i] = ' ';
			mensajeLCD(l1, "               ");

			// Habilitar capture
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
						// Enviar tiempo de reacción R<decimal>\r\n
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

				// Enviar scores S<J1>,<J2>\r\n
				enviar_str("S");
				if(victorias_j1 >= 10) enviar_char('0' + (victorias_j1 / 10));
				enviar_char('0' + (victorias_j1 % 10));
				enviar_char(',');
				if(victorias_j2 >= 10) enviar_char('0' + (victorias_j2 / 10));
				enviar_char('0' + (victorias_j2 % 10));
				enviar_str("\r\n");

				// Mostrar puntajes en LCD
				l1[0] = 'J'; l1[1] = '1'; l1[2] = ':'; l1[3] = ' ';
				l1[4] = '0' + (victorias_j1 % 10); l1[5] = ' ';
				for(int i = 6; i < 16; i++) l1[i] = ' ';
				l2[0] = 'J'; l2[1] = '2'; l2[2] = ':'; l2[3] = ' ';
				l2[4] = '0' + (victorias_j2 % 10); l2[5] = ' ';
				for(int i = 6; i < 16; i++) l2[i] = ' ';
				mensajeLCD(l1, l2);

				round_end_start = msTicks;
			}

			// Espera no bloqueante de 2 segundos
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

			// 3 tonos ascendentes cortos (20 samples cada uno a 10KHz)
			{
				uint16_t freqs[] = {440, 660, 880};
				for(int i = 0; i < 3; i++){
					Audio_GenerateTone(audio_buf, 20, freqs[i]);
					Audio_Play(audio_buf, 20);
					retardo_ms(50);
					Audio_Stop();
				}
			}

			retardo_ms(500); // pausa antes de volver a IDLE
			estado_actual = E_IDLE;
			break;

		default:
			break;
	}
}

// --- MANEJO DE INTERRUPCIONES (NVIC) ---
void TIMER0_IRQHandler(void)
{
    // MR0 tiene intEn = DISABLE (solo trigger DMA), esta IRQ no se dispara en operación normal.
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}

void TIMER1_IRQHandler(void) {
    uint8_t tecla_presionada = 0;

    // --- JUGADOR 1 (CAP1.0) ---
    if (TIM_GetIntStatus(LPC_TIM1, TIM_CR0_INT)) {
        // Limpiamos el flag de la interrupción
        TIM_ClearIntPending(LPC_TIM1, TIM_CR0_INT);

        // Guardamos el tiempo exacto de reacción
        tiempo_reaccion_jugador = TIM_GetCaptureValue(LPC_TIM1, TIM_CAPTURE_0);

        // Leemos qué tecla presionó J1 en su teclado
        tecla_presionada = EscanearTecladoJ1();

        // Comparamos contra la ÚNICA tecla objetivo
        if (tecla_presionada == tecla_objetivo) {
            resultado_ronda = 1; // J1 Correcto
        } else {
            resultado_ronda = 3; // J1 Incorrecto (Penalización)
        }

        // Avisamos a la máquina de estados y apagamos capturas
        flag_capture_event = 1;
        TIM_Disable(LPC_TIM1);
        NVIC_DisableIRQ(TIMER1_IRQn);
    }

    // --- JUGADOR 2 (CAP1.1) ---
    else if (TIM_GetIntStatus(LPC_TIM1, TIM_CR1_INT)) {
        // Limpiamos el flag de la interrupción
        TIM_ClearIntPending(LPC_TIM1, TIM_CR1_INT);

        // Guardamos el tiempo exacto de reacción
        tiempo_reaccion_jugador = TIM_GetCaptureValue(LPC_TIM1, TIM_CAPTURE_1);

        // Leemos qué tecla presionó J2 en su teclado
        tecla_presionada = EscanearTecladoJ2();

        // Comparamos contra la ÚNICA tecla objetivo
        if (tecla_presionada == tecla_objetivo) {
            resultado_ronda = 2; // J2 Correcto
        } else {
            resultado_ronda = 4; // J2 Incorrecto (Penalización)
        }

        // Avisamos a la máquina de estados y apagamos capturas
        flag_capture_event = 1;
        TIM_Disable(LPC_TIM1);
        NVIC_DisableIRQ(TIMER1_IRQn);
    }
}

void UART1_IRQHandler(void) {
	static char line_buf[16];
	static uint8_t line_idx = 0;

	uint8_t rx = UART_ReceiveByte(UART1);

	// Fin de línea: procesar comando
	if(rx == '\r' || rx == '\n') {
		if(line_idx > 0) {
			line_buf[line_idx] = '\0';
			char cmd = line_buf[0];
			if(cmd >= 'a' && cmd <= 'z') cmd -= 32;  // uppercase solo letras

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
					// QUERY: responder con scores actuales
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
	// buffer lleno sin \r\n: descartar silenciosamente
}

void DMA_IRQHandler(void) {
    // Limpiar flag de interrupción del DMA sin esto queda forever loop
    GPDMA_ClearIntPending(GPDMA_CLR_INTTC, GPDMA_CH_0);

    // Avisar a la FSM que terminó la reproducción
    flag_dma_audio_done = 1;
}
