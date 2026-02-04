#include "CodecHandler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct CodecHandler {
    CodecType type;
    void* decoder;
};

#ifdef _WIN32
#include <wchar.h>

CodecHandler* codec_open_w(const wchar_t* filename_w) {
    if (!filename_w) return NULL;

    const wchar_t* ext_dot = wcsrchr(filename_w, L'.');
    const wchar_t* ext = ext_dot ? ext_dot : L"";

    CodecHandler* ch = (CodecHandler*)malloc(sizeof(CodecHandler));
    if (!ch) return NULL;
    memset(ch, 0, sizeof(CodecHandler));
    ch->type = CODEC_TYPE_NONE;

#if defined(ENABLE_MPG123)
    if (_wcsicmp(ext, L".mp3") == 0) {
        MPG123Decoder* mp3 = MPG123Decoder_open_w(filename_w);
        if (mp3) {
            ch->type = CODEC_TYPE_MPG123;
            ch->decoder = mp3;
            return ch;
        }
    }
#endif

#if defined(ENABLE_SNDFILE)
    SndFileDecoder* sf = sndfile_open_w(filename_w);
    if (sf) {
        ch->type = CODEC_TYPE_SNDFILE;
        ch->decoder = sf;
        return ch;
    }
#endif

#if !defined(ENABLE_SNDFILE) && !defined(ENABLE_MPG123)
    printf("No codec had been chosen before compiling.\n");
    free(ch);
    return NULL;
#else
    free(ch);
    return NULL;
#endif
}
#endif

CodecHandler* codec_open(const char* filename) {
    if (!filename) return NULL;
    const char* filetype = get_file_format(filename);

    CodecHandler* ch = (CodecHandler*)malloc(sizeof(CodecHandler));
    if (!ch) return NULL;
    memset(ch, 0, sizeof(CodecHandler));
    ch->type = CODEC_TYPE_NONE;

#if defined(ENABLE_MPG123)
    if (strcmp(filetype, "MP3") == 0) {
        MPG123Decoder* mp3 = MPG123Decoder_open(filename);
        if (mp3) {
            ch->type = CODEC_TYPE_MPG123;
            ch->decoder = mp3;
            return ch;
        }
    }
#endif

#if defined(ENABLE_SNDFILE)
    SndFileDecoder* sf = sndfile_open(filename);
    if (sf) {
        ch->type = CODEC_TYPE_SNDFILE;
        ch->decoder = sf;
        return ch;
    }
#endif

#if !defined(ENABLE_SNDFILE) && !defined(ENABLE_MPG123)
    printf("No codec had been chosen before compiling.\n");
    free(ch);
    return NULL;
#else
    free(ch);
    return NULL;
#endif
}

int codec_get_channels(CodecHandler* ch) {
    if (!ch) return 0;
#if defined(ENABLE_SNDFILE)
    if (ch->type == CODEC_TYPE_SNDFILE) return sndfile_get_channels((SndFileDecoder*)ch->decoder);
#endif
#if defined(ENABLE_MPG123)
    if (ch->type == CODEC_TYPE_MPG123) return MPG123Decoder_get_channels((MPG123Decoder*)ch->decoder);
#endif
    return 0;
}

long codec_get_total_frames(CodecHandler* ch) {
    if (!ch) return 0;
#if defined(ENABLE_SNDFILE)
    if (ch->type == CODEC_TYPE_SNDFILE) return sndfile_get_total_frames((SndFileDecoder*)ch->decoder);
#endif
#if defined(ENABLE_MPG123)
    if (ch->type == CODEC_TYPE_MPG123) return MPG123Decoder_get_total_frames((MPG123Decoder*)ch->decoder);
#endif
    return 0;
}

int codec_get_samplerate(CodecHandler* ch) {
    if (!ch) return 0;
#if defined(ENABLE_SNDFILE)
    if (ch->type == CODEC_TYPE_SNDFILE) return sndfile_get_samplerate((SndFileDecoder*)ch->decoder);
#endif
#if defined(ENABLE_MPG123)
    if (ch->type == CODEC_TYPE_MPG123) return MPG123Decoder_get_samplerate((MPG123Decoder*)ch->decoder);
#endif
    return 0;
}

long codec_read_float(CodecHandler* ch, float* buffer, int frames) {
    if (!ch) return 0;
#if defined(ENABLE_SNDFILE)
    if (ch->type == CODEC_TYPE_SNDFILE) return sndfile_read_float((SndFileDecoder*)ch->decoder, buffer, frames);
#endif
#if defined(ENABLE_MPG123)
    if (ch->type == CODEC_TYPE_MPG123) {
        int channels = MPG123Decoder_get_channels((MPG123Decoder*)ch->decoder);
        int16_t* tmp = (int16_t*)malloc(sizeof(int16_t) * frames * channels);
        if (!tmp) return 0;

        long read_frames = MPG123Decoder_read_int16((MPG123Decoder*)ch->decoder, tmp, frames);

        for (long i = 0; i < read_frames * channels; i++) {
            buffer[i] = tmp[i] / 32768.0f;
        }

        free(tmp);
        return read_frames;
    }
#endif
    return 0;
}

long codec_seek(CodecHandler* ch, long frame) {
    if (!ch) return -1;
#if defined(ENABLE_SNDFILE)
    if (ch->type == CODEC_TYPE_SNDFILE) return sndfile_seek((SndFileDecoder*)ch->decoder, frame);
#endif
#if defined(ENABLE_MPG123)
    if (ch->type == CODEC_TYPE_MPG123) return MPG123Decoder_seek((MPG123Decoder*)ch->decoder, frame);
#endif
    return -1;
}

long codec_get_current_frame(CodecHandler* ch) {
    if (!ch) return -1;
#if defined(ENABLE_SNDFILE)
    if (ch->type == CODEC_TYPE_SNDFILE) return sndfile_get_current_frame((SndFileDecoder*)ch->decoder);
#endif
#if defined(ENABLE_MPG123)
    if (ch->type == CODEC_TYPE_MPG123) return MPG123Decoder_get_current_frame((MPG123Decoder*)ch->decoder);
#endif
    return -1;
}

void codec_close(CodecHandler* ch) {
    if (!ch) return;
#if defined(ENABLE_SNDFILE)
    if (ch->type == CODEC_TYPE_SNDFILE) sndfile_close((SndFileDecoder*)ch->decoder);
#endif
#if defined(ENABLE_MPG123)
    if (ch->type == CODEC_TYPE_MPG123) MPG123Decoder_close((MPG123Decoder*)ch->decoder);
#endif
    free(ch);
}

const char* codec_return_codec(CodecHandler* ch) {
    if (!ch) return "none";
#if defined(ENABLE_SNDFILE)
    if (ch->type == CODEC_TYPE_SNDFILE) return "sndfile";
#endif
#if defined(ENABLE_MPG123)
    if (ch->type == CODEC_TYPE_MPG123) return "mpg123";
#endif
    return "unknown";
}