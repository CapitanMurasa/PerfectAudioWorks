#ifndef LIBSNDFILEDECODER_H
#define LIBSNDFILEDECODER_H

#include <sndfile.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        SNDFILE* file;
        SF_INFO info;
        sf_count_t current_frame;
    } SndFileDecoder;

    SndFileDecoder* sndfile_open(const char* filename);
    sf_count_t sndfile_read_float(SndFileDecoder*, float*, sf_count_t frames);
    sf_count_t sndfile_seek(SndFileDecoder*, sf_count_t frame);
    sf_count_t sndfile_get_current_frame(SndFileDecoder*);
    sf_count_t sndfile_get_total_frames(SndFileDecoder*);
    int sndfile_get_channels(SndFileDecoder* decoder);
    int sndfile_get_samplerate(SndFileDecoder* decoder);
    void sndfile_close(SndFileDecoder* decoder);

#ifdef __cplusplus
}
#endif

#endif