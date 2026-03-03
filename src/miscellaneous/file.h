#ifndef FILE_H
#define FILE_H

#include <stddef.h> 
#define INFO_STRING_MAX_LEN 256
#define INFO_BUFFER_SIZE (INFO_STRING_MAX_LEN + 1) 

typedef struct {
    char format[32];
    char title[INFO_BUFFER_SIZE];
    char artist[INFO_BUFFER_SIZE];
    char album[INFO_BUFFER_SIZE];
    char genre[INFO_BUFFER_SIZE];

    int sampleRate;
    int channels;
    int bitrate;
    int length;
    int durationSeconds;


#if _WIN32
    const wchar_t* filename;
#else
    const char* filename;
#endif

    unsigned char* cover_image;
    size_t cover_size;

} FileInfo;

#ifdef __cplusplus
extern "C" {
#endif

    extern bool CanItUseExternalAlbumart;
    void setCanUseExternalAlbumart(bool value);
    const char* get_file_format(const char* filename);

    #ifdef _WIN32
    #include <wchar.h>
    int get_metadata_w(const wchar_t* filename_w, FileInfo* info);
    #else
    int get_metadata(const char* filename, FileInfo* info);
    #endif

    void FileInfo_cleanup(FileInfo* info);

#ifdef __cplusplus
}
#endif

#endif