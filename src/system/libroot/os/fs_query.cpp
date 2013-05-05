/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <fs_query.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <dirent_private.h>
#include <errno_private.h>
#include <syscalls.h>
#include <syscall_utils.h>


static DIR *
open_query_etc(dev_t device, const char *query,
	uint32 flags, port_id port, int32 token)
{
	if (device < 0 || query == NULL || query[0] == '\0') {
		__set_errno(B_BAD_VALUE);
		return NULL;
	}

	// open
	int fd = _kern_open_query(device, query, strlen(query), flags, port, token);
	if (fd < 0) {
		__set_errno(fd);
		return NULL;
	}

	// allocate the DIR structure
	DIR *dir = __create_dir_struct(fd);
	if (dir == NULL) {
		_kern_close(fd);
		return NULL;
	}

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
		__set_errno(B_BAD_VALUE);
		return NULL;
	}

	return open_query_etc(device, query, flags | B_LIVE_QUERY, port, token);
}


int
fs_close_query(DIR *dir)
{
	return closedir(dir);
}


struct dirent *
fs_read_query(DIR *dir)
{
	return readdir(dir);
}


status_t
get_path_for_dirent(struct dirent *dent, char *buffer, size_t length)
{
	if (dent == NULL || buffer == NULL)
		return B_BAD_VALUE;

	return _kern_entry_ref_to_path(dent->d_pdev, dent->d_pino, dent->d_name,
		buffer, length);
}

