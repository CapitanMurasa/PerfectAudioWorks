#include "portaudio_backend.h"
#include "CodecHandler.h"
#include "../miscellaneous/misc.h"
#include "AudioRingBuffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
typedef HANDLE ThreadHandle;
#define THREAD_FUNC_RETURN unsigned __stdcall
#define SLEEP_MS(x) Sleep(x)
#else
#include <pthread.h>
#include <unistd.h>
typedef pthread_t ThreadHandle;
#define THREAD_FUNC_RETURN void*
#define SLEEP_MS(x) usleep((x) * 1000)
#endif

#ifdef _WIN32
void pa_mutex_init(pa_mutex_t* lock) { InitializeCriticalSection(lock); }
void pa_mutex_lock(pa_mutex_t* lock) { EnterCriticalSection(lock); }
void pa_mutex_unlock(pa_mutex_t* lock) { LeaveCriticalSection(lock); }
void pa_mutex_destroy(pa_mutex_t* lock) { DeleteCriticalSection(lock); }
#else
void pa_mutex_init(pa_mutex_t* lock) { pthread_mutex_init(lock, NULL); }
void pa_mutex_lock(pa_mutex_t* lock) { pthread_mutex_lock(lock); }
void pa_mutex_unlock(pa_mutex_t* lock) { pthread_mutex_unlock(lock); }
void pa_mutex_destroy(pa_mutex_t* lock) { pthread_mutex_destroy(lock); }
#endif


THREAD_FUNC_RETURN DecoderThreadFunc(void* userData) {
    AudioPlayer* player = (AudioPlayer*)userData;
    float tempBuffer[4096];
    const int CHUNK_SIZE = 1024;

    while (!player->threadExitFlag) {
        if (player->paused) {
            SLEEP_MS(10);
            continue;
        }


        if (player->ringBuffer.available > (player->ringBuffer.capacity - (CHUNK_SIZE * player->channels))) {
            SLEEP_MS(10);
            continue;
        }

        pa_mutex_lock(&player->lock);
        long framesRead = codec_read_float(player->codec, tempBuffer, CHUNK_SIZE);
        pa_mutex_unlock(&player->lock);

        if (framesRead > 0) {
            AudioRing_Write(&player->ringBuffer, tempBuffer, framesRead * player->channels);
        }
        else {
            SLEEP_MS(100);
        }
    }
    return 0;
}

int audio_callback_c(const void* input, void* output, unsigned long frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    AudioPlayer* player = (AudioPlayer*)userData;
    float* out = (float*)output;
    unsigned long samplesNeeded = frameCount * player->channels;

    if (!player || !player->codec || player->paused) {
        memset(out, 0, frameCount * player->channels * sizeof(float));
        return paContinue;
    }

    size_t samplesRead = AudioRing_Read(&player->ringBuffer, out, samplesNeeded);

    float currentGain = player->lastGain;
    float gainStep = (player->gain - player->lastGain) / (float)frameCount;

    for (size_t i = 0; i < samplesRead; i++) {
        if (i % player->channels == 0) currentGain += gainStep;
        out[i] *= currentGain;
    }
    player->lastGain = player->gain;

    player->currentFrame += (samplesRead / player->channels);

    if (samplesRead < samplesNeeded) {
        memset(out + samplesRead, 0, (samplesNeeded - samplesRead) * sizeof(float));

        if (player->currentFrame >= player->totalFrames) {
            return paComplete;
        }
    }

    return paContinue;
}

int audio_init() { return Pa_Initialize(); }
int audio_terminate() { return Pa_Terminate(); }

static int setup_stream_internal(AudioPlayer* player, int device) {
    if (device == -1) device = Pa_GetDefaultOutputDevice();
    const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(device);
    if (!devInfo) return -1;

    PaStreamParameters output;
    memset(&output, 0, sizeof(PaStreamParameters));
    output.device = device;
    output.channelCount = player->channels;
    output.sampleFormat = paFloat32;
    output.suggestedLatency = devInfo->defaultLowOutputLatency;

    PaError err = Pa_OpenStream(&player->stream, NULL, &output, player->samplerate,
        512, paNoFlag, audio_callback_c, player);

    if (err != paNoError) return -1;

    err = Pa_StartStream(player->stream);
    if (err != paNoError) {
        Pa_CloseStream(player->stream);
        player->stream = NULL;
        return -1;
    }
    return 0;
}

static int audio_start_common(AudioPlayer* player, int device) {
    size_t bufferSize = player->samplerate * player->channels * 5;
    AudioRing_Init(&player->ringBuffer, bufferSize);


    player->threadExitFlag = 0;
#ifdef _WIN32
    player->decoderThreadHandle = (void*)_beginthreadex(NULL, 0, DecoderThreadFunc, player, 0, NULL);
#else
    pthread_create((pthread_t*)&player->decoderThreadHandle, NULL, DecoderThreadFunc, player);
#endif

    if (setup_stream_internal(player, device) != 0) {
        player->threadExitFlag = 1;
        AudioRing_Free(&player->ringBuffer);
        return -1;
    }
    return 0;
}

#ifdef _WIN32
int audio_play_w(AudioPlayer* player, const wchar_t* filename_w, int device) {
    if (!player || !filename_w) return -1;

    if (player->stream != NULL || player->decoderThreadHandle != 0) {
        audio_stop(player);
    }

    player->codec = codec_open_w(filename_w);
    if (!player->codec) return -1;

    player->CodecName = codec_return_codec(player->codec);
    player->channels = codec_get_channels(player->codec);
    player->samplerate = codec_get_samplerate(player->codec);
    player->totalFrames = codec_get_total_frames(player->codec);
    player->currentFrame = 0;
    player->paused = 0;
    player->lastGain = player->gain;

    return audio_start_common(player, device);
}
#else
int audio_play(AudioPlayer* player, const char* filename, int device) {
    if (!player || !filename) return -1;

    if (player->stream != NULL || player->decoderThreadHandle != 0) {
        audio_stop(player);
    }

    player->filename = filename; // 
    player->codec = codec_open(player->filename);
    if (!player->codec) return -1;

    player->CodecName = codec_return_codec(player->codec);
    player->channels = codec_get_channels(player->codec);
    player->samplerate = codec_get_samplerate(player->codec);
    player->totalFrames = codec_get_total_frames(player->codec);
    player->currentFrame = 0;
    player->paused = 0;
    player->lastGain = player->gain;

    return audio_start_common(player, device);
}
#endif

int audio_stop(AudioPlayer* player) {
    if (!player) return -1;

    if (player->stream) {
        Pa_StopStream(player->stream);
        Pa_CloseStream(player->stream);
        player->stream = NULL;
    }

    player->threadExitFlag = 1;
#ifdef _WIN32
    if (player->decoderThreadHandle) {
        WaitForSingleObject((HANDLE)player->decoderThreadHandle, INFINITE);
        CloseHandle((HANDLE)player->decoderThreadHandle);
        player->decoderThreadHandle = NULL;
    }
#else
    if (player->decoderThreadHandle) {
        pthread_join((pthread_t)player->decoderThreadHandle, NULL);
        player->decoderThreadHandle = 0;
    }
#endif

    if (player->codec) {
        codec_close(player->codec);
        player->codec = NULL;
    }

    AudioRing_Free(&player->ringBuffer);

    return 0;
}

int device_hotswap(AudioPlayer* player, int device_index) {
    if (!player) return -1;

    pa_mutex_lock(&player->lock);

    if (player->stream) {
        Pa_StopStream(player->stream);
        Pa_CloseStream(player->stream);
        player->stream = NULL;
    }

    if (device_index == -1) device_index = Pa_GetDefaultOutputDevice();
    const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(device_index);

    if (!devInfo) {
        pa_mutex_unlock(&player->lock);
        return -1;
    }

    PaStreamParameters output;
    memset(&output, 0, sizeof(PaStreamParameters));
    output.device = device_index;
    output.channelCount = player->channels;
    output.sampleFormat = paFloat32;
    output.suggestedLatency = devInfo->defaultLowOutputLatency;
    output.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(&player->stream, NULL, &output, player->samplerate,
        512, paNoFlag, audio_callback_c, player);

    if (err != paNoError) {
        player->stream = NULL;
        pa_mutex_unlock(&player->lock);
        return -1;
    }

    err = Pa_StartStream(player->stream);

    pa_mutex_unlock(&player->lock);

    return (err == paNoError) ? 0 : -1;
}

int audio_pause(AudioPlayer* player, int pause) {
    if (!player) return -1;
    player->paused = pause;
    return 0;
}


int audio_seek(AudioPlayer* player, int64_t frame) {
    if (!player || !player->codec) return -1;

    int was_paused = player->paused;
    player->paused = 1;

    pa_mutex_lock(&player->lock);

    long pos = codec_seek(player->codec, frame);
    if (pos < 0) pos = 0;
    if (pos > player->totalFrames) pos = player->totalFrames;
    player->currentFrame = pos;


    AudioRing_Clear(&player->ringBuffer);

    pa_mutex_unlock(&player->lock);

    player->paused = was_paused;
    return (int)player->currentFrame;
}