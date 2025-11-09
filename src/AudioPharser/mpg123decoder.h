#ifndef MPG123DECODER_H
#define MPG123DECODER_H

#include <stdint.h>

// Include wchar.h FIRST on Windows
#ifdef _WIN32
#include <wchar.h>
#endif

// Now include mpg123.h
#include <mpg123.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    mpg123_handle* mh;
    int channels;
    long samplerate;
    long total_frames;
    long current_frame;
} MPG123Decoder; // <-- FIX 1: Removed the 'S'

#ifdef _WIN32
MPG123Decoder* MPG123Decoder_open_w(const wchar_t* filename_w);
#endif

MPG123Decoder* MPG123Decoder_open(const char* filename);



int MPG123Decoder_get_channels(const MPG123Decoder* dec);
int MPG123Decoder_get_samplerate(const MPG123Decoder* dec);
long MPG123Decoder_get_total_frames(const MPG123Decoder* dec);
long MPG123Decoder_get_current_frame(const MPG123Decoder* dec);
long MPG123Decoder_read_int16(MPG123Decoder* dec, int16_t* buffer, int frames); // <-- FIX 2: Removed the 'S'
long MPG123Decoder_seek(MPG123Decoder* dec, long frame);
void MPG123Decoder_close(MPG123Decoder* dec);

#ifdef __cplusplus
}
#endif

#endif // MPG123DECODER_H