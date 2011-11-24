/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <fs_index.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include <dirent_private.h>
#include <errno_private.h>
#include <syscalls.h>
#include <syscall_utils.h>


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
		__set_errno(fd);
		return NULL;
	}

	// allocate the DIR structure
	if ((dir = __create_dir_struct(fd)) == NULL) {
		_kern_close(fd);
		return NULL;
	}

	return dir;
}


int
fs_close_index_dir(DIR *dir)
{
	return closedir(dir);
}


struct dirent *
fs_read_index_dir(DIR *dir)
{
	return readdir(dir);
}


void
fs_rewind_index_dir(DIR *dir)
{
	rewinddir(dir);
}

