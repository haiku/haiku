#ifndef _AUDIO_CONVERSION_H
#define _AUDIO_CONVERSION_H

#include <SupportDefs.h>

void uint8_to_uint8(void *dst, void *src, int32 count);
void uint8_to_int8(void *dst, void *src, int32 count);
void uint8_to_int16(void *dst, void *src, int32 count);
void uint8_to_int32(void *dst, void *src, int32 count);
void uint8_to_float32(void *dst, void *src, int32 count);

void int8_to_uint8(void *dst, void *src, int32 count);
void int8_to_int8(void *dst, void *src, int32 count);
void int8_to_int16(void *dst, void *src, int32 count);
void int8_to_int32(void *dst, void *src, int32 count);
void int8_to_float32(void *dst, void *src, int32 count);

void int16_to_uint8(void *dst, void *src, int32 count);
void int16_to_int8(void *dst, void *src, int32 count);
void int16_to_int16(void *dst, void *src, int32 count);
void int16_to_int32(void *dst, void *src, int32 count);
void int16_to_float32(void *dst, void *src, int32 count);

void int24_to_uint8(void *dst, void *src, int32 count);
void int24_to_int8(void *dst, void *src, int32 count);
void int24_to_int16(void *dst, void *src, int32 count);
void int24_to_int32(void *dst, void *src, int32 count);
void int24_to_float32(void *dst, void *src, int32 count);

void int32_to_uint8(void *dst, void *src, int32 count);
void int32_to_int8(void *dst, void *src, int32 count);
void int32_to_int16(void *dst, void *src, int32 count);
void int32_to_int32(void *dst, void *src, int32 count);
void int32_to_float32(void *dst, void *src, int32 count);

void float32_to_uint8(void *dst, void *src, int32 count);
void float32_to_int8(void *dst, void *src, int32 count);
void float32_to_int16(void *dst, void *src, int32 count);
void float32_to_int32(void *dst, void *src, int32 count);
void float32_to_float32(void *dst, void *src, int32 count);

void float64_to_uint8(void *dst, void *src, int32 count);
void float64_to_int8(void *dst, void *src, int32 count);
void float64_to_int16(void *dst, void *src, int32 count);
void float64_to_int32(void *dst, void *src, int32 count);
void float64_to_float32(void *dst, void *src, int32 count);

void swap_int16(void *data, int32 count);
void swap_int24(void *data, int32 count);
void swap_int32(void *data, int32 count);
void swap_float32(void *data, int32 count);
void swap_float64(void *data, int32 count);

#endif // _AUDIO_CONVERSION_H
