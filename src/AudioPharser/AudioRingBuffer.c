#include "AudioRingBuffer.h"
#include <stdlib.h>
#include <string.h> 

void AudioRing_Init(AudioRing* ring, size_t size) {
    ring->buffer = (float*)malloc(size * sizeof(float));
    ring->capacity = size;
    ring->readPos = 0;
    ring->writePos = 0;
    ring->available = 0;

    pthread_mutex_init(&ring->mutex, NULL);
}

void AudioRing_Free(AudioRing* ring) {
    pthread_mutex_destroy(&ring->mutex);
    if (ring->buffer) {
        free(ring->buffer);
        ring->buffer = NULL;
    }
}

void AudioRing_Write(AudioRing* ring, const float* data, size_t samples) {
    pthread_mutex_lock(&ring->mutex);

    if (ring->available + samples > ring->capacity) {
        pthread_mutex_unlock(&ring->mutex);
        return;
    }

    for (size_t i = 0; i < samples; ++i) {
        ring->buffer[ring->writePos] = data[i];
        ring->writePos = (ring->writePos + 1) % ring->capacity;
    }
    ring->available += samples;

    pthread_mutex_unlock(&ring->mutex);
}

size_t AudioRing_Read(AudioRing* ring, float* output, size_t samplesNeeded) {
    pthread_mutex_lock(&ring->mutex);

    if (ring->available < samplesNeeded) {
        size_t valid = ring->available;
        for (size_t i = 0; i < valid; ++i) {
            output[i] = ring->buffer[ring->readPos];
            ring->readPos = (ring->readPos + 1) % ring->capacity;
        }

        memset(output + valid, 0, (samplesNeeded - valid) * sizeof(float));

        ring->available = 0;
        pthread_mutex_unlock(&ring->mutex);
        return valid;
    }

    for (size_t i = 0; i < samplesNeeded; ++i) {
        output[i] = ring->buffer[ring->readPos];
        ring->readPos = (ring->readPos + 1) % ring->capacity;
    }
    ring->available -= samplesNeeded;

    pthread_mutex_unlock(&ring->mutex);
    return samplesNeeded;
}

void AudioRing_Clear(AudioRing* ring) {
    pthread_mutex_lock(&ring->mutex);
    ring->readPos = 0;
    ring->writePos = 0;
    ring->available = 0;
    pthread_mutex_unlock(&ring->mutex);
}