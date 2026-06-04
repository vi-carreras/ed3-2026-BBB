/**
 * @file        audio.h
 * @brief       Generación y reproducción de tonos por DAC + DMA.
 *             Define la frecuencia de sampleo (10 KHz) y las funciones
 *             para generar tonos, silencios y controlar la reproducción.
 * @version     1.0
 * @date        04. June. 2026
 * @author      Bombón, Burbuja y Bellota
 *
 * @par Refactor:
 * Last update: 04/06/2026, Author: David Trujillo Medina
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

/* ----------------------------- Definiciones ------------------------------- */
/** @brief  Frecuencia de sampleo del DAC (10 KHz). */
#define SAMPLE_RATE_HZ 10000

/* --------------------------- Funciones públicas --------------------------- */
/**
 * @brief  Inicia la reproducción de un buffer de audio por DMA.
 * @param  buffer  Puntero al buffer con las muestras en formato DAC.
 * @param  size    Cantidad de muestras a reproducir.
 */
void Audio_Play(uint32_t* buffer, uint32_t size);

/**
 * @brief  Detiene la reproducción de audio y apaga el timer del DAC.
 */
void Audio_Stop(void);

/**
 * @brief  Genera una onda sinusoidal en el buffer con la frecuencia indicada.
 * @param  buffer      Puntero al buffer de salida.
 * @param  samples     Cantidad de muestras a generar.
 * @param  frecuencia  Frecuencia del tono en Hz.
 */
void Audio_GenerateTone(uint32_t* buffer, uint32_t samples, uint32_t frecuencia);

/**
 * @brief  Llena el buffer con silencio (valor 0).
 * @param  buffer   Puntero al buffer de salida.
 * @param  samples  Cantidad de muestras a escribir con 0.
 */
void Audio_GenerateSilence(uint32_t* buffer, uint32_t samples);

#endif /* AUDIO_H */
