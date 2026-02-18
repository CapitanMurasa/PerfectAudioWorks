#ifndef PORTAUDIO_BACKEND_H
#define PORTAUDIO_BACKEND_H

#include "CodecHandler.h"
#include "AudioRingBuffer.h"
#include <portaudio.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
typedef CRITICAL_SECTION pa_mutex_t;
#else
#include <pthread.h>
typedef pthread_mutex_t pa_mutex_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct AudioPlayer{
        CodecHandler* codec;
        PaStream* stream;
        const char* CodecName;
        int channels;
        int samplerate;
        long totalFrames;
        long currentFrame;
        float gain;
        float lastGain;
        int paused;
        pa_mutex_t lock;
        const char* filename;

        AudioRing ringBuffer;      
        void* decoderThreadHandle;  
        int threadExitFlag;         
        int isBuffering;
    } AudioPlayer;

    int audio_init();
    int audio_terminate();

    #ifdef _WIN32
    #include <wchar.h>
    int audio_play_w(AudioPlayer* player, const wchar_t* filename_w, int device);
    #else
    int audio_play(AudioPlayer* player, const char* filename, int device);
    #endif

    int audio_stop(AudioPlayer* player);
    int audio_pause(AudioPlayer* player, int pause);
    int audio_seek(AudioPlayer* player, int64_t frame);
    int device_hotswap(AudioPlayer* player, int device);
    int audio_cleanup_resources(AudioPlayer* player);

    void pa_mutex_init(pa_mutex_t* lock);
    void pa_mutex_lock(pa_mutex_t* lock);
    void pa_mutex_unlock(pa_mutex_t* lock);
    void pa_mutex_destroy(pa_mutex_t* lock);

    int audio_callback_c(const void* input, void* output, unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData);

#ifdef __cplusplus
}
#endif

#endif 