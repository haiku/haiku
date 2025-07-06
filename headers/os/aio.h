/*
 * Copyright 2023 Jules ALTIS. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OS_AIO_H
#define _OS_AIO_H

#include <sys/types.h>
#include <signal.h> // For sigevent
#include <time.h>   // For timespec

#ifdef __cplusplus
extern "C" {
#endif

// Asynchronous I/O Control Block
struct aiocb {
	int             aio_fildes;     /* File descriptor */
	off_t           aio_offset;     /* File offset for I/O */
	volatile void*  aio_buf;        /* Location of buffer */
	size_t          aio_nbytes;     /* Length of transfer */
	int             aio_reqprio;    /* Request priority offset (unused in Haiku for now) */
	struct sigevent aio_sigevent;   /* Signal number and value */
	int             aio_lio_opcode; /* Operation to be performed (for lio_listio) */

	// Haiku specific or internal fields might be needed by the kernel,
	// but should not be directly manipulated by the user.
	// For userspace, this struct should match POSIX.
	// Kernel might use a more extended internal version.
	void*			_reserved[4];	/* Reserved for future use/internal state */
};

// Return values for aio_cancel()
#define AIO_CANCELED    0x1
#define AIO_NOTCANCELED 0x2
#define AIO_ALLDONE     0x3

// Operation codes for lio_listio()
#define LIO_READ        0x1
#define LIO_WRITE       0x2
#define LIO_NOP         0x0

// Synchronization options for lio_listio()
#define LIO_WAIT        0x1
#define LIO_NOWAIT      0x0

// Function prototypes
int      aio_read(struct aiocb *aiocbp);
int      aio_write(struct aiocb *aiocbp);
int      aio_fsync(int op, struct aiocb *aiocbp);
int      aio_error(const struct aiocb *aiocbp);
ssize_t  aio_return(struct aiocb *aiocbp);
int      aio_suspend(const struct aiocb * const list[], int nent,
             const struct timespec *timeout);
int      aio_cancel(int fildes, struct aiocb *aiocbp);
int      lio_listio(int mode, struct aiocb * const list[], int nent,
             struct sigevent *sig);

#ifdef __cplusplus
}
#endif

#endif /* _OS_AIO_H */
