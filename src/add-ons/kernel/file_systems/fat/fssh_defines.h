/*
 * Copyright 2001-2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FAT_FSSH_DEFINES_H
#define FAT_FSSH_DEFINES_H


// Macros used by the driver and not provided by the FS shell interface.

#include <assert.h>


#ifndef LONGLONG_MAX
#define LONGLONG_MAX LLONG_MAX
#endif

#ifdef B_USE_POSITIVE_POSIX_ERRORS
#define B_TO_POSIX_ERROR(error) (-(error))
#define B_FROM_POSIX_ERROR(error) (-(error))
#else
#define B_TO_POSIX_ERROR(error) (error)
#define B_FROM_POSIX_ERROR(error) (error)
#endif // B_USE_POSITIVE_POSIX_ERRORS

#define howmany(x, y) (((x) + ((y) - 1)) / (y))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#ifndef __cplusplus
#ifndef min
#define min(a, b) ((a) > (b) ? (b) : (a))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#endif // __cplusplus

#ifndef MAXNAMLEN
#ifdef NAME_MAX
#define MAXNAMLEN NAME_MAX
#else
#define MAXNAMLEN 256
#endif // NAME_MAX
#endif // MAXNAMLEN

#define ASSERT(x) assert(x)

#endif // FAT_FSSH_DEFINES_H
