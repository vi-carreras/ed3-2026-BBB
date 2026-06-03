#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

void Audio_Play(uint32_t* buffer, uint32_t size);

void Audio_Stop(void);

void Audio_GenerateTone(uint32_t* buffer, uint32_t samples, uint32_t frecuencia);

#endif
