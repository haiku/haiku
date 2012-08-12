/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FS_FREEBSD_H
#define FS_FREEBSD_H


#include <sys/uio.h>


ssize_t haiku_freebsd_read(int fd, void *buf, size_t nbytes);
ssize_t haiku_freebsd_write(int fd, const void *buf, size_t nbytes);
ssize_t haiku_freebsd_readv(int fd, const iovec *vecs, size_t count);
ssize_t haiku_freebsd_writev(int fd, const struct iovec *vecs, size_t count);

#endif	/* FS_FREEBSD_H */
