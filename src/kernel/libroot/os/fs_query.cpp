/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <fs_query.h>

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "syscalls.h"

#define RETURN_AND_SET_ERRNO(status) \
	{ \
		if (status < 0) { \
			errno = status; \
			return -1; \
		} \
		return status; \
	}

// for the DIR structure
#define BUFFER_SIZE 2048


DIR *
fs_open_query(dev_t device, const char *query, uint32 flags)
{
	// check parameters
	if (device < 0 || !query) {
		errno = B_BAD_VALUE;
		return NULL;
	}
	// open
	int fd = _kern_open_query(device, query, flags, -1, -1);
	if (fd < 0) {
		errno = fd;
		return NULL;
	}
	// allocate a DIR
	DIR *dir = (DIR*)malloc(BUFFER_SIZE);
	if (!dir) {
		_kern_close(fd);
		errno = B_NO_MEMORY;
		return NULL;
	}
	dir->fd = fd;
	return dir;
}


DIR *
fs_open_live_query(dev_t device, const char *query,
	uint32 flags, port_id port, int32 token)
{
	// check parameters
	if (device < 0 || !query || port < 0) {
		errno = B_BAD_VALUE;
		return NULL;
	}
	flags |= B_LIVE_QUERY;
	// open
	int fd = _kern_open_query(device, query, flags, port, token);
	if (fd < 0) {
		errno = fd;
		return NULL;
	}
	// allocate a DIR
	DIR *dir = (DIR*)malloc(BUFFER_SIZE);
	if (!dir) {
		_kern_close(fd);
		errno = B_NO_MEMORY;
		return NULL;
	}
	dir->fd = fd;
	return dir;
}


int
fs_close_query(DIR *d)
{
	if (!d)
		RETURN_AND_SET_ERRNO(B_BAD_VALUE);
	int fd = d->fd;
	free(d);
	return _kern_close(fd);
}


struct dirent *
fs_read_query(DIR *d)
{
	// check parameters
	if (!d) {
		errno = B_BAD_VALUE;
		return NULL;
	}
	// read
	int32 bufferSize = BUFFER_SIZE - ((uint8*)&d->ent - (uint8*)d);
	ssize_t result = _kern_read_dir(d->fd, &d->ent, bufferSize, 1);
	if (result < 0) {
		errno = result;
		return NULL;
	}
	if (result == 0) {
		errno = B_ENTRY_NOT_FOUND;
		return NULL;
	}
	return &d->ent;
}


status_t
get_path_for_dirent(struct dirent *dent, char *buf, size_t len)
{
	if (!dent || !buf)
		return B_BAD_VALUE;
	return _kern_entry_ref_to_path(dent->d_pdev, dent->d_pino, dent->d_name,
		buf, len);
}

