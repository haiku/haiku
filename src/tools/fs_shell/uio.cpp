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

#if (!defined(__BEOS__) && !defined(__HAIKU__))
	// Defined in libroot_build.so.
#	define _kern_readv	_kernbuild_readv
#	define _kern_writev	_kernbuild_writev
	extern "C" ssize_t _kern_readv(int fd, off_t pos, const struct iovec *vecs, size_t count);
	extern "C" ssize_t _kern_writev(int fd, off_t pos, const struct iovec *vecs, size_t count);
#endif


static const int kMaxIOVecs = 1024;


bool
prepare_iovecs(const struct fssh_iovec *vecs, int count,
	struct iovec* systemVecs)
{
	if (count < 0 || count > kMaxIOVecs) {
		errno = B_BAD_VALUE;
		return false;
	}

	for (int i = 0; i < count; i++) {
		systemVecs[i].iov_base = vecs[i].iov_base;
		systemVecs[i].iov_len = vecs[i].iov_len;
	}

	return true;
}


fssh_ssize_t
fssh_readv(int fd, const struct fssh_iovec *vector, int count)
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
		return _kern_readv(fd, lseek(fd, 0, SEEK_CUR), systemVecs, count);
	#endif
}


fssh_ssize_t
fssh_readv_pos(int fd, fssh_off_t pos, const struct fssh_iovec *vec, int count)
{
	struct iovec systemVecs[kMaxIOVecs];
	if (!prepare_iovecs(vec, count, systemVecs))
		return -1;

	fssh_size_t length = 0;
	if (FSShell::restricted_file_restrict_io(fd, pos, length) < 0)
		return -1;

#if defined(__HAIKU__)
	return readv_pos(fd, pos, systemVecs, count);
#else
	return _kern_readv(fd, pos, systemVecs, count);
#endif
}


fssh_ssize_t
fssh_writev(int fd, const struct fssh_iovec *vector, int count)
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
		return _kern_writev(fd, lseek(fd, 0, SEEK_CUR), systemVecs, count);
	#endif
}


fssh_ssize_t
fssh_writev_pos(int fd, fssh_off_t pos, const struct fssh_iovec *vec, int count)
{
	struct iovec systemVecs[kMaxIOVecs];
	if (!prepare_iovecs(vec, count, systemVecs))
		return -1;

	fssh_size_t length = 0;
	if (FSShell::restricted_file_restrict_io(fd, pos, length) < 0)
		return -1;

#if defined(__HAIKU__)
	return writev_pos(fd, pos, systemVecs, count);
#else
	return _kern_writev(fd, pos, systemVecs, count);
#endif
}
