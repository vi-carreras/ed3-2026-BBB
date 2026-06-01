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

volatile uint32_t msTicks = 0;
uint32_t sonido_countdown[100];

volatile uint8_t flag_start_game = 0;			//	flag para notificar que se recibe comando de empezar juego
volatile uint8_t flag_dma_audio_done = 0;		//
volatile uint8_t flag_capture_event = 0;		//
volatile uint32_t tiempo_reaccion_jugador = 0;	//	tiempo de reacción que se envía en caso de ganador de ronda
volatile uint8_t jugador_ganador = 0;

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
void configUART1(void);		//inicializa comunicación UART por canal 1
void configI2C0(void);		//inicializa comunicación I2C
void configDAC(void);		//inicializa DAC
void configDMA(void);		//inicializa DMA
void configTIMER0(void);	//inicializa Timer0 para
void configTIMER1(void);	//inicializa Timer1 para capture
void actualizarestado(); //función actualizar máquina de estados


int main(void) {
	configGPIO();
	configUART1();
	configI2C0();
	configDAC();

	while(1){
		actualizarestado();
	}

    return 0 ;
}

//---FUNCIONES-------------------------------------------------------------------------
void configGPIO(void)   //revisar que inicialice correctamente todos los pines a usar
{
	PINSEL_CFG_T pinCfg = {PORT_0, 0, PINSEL_FUNC_00, PINSEL_PULLUP, DISABLE};

    //Puerto 0[7:0] para lectura teclado J1
	PINSEL_ConfigMultiplePins(&pinCfg, 0x0F); //-> [3:0]
	pinCfg.mode = PINSEL_TRISTATE;
	PINSEL_ConfigMultiplePins(&pinCfg, 0xF0); //-> [7:4]
	GPIO_SetDir(PORT_0, 0x0F, GPIO_OUTPUT);	//[3:0] -> salidas
	GPIO_SetDir(PORT_0, 0xF0, GPIO_INPUT); // [7:4] ->entradas
	GPIO_ClearPins(PORT_0, 0xFF);
	GPIO_SetMask(PORT_0, 0XFFFFFF00, ENABLE);//Se enmascara los pines que no van a usar
	
    //Puerto 2[7:0] para lectura teclado J2
	pinCfg.port = PORT_2;
	pinCfg.mode = PINSEL_PULLUP;
	PINSEL_ConfigMultiplePins(&pinCfg, 0x0F); //-> [3:0]
	pinCfg.mode = PINSEL_TRISTATE;
	PINSEL_ConfigMultiplePins(&pinCfg, 0xF0); //-> [7:4]
	GPIO_SetDir(PORT_2, 0x0F, GPIO_OUTPUT);	//[3:0] -> salidas
	GPIO_SetDir(PORT_2, 0xF0, GPIO_INPUT); // [7:4] ->entradas
    GPIO_ClearPins(PORT_2, 0xFF);
    GPIO_SetMask(PORT_2, 0XFFFFFF00, ENABLE);//Se enmascara los pines que no van a usar
}

void configUART1(void)
{
    PINSEL_CFG_T pinCfg = {PORT_0, PIN_15, PINSEL_FUNC_01, PINSEL_PULLUP, DISABLE};
    PINSEL_ConfigPin(&pinCfg);	//config TXD1
    pinCfg.Pinnum = PIN_16;
    PINSEL_ConfigPin(&pinCfg);	//config RXD1

    UART_CFG_T uartCfg = {115200 , UART_PARITY_NONE , UART_DBITS_8 , UART_STOPBIT_1};
    UART_Init(UART1, &uartCfg);	//Config inicial UART1 p transmitir a 115200 baudios en cfg 8N1

    UART_FIFO_CFG_T fifoCfg = {ENABLE , ENABLE , DISABLE , UART_FIFO_TRGLEV0};
    UART_FIFOConfig(UART1, &fifoCfg);	//cfg inicial de FIFO
    
    UART_TxEnable(UART1);
}

void configI2C0(void)
{
	PINSEL_CFG_T pinCfg = {PORT_0, PIN_27, PINSEL_FUNC_01, PINSEL_PULLUP, ENABLE};
    PINSEL_ConfigPin(&pinCfg);	//cfg de SDA0

    pinCfg.pin = PIN_28;
    PINSEL_ConfigPin(&pinCfg);	//cfg de SCL0

    I2C_Init(LPC_I2C0, 100000);	//inicializa en 100KHz
    
    I2C_Cmd(LPC_I2C0, ENABLE);	//habilita interfaz
}

void configDAC(void)
{
    PINSEL_CFG_T pinCfg= {PORT_0, PIN_26, PINSEL_FUNC_10, PINSEL_TRISTATE, DISABLE};
    PINSEL_ConfigPin(&pinCfg);	//conf de DAC output P0[26]
    
    DAC_Init();
    DAC_SetBias(DAC_700uA);
    
    DAC_UpdateValue(0);
}

GPDMA_Channel_CFG_T dmaCfg={0}; //estructura global para poder recargar en cada "bip" del juego
void configDMA(void)
{
	GPDMA_Init();
	
	dmaCfg.channelNum = GPDMA_CH_0;            // canal 0
	dmaCfg.transferSize = 100;        		   // cantidad de datos: 100
	dmaCfg.type = GPDMA_M2P;                   // mem a periférico
	dmaCfg.srcMemAddr = (uint32_t)sonido_countdown;   // dir del arreglo en RAM
	dmaCfg.dstMemAddr = (uint32_t)&(LPC_DAC->DACR);            
	dmaCfg.dstConn = GPDMA_MAT0_0;                             
	// configuracion origen
	dmaCfg.src.width = GPDMA_WORD;        // 32 bits
	dmaCfg.src.burst = GPDMA_BSIZE_1;     // 1 elemento
	dmaCfg.src.increment = ENABLE;        // habilitacion p/recorrer el arreglo
	// configuracion destino
	dmaCfg.dst.width = GPDMA_WORD;        // 32 bits
	dmaCfg.dst.burst = GPDMA_BSIZE_1;     
	dmaCfg.dst.increment = DISABLE;       // destino fijo
	dmaCfg.intTC = ENABLE;                                     
	dmaCfg.intErr = ENABLE;                                    
	dmaCfg.linkedList = 0;                // 0 para que se detenga al terminar las 100 muestras
	GPDMA_SetupChannel(&dmaCfg);
	
}

void configTIMER0(void)
{
    TIM_TIMERCFG_T timerCfg;
    timerCfg.prescaleOpt = TIM_PRESCALE_USVAL;
    timerCfg.prescaleValue = 1000;

    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timerCfg);

    TIM_MATCHCFG_T matchCfg;
    matchCfg.channel = 0;
    matchCfg.intEn  = DISABLE;
    matchCfg.resetEn = ENABLE;
    matchCfg.stopEn  = DISABLE;
    matchCfg.extOpt = TIM_NOTHING;
    matchCfg.matchValue   = 1;

    TIM_ConfigMatch(LPC_TIM0, &matchCfg);

    NVIC_EnableIRQ(TIMER0_IRQn);

    TIM_Cmd(LPC_TIM0, ENABLE);
}


void configTIMER1(void){
	TIM_ResetCounter(LPC_TIM1);
	
	TIM_TIMERCFG_Type timerCfg;
	timerCfg.PrescaleOption = TIM_PRESCALE_USVAL;
	timerCfg.PrescaleValue = 1;		//1us
	TIM_InitTimer(LPC_TIM1, &timerCfg);
	
	TIM_CAPTURECFG_Type capCfg;
	capCfg.CaptureChannel = 0;                 // J1 -> Canal 0 (CAP1.0)
	capCfg.RisingEdge     = ENABLE;		 	 // Flanco de subida 
	capCfg.FallingEdge    = DISABLE;          
	capCfg.IntOnCaption   = ENABLE;          // Interrumpir al capturar para guardar el dato
	TIM_ConfigCapture(LPC_TIM1, &capCfg);
	TIM_PinConfig(TIM_CAP1_0_P1_18);
	
	capCfg.CaptureChannel = 1;                 // J2 -> Canal 1 (CAP1.1)
	TIM_ConfigCapture(LPC_TIM1, &capCfg);
	TIM_PinConfig(TIM_CAP1_1_P1_19);
	
	NVIC_EnableIRQ(TIMER1_IRQn);	//habilitacion de interrupciones por timer1
}

void actualizarestado(){
	switch(estado_actual){
		case E_IDLE:
			/* * El microcontrolador entra en estado de espera.
			 * Acciones:
			 */
			
			//Desactivar periféricos que no se usan (timers, DMA).
			TIM_Disable(LPC_TIM1);	//desactiva timer1
			TIM_ResetCounter(LPC_TIM1);
			TIM_Disable(LPC_TIM0);	//desactiva timer0
			TIM_ResetCounter(LPC_TIM0);
			GPDMA_ChannelStop(GPDMA_CH_0);	//desactiva DMA
			
			//Mostrar mensaje de espera en LCD 16x2
			mensajeLCD();	//FALTA IMPLEMENTAR FUNCION EN LIBRERIA
			
			//Si se recibe el comando START_GAME por UART, cambiar a E_CONFIG.
			if (flag_start_game) {
				flag_start_game = 0;
				estado_actual = E_CONFIG;
			}
			break;

		case E_CONFIG:
			/* * La aplicación Java configura la partida mediante UART.
			 * Acciones:
			 */
			
			//Reiniciar puntajes y estadísticas al comenzar una nueva partida completa.
			victorias_j1 = 0;
			victorias_j2 = 0;
			tiempo_reaccion_jugador = 0;
			jugador_ganador = 0;
			tecla_objetivo = 0;
			resultado_ronda = 0;

			// FALTA Procesar la configuración recibida (configurar teclas, seleccionar tono)
			
			//Transición automática a E_COUNTDOWN una vez procesada la configuración.
			estado_actual = E_COUNTDOWN;
			break;

		case E_COUNTDOWN:
			/* * Se inicia la cuenta regresiva auditiva mediante DAC.
			 * Acciones:
			 * - Transición: Automática a E_WAIT_GO. La finalización de los tonos se
			 * manejará mediante interrupciones (DMA IRQ / Timer IRQ).
			 */

			//Habilitar Timer0 y DMA para transferir samples al DAC.
			
			//Los sonidos se generan sincronizados por un Timer Match.
			
			//Transición automática a E_WAIT_GO. 
			//La finalización de los tonos se manejará mediante interrupciones (DMA IRQ / Timer IRQ).
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

			// Audio_Play(buffer_game_over, size); // Sonido épico de fin de juego

			// Al terminar de mostrar el ganador, volvemos a IDLE para esperar un nuevo START_GAME
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

/*
 * Capture IRQ: Almacenamiento automático de timestamps de reacción.
 * Asumiendo que usas el TIMER1 para el Input Capture de los jugadores.
 */
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

// Interrupción UART (Esqueleto necesario para tu E_IDLE y E_CONFIG)

void UART0_IRQHandler(void) {
    // Recepción de comandos desde PC[cite: 98].
    // Leer el comando y activar flags como flag_start_game.
}

// Interrupción DMA (Esqueleto necesario para tu E_COUNTDOWN)

void DMA_IRQHandler(void) {
    // Control de finalización de reproducción de audio[cite: 96].
    // Limpiar flag de interrupción del DMA y avisar a la FSM.
    flag_dma_audio_done = 1;
}
