#include "ffmpegdecoder.h"
#include <stdio.h>
#include <stdlib.h>

static int init_resampler(FFmpegDecoder* dec) {
    if (dec->swr_ctx) swr_free(&dec->swr_ctx);

    AVChannelLayout out_layout;
    if (av_channel_layout_copy(&out_layout, &dec->codec_ctx->ch_layout) < 0)
        return -1;

    int ret = swr_alloc_set_opts2(
        &dec->swr_ctx,
        &out_layout,
        AV_SAMPLE_FMT_FLT, 
        dec->codec_ctx->sample_rate,
        &dec->codec_ctx->ch_layout,
        dec->codec_ctx->sample_fmt,
        dec->codec_ctx->sample_rate,
        0, NULL);

    av_channel_layout_uninit(&out_layout);

    if (ret < 0) return -1;
    return swr_init(dec->swr_ctx);
}

FFmpegDecoder* ffmpeg_open(const char* filename) {
    FFmpegDecoder* dec = (FFmpegDecoder*)calloc(1, sizeof(FFmpegDecoder));

    if (avformat_open_input(&dec->fmt_ctx, filename, NULL, NULL) < 0) {
        ffmpeg_close(dec); return NULL;
    }

    if (avformat_find_stream_info(dec->fmt_ctx, NULL) < 0) {
        ffmpeg_close(dec); return NULL;
    }

    dec->stream_index = av_find_best_stream(dec->fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (dec->stream_index < 0) {
        ffmpeg_close(dec); return NULL;
    }

    AVStream* stream = dec->fmt_ctx->streams[dec->stream_index];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        ffmpeg_close(dec); return NULL;
    }

    dec->codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(dec->codec_ctx, stream->codecpar);

    if (avcodec_open2(dec->codec_ctx, codec, NULL) < 0) {
        ffmpeg_close(dec); return NULL;
    }

    dec->channels = dec->codec_ctx->ch_layout.nb_channels;
    dec->samplerate = dec->codec_ctx->sample_rate;

    if (stream->duration != AV_NOPTS_VALUE) {
        dec->total_frames = (long)(stream->duration * av_q2d(stream->time_base) * dec->samplerate);
    }

    dec->frame = av_frame_alloc();
    dec->pkt = av_packet_alloc();

    if (init_resampler(dec) < 0) {
        ffmpeg_close(dec); return NULL;
    }

    return dec;
}

long ffmpeg_read_float(FFmpegDecoder* dec, float* buffer, int frames) {
    if (!dec) return 0;

    int samples_filled = 0;
    int samples_needed = frames * dec->channels;

    uint8_t* dest_ptr = (uint8_t*)buffer;

    while (samples_filled < samples_needed) {
        if (dec->output_buffer && dec->output_buffer_pos < dec->output_buffer_len) {
            int available_bytes = dec->output_buffer_len - dec->output_buffer_pos;
            int needed_bytes = (samples_needed - samples_filled) * sizeof(float);

            int to_copy = (available_bytes < needed_bytes) ? available_bytes : needed_bytes;

            memcpy(dest_ptr, dec->output_buffer + dec->output_buffer_pos, to_copy);

            dest_ptr += to_copy;
            dec->output_buffer_pos += to_copy;
            samples_filled += to_copy / sizeof(float);
            continue;
        }

        int ret = avcodec_receive_frame(dec->codec_ctx, dec->frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            if (av_read_frame(dec->fmt_ctx, dec->pkt) < 0) break; 

            if (dec->pkt->stream_index == dec->stream_index) {
                avcodec_send_packet(dec->codec_ctx, dec->pkt);
            }
            av_packet_unref(dec->pkt);
            continue;
        }
        else if (ret < 0) {
            break; 
        }


        int max_samples_out = av_rescale_rnd(swr_get_delay(dec->swr_ctx, dec->samplerate) +
            dec->frame->nb_samples, dec->samplerate, dec->samplerate, AV_ROUND_UP);

        int buffer_size_needed = max_samples_out * dec->channels * sizeof(float);
        if (!dec->output_buffer || dec->output_buffer_size < buffer_size_needed) {
            free(dec->output_buffer);
            dec->output_buffer_size = buffer_size_needed * 2;
            dec->output_buffer = (uint8_t*)malloc(dec->output_buffer_size);
        }

        int samples_converted = swr_convert(dec->swr_ctx,
            &dec->output_buffer, max_samples_out,
            (const uint8_t**)dec->frame->data, dec->frame->nb_samples);

        if (samples_converted > 0) {
            dec->output_buffer_len = samples_converted * dec->channels * sizeof(float);
            dec->output_buffer_pos = 0;
            dec->current_pts = dec->frame->pts;
        }
    }

    return samples_filled / dec->channels; 
}

long ffmpeg_seek(FFmpegDecoder* dec, long frame) {
    if (!dec) return -1;

    AVStream* st = dec->fmt_ctx->streams[dec->stream_index];
    int64_t timestamp = (int64_t)((double)frame / dec->samplerate / av_q2d(st->time_base));

    if (av_seek_frame(dec->fmt_ctx, dec->stream_index, timestamp, AVSEEK_FLAG_BACKWARD) >= 0) {
        avcodec_flush_buffers(dec->codec_ctx);
        dec->output_buffer_len = 0; 
        return frame;
    }
    return -1;
}

void ffmpeg_close(FFmpegDecoder* dec) {
    if (!dec) return;
    if (dec->swr_ctx) swr_free(&dec->swr_ctx);
    if (dec->codec_ctx) avcodec_free_context(&dec->codec_ctx);
    if (dec->fmt_ctx) avformat_close_input(&dec->fmt_ctx);
    if (dec->frame) av_frame_free(&dec->frame);
    if (dec->pkt) av_packet_free(&dec->pkt);
    if (dec->output_buffer) free(dec->output_buffer);
    free(dec);
}

int ffmpeg_get_channels(FFmpegDecoder* dec) { return dec ? dec->channels : 0; }
int ffmpeg_get_samplerate(FFmpegDecoder* dec) { return dec ? dec->samplerate : 0; }
long ffmpeg_get_total_frames(FFmpegDecoder* dec) { return dec ? dec->total_frames : 0; }
long ffmpeg_get_current_frame(FFmpegDecoder* dec) {
    if (!dec || !dec->fmt_ctx) return 0;
    AVStream* st = dec->fmt_ctx->streams[dec->stream_index];
    return (long)(dec->current_pts * av_q2d(st->time_base) * dec->samplerate);
}