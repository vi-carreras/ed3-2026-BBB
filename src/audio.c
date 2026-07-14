/**
 * @file        audio.c
 * @brief       Generación de audio por DAC en modo polling para LPC1769.
 */

#include "audio.h"
#include "lcd.h"
#include "LPC17xx.h"
#include "lpc17xx_dac.h"
#include <math.h>

static uint32_t audio_buf[AUDIO_BUF_SIZE];

static void Audio_GenerateTone(uint32_t *buffer, uint32_t samples, uint32_t frecuencia) {
    for (uint32_t i = 0; i < samples; i++) {
        float angulo  = 2.0f * 3.14159f * frecuencia * i / SAMPLE_RATE_HZ;
        int32_t valor = (int32_t)(512 + 512 * sinf(angulo));

        if (valor < 0)    valor = 0;
        if (valor > 1023) valor = 1023;

        buffer[i] = (uint32_t)valor;
    }
}

static void Audio_PlayBuffer_Polling(uint32_t *buffer, uint32_t samples) {
    const uint32_t periodo_us = 1000000UL / SAMPLE_RATE_HZ;
    for (uint32_t i = 0; i < samples; i++) {
        DAC_UpdateValue(buffer[i]);
        delay_us(periodo_us);
    }
}

void Audio_PlayTone(uint32_t frecuencia, uint32_t dur_ms) {
    uint32_t samples = (SAMPLE_RATE_HZ * dur_ms) / 1000;
    if (samples > AUDIO_BUF_SIZE) samples = AUDIO_BUF_SIZE;

    Audio_GenerateTone(audio_buf, samples, frecuencia);
    Audio_PlayBuffer_Polling(audio_buf, samples);
    DAC_UpdateValue(0);
}

void Audio_PlaySilence(uint32_t dur_ms) {
    DAC_UpdateValue(0);
    delay_ms(dur_ms);
}

void Audio_Countdown(void) {
    LCD_Linea(0, "3...");
    LCD_Linea(1, "");
    Audio_PlayTone(440, 100);
    Audio_PlaySilence(900);

    LCD_Linea(0, "2...");
    Audio_PlayTone(440, 100);
    Audio_PlaySilence(900);

    LCD_Linea(0, "1...");
    Audio_PlayTone(440, 100);
    Audio_PlaySilence(900);

    LCD_Linea(0, "GO!");
    Audio_PlayTone(880, 100);
    Audio_PlaySilence(400);
}

void Audio_BeepReaccion(void) {
    Audio_PlayTone(1000, 80);
}

void Audio_Victoria(void) {
    Audio_PlayTone(523, 150);
    Audio_PlaySilence(30);
    Audio_PlayTone(659, 150);
    Audio_PlaySilence(30);
    Audio_PlayTone(784, 300);
}
