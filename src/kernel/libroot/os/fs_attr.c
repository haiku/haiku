/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <fs_attr.h>

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


ssize_t 
fs_read_attr(int fd, const char *attribute, uint32 type, off_t pos, void *buffer, size_t readBytes)
{
	ssize_t bytes;

	int attr = _kern_open_attr(fd, attribute, O_RDONLY);
	if (attr < 0)
		RETURN_AND_SET_ERRNO(attr);

	// type is not used at all in this function
	(void)type;

	bytes = _kern_read(fd, pos, buffer, readBytes);
	_kern_close(attr);

	RETURN_AND_SET_ERRNO(bytes);
}


ssize_t 
fs_write_attr(int fd, const char *attribute, uint32 type, off_t pos, const void *buffer, size_t readBytes)
{
	ssize_t bytes;

	int attr = _kern_create_attr(fd, attribute, type, O_WRONLY);
	if (attr < 0)
		RETURN_AND_SET_ERRNO(attr);

	bytes = _kern_write(fd, pos, buffer, readBytes);
	_kern_close(attr);

	RETURN_AND_SET_ERRNO(bytes);
}


int 
fs_remove_attr(int fd, const char *attribute)
{
	status_t status = _kern_remove_attr(fd, attribute);

	RETURN_AND_SET_ERRNO(status);
}


int 
fs_stat_attr(int fd, const char *attribute, struct attr_info *attrInfo)
{
	struct stat stat;
	status_t status;

	int attr = _kern_open_attr(fd, attribute, O_RDONLY);
	if (attr < 0)
		RETURN_AND_SET_ERRNO(attr);

	status = _kern_read_stat(attr, &stat, sizeof(struct stat));
	if (status == B_OK) {
		attrInfo->type = stat.st_type;
		attrInfo->size = stat.st_size;
	}
	_kern_close(attr);

	RETURN_AND_SET_ERRNO(status);
}


/*
int 
fs_open_attr(const char *path, const char *attribute, uint32 type, int openMode)
{
	// ToDo: implement fs_open_attr() - or remove it completely
	//	if it will be implemented, rename the current fs_open_attr() to fs_fopen_attr()
	return B_ERROR;
}
*/


int 
fs_open_attr(int fd, const char *attribute, uint32 type, int openMode)
{
	status_t status;

	if (openMode & O_CREAT)
		status = _kern_create_attr(fd, attribute, type, openMode);
	else
		status = _kern_open_attr(fd, attribute, openMode);

	RETURN_AND_SET_ERRNO(status);
}


int 
fs_close_attr(int fd)
{
	status_t status = _kern_close(fd);

	RETURN_AND_SET_ERRNO(status);
}


static DIR *
open_attr_dir(int file, const char *path)
{
	DIR *dir;

	int fd = _kern_open_attr_dir(file, path);
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


DIR *
fs_open_attr_dir(const char *path)
{
	return open_attr_dir(-1, path);
}


DIR *
fs_fopen_attr_dir(int fd)
{
	return open_attr_dir(fd, NULL);
}


int 
fs_close_attr_dir(DIR *dir)
{
	int status = _kern_close(dir->fd);

	free(dir);

	RETURN_AND_SET_ERRNO(status);
}


struct dirent *
fs_read_attr_dir(DIR *dir)
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
fs_rewind_attr_dir(DIR *dir)
{
	int status = _kern_rewind_dir(dir->fd);
	if (status < 0)
		errno = status;
}

