/* uio.h */

#ifndef NET_SYS_UIO_H
#define NET_SYS_UIO_H

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

int uiomove(caddr_t cp, int n, struct uio *uio);

#endif /* NET_SYS_UIO_H */
