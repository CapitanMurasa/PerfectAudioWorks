#include "file.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// Include the new cross-platform definitions header to resolve linker errors for
// ntohl, le32toh, and strncasecmp on Windows (MSVC).
#include "cross_platform_defs.h"

// Helper function to read a Big-Endian 32-bit integer and convert to host byte order.
static uint32_t read_be32(FILE* f) {
    uint32_t v;
    // Check if fread succeeded to clear static analyzer warning.
    if (fread(&v, 4, 1, f) != 1) return 0;
    return ntohl(v);
}

// Helper function to read a Little-Endian 32-bit integer and convert to host byte order.
static uint32_t read_le32(FILE* f) {
    uint32_t v;
    // Check if fread succeeded to clear static analyzer warning.
    if (fread(&v, 4, 1, f) != 1) return 0;
    return le32toh(v);
}

// NOTE: FileInfo struct is assumed to be defined in file.h
#define INFO_STRING_MAX_LEN 256 // Assuming a common max length for safety

const char* get_file_format(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return "Unknown";

    unsigned char buffer[64];
    // Added safety check for fread result
    size_t n = fread(buffer, 1, sizeof(buffer), file);

    fclose(file);

    if (n >= 12) {
        if (memcmp(buffer, "RIFF", 4) == 0) return "WAV";
        if (memcmp(buffer, "fLaC", 4) == 0) return "FLAC";
        if (memcmp(buffer, "OggS", 4) == 0) return "OGG";
        if (memcmp(buffer, "ID3", 3) == 0) return "MP3";
        if ((buffer[0] == 0xFF) && ((buffer[1] & 0xE0) == 0xE0)) return "MP3";
        if (memcmp(buffer, "MThd", 4) == 0) return "MIDI";
        if (memcmp(buffer, "OpusHead", 8) == 0) return "OPUS";
        if (memcmp(buffer + 28, "OpusHead", 8) == 0) return "OPUS";
        if (memcmp(buffer, "\x30\x26\xB2\x75", 4) == 0) return "WMA";
        if (memcmp(buffer + 4, "ftypM4A", 7) == 0 || memcmp(buffer + 4, "ftypisom", 8) == 0) return "M4A";
    }
    return "Unknown";
}

int get_metadata(const char* filename, FileInfo* info) {
    memset(info, 0, sizeof(FileInfo));
    const char* type = get_file_format(filename);

    // Assuming FileInfo.format is large enough (e.g., 8 bytes)
    strncpy(info->format, type, sizeof(info->format) - 1);
    info->format[sizeof(info->format) - 1] = '\0';

    FILE* f = fopen(filename, "rb");
    if (!f) return -1;

    // --- MP3 Parsing ---
    if (strcmp(type, "MP3") == 0) {
        unsigned char hdr[10];
        if (fread(hdr, 1, 10, f) == 10) { // Check for successful read
            if (memcmp(hdr, "ID3", 3) == 0) {
                int size = ((hdr[6] & 0x7F) << 21) | ((hdr[7] & 0x7F) << 14) | ((hdr[8] & 0x7F) << 7) | (hdr[9] & 0x7F);

                unsigned char* data = malloc(size);
                if (data && fread(data, 1, size, f) == size) { // Check malloc and fread
                    int pos = 0;
                    while (pos + 10 < size) {
                        char fid[5] = { 0 };
                        memcpy(fid, data + pos, 4);

                        int fsize = (data[pos + 4] << 24) | (data[pos + 5] << 16) | (data[pos + 6] << 8) | data[pos + 7];

                        if (fsize <= 0 || pos + 10 + fsize > size) break;

                        // Safely copy string data, limiting size to the field's max length
                        if (strcmp(fid, "TIT2") == 0) strncpy(info->title, (char*)data + pos + 10, fsize > INFO_STRING_MAX_LEN ? INFO_STRING_MAX_LEN : fsize);
                        if (strcmp(fid, "TPE1") == 0) strncpy(info->artist, (char*)data + pos + 10, fsize > INFO_STRING_MAX_LEN ? INFO_STRING_MAX_LEN : fsize);
                        if (strcmp(fid, "TALB") == 0) strncpy(info->album, (char*)data + pos + 10, fsize > INFO_STRING_MAX_LEN ? INFO_STRING_MAX_LEN : fsize);
                        if (strcmp(fid, "TCON") == 0) strncpy(info->genre, (char*)data + pos + 10, fsize > INFO_STRING_MAX_LEN ? INFO_STRING_MAX_LEN : fsize);

                        // Cover image logic (retained original logic)
                        if (strcmp(fid, "APIC") == 0) {
                            int img_size = fsize - 11;
                            if (img_size > 0) {
                                unsigned char* img = malloc(img_size);
                                if (img) {
                                    memcpy(img, data + pos + 11, img_size);
                                    info->cover_image = img;
                                    info->cover_size = img_size;
                                }
                                else {
                                    fprintf(stderr, "Warning: Failed to allocate memory for cover image.\n");
                                }
                            }
                        }
                        pos += 10 + fsize;
                    }
                }
                free(data);

                // Ensure null termination for all fields after ID3v2 parsing
                info->title[INFO_STRING_MAX_LEN] = '\0';
                info->artist[INFO_STRING_MAX_LEN] = '\0';
                info->album[INFO_STRING_MAX_LEN] = '\0';
                info->genre[INFO_STRING_MAX_LEN] = '\0';
            }
        }

        // ID3v1 (TAG) parsing
        if (fseek(f, -128, SEEK_END) == 0) {
            unsigned char tag[128];
            if (fread(tag, 1, 128, f) == 128 && memcmp(tag, "TAG", 3) == 0) {
                // Only copy if ID3v2 didn't populate the field
                if (info->title[0] == '\0') strncpy(info->title, (char*)tag + 3, 30);
                if (info->artist[0] == '\0') strncpy(info->artist, (char*)tag + 33, 30);
                if (info->album[0] == '\0') strncpy(info->album, (char*)tag + 63, 30);

                // Ensure null termination after ID3v1 parsing
                info->title[30] = '\0'; info->artist[30] = '\0'; info->album[30] = '\0';
            }
        }

        // --- FLAC Parsing (VORBIS_COMMENT) ---
    }
    else if (strcmp(type, "FLAC") == 0) {
        char sig[4];
        if (fread(sig, 1, 4, f) != 4) goto end_flac_parsing; // Skip 4 bytes of fLaC signature

        while (!feof(f)) {
            unsigned char hdr[4];
            if (fread(hdr, 1, 4, f) != 4) break;

            int last = hdr[0] & 0x80;
            int btype = hdr[0] & 0x7F;
            int size = (hdr[1] << 16) | (hdr[2] << 8) | hdr[3];

            if (btype == 4) { // VORBIS_COMMENT
                uint32_t vlen = read_le32(f);
                fseek(f, vlen, SEEK_CUR); // Skip vendor string length and string

                uint32_t count = read_le32(f);
                for (uint32_t i = 0; i < count; i++) {
                    uint32_t slen = read_le32(f);

                    if (slen > 0) { // Guard against zero length to clear static analyzer warnings
                        char* entry = malloc(slen + 1);
                        if (entry) {
                            if (fread(entry, 1, slen, f) == slen) {
                                entry[slen] = 0; // Null terminate

                                // Use the cross-platform strncasecmp
                                if (strncasecmp(entry, "TITLE=", 6) == 0) strncpy(info->title, entry + 6, INFO_STRING_MAX_LEN);
                                else if (strncasecmp(entry, "ARTIST=", 7) == 0) strncpy(info->artist, entry + 7, INFO_STRING_MAX_LEN);
                                else if (strncasecmp(entry, "ALBUM=", 6) == 0) strncpy(info->album, entry + 6, INFO_STRING_MAX_LEN);

                                info->title[INFO_STRING_MAX_LEN] = '\0'; // Ensure null termination
                                info->artist[INFO_STRING_MAX_LEN] = '\0';
                                info->album[INFO_STRING_MAX_LEN] = '\0';
                            }
                            else {
                                fprintf(stderr, "Warning: Failed to read FLAC comment entry.\n");
                            }
                            free(entry);
                        }
                        else {
                            fprintf(stderr, "Warning: Failed to allocate memory for FLAC comment entry.\n");
                            fseek(f, slen, SEEK_CUR); // Skip data if allocation failed
                        }
                    }
                }
                break;
            }
            else {
                fseek(f, size, SEEK_CUR);
            }
            if (last) break;
        }
    end_flac_parsing:; // Label for jump/break

        // --- WAV Parsing (INFO LIST) ---
    }
    else if (strcmp(type, "WAV") == 0) {
        // Existing WAV parsing logic (simple checks retained)
        // Note: size conversions may be needed for true cross-platform safety, 
        // but sticking close to original code logic for now.
        fseek(f, 12, SEEK_SET); // Skip RIFF header and size

        char chunk[4];
        uint32_t size;

        while (fread(chunk, 1, 4, f) == 4) {
            if (fread(&size, 4, 1, f) != 1) break;

            if (memcmp(chunk, "LIST", 4) == 0) {
                char type[4];
                if (fread(type, 1, 4, f) != 4) break;

                if (memcmp(type, "INFO", 4) == 0) {
                    uint32_t list_end = ftell(f) + size - 4; // LIST chunk size includes INFO/fmt tag

                    while (ftell(f) < list_end) {
                        char sub[4];
                        uint32_t subsize;

                        if (fread(sub, 1, 4, f) != 4) break;
                        if (fread(&subsize, 4, 1, f) != 1) break;

                        // Safety check on size
                        if (subsize > 0 && ftell(f) + subsize <= list_end) {
                            if (memcmp(sub, "INAM", 4) == 0) {
                                fread(info->title, 1, subsize > INFO_STRING_MAX_LEN ? INFO_STRING_MAX_LEN : subsize, f);
                                info->title[subsize > INFO_STRING_MAX_LEN ? INFO_STRING_MAX_LEN : subsize] = '\0';
                            }
                            else if (memcmp(sub, "IART", 4) == 0) {
                                fread(info->artist, 1, subsize > INFO_STRING_MAX_LEN ? INFO_STRING_MAX_LEN : subsize, f);
                                info->artist[subsize > INFO_STRING_MAX_LEN ? INFO_STRING_MAX_LEN : subsize] = '\0';
                            }
                            else {
                                fseek(f, subsize, SEEK_CUR); // Skip unknown sub-chunk
                            }
                        }
                        else {
                            fseek(f, size, SEEK_CUR); // Skip remaining LIST chunk if parsing failed
                            break;
                        }
                    }
                    fseek(f, list_end, SEEK_SET); // Ensure we are at the end of the LIST chunk
                }
                else {
                    fseek(f, size - 4, SEEK_CUR); // Skip to the next chunk (LIST includes 'INFO' header in its size)
                }
            }
            else {
                fseek(f, size, SEEK_CUR); // Skip to the next chunk
            }
        }

        // --- M4A Parsing ---
    }
    else if (strcmp(type, "M4A") == 0) {
        while (!feof(f)) {
            uint32_t sz; char tp[5] = { 0 };
            if (fread(&sz, 4, 1, f) != 1) break;
            if (fread(tp, 4, 1, f) != 1) break;

            sz = ntohl(sz);

            // Note: M4A strings are complex (atom headers + data size offset). This assumes simple UTF-8 text.
            // Using sz-8 to account for size and type atoms, which may not be correct for all atoms.
            size_t data_len = sz > 8 ? sz - 8 : 0;

            if (data_len > 0) {
                if (strcmp(tp, "©nam") == 0) {
                    fread(info->title, 1, data_len > INFO_STRING_MAX_LEN ? INFO_STRING_MAX_LEN : data_len, f);
                    info->title[data_len > INFO_STRING_MAX_LEN ? INFO_STRING_MAX_LEN : data_len] = '\0';
                }
                else if (strcmp(tp, "©ART") == 0) {
                    fread(info->artist, 1, data_len > INFO_STRING_MAX_LEN ? INFO_STRING_MAX_LEN : data_len, f);
                    info->artist[data_len > INFO_STRING_MAX_LEN ? INFO_STRING_MAX_LEN : data_len] = '\0';
                }
                else if (strcmp(tp, "©alb") == 0) {
                    fread(info->album, 1, data_len > INFO_STRING_MAX_LEN ? INFO_STRING_MAX_LEN : data_len, f);
                    info->album[data_len > INFO_STRING_MAX_LEN ? INFO_STRING_MAX_LEN : data_len] = '\0';
                }
                else {
                    fseek(f, data_len, SEEK_CUR);
                }
            }
            else {
                // Should not happen if size is valid, but prevents infinite loop if sz=8
                break;
            }
        }
    }

    fclose(f);
    return 0;
}