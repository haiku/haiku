/* 
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <fs_query.h>

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "syscalls.h"


// for the DIR structure
#define BUFFER_SIZE 2048


static DIR *
open_query_etc(dev_t device, const char *query,
	uint32 flags, port_id port, int32 token)
{
	if (device < 0 || query == NULL || query[0] == '\0') {
		errno = B_BAD_VALUE;
		return NULL;
	}

	// open
	int fd = _kern_open_query(device, query, strlen(query), flags, port, token);
	if (fd < 0) {
		errno = fd;
		return NULL;
	}

	// allocate a DIR
	DIR *dir = (DIR *)malloc(BUFFER_SIZE);
	if (!dir) {
		_kern_close(fd);
		errno = B_NO_MEMORY;
		return NULL;
	}
	dir->fd = fd;
	return dir;
}


DIR *
fs_open_query(dev_t device, const char *query, uint32 flags)
{
	return open_query_etc(device, query, flags, -1, -1);
}


DIR *
fs_open_live_query(dev_t device, const char *query,
	uint32 flags, port_id port, int32 token)
{
	// check parameters
	if (port < 0) {
		errno = B_BAD_VALUE;
		return NULL;
	}

	return open_query_etc(device, query, flags | B_LIVE_QUERY, port, token);
}


int
fs_close_query(DIR *dir)
{
	if (dir == NULL) {
		errno = B_BAD_VALUE;
		return -1;
	}

	int fd = dir->fd;
	free(dir);
	return _kern_close(fd);
}


struct dirent *
fs_read_query(DIR *dir)
{
	// check parameters
	if (dir == NULL) {
		errno = B_BAD_VALUE;
		return NULL;
	}

	// read
	int32 bufferSize = BUFFER_SIZE - ((uint8 *)&dir->ent - (uint8 *)dir);
	ssize_t result = _kern_read_dir(dir->fd, &dir->ent, bufferSize, 1);
	if (result < 0) {
		errno = result;
		return NULL;
	}
	if (result == 0) {
		errno = B_ENTRY_NOT_FOUND;
		return NULL;
	}
	return &dir->ent;
}


status_t
get_path_for_dirent(struct dirent *dent, char *buffer, size_t length)
{
	if (dent == NULL || buffer == NULL)
		return B_BAD_VALUE;

	return _kern_entry_ref_to_path(dent->d_pdev, dent->d_pino, dent->d_name,
		buffer, length);
}

