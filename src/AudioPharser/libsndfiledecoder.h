#ifndef LIBSNDFILEDECODER_H
#define LIBSNDFILEDECODER_H

#include <sndfile.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    SNDFILE* file;
    SF_INFO info;
    sf_count_t current_frame;
} SndFileDecoder;

SndFileDecoder* sndfile_open(const char* filename);

#ifdef _WIN32
#include <wchar.h>
SndFileDecoder* sndfile_open_w(const wchar_t* filename_w);
#endif

sf_count_t sndfile_read_float(SndFileDecoder* decoder, float* buffer, sf_count_t frames);
sf_count_t sndfile_seek(SndFileDecoder* decoder, sf_count_t frame);
sf_count_t sndfile_get_current_frame(SndFileDecoder* decoder);
sf_count_t sndfile_get_total_frames(SndFileDecoder* decoder);
int sndfile_get_channels(SndFileDecoder* decoder);
int sndfile_get_samplerate(SndFileDecoder* decoder);
void sndfile_close(SndFileDecoder* decoder);

#ifdef __cplusplus
}
#endif

#endif // LIBSNDFILEDECODER_H