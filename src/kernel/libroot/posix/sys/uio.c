/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <syscalls.h>

#include <sys/uio.h>
#include <errno.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


ssize_t
readv(int fd, const struct iovec *vecs, size_t count)
{
	ssize_t bytes = _kern_readv(fd, -1, vecs, count);

	RETURN_AND_SET_ERRNO(bytes);
}


ssize_t
readv_pos(int fd, off_t pos, const struct iovec *vecs, size_t count)
{
	ssize_t bytes = _kern_readv(fd, pos, vecs, count);

	RETURN_AND_SET_ERRNO(bytes);
}


ssize_t
writev(int fd, const struct iovec *vecs, size_t count)
{
	ssize_t bytes = _kern_writev(fd, -1, vecs, count);

	RETURN_AND_SET_ERRNO(bytes);
}


ssize_t
writev_pos(int fd, off_t pos, const struct iovec *vecs, size_t count)
{
	ssize_t bytes = _kern_writev(fd, pos, vecs, count);

	RETURN_AND_SET_ERRNO(bytes);
}

