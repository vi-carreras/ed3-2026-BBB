//---LIBRERIAS--------------------------------------------------------------------------
#include "LPC17xx.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpdma.h"
#include "teclado.h"
#include "uart.h"
#include "lpc17xx_systick.h"
#include "perifericos.h"
#include "maquinaestados.h"

//---Variables de SysTick para msTicks (reloj del sistema)---
volatile uint32_t msTicks = 0;

void SysTick_Handler(void){
	msTicks++;
}

//---VARIABLES-------------------------------------------------------------------------
volatile estado_juego_t estado_actual = E_IDLE;

// --- Audio ---
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
