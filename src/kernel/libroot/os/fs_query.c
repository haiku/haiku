/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <fs_query.h>

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "syscalls.h"

// ToDo: implement these for real - the VFS functions are still missing!

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
	return NULL;
}


DIR *
fs_open_live_query(dev_t device, const char *query,
	uint32 flags, port_id port, int32 token)
{
	return NULL;
}


int
fs_close_query(DIR *d)
{
	return 0;
}


struct dirent *
fs_read_query(DIR *d)
{
	return NULL;
}


status_t
get_path_for_dirent(struct dirent *dent, char *buf, size_t len)
{
	return B_ERROR;
}

