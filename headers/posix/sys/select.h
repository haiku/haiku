/*
 * Copyright 2002-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYS_SELECT_H
#define _SYS_SELECT_H


#include <config/types.h>

#include <sys/time.h>
#include <signal.h>


/* If FD_SET is already defined, only the select() prototype is
 * exported in this header.
 */
#ifndef FD_SET

/* You can define your own FDSETSIZE if you need more bits - but
 * it should be enough for most uses.
 */
#ifndef FD_SETSIZE
#	define FD_SETSIZE 1024
#endif

typedef __haiku_uint32 fd_mask;

#ifndef _howmany
#	define _howmany(x, y) (((x) + ((y) - 1)) / (y))
#endif

#ifndef howmany
#	define howmany(x, y) (((x) + ((y) - 1)) / (y))
#endif

#define NFDBITS (sizeof(fd_mask) * 8)	/* bits per mask */

typedef struct fd_set {
	fd_mask bits[_howmany(FD_SETSIZE, NFDBITS)];
} fd_set;

#define _FD_BITSINDEX(fd) ((fd) / NFDBITS)
#define _FD_BIT(fd) (1L << ((fd) % NFDBITS))

#define FD_ZERO(set) memset((set), 0, sizeof(fd_set))
#define FD_SET(fd, set) ((set)->bits[_FD_BITSINDEX(fd)] |= _FD_BIT(fd))
#define FD_CLR(fd, set) ((set)->bits[_FD_BITSINDEX(fd)] &= ~_FD_BIT(fd))
#define FD_ISSET(fd, set) ((set)->bits[_FD_BITSINDEX(fd)] & _FD_BIT(fd))
#define FD_COPY(source, target) (*(target) = *(source))

#endif	/* FD_SET */

#ifdef __cplusplus
extern "C" {
#endif

extern int pselect(int numBits, struct fd_set *readBits, struct fd_set *writeBits,
			struct fd_set *errorBits, const struct timespec *timeout, const sigset_t *sigMask);
extern int select(int numBits, struct fd_set *readBits, struct fd_set *writeBits,
			struct fd_set *errorBits, struct timeval *timeout);

#ifdef __cplusplus
}
#endif

#endif	/* _SYS_SELECT_H */
