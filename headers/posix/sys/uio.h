/* uio.h */

#ifndef _SYS_UIO_H
#define _SYS_UIO_H

#include <ktypes.h>

typedef struct iovec {
	void  *iov_base;
	size_t iov_len;
} iovec;


#ifdef _KERNEL_MODE
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

int uiomove(char *cp, int n, struct uio *uio);
#endif

ssize_t readv(int fd, const struct iovec *vector, size_t count);
ssize_t readv_pos(int fd, off_t pos, const struct iovec *vec, size_t count);
ssize_t writev(int fd, const struct iovec *vector, size_t count);
ssize_t writev_pos(int fd, off_t pos, const struct iovec *vec, size_t count);

#endif /* _SYS_UIO_H */
