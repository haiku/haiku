#ifndef _SYS_UIO_H
#define _SYS_UIO_H
/* 
** Distributed under the terms of the Haiku License.
*/


#include <sys/types.h>


typedef struct iovec {
	void  *iov_base;
	size_t iov_len;
} iovec;


enum    uio_rw { UIO_READ, UIO_WRITE };

/* Segment flag values. */
enum uio_seg {
	UIO_USERSPACE,		/* from user data space */
	UIO_SYSSPACE		/* from system space */
};

struct uio {
	struct  iovec *uio_iov;		/* pointer to array of iovecs */
	int     uio_iovcnt;			/* number of iovecs in array */
	off_t   uio_offset;			/* offset into file this uio corresponds to */
	size_t  uio_resid;			/* residual i/o count */
	enum    uio_seg uio_segflg;	/* see above */
	enum    uio_rw uio_rw;		/* see above */
//	struct  proc *uio_procp;	/* process if UIO_USERSPACE */
};


#ifdef __cplusplus
extern "C" {
#endif

ssize_t readv(int fd, const struct iovec *vector, size_t count);
ssize_t readv_pos(int fd, off_t pos, const struct iovec *vec, size_t count);
ssize_t writev(int fd, const struct iovec *vector, size_t count);
ssize_t writev_pos(int fd, off_t pos, const struct iovec *vec, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_UIO_H */
