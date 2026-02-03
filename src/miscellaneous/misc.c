#include "misc.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <stdlib.h>

int clamp_int(int value, int min_val, int max_val) {
    if (value < min_val) {
        return min_val;
    } else if (value > max_val) {
        return max_val;
    } else {
        return value;
    }
}

float clamp_float(float value, float min_val, float max_val) {
    if (value < min_val) {
        return min_val;
    } else if (value > max_val) {
        return max_val;
    } else {
        return value;
    }
}

#ifdef _WIN32
char* wchar_to_char_alloc(const wchar_t* wstr) {
    if (!wstr) return NULL;

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* strTo = (char*)malloc(size_needed);
    if (!strTo) return NULL; 

    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, strTo, size_needed, NULL, NULL);
    
    return strTo;
}
#endif

