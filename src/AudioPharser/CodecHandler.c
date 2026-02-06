#include "CodecHandler.h"
#include "../miscellaneous/misc.h"
#include <stdio.h>

#ifndef _WIN32
#include <strings.h>      
#define _stricmp strcasecmp 
#endif

struct CodecHandler {
    CodecType type;
    void* decoder;
};

// --- Windows Wide-Char Opener ---
#ifdef _WIN32
CodecHandler* codec_open_w(const wchar_t* filename_w) {
    if (!filename_w) return NULL;

    CodecHandler* ch = (CodecHandler*)malloc(sizeof(CodecHandler));
    if (!ch) return NULL;
    memset(ch, 0, sizeof(CodecHandler));

#ifdef ENABLE_SNDFILE
    SndFileDecoder* sf = sndfile_open_w(filename_w);
    if (sf) {
        ch->type = CODEC_TYPE_SNDFILE;
        ch->decoder = sf;
        return ch;
    }
#endif

#ifdef ENABLE_MPG123
    const wchar_t* ext = wcsrchr(filename_w, L'.');
    if (ext && _wcsicmp(ext, L".mp3") == 0) {
        MPG123Decoder* mp3 = MPG123Decoder_open_w(filename_w);
        if (mp3) {
            ch->type = CODEC_TYPE_MPG123;
            ch->decoder = mp3;
            return ch;
        }
    }
#endif

#ifdef ENABLE_FFMPEG
    char* utf8_filename = wchar_to_char_alloc(filename_w);

    if (utf8_filename) {
        FFmpegDecoder* ff = ffmpeg_open(utf8_filename);

        free(utf8_filename);

        if (ff) {
            ch->type = CODEC_TYPE_FFMPEG;
            ch->decoder = ff;
            return ch;
        }
    }
#endif

    free(ch);
    return NULL;
}
#endif

CodecHandler* codec_open(const char* filename) {
    if (!filename) return NULL;

    CodecHandler* ch = (CodecHandler*)malloc(sizeof(CodecHandler));
    if (!ch) return NULL;
    memset(ch, 0, sizeof(CodecHandler));

#ifdef ENABLE_SNDFILE
    SndFileDecoder* sf = sndfile_open(filename);
    if (sf) {
        ch->type = CODEC_TYPE_SNDFILE;
        ch->decoder = sf;
        return ch;
    }
#endif

#ifdef ENABLE_MPG123
    const char* ext = strrchr(filename, '.');
    if (ext && _stricmp(ext, ".mp3") == 0) {
        MPG123Decoder* mp3 = MPG123Decoder_open(filename);
        if (mp3) {
            ch->type = CODEC_TYPE_MPG123;
            ch->decoder = mp3;
            return ch;
        }
    }
#endif

#ifdef ENABLE_FFMPEG
    FFmpegDecoder* ff = ffmpeg_open(filename);
    if (ff) {
        ch->type = CODEC_TYPE_FFMPEG;
        ch->decoder = ff;
        return ch;
    }
#endif

    free(ch);
    return NULL;
}


int codec_get_channels(CodecHandler* ch) {
    if (!ch) return 0;
    switch (ch->type) {
#ifdef ENABLE_FFMPEG
    case CODEC_TYPE_FFMPEG: return ffmpeg_get_channels((FFmpegDecoder*)ch->decoder);
#endif
#ifdef ENABLE_SNDFILE
    case CODEC_TYPE_SNDFILE: return sndfile_get_channels((SndFileDecoder*)ch->decoder);
#endif
#ifdef ENABLE_MPG123
    case CODEC_TYPE_MPG123: return MPG123Decoder_get_channels((MPG123Decoder*)ch->decoder);
#endif
    default: return 0;
    }
}

long codec_get_total_frames(CodecHandler* ch) {
    if (!ch) return 0;
    switch (ch->type) {
#ifdef ENABLE_FFMPEG
    case CODEC_TYPE_FFMPEG: return ffmpeg_get_total_frames((FFmpegDecoder*)ch->decoder);
#endif
#ifdef ENABLE_SNDFILE
    case CODEC_TYPE_SNDFILE: return sndfile_get_total_frames((SndFileDecoder*)ch->decoder);
#endif
#ifdef ENABLE_MPG123
    case CODEC_TYPE_MPG123: return MPG123Decoder_get_total_frames((MPG123Decoder*)ch->decoder);
#endif
    default: return 0;
    }
}

int codec_get_samplerate(CodecHandler* ch) {
    if (!ch) return 0;
    switch (ch->type) {
#ifdef ENABLE_FFMPEG
    case CODEC_TYPE_FFMPEG: return ffmpeg_get_samplerate((FFmpegDecoder*)ch->decoder);
#endif
#ifdef ENABLE_SNDFILE
    case CODEC_TYPE_SNDFILE: return sndfile_get_samplerate((SndFileDecoder*)ch->decoder);
#endif
#ifdef ENABLE_MPG123
    case CODEC_TYPE_MPG123: return MPG123Decoder_get_samplerate((MPG123Decoder*)ch->decoder);
#endif
    default: return 0;
    }
}

long codec_read_float(CodecHandler* ch, float* buffer, int frames) {
    if (!ch) return 0;
    switch (ch->type) {
#ifdef ENABLE_FFMPEG
    case CODEC_TYPE_FFMPEG: return ffmpeg_read_float((FFmpegDecoder*)ch->decoder, buffer, frames);
#endif
#ifdef ENABLE_SNDFILE
    case CODEC_TYPE_SNDFILE: return sndfile_read_float((SndFileDecoder*)ch->decoder, buffer, frames);
#endif
#ifdef ENABLE_MPG123
    case CODEC_TYPE_MPG123: {
        int channels = MPG123Decoder_get_channels((MPG123Decoder*)ch->decoder);
        short* tmp = (short*)malloc(sizeof(short) * frames * channels);
        if (!tmp) return 0;
        long read = MPG123Decoder_read_int16((MPG123Decoder*)ch->decoder, tmp, frames);
        for (long i = 0; i < read * channels; i++) buffer[i] = tmp[i] / 32768.0f;
        free(tmp);
        return read;
    }
#endif
    default: return 0;
    }
}

long codec_seek(CodecHandler* ch, long frame) {
    if (!ch) return -1;
    switch (ch->type) {
#ifdef ENABLE_FFMPEG
    case CODEC_TYPE_FFMPEG: return ffmpeg_seek((FFmpegDecoder*)ch->decoder, frame);
#endif
#ifdef ENABLE_SNDFILE
    case CODEC_TYPE_SNDFILE: return sndfile_seek((SndFileDecoder*)ch->decoder, frame);
#endif
#ifdef ENABLE_MPG123
    case CODEC_TYPE_MPG123: return MPG123Decoder_seek((MPG123Decoder*)ch->decoder, frame);
#endif
    default: return -1;
    }
}

long codec_get_current_frame(CodecHandler* ch) {
    if (!ch) return -1;
    switch (ch->type) {
#ifdef ENABLE_FFMPEG
    case CODEC_TYPE_FFMPEG: return ffmpeg_get_current_frame((FFmpegDecoder*)ch->decoder);
#endif
#ifdef ENABLE_SNDFILE
    case CODEC_TYPE_SNDFILE: return sndfile_get_current_frame((SndFileDecoder*)ch->decoder);
#endif
#ifdef ENABLE_MPG123
    case CODEC_TYPE_MPG123: return MPG123Decoder_get_current_frame((MPG123Decoder*)ch->decoder);
#endif
    default: return -1;
    }
}

void codec_close(CodecHandler* ch) {
    if (!ch) return;
    switch (ch->type) {
#ifdef ENABLE_FFMPEG
    case CODEC_TYPE_FFMPEG: ffmpeg_close((FFmpegDecoder*)ch->decoder); break;
#endif
#ifdef ENABLE_SNDFILE
    case CODEC_TYPE_SNDFILE: sndfile_close((SndFileDecoder*)ch->decoder); break;
#endif
#ifdef ENABLE_MPG123
    case CODEC_TYPE_MPG123: MPG123Decoder_close((MPG123Decoder*)ch->decoder); break;
#endif
    default: break;
    }
    free(ch);
}

const char* codec_return_codec(CodecHandler* ch) {
    if (!ch) return "none";
    switch (ch->type) {
    case CODEC_TYPE_FFMPEG: return "ffmpeg";
    case CODEC_TYPE_SNDFILE: return "sndfile";
    case CODEC_TYPE_MPG123: return "mpg123";
    default: return "unknown";
    }
}