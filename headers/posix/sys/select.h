#ifndef _SYS_SELECT_H
#define _SYS_SELECT_H
/* 
** Distributed under the terms of the OpenBeOS License.
*/

#include <sys/time.h>


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

/* Compatibily only: use FD_SETSIZE instead */
#ifndef FDSETSIZE
#	define FDSETSIZE FD_SETSIZE
#endif

/* compatibility with BSD */
#define NBBY	8	/* number of bits in a byte */

typedef unsigned long fd_mask;

#ifndef howmany
#	define howmany(x, y) (((x) + ((y) - 1)) / (y))
#endif

#define NFDBITS (sizeof(fd_mask) * NBBY)

typedef struct fd_set {
	fd_mask bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;

#define _FD_BITSINDEX(fd) ((fd) / NFDBITS)
#define _FD_BIT(fd) (1L << ((fd) % NFDBITS))

#define FD_ZERO(set) memset((set), 0, sizeof(fd_set))
#define FD_SET(fd, set) ((set)->bits[_FD_BITSINDEX(fd)] |= _FD_BIT(fd))
#define FD_CLR(fd, set) ((set)->bits[_FD_BITSINDEX(fd)] &= ~_FD_BIT(fd))
#define FD_ISSET(fd, set) ((set)->bits[_FD_BITSINDEX(fd)] & _FD_BIT(fd))

#endif	/* FD_SET */

extern
#ifdef __cplusplus
"C"
#endif
int select(int nbits, struct fd_set *rbits, struct fd_set *wbits, 
		struct fd_set *ebits, struct timeval *timeout);

#endif	/* _SYS_SELECT_H */
