//---LIBRERIAS--------------------------------------------------------------------------
#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_nvic.h"

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

volatile uint8_t flag_start_game = 0;			//	flag para notificar que se recibió comando de empezar juego
volatile uint8_t flag_dma_audio_done = 0;		//
volatile uint8_t flag_capture_event = 0;		//  
volatile uint32_t tiempo_reaccion_jugador = 0;	//	tiempo de reacción que se envía en caso de ganador de ronda
volatile uint8_t jugador_ganador = 0;
volatile uint32_t msTicks = 0;                  //Contador de milisegundos

volatile uint8_t tecla_objetivo = 0; // misma tecla para J1 y J2

// 0: Ninguno, 1: J1 Correcto, 2: J2 Correcto, 3: J1 Incorrecto, 4: J2 Incorrecto
volatile uint8_t resultado_ronda = 0;

//puntaje de ambos jugadores
volatile uint8_t victorias_j1 = 0;
volatile uint8_t victorias_j2 = 0;
#define MAX_VICTORIAS 10

#define TIMEOUT_MS 5000		//tiempo máximo de ingreso de respuesta

//---FUNCIONES-------------------------------------------------------------------------
void configGPIO(void);		//pines
void configUART0(void);		//inicializa comunicación UART
void configI2C0(void);		//inicializa comunicación I2C
void configDAC(void);		//inicializa DAC
void configDMA(void);		//inicializa DMA
void configTIMER0(void);	//inicializa Timer0 
void configTIMER1(void);    //inicializa Timer1

void actualizarestado();



int main(void) {
	//configuración de periféricos

	while(1){
		actualizarestado();//función actualizar máquina de estados
	}

    return 0 ;
}

//---FUNCIONES-------------------------------------------------------------------------
void configGPIO(void)
{
    GPIO_SetDir(PORT_1, BIT(0), OUTPUT);
    GPIO_ClearValue(PORT_1, BIT(0));

    GPIO_SetDir(PORT_2, BIT(10), INPUT);
}

void configUART0(void)
{
    PINSEL_CFG_Type pinCfg;

    pinCfg.Portnum   = PINSEL_PORT_0;
    pinCfg.Pinnum    = PINSEL_PIN_2;
    pinCfg.Funcnum   = PINSEL_FUNC_1;
    pinCfg.Pinmode   = PINSEL_PINMODE_PULLUP;
    pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pinCfg);

    pinCfg.Pinnum = PINSEL_PIN_3;
    PINSEL_ConfigPin(&pinCfg);

    UART_CFG_Type uartCfg;

    UART_ConfigStructInit(&uartCfg);

    uartCfg.Baud_rate = 115200;

    UART_Init(LPC_UART0, &uartCfg);

    UART_TxCmd(LPC_UART0, ENABLE);
}
void configI2C0(void)
{
    PINSEL_CFG_Type pinCfg;

    pinCfg.Portnum   = PINSEL_PORT_0;
    pinCfg.Funcnum   = PINSEL_FUNC_1;
    pinCfg.Pinmode   = PINSEL_PINMODE_TRISTATE;
    pinCfg.OpenDrain = PINSEL_PINMODE_OPENDRAIN;

    pinCfg.Pinnum = PINSEL_PIN_27;
    PINSEL_ConfigPin(&pinCfg);

    pinCfg.Pinnum = PINSEL_PIN_28;
    PINSEL_ConfigPin(&pinCfg);

    I2C_Init(LPC_I2C0, 100000);

    I2C_Cmd(LPC_I2C0, ENABLE);
}

void configDAC(void)
{
    PINSEL_CFG_Type pinCfg;

    pinCfg.Portnum   = PINSEL_PORT_0;
    pinCfg.Pinnum    = PINSEL_PIN_26;
    pinCfg.Funcnum   = PINSEL_FUNC_2;
    pinCfg.Pinmode   = PINSEL_PINMODE_TRISTATE;
    pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pinCfg);

    DAC_Init(LPC_DAC);

    DAC_UpdateValue(LPC_DAC, 0);

    DAC_SetBias(LPC_DAC, DAC_MAX_CURRENT_700uA);
}
void configDMA(void)
{
	//completar mem a periferico para DAC

    GPDMA_Init();
}

void configTIMER0(void)
{
    TIM_TIMERCFG_T timerCfg;

    timerCfg.prescaleOpt = TIM_PRESCALE_USVAL;
    timerCfg.prescaleValue = 1000;

    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timerCfg);

    TIM_MATCHCFG_T matchCfg;

    matchCfg.MatchChannel = 0;
    matchCfg.IntOnMatch   = ENABLE;
    matchCfg.ResetOnMatch = ENABLE;
    matchCfg.StopOnMatch  = DISABLE;
    matchCfg.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    matchCfg.MatchValue   = 1;

    TIM_ConfigMatch(LPC_TIM0, &matchCfg);

    NVIC_EnableIRQ(TIMER0_IRQn);

    TIM_Cmd(LPC_TIM0, ENABLE);
}

void configTIMER1(void){
    //inicializar timer1 habilitando capture
}


void actualizarestado(){
	switch(estado_actual){
		case E_IDLE:
			/* * El microcontrolador entra en estado de espera.
			 * Acciones:
			 * - Desactivar periféricos que no se usan (Capture, DAC, DMA).
			 * - Mostrar mensaje de espera en LCD 16x2.
			 * - Transición: Si se recibe el comando START_GAME por UART,
			 * cambiar a E_CONFIG.
			 */

			if (flag_start_game) {
				flag_start_game = 0;
				estado_actual = E_CONFIG;
			}
			break;

		case E_CONFIG:
			/* * La aplicación Java configura la partida mediante UART.
			 * Acciones:
			 * - Reiniciar puntajes al comenzar una nueva partida completa.
			 * - Procesar la configuración recibida (configurar teclas, seleccionar tono).
			 * - Transición: Automática a E_COUNTDOWN una vez procesada la configuración.
			 */

			victorias_j1 = 0;
			victorias_j2 = 0;

			// Falta código de procesamiento de configuración

			estado_actual = E_COUNTDOWN;
			break;

		case E_COUNTDOWN:
			/* * Se inicia la cuenta regresiva auditiva mediante DAC.
			 * Acciones:
			 * - Configurar y habilitar DMA para transferir samples al DAC.
			 * - Los sonidos se generan sincronizados por un Timer Match.
			 * - Transición: Automática a E_WAIT_GO. La finalización de los tonos se
			 * manejará mediante interrupciones (DMA IRQ / Timer IRQ).
			 */

			// Falta habilitar DMA y Timer para audio...
			estado_actual = E_WAIT_GO;
			break;

		case E_WAIT_GO:
			if (flag_dma_audio_done) {
				// Mostrar tecla en LCD...

				// Habilitar interrupciones de Input Capture
				NVIC_ClearPendingIRQ(TIMER1_IRQn);
				NVIC_EnableIRQ(TIMER1_IRQn);

				flag_capture_event = 0;
				resultado_ronda = 0;

				// Guardamos el "timestamp" de inicio usando msTicks para el timeout
				tiempo_inicio_espera = msTicks;

				estado_actual = E_WAIT_INPUT;
			}
			break;

		case E_WAIT_INPUT:
			// Opción 1: Alguien presionó una tecla (IRQ activó el flag)
			if (flag_capture_event) {
				estado_actual = E_ROUND_END;
			}
			// Opción 2: Pasó el tiempo de Timeout (Ej: 5000 ms)
			else if ((msTicks - tiempo_inicio_espera) >= TIMEOUT_MS) {
				// Desactivar capturas para que no interrumpan fuera de tiempo
				TIM_Cmd(LPC_TIM1, DISABLE);
				NVIC_DisableIRQ(TIMER1_IRQn);

				resultado_ronda = 0; // 0 indicará un Empate/Timeout
				estado_actual = E_ROUND_END;
			}
			break;

		case E_ROUND_END:
			/* Procesar el resultado de la ronda */
			switch(resultado_ronda) {
				case 0: // Timeout
					// Ambos fallaron por lentos. No se suman ni restan puntos.
					// Mostrar mensaje "TIEMPO AGOTADO" en LCD
					break;
				case 1: // J1 Acertó
					victorias_j1++;
					break;
				case 2: // J2 Acertó
					victorias_j2++;
					break;
				case 3: // J1 Presionó tecla incorrecta (Penalización)
					if (victorias_j1 > 0) victorias_j1--;
					break;
				case 4: // J2 Presionó tecla incorrecta (Penalización)
					if (victorias_j2 > 0) victorias_j2--;
					break;
			}

			// Evaluar condición de victoria final
			if (victorias_j1 >= MAX_VICTORIAS || victorias_j2 >= MAX_VICTORIAS) {
				estado_actual = E_GAME_OVER;
			} else {
				// Nueva ronda
				estado_actual = E_COUNTDOWN;
			}

            //Enviar datos de ronda por UART
			break;

		case E_GAME_OVER:
			/* * Acciones:
			 * - Anunciar al ganador definitivo en el LCD.
			 * - Enviar comando a Java indicando el fin de la partida.
			 * - Esperar un tiempo o un comando para volver a E_IDLE.
			 */

			if (victorias_j1 >= MAX_VICTORIAS) {
				// Display_ShowWinner(1);
			} else {
				// Display_ShowWinner(2);
			}

			// Audio_Play(buffer_game_over, size); // Sonido de fin de juego

			// Al terminar de mostrar el ganador,se vuelve a IDLE para esperar un nuevo START_GAME
			estado_actual = E_IDLE;

			break;
	}
}

// --- MANEJO DE INTERRUPCIONES (NVIC) ---
void TIMER0_IRQHandler(void)
{
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);

    msTicks++;
}

void TIMER1_IRQHandler(void) {
    uint8_t tecla_presionada = 0;

    // --- JUGADOR 1 (CAP1.0) ---
    if (TIM_GetIntStatus(LPC_TIM1, TIM_CR0_INT)) {
        // Limpiamos el flag de la interrupción
        TIM_ClearIntPending(LPC_TIM1, TIM_CR0_INT);

        // Guardamos el tiempo exacto de reacción
        tiempo_reaccion_jugador = TIM_GetCaptureValue(LPC_TIM1, TIM_COUNTER_INCAP0);

        // Leemos qué tecla se presionó físicamente
        tecla_presionada = LeerTeclaMatricial();

        // Comparamos contra la ÚNICA tecla objetivo
        if (tecla_presionada == tecla_objetivo) {
            resultado_ronda = 1; // J1 Correcto
        } else {
            resultado_ronda = 3; // J1 Incorrecto (Penalización)
        }

        // Avisamos a la máquina de estados y apagamos capturas
        flag_capture_event = 1;
        TIM_Cmd(LPC_TIM1, DISABLE);
        NVIC_DisableIRQ(TIMER1_IRQn);
    }

    // --- JUGADOR 2 (CAP1.1) ---
    else if (TIM_GetIntStatus(LPC_TIM1, TIM_CR1_INT)) {
        // Limpiamos el flag de la interrupción
        TIM_ClearIntPending(LPC_TIM1, TIM_CR1_INT);

        // Guardamos el tiempo exacto de reacción
        tiempo_reaccion_jugador = TIM_GetCaptureValue(LPC_TIM1, TIM_COUNTER_INCAP1);

        // Leemos qué tecla se presionó físicamente
        tecla_presionada = LeerTeclaMatricial();

        // Comparamos contra la ÚNICA tecla objetivo
        if (tecla_presionada == tecla_objetivo) {
            resultado_ronda = 2; // J2 Correcto
        } else {
            resultado_ronda = 4; // J2 Incorrecto (Penalización)
        }

        // Avisamos a la máquina de estados y apagamos capturas
        flag_capture_event = 1;
        TIM_Cmd(LPC_TIM1, DISABLE);
        NVIC_DisableIRQ(TIMER1_IRQn);
    }
}

// Interrupción UART (Esqueleto necesario para E_IDLE y E_CONFIG)

void UART0_IRQHandler(void) {
    // Recepción de comandos desde PC.
    // Leer el comando y activar flags como flag_start_game.
}

// Interrupción DMA (Esqueleto necesario para E_COUNTDOWN)

void DMA_IRQHandler(void) {
    // Control de finalización de reproducción de audio.
    // Limpiar flag de interrupción del DMA y avisar a la FSM.
    flag_dma_audio_done = 1;
}
