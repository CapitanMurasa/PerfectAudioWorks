#include "AudioRingBuffer.h"
#include <stdlib.h>
#include <string.h> 

void AudioRing_Init(AudioRing* ring, size_t size) {
    ring->buffer = (float*)malloc(size * sizeof(float));
    ring->capacity = size;
    ring->readPos = 0;
    ring->writePos = 0;
}

void AudioRing_Free(AudioRing* ring) {
    if (ring->buffer) {
        free(ring->buffer);
        ring->buffer = NULL;
    }
}

void AudioRing_Write(AudioRing* ring, const float* data, size_t samples) {
    size_t freeSpace = (ring->readPos - ring->writePos + ring->capacity - 1) % ring->capacity;

    if (samples > freeSpace) {
        return; 
    }

    size_t chunk1 = ring->capacity - ring->writePos;

    if (samples <= chunk1) {
        memcpy(&ring->buffer[ring->writePos], data, samples * sizeof(float));
        ring->writePos = (ring->writePos + samples) % ring->capacity;
    }
    else {
        memcpy(&ring->buffer[ring->writePos], data, chunk1 * sizeof(float));

        size_t chunk2 = samples - chunk1; 
        memcpy(&ring->buffer[0], data + chunk1, chunk2 * sizeof(float));

        ring->writePos = chunk2;
    }
}

size_t AudioRing_Read(AudioRing* ring, float* output, size_t samplesNeeded) {
    size_t available = (ring->writePos - ring->readPos + ring->capacity) % ring->capacity;

    if (available < samplesNeeded) {
        size_t readCount = AudioRing_Read(ring, output, available);
        memset(output + readCount, 0, (samplesNeeded - readCount) * sizeof(float));
        return readCount;
    }


    size_t chunk1 = ring->capacity - ring->readPos;

    if (samplesNeeded <= chunk1) {
        memcpy(output, &ring->buffer[ring->readPos], samplesNeeded * sizeof(float));
        ring->readPos = (ring->readPos + samplesNeeded) % ring->capacity;
    }
    else {
        memcpy(output, &ring->buffer[ring->readPos], chunk1 * sizeof(float));

        size_t chunk2 = samplesNeeded - chunk1;
        memcpy(output + chunk1, &ring->buffer[0], chunk2 * sizeof(float));

        ring->readPos = chunk2;
    }

    return samplesNeeded;
}

void AudioRing_Clear(AudioRing* ring) {
    ring->readPos = 0;
    ring->writePos = 0;
}