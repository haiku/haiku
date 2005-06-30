/*
 * Copyright (c) 2003, Marcus Overhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _AUDIO_CONVERSION_H
#define _AUDIO_CONVERSION_H

#include <SupportDefs.h>

void uint8_to_uint8(void *dst, const void *src, int32 count);
void uint8_to_int8(void *dst, const void *src, int32 count);
void uint8_to_int16(void *dst, const void *src, int32 count);
void uint8_to_int32(void *dst, const void *src, int32 count);
void uint8_to_float32(void *dst, const void *src, int32 count);

void int8_to_uint8(void *dst, const void *src, int32 count);
void int8_to_int8(void *dst, const void *src, int32 count);
void int8_to_int16(void *dst, const void *src, int32 count);
void int8_to_int32(void *dst, const void *src, int32 count);
void int8_to_float32(void *dst, const void *src, int32 count);

void int16_to_uint8(void *dst, const void *src, int32 count);
void int16_to_int8(void *dst, const void *src, int32 count);
void int16_to_int16(void *dst, const void *src, int32 count);
void int16_to_int32(void *dst, const void *src, int32 count);
void int16_to_float32(void *dst, const void *src, int32 count);

void int24_to_uint8(void *dst, const void *src, int32 count);
void int24_to_int8(void *dst, const void *src, int32 count);
void int24_to_int16(void *dst, const void *src, int32 count);
void int24_to_int32(void *dst, const void *src, int32 count);
void int24_to_float32(void *dst, const void *src, int32 count);

void int32_to_uint8(void *dst, const void *src, int32 count);
void int32_to_int8(void *dst, const void *src, int32 count);
void int32_to_int16(void *dst, const void *src, int32 count);
void int32_to_int32(void *dst, const void *src, int32 count);
void int32_to_float32(void *dst, const void *src, int32 count);

void float32_to_uint8(void *dst, const void *src, int32 count);
void float32_to_int8(void *dst, const void *src, int32 count);
void float32_to_int16(void *dst, const void *src, int32 count);
void float32_to_int32(void *dst, const void *src, int32 count);
void float32_to_float32(void *dst, const void *src, int32 count);

void float64_to_uint8(void *dst, const void *src, int32 count);
void float64_to_int8(void *dst, const void *src, int32 count);
void float64_to_int16(void *dst, const void *src, int32 count);
void float64_to_int32(void *dst, const void *src, int32 count);
void float64_to_float32(void *dst, const void *src, int32 count);

void swap_int16(void *data, int32 count);
void swap_int24(void *data, int32 count);
void swap_int32(void *data, int32 count);
void swap_float32(void *data, int32 count);
void swap_float64(void *data, int32 count);

#endif // _AUDIO_CONVERSION_H
