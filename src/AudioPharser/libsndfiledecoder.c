#include "libsndfiledecoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <wchar.h>

SndFileDecoder* sndfile_open_w(const wchar_t* filename_w) {
    SndFileDecoder* decoder = (SndFileDecoder*)malloc(sizeof(SndFileDecoder));
    if (!decoder) {
        return NULL;
    }
    
    memset(decoder, 0, sizeof(SndFileDecoder));
    decoder->info.format = 0;

    decoder->file = sf_wchar_open(filename_w, SFM_READ, &decoder->info);

    if (decoder->file == NULL) {
        fwprintf(stderr, L"Error opening file %s: %s\n", filename_w, sf_strerror(NULL));
        free(decoder);
        return NULL;
    }

    decoder->current_frame = 0;
    return decoder;
}
#endif

SndFileDecoder* sndfile_open(const char* filename) {
    SndFileDecoder* decoder = (SndFileDecoder*)malloc(sizeof(SndFileDecoder));
    if (!decoder) {
        return NULL;
    }

    memset(decoder, 0, sizeof(SndFileDecoder));
    decoder->info.format = 0;
    decoder->file = sf_open(filename, SFM_READ, &decoder->info);

    if (decoder->file == NULL) {
        fprintf(stderr, "Error opening file %s: %s\n", filename, sf_strerror(NULL));
        free(decoder);
        return NULL;
    }

    decoder->current_frame = 0;
    return decoder;
}

sf_count_t sndfile_read_float(SndFileDecoder* decoder, float* buffer, sf_count_t frames) {
    if (!decoder || !decoder->file) return 0;
    sf_count_t read_frames = sf_readf_float(decoder->file, buffer, frames);
    decoder->current_frame += read_frames;
    return read_frames;
}

sf_count_t sndfile_seek(SndFileDecoder* decoder, sf_count_t frame) {
    if (!decoder || !decoder->file) return 0;
    sf_count_t result = sf_seek(decoder->file, frame, SEEK_SET);
    if (result >= 0) {
        decoder->current_frame = result;
    }
    return decoder->current_frame;
}

sf_count_t sndfile_get_current_frame(SndFileDecoder* decoder) {
    if (!decoder) return 0;
    return decoder->current_frame;
}

sf_count_t sndfile_get_total_frames(SndFileDecoder* decoder) {
    if (!decoder) return 0;
    return decoder->info.frames;
}

int sndfile_get_channels(SndFileDecoder* decoder) {
    if (!decoder) return 0;
    return decoder->info.channels;
}

int sndfile_get_samplerate(SndFileDecoder* decoder) {
    if (!decoder) return 0;
    return decoder->info.samplerate;
}

void sndfile_close(SndFileDecoder* decoder) {
    if (decoder) {
        if (decoder->file) {
            sf_close(decoder->file);
        }
        free(decoder);
    }
}