/*
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_SYS_UIO_H_
#define _BSD_SYS_UIO_H_


#include_next <sys/uio.h>
#include <features.h>


#ifdef _DEFAULT_SOURCE


#ifdef __cplusplus
extern "C" {
#endif


static inline ssize_t
preadv(int fd, const struct iovec *vecs, int count, off_t pos)
{
	return readv_pos(fd, pos, vecs, count);
}


static inline ssize_t
pwritev(int fd, const struct iovec *vecs, int count, off_t pos)
{
	return writev_pos(fd, pos, vecs, count);
}


#ifdef __cplusplus
}
#endif


#endif


#endif  /* _BSD_SYS_UIO_H_ */
