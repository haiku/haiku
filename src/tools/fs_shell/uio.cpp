/*
 * Copyright 2007-2008, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include "compatibility.h"

#include "fssh_uio.h"

#if defined(HAIKU_HOST_PLATFORM_FREEBSD)
#include <unistd.h>
#endif

#include <new>

#include <errno.h>
#include <sys/uio.h>

#include "partition_support.h"


static const fssh_size_t kMaxIOVecs = 1024;


bool
prepare_iovecs(const struct fssh_iovec *vecs, fssh_size_t count,
	struct iovec* systemVecs)
{
	if (count > kMaxIOVecs) {
		errno = B_BAD_VALUE;
		return false;
	}

	for (fssh_size_t i = 0; i < count; i++) {
		systemVecs[i].iov_base = vecs[i].iov_base;
		systemVecs[i].iov_len = vecs[i].iov_len;
	}

	return true;
}


fssh_ssize_t
fssh_readv(int fd, const struct fssh_iovec *vector, fssh_size_t count)
{
	struct iovec systemVecs[kMaxIOVecs];
	if (!prepare_iovecs(vector, count, systemVecs))
		return -1;

	fssh_off_t pos = -1;
	fssh_size_t length = 0;
	if (FSShell::restricted_file_restrict_io(fd, pos, length) < 0)
		return -1;

	#if !defined(HAIKU_HOST_PLATFORM_FREEBSD)
		return readv(fd, systemVecs, count);
	#else
		return readv_pos(fd, lseek(fd, 0, SEEK_CUR), systemVecs, count);
	#endif
}


fssh_ssize_t
fssh_readv_pos(int fd, fssh_off_t pos, const struct fssh_iovec *vec,
	fssh_size_t count)
{
	struct iovec systemVecs[kMaxIOVecs];
	if (!prepare_iovecs(vec, count, systemVecs))
		return -1;

	fssh_size_t length = 0;
	if (FSShell::restricted_file_restrict_io(fd, pos, length) < 0)
		return -1;

	return readv_pos(fd, pos, systemVecs, count);
}


fssh_ssize_t
fssh_writev(int fd, const struct fssh_iovec *vector, fssh_size_t count)
{
	struct iovec systemVecs[kMaxIOVecs];
	if (!prepare_iovecs(vector, count, systemVecs))
		return -1;

	fssh_off_t pos = -1;
	fssh_size_t length = 0;
	if (FSShell::restricted_file_restrict_io(fd, pos, length) < 0)
		return -1;

	#if !defined(HAIKU_HOST_PLATFORM_FREEBSD)
		return writev(fd, systemVecs, count);
	#else
		return writev_pos(fd, lseek(fd, 0, SEEK_CUR), systemVecs, count);
	#endif
}


fssh_ssize_t
fssh_writev_pos(int fd, fssh_off_t pos, const struct fssh_iovec *vec,
				fssh_size_t count)
{
	struct iovec systemVecs[kMaxIOVecs];
	if (!prepare_iovecs(vec, count, systemVecs))
		return -1;

	fssh_size_t length = 0;
	if (FSShell::restricted_file_restrict_io(fd, pos, length) < 0)
		return -1;

	return writev_pos(fd, pos, systemVecs, count);
}
