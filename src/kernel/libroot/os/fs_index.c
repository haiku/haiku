/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <fs_index.h>

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


int
fs_create_index(dev_t device, const char *name, uint32 type, uint32 flags)
{
	status_t status = _kern_create_index(device, name, type, flags);

	RETURN_AND_SET_ERRNO(status);
}


int 
fs_remove_index(dev_t device, const char *name)
{
	status_t status = _kern_remove_index(device, name);

	RETURN_AND_SET_ERRNO(status);
}


int 
fs_stat_index(dev_t device, const char *name, struct index_info *indexInfo)
{
	struct stat stat;

	status_t status = _kern_read_index_stat(device, name, &stat);
	if (status == B_OK) {
		indexInfo->type = stat.st_type;
		indexInfo->size = stat.st_size;
		indexInfo->modification_time = stat.st_mtime;
		indexInfo->creation_time = stat.st_crtime;
		indexInfo->uid = stat.st_uid;
		indexInfo->gid = stat.st_gid;
	}

	RETURN_AND_SET_ERRNO(status);
}


DIR *
fs_open_index_dir(dev_t device)
{
	DIR *dir;

	int fd = _kern_open_index_dir(device);
	if (fd < 0) {
		errno = fd;
		return NULL;
	}

	/* allocate the memory for the DIR structure */
	if ((dir = (DIR *)malloc(sizeof(DIR) + BUFFER_SIZE)) == NULL) {
		errno = B_NO_MEMORY;
		_kern_close(fd);
		return NULL;
	}

	dir->fd = fd;

	return dir;
}


int 
fs_close_index_dir(DIR *dir)
{
	int status = _kern_close(dir->fd);

	free(dir);

	RETURN_AND_SET_ERRNO(status);
}


struct dirent *
fs_read_index_dir(DIR *dir)
{
	ssize_t count = _kern_read_dir(dir->fd, &dir->ent, BUFFER_SIZE, 1);
	if (count <= 0) {
		if (count < 0)
			errno = count;
		return NULL;
	}

	return &dir->ent;
}


void 
fs_rewind_index_dir(DIR *dir)
{
	int status = _kern_rewind_dir(dir->fd);
	if (status < 0)
		errno = status;
}

