#include "libsndfiledecoder.h"
#include <stdio.h>

// Note: The cross-platform include block for unistd.h/pthread.h was removed from this file 
// because those headers and functions were not used here. This resolves the 
// "cannot open source file 'pthread.h'" error for this specific file.

/**
 * @brief Opens an audio file using libsndfile.
 * @param filename The path to the audio file.
 * @return A pointer to the SndFileDecoder struct, or NULL on failure.
 */
SndFileDecoder* sndfile_open(const char* filename) {
    SndFileDecoder* decoder = (SndFileDecoder*)malloc(sizeof(SndFileDecoder));
    if (!decoder) {
        return NULL;
    }

    decoder->info.format = 0; 
    decoder->file = sf_open(filename, SFM_READ, &decoder->info);

    if (decoder->file == NULL) {
        // sf_open failed
        fprintf(stderr, "Error opening file %s: %s\n", filename, sf_strerror(NULL));
        free(decoder);
        return NULL;
    }

    decoder->current_frame = 0;
    return decoder;
}

/**
 * @brief Reads frames of audio data into a float buffer.
 * @param decoder The decoder instance.
 * @param buffer The destination buffer.
 * @param frames The number of frames to read.
 * @return The actual number of frames read.
 */
sf_count_t sndfile_read_float(SndFileDecoder* decoder, float* buffer, sf_count_t frames) {
    if (!decoder || !decoder->file) return 0;
    sf_count_t read_frames = sf_readf_float(decoder->file, buffer, frames);
    decoder->current_frame += read_frames;
    return read_frames;
}

/**
 * @brief Seeks to a specific frame position in the file.
 * @param decoder The decoder instance.
 * @param frame The frame index to seek to.
 * @return The new current frame position.
 */
sf_count_t sndfile_seek(SndFileDecoder* decoder, sf_count_t frame) {
    if (!decoder || !decoder->file) return 0;
    sf_count_t result = sf_seek(decoder->file, frame, SEEK_SET);
    if (result >= 0) {
        decoder->current_frame = result;
    }
    return decoder->current_frame;
}

/**
 * @brief Gets the current frame position.
 */
sf_count_t sndfile_get_current_frame(SndFileDecoder* decoder) {
    if (!decoder) return 0;
    return decoder->current_frame;
}

/**
 * @brief Gets the total number of frames in the file.
 */
sf_count_t sndfile_get_total_frames(SndFileDecoder* decoder) {
    if (!decoder) return 0;
    return decoder->info.frames;
}

/**
 * @brief Gets the number of channels.
 */
int sndfile_get_channels(SndFileDecoder* decoder) {
    if (!decoder) return 0;
    return decoder->info.channels;
}

/**
 * @brief Gets the sample rate of the file.
 */
int sndfile_get_samplerate(SndFileDecoder* decoder) {
    if (!decoder) return 0;
    return decoder->info.samplerate;
}

/**
 * @brief Closes the audio file and frees the decoder memory.
 */
void sndfile_close(SndFileDecoder* decoder) {
    if (decoder) {
        if (decoder->file) {
            sf_close(decoder->file);
        }
        free(decoder);
    }
}