/**
 * @file        audio.h
 * @brief       Generación de audio por DAC en modo polling para LPC1769.
 * @version     1.0
 * @date        14. July. 2026
 * @author      Bombón, Burbuja y Bellota
 */

#ifndef AUDIO_H_
#define AUDIO_H_

#include <stdint.h>

#define SAMPLE_RATE_HZ 8000
#define AUDIO_BUF_SIZE 2400

/**
 * @brief  Reproduce un tono de la frecuencia y duración indicadas.
 * @param  frecuencia  Frecuencia en Hz.
 * @param  dur_ms      Duración en milisegundos.
 */
void Audio_PlayTone(uint32_t frecuencia, uint32_t dur_ms);

/**
 * @brief  Silencio real: DAC a 0 durante la duración indicada.
 * @param  dur_ms  Duración en milisegundos.
 */
void Audio_PlaySilence(uint32_t dur_ms);

/**
 * @brief  Cuenta regresiva auditiva 3-2-1-GO con actualización de LCD.
 */
void Audio_Countdown(void);

/**
 * @brief  Beep corto de "reacción registrada".
 */
void Audio_BeepReaccion(void);

/**
 * @brief  Melodía ascendente de victoria.
 */
void Audio_Victoria(void);

#endif /* AUDIO_H_ */
