#include "faaddecoder.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
uint32_t read_callback(void* user_data, void* buffer, uint32_t length) {
    return (uint32_t)fread(buffer, 1, length, (FILE*)user_data);
}
uint32_t seek_callback(void* user_data, uint64_t position) {
    return (uint32_t)fseek((FILE*)user_data, (long)position, SEEK_SET);
}
#endif

static int init_decoder(FaadDecoder* dec) {
    int tracks = mp4ff_total_tracks(dec->infile);
    dec->track = -1;

    for (int i = 0; i < tracks; i++) {

        dec->track = i;
        break; 
    }

    if (dec->track < 0) return -1;

    unsigned char* buffer = NULL;
    unsigned int bufferSize = 0;
    mp4ff_get_decoder_config(dec->infile, dec->track, &buffer, &bufferSize);

    dec->decoder = NeAACDecOpen();
    NeAACDecConfigurationPtr config = NeAACDecGetCurrentConfiguration(dec->decoder);
    config->outputFormat = FAAD_FMT_FLOAT;
    NeAACDecSetConfiguration(dec->decoder, config);

    unsigned long sr;
    unsigned char ch;
    long result = NeAACDecInit2(dec->decoder, buffer, bufferSize, &sr, &ch);

    if (result < 0) {
        if (buffer) free(buffer);
        return -1;
    }

    dec->samplerate = sr;
    dec->channels = ch;

    dec->total_frames = mp4ff_get_track_duration(dec->infile, dec->track);
    dec->current_frame = 0;
    dec->internal_buffer = NULL;
    dec->internal_buffer_len = 0;
    dec->internal_buffer_pos = 0;
    dec->mp4_buffer = NULL;
    dec->mp4_buffer_size = 0;

    if (buffer) free(buffer);
    return 0;
}

#ifdef _WIN32
FaadDecoder* faad_open_w(const wchar_t* filename) {
    FaadDecoder* dec = (FaadDecoder*)calloc(1, sizeof(FaadDecoder));
    if (!dec) return NULL;

    FILE* fp = _wfopen(filename, L"rb");
    if (!fp) { free(dec); return NULL; }

    dec->infile = mp4ff_open_read_cb(fp, read_callback, seek_callback);
    if (!dec->infile) { fclose(fp); free(dec); return NULL; }

    if (init_decoder(dec) != 0) {
        faad_close(dec);
        return NULL;
    }
    return dec;
}
#endif

FaadDecoder* faad_open(const char* filename) {
    FaadDecoder* dec = (FaadDecoder*)calloc(1, sizeof(FaadDecoder));
    if (!dec) return NULL;

    FILE* fp = fopen(filename, "rb");
    if (!fp) { free(dec); return NULL; }

    dec->infile = mp4ff_open_read(fp);
    if (!dec->infile) { fclose(fp); free(dec); return NULL; }

    if (init_decoder(dec) != 0) {
        faad_close(dec);
        return NULL;
    }
    return dec;
}

long faad_read_float(FaadDecoder* dec, float* buffer, int frames) {
    if (!dec) return 0;

    int frames_filled = 0;
    int samples_needed = frames * dec->channels; 
    int samples_filled = 0;

    while (samples_filled < samples_needed) {
        if (dec->internal_buffer && dec->internal_buffer_pos < dec->internal_buffer_len) {
            int available = dec->internal_buffer_len - dec->internal_buffer_pos;
            int to_copy = samples_needed - samples_filled;

            if (available < to_copy) to_copy = available;

            memcpy(buffer + samples_filled,
                dec->internal_buffer + dec->internal_buffer_pos,
                to_copy * sizeof(float));

            samples_filled += to_copy;
            dec->internal_buffer_pos += to_copy;

            dec->current_frame += (to_copy / dec->channels);
            continue;
        }

        int current_sample_idx = mp4ff_get_sample_position(dec->infile, dec->track);
        if (current_sample_idx >= mp4ff_num_samples(dec->infile, dec->track)) {
            break; 
        }

        int rc = mp4ff_read_sample(dec->infile, dec->track, current_sample_idx,
            &dec->mp4_buffer, &dec->mp4_buffer_size);

        if (rc == 0) break;

        NeAACDecFrameInfo frameInfo;
        void* raw_pcm = NeAACDecDecode(dec->decoder, &frameInfo, dec->mp4_buffer, rc);

        if (frameInfo.error > 0) {
            continue;
        }



        if (dec->internal_buffer) free(dec->internal_buffer);
        dec->internal_buffer_len = frameInfo.samples;
        dec->internal_buffer = (float*)malloc(dec->internal_buffer_len * sizeof(float));
        memcpy(dec->internal_buffer, raw_pcm, dec->internal_buffer_len * sizeof(float));
        dec->internal_buffer_pos = 0;
    }

    return samples_filled / dec->channels; 
}

long faad_seek(FaadDecoder* dec, long frame) {

    float time_seconds = (float)frame / dec->samplerate;
    int timescale = mp4ff_time_scale(dec->infile, dec->track);
    long long time_ticks = (long long)(time_seconds * timescale);

    int chunk = mp4ff_find_sample_use_offsets(dec->infile, dec->track, time_ticks);
    mp4ff_set_sample_position(dec->infile, dec->track, chunk);

    dec->current_frame = frame; 

    dec->internal_buffer_len = 0;
    dec->internal_buffer_pos = 0;

    return dec->current_frame;
}

void faad_close(FaadDecoder* dec) {
    if (!dec) return;
    if (dec->decoder) NeAACDecClose(dec->decoder);
    if (dec->infile) mp4ff_close(dec->infile);
    if (dec->internal_buffer) free(dec->internal_buffer);
    if (dec->mp4_buffer) free(dec->mp4_buffer);
    free(dec);
}

int faad_get_channels(FaadDecoder* dec) { return dec ? dec->channels : 0; }
int faad_get_samplerate(FaadDecoder* dec) { return dec ? dec->samplerate : 0; }
long faad_get_total_frames(FaadDecoder* dec) { return dec ? dec->total_frames : 0; }
long faad_get_current_frame(FaadDecoder* dec) { return dec ? dec->current_frame : 0; }