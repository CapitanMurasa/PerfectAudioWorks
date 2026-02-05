#ifndef FAADDECODER_H
#define FAADDECODER_H

#include <neaacdec.h>
#include <mp4ff.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        mp4ff_t* infile;
        int track;
        NeAACDecHandle decoder;
        unsigned long samplerate;
        unsigned char channels;
        long total_frames;
        long current_frame; 

        float* internal_buffer;
        long internal_buffer_len; 
        long internal_buffer_pos; 
        unsigned long frame_len; 
        unsigned char* mp4_buffer; 
        uint32_t mp4_buffer_size;
    } FaadDecoder;

    FaadDecoder* faad_open(const char* filename);
#ifdef _WIN32
#include <wchar.h>
    FaadDecoder* faad_open_w(const wchar_t* filename);
#endif

    long faad_read_float(FaadDecoder* dec, float* buffer, int frames);
    long faad_seek(FaadDecoder* dec, long frame);
    void faad_close(FaadDecoder* dec);

    // Getters
    int faad_get_channels(FaadDecoder* dec);
    int faad_get_samplerate(FaadDecoder* dec);
    long faad_get_total_frames(FaadDecoder* dec);
    long faad_get_current_frame(FaadDecoder* dec);

#ifdef __cplusplus
}
#endif

#endif // FAADDECODER_H