/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include <dirent_private.h>
#include <syscalls.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


DIR *
opendir(const char *path)
{
	DIR *dir;

	int fd = _kern_open_dir(-1, path);
	if (fd < 0) {
		errno = fd;
		return NULL;
	}

	/* allocate the memory for the DIR structure */
	if ((dir = (DIR *)malloc(DIR_BUFFER_SIZE)) == NULL) {
		errno = B_NO_MEMORY;
		_kern_close(fd);
		return NULL;
	}

	dir->fd = fd;
	dir->entries_left = 0;

	return dir;
}


int
closedir(DIR *dir)
{
	int status = _kern_close(dir->fd);

	free(dir);

	RETURN_AND_SET_ERRNO(status);
}


struct dirent *
readdir(DIR *dir)
{
	ssize_t count;

	if (dir->entries_left > 0) {
		struct dirent *dirent
			= (struct dirent *)((uint8 *)&dir->first_entry + dir->next_entry);

		dir->entries_left--;
		dir->next_entry += dirent->d_reclen;

		return dirent;
	}

	// we need to retrieve new entries

	count = _kern_read_dir(dir->fd, &dir->first_entry, DIRENT_BUFFER_SIZE,
		USHRT_MAX);
	if (count <= 0) {
		if (count < 0)
			errno = count;

		// end of directory
		return NULL;
	}

	dir->entries_left = count - 1;
	dir->next_entry = dir->first_entry.d_reclen;

	return &dir->first_entry;
}


int
readdir_r(DIR *dir, struct dirent *entry, struct dirent **_result)
{
	ssize_t count = _kern_read_dir(dir->fd, entry, sizeof(struct dirent)
		+ B_FILE_NAME_LENGTH, 1);
	if (count < B_OK)
		return count;

	if (count == 0) {
		// end of directory
		*_result = NULL;
	} else
		*_result = entry;

	return 0;
}


void
rewinddir(DIR *dir)
{
	status_t status = _kern_rewind_dir(dir->fd);
	if (status < 0)
		errno = status;
	else
		dir->entries_left = 0;
}


#ifndef _KERNEL_MODE

/* This is no POSIX compatible call; it's not exported in the headers
 * but here for BeOS compatiblity.
 */

int dirfd(DIR *dir);

int
dirfd(DIR *dir)
{
	return dir->fd;
}

#endif	// !_KERNEL_MODE
