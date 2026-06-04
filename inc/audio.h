#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

#define SAMPLE_RATE_HZ 10000

void Audio_Play(uint32_t* buffer, uint32_t size);

void Audio_Stop(void);

void Audio_GenerateTone(uint32_t* buffer, uint32_t samples, uint32_t frecuencia);

void Audio_GenerateSilence(uint32_t* buffer, uint32_t samples);

#endif
