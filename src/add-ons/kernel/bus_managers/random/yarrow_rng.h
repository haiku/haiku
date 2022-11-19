/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@berlios.de
 */
#ifndef _YARROW_RNG_H
#define _YARROW_RNG_H


#include <OS.h>

#include "random.h"


#ifdef __cplusplus
extern "C" {
#endif

status_t yarrow_init();
void yarrow_uninit();
void yarrow_enqueue_randomness(const uint64 value);

status_t yarrow_rng_read(void* cookie, void *_buffer, size_t *_numBytes);
status_t yarrow_rng_write(void* cookie, const void *_buffer, size_t *_numBytes);


#define RANDOM_INIT yarrow_init
#define RANDOM_UNINIT yarrow_uninit
#define RANDOM_ENQUEUE yarrow_enqueue_randomness
#define RANDOM_READ yarrow_rng_read
#define RANDOM_WRITE yarrow_rng_write


#ifdef __cplusplus
}
#endif


#endif /* _YARROW_RNG_H */
