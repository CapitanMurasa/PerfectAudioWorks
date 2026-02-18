#ifndef AUDIORINGBUFFER_H
#define AUDIORINGBUFFER_H

#include <stddef.h>
#include <pthread.h>

typedef struct {
    float* buffer;
    size_t readPos;
    size_t writePos;
    size_t available;
    size_t capacity;
    pthread_mutex_t mutex;
} AudioRing;

void AudioRing_Init(AudioRing* ring, size_t size);
void AudioRing_Free(AudioRing* ring);
void AudioRing_Write(AudioRing* ring, const float* data, size_t samples);
size_t AudioRing_Read(AudioRing* ring, float* output, size_t samplesNeeded);
void AudioRing_Clear(AudioRing* ring);

#endif