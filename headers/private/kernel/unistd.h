/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

/**
 * @file unistd.h
 * @brief Miscellaneous macro's and functions
 */

#ifndef _UNISTD_H
#define _UNISTD_H

/**
 * @defgroup Unistd unistd.h
 * @brief Miscellaneous macro's and functions
 * @ingroup OpenBeOS_POSIX
 * @{
 */

#include <ktypes.h>

#ifdef __cplusplus
extern "C" {
#endif

int     close(int);
int     dup(int);
int     dup2(int, int);

ssize_t read(int, void *, size_t);
ssize_t write(int, const void *, size_t);

int     getopt(int, char * const *, const char *);
extern	 char *optarg;			/* getopt(3) external variables */
extern	 int opterr;
extern	 int optind;
extern	 int optopt;
extern	 int optreset;

off_t   lseek(int, off_t, int);
ssize_t read(int, void *, size_t);
ssize_t pread(int, void *, size_t, off_t);
ssize_t write(int, void const*, size_t);
ssize_t pwrite(int, void const*, size_t, off_t);


unsigned sleep(unsigned);
int      usleep(unsigned);

int getdtablesize(void);

#define STDIN_FILENO     0
#define STDOUT_FILENO    1
#define STDERR_FILENO    2

/** @} */
#ifdef __cplusplus
} /* extern "C" */
#endif


#endif
