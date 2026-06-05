//---LIBRERIAS--------------------------------------------------------------------------
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
	configTIMER3();

	// SysTick cada 1ms para msTicks (reemplaza TIM0 MR1 que no puede convivir con MR0)
	SYSTICK_InternalInit(1);
	SYSTICK_IntCmd(ENABLE);
	SYSTICK_Cmd(ENABLE);

	while(1){
		actualizarestado();
	}
	// return 0; — no se llega nunca
}

