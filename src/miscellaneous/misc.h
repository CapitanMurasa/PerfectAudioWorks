#ifndef MISC_H
#define MISC_H

#include <stddef.h> // <--- Add this to define wchar_t
#include <stdlib.h> // Good to have here for size_t

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

int clamp_int(int value, int min_val, int max_val);
float clamp_float(float value, float min_val, float max_val);

#ifdef _WIN32
char* wchar_to_char_alloc(const wchar_t* wstr);
#endif

#ifdef __cplusplus
}
#endif

#endif