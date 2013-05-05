/*
 * Copyright 2002-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <fs_attr.h>

#include <syscall_utils.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include <dirent_private.h>
#include <errno_private.h>
#include <syscalls.h>
#include <syscall_utils.h>


// TODO: think about adding special syscalls for the read/write/stat functions
// to speed them up


static DIR *
open_attr_dir(int file, const char *path, bool traverse)
{
	DIR *dir;

	int fd = _kern_open_attr_dir(file, path, traverse);
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


//	#pragma mark -


extern "C" ssize_t
fs_read_attr(int fd, const char* attribute, uint32 /*type*/, off_t pos,
	void* buffer, size_t readBytes)
{
	ssize_t bytes = _kern_read_attr(fd, attribute, pos, buffer, readBytes);
	RETURN_AND_SET_ERRNO(bytes);
}


extern "C" ssize_t
fs_write_attr(int fd, const char* attribute, uint32 type, off_t pos,
	const void* buffer, size_t writeBytes)
{
	// TODO: move this documentation into the Haiku book!

	// NOTE: This call is deprecated in Haiku and has a number of problems:
	// On BeOS, it was documented that the "pos" argument is ignored, however,
	// that did not actually happen if the attribute was backed up by a real
	// file.
	// Also, it will truncate any existing attribute, disregarding the specified
	// position.

	// The implementation of this function tries to stay compatible with
	// BeOS in that it clobbers the existing attribute when you write at offset
	// 0, but it also tries to support programs which continue to write more
	// chunks.
	// The new Haiku way is to use fs_open_attr() to get a regular file handle
	// and use that for writing, then use fs_close_attr() when done. As you
	// see from this implementation, it saves 2 syscalls per writing a chunk
	// of data.

	ssize_t bytes = _kern_write_attr(fd, attribute, type, pos, buffer,
		writeBytes);
	RETURN_AND_SET_ERRNO(bytes);
}


extern "C" int
fs_remove_attr(int fd, const char* attribute)
{
	status_t status = _kern_remove_attr(fd, attribute);

	RETURN_AND_SET_ERRNO(status);
}


extern "C" int
fs_stat_attr(int fd, const char* attribute, struct attr_info* attrInfo)
{
	status_t status = _kern_stat_attr(fd, attribute, attrInfo);
	RETURN_AND_SET_ERRNO(status);
}


int
fs_open_attr(const char *path, const char *attribute, uint32 type, int openMode)
{
	status_t status = _kern_open_attr(-1, path, attribute, type, openMode);
	RETURN_AND_SET_ERRNO(status);
}


extern "C" int
fs_fopen_attr(int fd, const char* attribute, uint32 type, int openMode)
{
	status_t status = _kern_open_attr(fd, NULL, attribute, type, openMode);
	RETURN_AND_SET_ERRNO(status);
}


extern "C" int
fs_close_attr(int fd)
{
	status_t status = _kern_close(fd);

	RETURN_AND_SET_ERRNO(status);
}


extern "C" DIR*
fs_open_attr_dir(const char* path)
{
	return open_attr_dir(-1, path, true);
}


extern "C" DIR*
fs_lopen_attr_dir(const char* path)
{
	return open_attr_dir(-1, path, false);
}

extern "C" DIR*
fs_fopen_attr_dir(int fd)
{
	return open_attr_dir(fd, NULL, false);
}


extern "C" int
fs_close_attr_dir(DIR* dir)
{
	return closedir(dir);
}


extern "C" struct dirent*
fs_read_attr_dir(DIR* dir)
{
	return readdir(dir);
}


extern "C" void
fs_rewind_attr_dir(DIR* dir)
{
	rewinddir(dir);
}

