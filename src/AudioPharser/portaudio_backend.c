#include "portaudio_backend.h"
#include "CodecHandler.h"
#include "../miscellaneous/misc.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h> 

// --- Mutex Implementation ---
#ifdef _WIN32
void pa_mutex_init(pa_mutex_t* lock) {
    InitializeCriticalSection(lock);
}
void pa_mutex_lock(pa_mutex_t* lock) {
    EnterCriticalSection(lock);
}
void pa_mutex_unlock(pa_mutex_t* lock) {
    LeaveCriticalSection(lock);
}
void pa_mutex_destroy(pa_mutex_t* lock) {
    DeleteCriticalSection(lock);
}
#else
#include <pthread.h>
void pa_mutex_init(pa_mutex_t* lock) {
    pthread_mutex_init(lock, NULL);
}
void pa_mutex_lock(pa_mutex_t* lock) {
    pthread_mutex_lock(lock);
}
void pa_mutex_unlock(pa_mutex_t* lock) {
    pthread_mutex_unlock(lock);
}
void pa_mutex_destroy(pa_mutex_t* lock) {
    pthread_mutex_destroy(lock);
}
#endif

// --- Audio Callback ---
int audio_callback_c(const void* input, void* output, unsigned long frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    AudioPlayer* player = (AudioPlayer*)userData;
    float* out = (float*)output;

    // Safety check: ensure player and codec exist
    if (!player || !player->codec) {
        memset(out, 0, frameCount * sizeof(float)); // Silence
        return paAbort;
    }

    if (player->paused) {
        memset(out, 0, frameCount * player->channels * sizeof(float));
        return paContinue;
    }

    // Critical Section: Reading from disk/codec
    pa_mutex_lock(&player->lock);
    long readFrames = codec_read_float(player->codec, out, frameCount);
    pa_mutex_unlock(&player->lock);

    // Apply Gain Ramping (prevents clicking)
    float currentGain = player->lastGain;
    float gainStep = (player->gain - player->lastGain) / (float)frameCount;

    int totalSamples = readFrames * player->channels;
    for (int i = 0; i < totalSamples; i++) {
        if (i % player->channels == 0) {
            currentGain += gainStep;
        }
        out[i] *= currentGain;
    }
    player->lastGain = player->gain;

    player->currentFrame += readFrames;

    // Handle End of File (Padding with silence if needed)
    if (readFrames < (long)frameCount) {
        memset(out + (readFrames * player->channels), 0,
            (frameCount - readFrames) * player->channels * sizeof(float));

        if (player->currentFrame >= player->totalFrames)
            return paComplete;
    }

    return paContinue;
}

// --- Init / Terminate ---
int audio_init() {
    return Pa_Initialize();
}

int audio_terminate() {
    return Pa_Terminate();
}

// --- Helper for Stream Setup ---
static int setup_stream_internal(AudioPlayer* player, int device) {
    if (device == -1) device = Pa_GetDefaultOutputDevice();

    const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(device);
    if (!devInfo) {
        fprintf(stderr, "Error: Invalid audio device ID %d.\n", device);
        return -1;
    }

    int outputChannels = player->channels;
    if (player->channels > devInfo->maxOutputChannels) {
        outputChannels = devInfo->maxOutputChannels;
    }

    PaStreamParameters output;
    memset(&output, 0, sizeof(PaStreamParameters));
    output.device = device;
    output.channelCount = outputChannels;
    output.sampleFormat = paFloat32;
    output.suggestedLatency = devInfo->defaultLowOutputLatency;
    output.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(&player->stream, NULL, &output, player->samplerate,
        512, paNoFlag, audio_callback_c, player);

    if (err != paNoError) {
        fprintf(stderr, "PortAudio OpenStream Error: %s\n", Pa_GetErrorText(err));
        return -1;
    }

    err = Pa_StartStream(player->stream);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio StartStream Error: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(player->stream);
        player->stream = NULL;
        return -1;
    }

    return 0;
}

#ifdef _WIN32
int audio_play_w(AudioPlayer* player, const wchar_t* filename_w, int device) {
    if (!player || !filename_w) return -1;


    player->codec = codec_open_w(filename_w);
    if (!player->codec) {
        fwprintf(stderr, L"Error: Failed to open codec for file: %s\n", filename_w);
        return -1;
    }

    player->CodecName = codec_return_codec(player->codec);
    player->channels = codec_get_channels(player->codec);
    player->samplerate = codec_get_samplerate(player->codec);
    player->totalFrames = codec_get_total_frames(player->codec);
    player->currentFrame = 0;
    player->paused = 0;

    player->lastGain = player->gain;

    if (setup_stream_internal(player, device) != 0) {
        codec_close(player->codec);
        player->codec = NULL;
        return -1;
    }

    return 0;
}
#else
int audio_play(AudioPlayer* player, const char* filename, int device) {
    if (!player || !filename) return -1;

    player->filename = filename;


    player->codec = codec_open(player->filename);
    if (!player->codec) {
        fprintf(stderr, "Error: Failed to open codec for file: %s\n", player->filename);
        return -1;
    }

    player->CodecName = codec_return_codec(player->codec);
    player->channels = codec_get_channels(player->codec);
    player->samplerate = codec_get_samplerate(player->codec);
    player->totalFrames = codec_get_total_frames(player->codec);
    player->currentFrame = 0;
    player->paused = 0;
    player->lastGain = player->gain;

    if (setup_stream_internal(player, device) != 0) {
        codec_close(player->codec);
        player->codec = NULL;
        return -1;
    }

    return 0;
}
#endif

int audio_stop(AudioPlayer* player) {
    if (!player) return -1;

    if (player->stream) {
        Pa_StopStream(player->stream);
        Pa_CloseStream(player->stream);
        player->stream = NULL;
    }

    if (player->codec) {
        codec_close(player->codec);
        player->codec = NULL;
    }

    return 0;
}

int audio_pause(AudioPlayer* player, int pause) {
    if (!player) return -1;
    player->paused = pause;
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

int audio_seek(AudioPlayer* player, int64_t frame) {
    if (!player || !player->codec) return -1;

    int was_paused = player->paused;
    player->paused = 1;

    pa_mutex_lock(&player->lock);

    long pos = codec_seek(player->codec, frame);
    if (pos < 0) pos = 0;
    if (pos > player->totalFrames) pos = player->totalFrames;
    player->currentFrame = pos;


    size_t buffer_size_frames = 512;
    size_t buffer_size_elements = buffer_size_frames * player->channels;
    float* tmp = (float*)malloc(buffer_size_elements * sizeof(float));

    if (tmp) {
        long r = codec_read_float(player->codec, tmp, buffer_size_frames);
        player->currentFrame += r;
        free(tmp);
    }

    pa_mutex_unlock(&player->lock);

    player->paused = was_paused;
    return (int)player->currentFrame;
}