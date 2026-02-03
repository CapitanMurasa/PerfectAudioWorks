#include "mpg123decoder.h"
#include "../miscellaneous/misc.h"
#include <string.h>
#include <stdlib.h>
#include <mpg123.h> 
#include <stdio.h> 

#ifdef _WIN32

#include <wchar.h>

MPG123Decoder* MPG123Decoder_open_w(const wchar_t* filename_w) {
    if (mpg123_init() != MPG123_OK) return NULL;

    MPG123Decoder* dec = (MPG123Decoder*)malloc(sizeof(MPG123Decoder));
    if (!dec) return NULL;

    memset(dec, 0, sizeof(MPG123Decoder));

    dec->mh = mpg123_new(NULL, NULL);
    if (!dec->mh) {
        free(dec);
        return NULL;
    }

    char* utf8_path = wchar_to_char_alloc(filename_w);
    if (utf8_path) {
        int result = mpg123_open(dec->mh, utf8_path);
        free(utf8_path);
        if (result != MPG123_OK) return NULL;
    }

    long rate;
    int channels, encoding;
    if (mpg123_getformat(dec->mh, &rate, &channels, &encoding) != MPG123_OK) {
        mpg123_close(dec->mh);
        mpg123_delete(dec->mh);
        free(dec);
        return NULL;
    }

    mpg123_format_none(dec->mh);
    if (mpg123_format(dec->mh, rate, channels, MPG123_ENC_SIGNED_16) != MPG123_OK) {
        mpg123_close(dec->mh);
        mpg123_delete(dec->mh);
        free(dec);
        return NULL;
    }

    dec->channels = channels;
    dec->samplerate = rate;
    dec->current_frame = 0;
    dec->total_frames = mpg123_length(dec->mh);

    return dec;
}
#endif // <-- FIX 5: This #endif was missing

MPG123Decoder* MPG123Decoder_open(const char* filename) {
    if (mpg123_init() != MPG123_OK) return NULL;

    MPG123Decoder* dec = (MPG123Decoder*)malloc(sizeof(MPG123Decoder));
    if (!dec) return NULL;

    memset(dec, 0, sizeof(MPG123Decoder));

    dec->mh = mpg123_new(NULL, NULL);
    if (!dec->mh) {
        free(dec);
        return NULL;
    }

    if (mpg123_open(dec->mh, filename) != MPG123_OK) {
        mpg123_delete(dec->mh);
        free(dec);
        return NULL;
    }

    long rate;
    int channels, encoding;
    if (mpg123_getformat(dec->mh, &rate, &channels, &encoding) != MPG123_OK) {
        mpg123_close(dec->mh);
        mpg123_delete(dec->mh);
        free(dec);
        return NULL;
    }

    mpg123_format_none(dec->mh);
    if (mpg123_format(dec->mh, rate, channels, MPG123_ENC_SIGNED_16) != MPG123_OK) {
        mpg123_close(dec->mh);
        mpg123_delete(dec->mh);
        free(dec);
        return NULL;
    }

    dec->channels = channels;
    dec->samplerate = rate;
    dec->current_frame = 0;
    dec->total_frames = mpg123_length(dec->mh);

    return dec;
}

int MPG123Decoder_get_channels(const MPG123Decoder* dec) {
    return dec ? dec->channels : 0;
}

int MPG123Decoder_get_samplerate(const MPG123Decoder* dec) {
    return dec ? dec->samplerate : 0;
}

long MPG123Decoder_get_total_frames(const MPG123Decoder* dec) {
    return dec ? dec->total_frames : 0;
}

long MPG123Decoder_get_current_frame(const MPG123Decoder* dec) {
    return dec ? dec->current_frame : -1;
}

long MPG123Decoder_read_int16(MPG123Decoder* dec, int16_t* buffer, int frames) {
    if (!dec || !buffer) return 0;

    size_t bytes_to_read = (size_t)frames * dec->channels * sizeof(int16_t);
    size_t bytes_read = 0;

    int err = mpg123_read(dec->mh, (unsigned char*)buffer, bytes_to_read, &bytes_read);
    if (err != MPG123_OK && err != MPG123_DONE) return 0;

    size_t samples_read = bytes_read / sizeof(int16_t);
    long frames_read = samples_read / dec->channels;

    dec->current_frame += frames_read;
    return frames_read;
}

long MPG123Decoder_seek(MPG123Decoder* dec, long frame) {
    if (!dec || !dec->mh) return -1;

    off_t result = mpg123_seek(dec->mh, frame, SEEK_SET);
    if (result >= 0) dec->current_frame = result;
    return result;
}

void MPG123Decoder_close(MPG123Decoder* dec) {
    if (!dec) return;
    if (dec->mh) {
        mpg123_close(dec->mh);
        mpg123_delete(dec->mh);
    }
    free(dec);
}