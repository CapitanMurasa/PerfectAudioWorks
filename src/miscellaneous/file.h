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


    unsigned char* cover_image;
    size_t cover_size;

} FileInfo;

#ifdef __cplusplus
extern "C" {
#endif

    const char* get_file_format(const char* filename);

    int get_metadata(const char* filename, FileInfo* info);


    void FileInfo_cleanup(FileInfo* info);

#ifdef __cplusplus
}
#endif

#endif 