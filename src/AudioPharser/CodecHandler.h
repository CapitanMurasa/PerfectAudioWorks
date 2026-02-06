#ifndef CODECHANDLER_H
#define CODECHANDLER_H

#include <stdlib.h>
#include <string.h>
#include <portaudio.h>

#ifdef ENABLE_FFMPEG
#include "ffmpegdecoder.h"
#endif

#ifdef ENABLE_SNDFILE
#include "libsndfiledecoder.h"
#endif

#ifdef ENABLE_MPG123
#include "mpg123decoder.h"
#endif

#include "../miscellaneous/file.h"

typedef enum {
    CODEC_TYPE_NONE = 0,
    CODEC_TYPE_FFMPEG,
    CODEC_TYPE_SNDFILE,
    CODEC_TYPE_MPG123
} CodecType;

typedef struct CodecHandler CodecHandler;

CodecHandler* codec_open(const char* filename);

#ifdef _WIN32
#include <wchar.h>
CodecHandler* codec_open_w(const wchar_t* filename_w);
#endif

int codec_get_channels(CodecHandler* ch);
long codec_get_total_frames(CodecHandler* ch);
long codec_get_current_frame(CodecHandler* ch);
int codec_get_samplerate(CodecHandler* ch);
long codec_read_float(CodecHandler* ch, float* buffer, int frames);
long codec_seek(CodecHandler* ch, long frame);
void codec_close(CodecHandler* ch);
const char* codec_return_codec(CodecHandler* ch);

#endif