#ifndef MISC_H
#define MISC_H



#ifdef __cplusplus
extern "C" {
#endif

int clamp_int(int value, int min_val, int max_val);
float clamp_float(float value, float min_val, float max_val);

#ifdef __cplusplus
}
#endif

#endif