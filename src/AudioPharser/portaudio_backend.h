#ifndef PORTAUDIO_BACKEND_H
#define PORTAUDIO_BACKEND_H

#include "CodecHandler.h"
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

    typedef struct {
        CodecHandler* codec;
        PaStream* stream;
        const char* CodecName;
        int channels;
        int samplerate;
        long totalFrames;
        long currentFrame;
        int paused;
        pa_mutex_t lock;
    } AudioPlayer;

    int audio_init();
    int audio_terminate();

    int audio_play(AudioPlayer* player, const char* filename, int device);

    #ifdef _WIN32
    #include <wchar.h>
    int audio_play_w(AudioPlayer* player, const wchar_t* filename_w, int device);
    #endif

    int audio_stop(AudioPlayer* player);
    int audio_pause(AudioPlayer* player, int pause);
    int audio_seek(AudioPlayer* player, int64_t frame);

    int audio_callback_c(const void* input, void* output, unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData);

#ifdef __cplusplus
}
#endif

#endif // PORTAUDIO_BACKEND_H