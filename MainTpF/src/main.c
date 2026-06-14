/**
 * @file    main_paso7_audio_polling.c
 * @brief   PASO 7: Audio por DAC en modo polling (sin DMA todavía).
 *
 * Qué cambia respecto al Paso 6:
 *  - Se agrega DAC_Init() (configura P0.26 = AOUT automáticamente).
 *  - Antes de habilitar el capture, se reproduce una cuenta regresiva
 *    auditiva: "3... 2... 1... GO!" (4 tonos, los tres primeros
 *    iguales y graves, el último más agudo y largo).
 *  - En el momento exacto que se detecta el capture (cualquier
 *    jugador), se reproduce un beep corto de "reacción registrada".
 *  - Si algún jugador llega a MAX_VICTORIAS, se reproduce una
 *    melodía ascendente de victoria (3 notas).
 *
 * Generación de audio:
 *  - Audio_GenerateTone(buffer, samples, freq) llena el buffer con
 *    una onda senoidal en formato 0-1023 (10 bits) listo para
 *    DAC_UpdateValue() (sin shift <<6, a diferencia del audio.c
 *    original que sí lo necesitaba para DMA).
 *  - Audio_PlayBuffer_Polling(buffer, samples) recorre el buffer y
 *    escribe cada muestra al DAC con delay_us(1000000/SAMPLE_RATE_HZ)
 *    entre muestras. Es bloqueante (por eso "polling"); la migración
 *    a DMA del Paso 8 lo hará no bloqueante.
 *  - Audio_PlaySilence(dur_ms) NO usa el buffer: pone el DAC en 0
 *    (silencio real, el transistor BC548B corta) y espera con
 *    delay_ms. Mucho más liviano para los silencios largos del
 *    countdown.
 *
 * SAMPLE_RATE_HZ = 8000 Hz. AUDIO_BUF_SIZE = 2400 cubre el tono más
 * largo usado (300 ms para "GO!" y la nota final de victoria).
 *
 * Hardware de audio (confirmado funcionando):
 *  P0.26 (DAC AOUT) -> [1kΩ] -> Base BC548B
 *  Buzzer(+) -> VCC 3.3V
 *  Buzzer(-) -> Colector BC548B
 *  Emisor BC548B -> GND
 *
 * Patrón de countdown (1s por número, 500ms para GO!):
 *  "3..." -> beep 100ms -> silencio 900ms
 *  "2..." -> beep 100ms -> silencio 900ms
 *  "1..." -> beep 100ms -> silencio 900ms
 *  "GO!"  -> beep 100ms (más agudo) -> silencio 400ms
 *
 * Pines:
 *  P0.26 = AOUT (DAC) -> hacia tu buffer/amplificador, igual que el PDF.
 *  Resto de pines: igual que Paso 6 (UART2, LCD I2C, capture TIM3,
 *  teclados J1/J2).
 */

#include "LPC17xx.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_dac.h"
#include <math.h>

/* ─── LCD I2C (PCF8574) ───────────────────────────────────────────────────── */
#define LCD_ADDR  0x27
#define LCD_BL    (1<<3)
#define LCD_EN    (1<<2)
#define LCD_RS    (1<<0)

/* ─── Audio ───────────────────────────────────────────────────────────────── */
#define SAMPLE_RATE_HZ   8000
#define AUDIO_BUF_SIZE   2400   // suficiente para 300 ms a 8 kHz (el tono mas largo)
static uint32_t audio_buf[AUDIO_BUF_SIZE];

/* ─── Tecla objetivo (modificable por comando UART "K<c>") ───────────────── */
volatile char tecla_objetivo = '5';

/* ─── Flag: comando "S" recibido por UART (inicia ronda) ─────────────────── */
volatile uint8_t flag_start_game = 0;

/* ─── Puntaje y condición de victoria ────────────────────────────────────── */
volatile uint8_t victorias_j1 = 0;
volatile uint8_t victorias_j2 = 0;
#define MAX_VICTORIAS 3

/* ─── Timeout de espera (ms) ──────────────────────────────────────────────── */
#define TIMEOUT_MS  10000

/* ─── Mapa de teclas ──────────────────────────────────────────────────────── */
static const char keymap[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

#define ROW_MASK (0x0F)
#define COL_MASK (0xF0)

/* ─── Variables de la IRQ de capture ─────────────────────────────────────── */
volatile uint8_t  flag_capture = 0;
volatile uint8_t  quien_gano   = 0;
volatile uint32_t tiempo_us    = 0;

/* ─── SysTick ─────────────────────────────────────────────────────────────── */
volatile uint32_t msTicks = 0;

void SysTick_Handler(void) {
    msTicks++;
}

/* ─── Delays busy-wait (se mantienen para LCD/I2C, no para el timeout) ──── */
static void delay_ms(uint32_t ms) {
    volatile uint32_t i;
    for (i = 0; i < ms * 8000; i++);
}

static void delay_us(uint32_t us) {
    volatile uint32_t i;
    for (i = 0; i < us * 8; i++);
}

/* ─── I2C / LCD ──────────────────────────────────────────────────────────── */
static void I2C_Write(uint8_t data) {
    I2C_M_SETUP_Type cfg;
    cfg.sl_addr7bit          = LCD_ADDR;
    cfg.tx_data               = &data;
    cfg.tx_length             = 1;
    cfg.tx_count              = 0;
    cfg.rx_data               = NULL;
    cfg.rx_length             = 0;
    cfg.rx_count              = 0;
    cfg.retransmissions_max   = 3;
    cfg.retransmissions_count = 0;
    cfg.status                = 0;
    cfg.callback              = NULL;
    I2C_MasterTransferData(LPC_I2C0, &cfg, I2C_TRANSFER_POLLING);
}

static void LCD_Pulse(uint8_t data) {
    I2C_Write(data | LCD_EN);
    delay_us(2);
    I2C_Write(data & ~LCD_EN);
    delay_us(100);
}

static void LCD_Send(uint8_t byte, uint8_t rs) {
    LCD_Pulse((byte & 0xF0)        | LCD_BL | rs);
    LCD_Pulse(((byte << 4) & 0xF0) | LCD_BL | rs);
}

static void LCD_Cmd(uint8_t cmd)  { LCD_Send(cmd, 0);          }
static void LCD_Char(char c)      { LCD_Send((uint8_t)c, LCD_RS); }

static void LCD_Init(void) {
    delay_ms(100);
    LCD_Pulse(0x30 | LCD_BL); delay_ms(5);
    LCD_Pulse(0x30 | LCD_BL); delay_ms(1);
    LCD_Pulse(0x30 | LCD_BL); delay_ms(1);
    LCD_Pulse(0x20 | LCD_BL); delay_ms(1);
    LCD_Cmd(0x28); delay_ms(1);
    LCD_Cmd(0x08); delay_ms(1);
    LCD_Cmd(0x01); delay_ms(2);
    LCD_Cmd(0x06); delay_ms(1);
    LCD_Cmd(0x0C); delay_ms(1);
}

static void LCD_Linea(uint8_t fila, const char *s) {
    LCD_Cmd(fila == 0 ? 0x80 : 0xC0);
    uint8_t i = 0;
    while (*s && i < 16) { LCD_Char(*s++); i++; }
    while (i++ < 16)     { LCD_Char(' '); }
}

/* ─── Utilidad: uint32 → string decimal sin stdlib ───────────────────────── */
static void uint_to_str(uint32_t val, char *buf) {
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char tmp[12];
    uint8_t len = 0;
    while (val > 0) { tmp[len++] = '0' + (uint8_t)(val % 10); val /= 10; }
    uint8_t i;
    for (i = 0; i < len; i++) buf[i] = tmp[len - 1 - i];
    buf[len] = '\0';
}

/* ─── Inicialización I2C0 ─────────────────────────────────────────────────── */
static void I2C0_Init(void) {
    PINSEL_CFG_T p = {PORT_0, PIN_27, PINSEL_FUNC_01, PINSEL_TRISTATE, ENABLE};
    PINSEL_ConfigPin(&p);
    p.pin = PIN_28;
    PINSEL_ConfigPin(&p);
    PINSEL_SetI2CPins(PINSEL_I2C_FAST, DISABLE);
    I2C_Init(LPC_I2C0, 100000);
    I2C_Cmd(LPC_I2C0, ENABLE);
}

/* ─── Inicialización UART2 (115200 8N1, TX + RX con IRQ) ──────────────────── */
static void configUART2(void) {
    LPC_SC->PCONP |= (1 << 24); // PCUART2 (por si el driver no lo hace)

    PINSEL_CFG_T pinCfg = {PORT_0, PIN_10, PINSEL_FUNC_01, PINSEL_PULLUP, DISABLE};
    PINSEL_ConfigPin(&pinCfg);   // TXD2 (P0.10)

    pinCfg.pin = PIN_11;
    PINSEL_ConfigPin(&pinCfg);   // RXD2 (P0.11)

    UART_CFG_T uartCfg = {115200, UART_PARITY_NONE, UART_DBITS_8, UART_STOPBIT_1};
    UART_Init(UART2, &uartCfg);

    UART_FIFO_CFG_T fifoCfg = {ENABLE, ENABLE, DISABLE, UART_FIFO_TRGLEV0};
    UART_FIFOConfig(UART2, &fifoCfg);

    UART_TxEnable(UART2);

    UART_IntConfig(UART2, UART_INT_RBR, ENABLE);
    NVIC_EnableIRQ(UART2_IRQn);
}

/* ─── Envío UART en modo polling ─────────────────────────────────────────── */
static void enviar_char(uint8_t c) {
    while (!(UART_GetLineStatus(UART2) & UART_LSR_THRE));
    UART_SendByte(UART2, c);
}

static void enviar_str(const char *s) {
    while (*s) enviar_char((uint8_t)*s++);
}

/* ─── IRQ Handler UART2: recepción de comandos ────────────────────────────── */
void UART2_IRQHandler(void) {
    static char line_buf[16];
    static uint8_t line_idx = 0;

    uint8_t rx = UART_ReceiveByte(UART2);

    if (rx == '\r' || rx == '\n') {
        if (line_idx > 0) {
            line_buf[line_idx] = '\0';
            char cmd = line_buf[0];
            if (cmd >= 'a' && cmd <= 'z') cmd -= 32;

            switch (cmd) {
                case 'S':
                    flag_start_game = 1;
                    break;

                case 'K':
                    if (line_idx >= 2) {
                        tecla_objetivo = line_buf[1];
                    }
                    break;

                case 'R':
                    victorias_j1 = 0;
                    victorias_j2 = 0;
                    enviar_str("P0,0\r\n");
                    break;

                default:
                    break;
            }
        }
        line_idx = 0;
    } else if (line_idx < sizeof(line_buf) - 1) {
        line_buf[line_idx++] = (char)rx;
    }
}

/* ─── AUDIO: generación de buffers ───────────────────────────────────────── */

/* Llena buffer con una onda senoidal de 'frecuencia' Hz, 'samples' muestras,
 * en formato 0-1023 (10 bits) listo para DAC_UpdateValue(). */
static void Audio_GenerateTone(uint32_t *buffer, uint32_t samples, uint32_t frecuencia) {
    for (uint32_t i = 0; i < samples; i++) {
        float angulo  = 2.0f * 3.14159f * frecuencia * i / SAMPLE_RATE_HZ;
        int32_t valor = (int32_t)(512 + 512 * sinf(angulo));

        if (valor < 0)    valor = 0;
        if (valor > 1023) valor = 1023;

        buffer[i] = (uint32_t)valor;
    }
}

/* Reproduce un buffer por polling: escribe cada muestra al DAC y espera
 * el tiempo correspondiente a SAMPLE_RATE_HZ. Bloqueante. */
static void Audio_PlayBuffer_Polling(uint32_t *buffer, uint32_t samples) {
    const uint32_t periodo_us = 1000000UL / SAMPLE_RATE_HZ; // ~125 us @ 8kHz
    for (uint32_t i = 0; i < samples; i++) {
        DAC_UpdateValue(buffer[i]);
        delay_us(periodo_us);
    }
}

/* Reproduce un tono de 'frecuencia' Hz durante 'dur_ms' milisegundos */
static void Audio_PlayTone(uint32_t frecuencia, uint32_t dur_ms) {
    uint32_t samples = (SAMPLE_RATE_HZ * dur_ms) / 1000;
    if (samples > AUDIO_BUF_SIZE) samples = AUDIO_BUF_SIZE;

    Audio_GenerateTone(audio_buf, samples, frecuencia);
    Audio_PlayBuffer_Polling(audio_buf, samples);
    DAC_UpdateValue(0); // silencio real al terminar el tono
}

/* Silencio real: DAC a 0 (transistor corta) durante 'dur_ms' milisegundos.
 * No usa el buffer; mucho más eficiente para silencios largos. */
static void Audio_PlaySilence(uint32_t dur_ms) {
    DAC_UpdateValue(0);
    delay_ms(dur_ms);
}

/* Cuenta regresiva auditiva: 3... 2... 1... GO!
 * Tres tonos cortos graves + un tono final más agudo y un poco más largo.
 * Actualiza el LCD en cada paso. */
static void Audio_Countdown(void) {
    LCD_Linea(0, "3...");
    LCD_Linea(1, "");
    Audio_PlayTone(440, 100);    // beep corto: 100 ms
    Audio_PlaySilence(900);      // resto del segundo

    LCD_Linea(0, "2...");
    Audio_PlayTone(440, 100);
    Audio_PlaySilence(900);

    LCD_Linea(0, "1...");
    Audio_PlayTone(440, 100);
    Audio_PlaySilence(900);

    LCD_Linea(0, "GO!");
    Audio_PlayTone(880, 100);    // beep más agudo
    Audio_PlaySilence(400);      // completa los 500 ms
}

/* Beep corto de "reacción registrada" (cuando se detecta el capture) */
static void Audio_BeepReaccion(void) {
    Audio_PlayTone(1000, 80);
}

/* Melodía ascendente de victoria: 3 notas */
static void Audio_Victoria(void) {
    Audio_PlayTone(523, 150);  // Do5
    Audio_PlaySilence(30);
    Audio_PlayTone(659, 150);  // Mi5
    Audio_PlaySilence(30);
    Audio_PlayTone(784, 300);  // Sol5
}

/* ─── Inicialización TIM3 Capture ────────────────────────────────────────── */
static void TIM3_Capture_Init(void) {
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

/* ─── Arrancar una nueva medición ─────────────────────────────────────────── */
static void iniciar_medicion(void) {
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

/* ─── IRQ Handler TIM3 ───────────────────────────────────────────────────── */
void TIMER3_IRQHandler(void) {
    uint8_t  j1 = 0, j2 = 0;
    uint32_t t1 = 0, t2 = 0;

    if (TIM_GetIntStatus(LPC_TIM3, TIM_CR0_INT)) {
        TIM_ClearIntPending(LPC_TIM3, TIM_CR0_INT);
        t1 = TIM_GetCaptureValue(LPC_TIM3, TIM_CAPTURE_0);
        j1 = 1;
    }
    if (TIM_GetIntStatus(LPC_TIM3, TIM_CR1_INT)) {
        TIM_ClearIntPending(LPC_TIM3, TIM_CR1_INT);
        t2 = TIM_GetCaptureValue(LPC_TIM3, TIM_CAPTURE_1);
        j2 = 1;
    }

    if (j1 && j2) {
        if (t1 <= t2) { quien_gano = 1; tiempo_us = t1; }
        else          { quien_gano = 2; tiempo_us = t2; }
    } else if (j1) {
        quien_gano = 1; tiempo_us = t1;
    } else if (j2) {
        quien_gano = 2; tiempo_us = t2;
    }

    if (quien_gano) {
        TIM_Disable(LPC_TIM3);
        NVIC_DisableIRQ(TIMER3_IRQn);
        flag_capture = 1;
    }
}

/* ─── Identificar tecla presionada escaneando fila por fila ──────────────── */
static char identificar_tecla(LPC_GPIO_TypeDef *gpio) {
    uint8_t row, col;
    uint32_t portVal;

    for (row = 0; row < 4; row++) {
        gpio->FIOSET = ROW_MASK;
        gpio->FIOCLR = (1 << row);

        delay_us(50);

        portVal = gpio->FIOPIN;

        for (col = 0; col < 4; col++) {
            if (!(portVal & (1 << (col + 4)))) {
                gpio->FIOCLR = ROW_MASK;
                return keymap[row][col];
            }
        }
    }

    gpio->FIOCLR = ROW_MASK;
    return '?';
}

/* ─── MAIN ───────────────────────────────────────────────────────────────── */
int main(void) {
    SysTick_Config(SystemCoreClock / 1000);

    I2C0_Init();
    LCD_Init();
    configUART2();
    DAC_Init();
    DAC_UpdateValue(0); // silencio real desde el arranque
    TIM3_Capture_Init();

    LCD_Linea(0, "PASO 7: Audio");
    LCD_Linea(1, "DAC polling");
    delay_ms(2000);

    while (1) {
        /* ── Fase: esperar comando "S" desde la PC ────────────────────────── */
        LCD_Linea(0, "Esperando");
        LCD_Linea(1, "inicio (S)...");

        flag_start_game = 0;
        while (!flag_start_game) {
            // loop vacío; UART2_IRQHandler setea flag_start_game al recibir "S"
        }

        /* ── Cuenta regresiva auditiva 3-2-1-GO ───────────────────────────── */
        Audio_Countdown();

        /* ── Fase espera: mostrar tecla objetivo ─────────────────────────── */
        char linea_obj[17] = "Presiona: ?    ";
        linea_obj[10] = tecla_objetivo;
        LCD_Linea(0, linea_obj);
        LCD_Linea(1, "Esperando...");

        // Filas de ambos teclados a GND
        LPC_GPIO0->FIODIR |= 0x0F;
        LPC_GPIO0->FIOCLR  = 0x0F;

        LPC_GPIO2->FIODIR |= 0x0F;
        LPC_GPIO2->FIOCLR  = 0x0F;

        iniciar_medicion();

        /* Timeout no bloqueante basado en msTicks */
        uint32_t inicio_espera = msTicks;
        while (!flag_capture && (msTicks - inicio_espera) < TIMEOUT_MS) {
            // loop vacío
        }

        TIM_Disable(LPC_TIM3);
        NVIC_DisableIRQ(TIMER3_IRQn);

        /* ── Beep inmediato si hubo capture (antes de procesar resultado) ─── */
        if (flag_capture) {
            Audio_BeepReaccion();
        }

        /* ── Fase resultado ──────────────────────────────────────────────── */
        if (!flag_capture) {
            LCD_Linea(0, "TIMEOUT");
            LCD_Linea(1, "Sin entrada");
            enviar_str("T\r\n");
            // Sin cambios de puntaje en timeout
        } else {
            char tecla;

            if (quien_gano == 1) {
                tecla = identificar_tecla(LPC_GPIO0);
            } else {
                tecla = identificar_tecla(LPC_GPIO2);
            }

            char linea1[17];
            char linea2[17];
            char numstr[12];

            if (tecla == tecla_objetivo) {
                const char *base = "CORRECTO J?    ";
                for (uint8_t k = 0; k < 16; k++) linea1[k] = base[k];
                linea1[16] = '\0';
                linea1[10] = (quien_gano == 1) ? '1' : '2';

                linea2[0] = 'T';
                uint_to_str(tiempo_us, numstr);
                uint8_t ii = 1, jj = 0;
                while (numstr[jj] && ii < 11) linea2[ii++] = numstr[jj++];
                linea2[ii++] = ' '; linea2[ii++] = 'u'; linea2[ii++] = 's';
                while (ii < 16) linea2[ii++] = ' ';
                linea2[16] = '\0';

                enviar_str(quien_gano == 1 ? "W1\r\n" : "W2\r\n");
                enviar_char('R');
                enviar_str(numstr);
                enviar_str("\r\n");

                if (quien_gano == 1) victorias_j1++;
                else                 victorias_j2++;
            } else {
                const char *base = "INCORRECTO J?  ";
                for (uint8_t k = 0; k < 16; k++) linea1[k] = base[k];
                linea1[16] = '\0';
                linea1[12] = (quien_gano == 1) ? '1' : '2';

                const char *base2 = "Tecla: ?       ";
                for (uint8_t k = 0; k < 16; k++) linea2[k] = base2[k];
                linea2[16] = '\0';
                linea2[7] = tecla;

                enviar_str(quien_gano == 1 ? "E1\r\n" : "E2\r\n");

                if (quien_gano == 1) {
                    if (victorias_j1 > 0) victorias_j1--;
                } else {
                    if (victorias_j2 > 0) victorias_j2--;
                }
            }

            LCD_Linea(0, linea1);
            LCD_Linea(1, linea2);
        }

        delay_ms(2000);

        /* ── Enviar marcador actualizado ───────────────────────────────── */
        {
            char numj1[4], numj2[4];
            uint_to_str(victorias_j1, numj1);
            uint_to_str(victorias_j2, numj2);
            enviar_char('P');
            enviar_str(numj1);
            enviar_char(',');
            enviar_str(numj2);
            enviar_str("\r\n");
        }

        /* ── Verificar condición de victoria ──────────────────────────────── */
        if (victorias_j1 >= MAX_VICTORIAS || victorias_j2 >= MAX_VICTORIAS) {
            char linea_win[17] = "GANADOR: J?     ";
            linea_win[10] = (victorias_j1 >= MAX_VICTORIAS) ? '1' : '2';
            LCD_Linea(0, linea_win);
            LCD_Linea(1, "Reiniciando...");

            enviar_str((victorias_j1 >= MAX_VICTORIAS) ? "G1\r\n" : "G2\r\n");

            /* Sonido de victoria */
            Audio_Victoria();

            victorias_j1 = 0;
            victorias_j2 = 0;
            enviar_str("P0,0\r\n");

            delay_ms(1500);
        } else {
            char linea_score[17] = "J1: ?   J2: ?   ";
            linea_score[4]  = '0' + victorias_j1;
            linea_score[12] = '0' + victorias_j2;
            LCD_Linea(0, "Marcador:");
            LCD_Linea(1, linea_score);
            delay_ms(1500);
        }
    }
}
