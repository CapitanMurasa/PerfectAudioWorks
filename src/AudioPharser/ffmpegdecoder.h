#ifndef FFMPEGDECODER_H
#define FFMPEGDECODER_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        AVFormatContext* fmt_ctx;
        AVCodecContext* codec_ctx;
        int stream_index;

        SwrContext* swr_ctx;

        AVFrame* frame;
        AVPacket* pkt;

        uint8_t* output_buffer;
        int output_buffer_size;
        int output_buffer_pos;
        int output_buffer_len; 

        int64_t current_pts;
        long total_frames;
        int channels;
        int samplerate;

    } FFmpegDecoder;

    FFmpegDecoder* ffmpeg_open(const char* filename);

    long ffmpeg_read_float(FFmpegDecoder* dec, float* buffer, int frames);

    long ffmpeg_seek(FFmpegDecoder* dec, long frame);
    void ffmpeg_close(FFmpegDecoder* dec);

    int ffmpeg_get_channels(FFmpegDecoder* dec);
    int ffmpeg_get_samplerate(FFmpegDecoder* dec);
    long ffmpeg_get_total_frames(FFmpegDecoder* dec);
    long ffmpeg_get_current_frame(FFmpegDecoder* dec);

#ifdef __cplusplus
}
#endif

#endif // FFMPEGDECODER_H