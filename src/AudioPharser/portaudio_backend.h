#ifndef PORTAUDIO_BACKEND_H
#define PORTAUDIO_BACKEND_H

#include "CodecHandler.h"
#include <portaudio.h>
#include <stdint.h>

// --- Cross-Platform Threading Fix ---
#ifdef _WIN32
#include <windows.h>
// Use the standard Windows synchronization object as the substitute
typedef CRITICAL_SECTION pa_mutex_t;
#else
#include <pthread.h>
// Use the standard POSIX name
typedef pthread_mutex_t pa_mutex_t;
#endif
// ------------------------------------

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
        pa_mutex_t lock;    // Use the cross-platform type here
    } AudioPlayer;

    int audio_init();
    int audio_terminate();

    int audio_play(AudioPlayer* player, const char* filename, int device);
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