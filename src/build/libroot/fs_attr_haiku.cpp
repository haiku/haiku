/*
 * Copyright 2017, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*!	Shim over the host Haiku fs_attr API */


#ifdef BUILDING_FS_SHELL
#	include "compat.h"
#	define B_OK					0
#	define B_BAD_VALUE			EINVAL
#	define B_FILE_ERROR			EBADF
#	define B_ERROR				EINVAL
#	define B_ENTRY_NOT_FOUND	ENOENT
#	define B_NO_MEMORY			ENOMEM
#else
#	include <syscalls.h>

#	include "fs_impl.h"
#	include "fs_descriptors.h"
#endif

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <map>
#include <string>

#include <fs_attr.h>


namespace BPrivate {}
using namespace BPrivate;


namespace {

// LocalFD
#include "LocalFD.h"

} // unnamed namspace


// # pragma mark - Public API


// fs_open_attr_dir
extern "C" DIR *
_haiku_build_fs_open_attr_dir(const char *path)
{
	return fs_open_attr_dir(path);
}

// fs_lopen_attr_dir
extern "C" DIR*
_haiku_build_fs_lopen_attr_dir(const char *path)
{
	return fs_lopen_attr_dir(path);
}

// fs_fopen_attr_dir
extern "C" DIR*
_haiku_build_fs_fopen_attr_dir(int fd)
{
	LocalFD localFD;
	status_t error = localFD.Init(fd);
	if (error != B_OK) {
		errno = error;
		return NULL;
	}

	if (localFD.FD() < 0) {
		return fs_lopen_attr_dir(localFD.Path());
	} else {
		return fs_fopen_attr_dir(localFD.FD());
	}
}

// fs_close_attr_dir
extern "C" int
_haiku_build_fs_close_attr_dir(DIR *dir)
{
	return fs_close_attr_dir(dir);
}

// fs_read_attr_dir
extern "C" struct dirent *
_haiku_build_fs_read_attr_dir(DIR *dir)
{
	return fs_read_attr_dir(dir);
}

// fs_rewind_attr_dir
extern "C" void
_haiku_build_fs_rewind_attr_dir(DIR *dir)
{
	return fs_rewind_attr_dir(dir);
}

// fs_fopen_attr
extern "C" int
_haiku_build_fs_fopen_attr(int fd, const char *attribute, uint32 type, int openMode)
{
	if (fd < 0) {
		errno = B_BAD_VALUE;
		return -1;
	}

	LocalFD localFD;
	status_t error = localFD.Init(fd);
	if (error != B_OK) {
		errno = error;
		return -1;
	}

	if (localFD.FD() < 0) {
		return fs_open_attr(localFD.Path(), attribute, type,
			openMode | O_NOTRAVERSE);
	} else {
		return fs_fopen_attr(localFD.FD(), attribute, type, openMode);
	}
}

// fs_close_attr
extern "C" int
_haiku_build_fs_close_attr(int fd)
{
	return fs_close_attr(fd);
}

// fs_read_attr
extern "C" ssize_t
_haiku_build_fs_read_attr(int fd, const char* attribute, uint32 type, off_t pos,
	void *buffer, size_t readBytes)
{
	LocalFD localFD;
	status_t error = localFD.Init(fd);
	if (error != B_OK) {
		errno = error;
		return -1;
	}

	ssize_t bytesRead;
	if (localFD.FD() < 0) {
		int fd = open(localFD.Path(), O_RDONLY | O_NOTRAVERSE);
		bytesRead = fs_read_attr(fd, attribute, type,
			pos, buffer, readBytes);
		close(fd);
	} else {
		bytesRead = fs_read_attr(localFD.FD(), attribute, type,
			pos, buffer, readBytes);
	}
	if (bytesRead < 0) {
		// Make sure, the error code is B_ENTRY_NOT_FOUND, if the attribute
		// doesn't exist.
		if (errno == ENOATTR || errno == ENODATA)
			errno = B_ENTRY_NOT_FOUND;
		return -1;
	}

	return bytesRead;
}

// fs_write_attr
extern "C" ssize_t
_haiku_build_fs_write_attr(int fd, const char* attribute, uint32 type, off_t pos,
	const void *buffer, size_t writeBytes)
{
	LocalFD localFD;
	status_t error = localFD.Init(fd);
	if (error != B_OK) {
		errno = error;
		return -1;
	}

	ssize_t written;
	if (localFD.FD() < 0) {
		int fd = open(localFD.Path(), O_NOTRAVERSE | O_WRONLY);
		written = fs_write_attr(fd, attribute, type,
			pos, buffer, writeBytes);
		close(fd);
	} else {
		written = fs_write_attr(localFD.FD(), attribute, type,
			pos, buffer, writeBytes);
	}

	return written;
}

// fs_remove_attr
extern "C" int
_haiku_build_fs_remove_attr(int fd, const char* attribute)
{
	LocalFD localFD;
	status_t error = localFD.Init(fd);
	if (error != B_OK) {
		errno = error;
		return -1;
	}

	// remove attribute
	int result;
	if (localFD.FD() < 0) {
		int fd = open(localFD.Path(), O_NOTRAVERSE | O_WRONLY);
		result = fs_remove_attr(fd, attribute);
		close(fd);
	} else {
		result = fs_remove_attr(localFD.FD(), attribute);
	}

	if (result < 0) {
		// Make sure, the error code is B_ENTRY_NOT_FOUND, if the attribute
		// doesn't exist.
		if (errno == ENOATTR || errno == ENODATA)
			errno = B_ENTRY_NOT_FOUND;
		return -1;
	}
	return 0;
}

// fs_stat_attr
extern "C" int
_haiku_build_fs_stat_attr(int fd, const char *attribute, struct attr_info *attrInfo)
{
	if (!attribute || !attrInfo) {
		errno = B_BAD_VALUE;
		return -1;
	}

	LocalFD localFD;
	status_t error = localFD.Init(fd);
	if (error != B_OK) {
		errno = error;
		return -1;
	}

	int result;
	if (localFD.FD() < 0) {
		int fd = open(localFD.Path(), O_NOTRAVERSE | O_RDONLY);
		result = fs_stat_attr(fd, attribute, attrInfo);
		close(fd);
	} else {
		result = fs_stat_attr(localFD.FD(), attribute, attrInfo);
	}

	return result;
}


// #pragma mark - Private Syscalls


#ifndef BUILDING_FS_SHELL

// _kern_open_attr_dir
int
_kern_open_attr_dir(int fd, const char *path)
{
	// get node ref for the node
	struct stat st;
	status_t error = _kern_read_stat(fd, path, false, &st,
		sizeof(struct stat));
	if (error != B_OK) {
		errno = error;
		return -1;
	}
	NodeRef ref(st);

	DIR* dir;
	if (path) {
		// If a path was given, get a usable path.
		string realPath;
		status_t error = get_path(fd, path, realPath);
		if (error != B_OK)
			return error;

		dir = _haiku_build_fs_open_attr_dir(realPath.c_str());
	} else
		dir = _haiku_build_fs_fopen_attr_dir(fd);

	if (!dir)
		return errno;

	// create descriptor
	AttrDirDescriptor *descriptor = new AttrDirDescriptor(dir, ref);
	return add_descriptor(descriptor);
}

// _kern_rename_attr
status_t
_kern_rename_attr(int fromFile, const char *fromName, int toFile,
	const char *toName)
{
	// not supported ATM
	return B_BAD_VALUE;
}

// _kern_remove_attr
status_t
_kern_remove_attr(int fd, const char *name)
{
	if (!name)
		return B_BAD_VALUE;

	if (_haiku_build_fs_remove_attr(fd, name) < 0)
		return errno;
	return B_OK;
}

#endif	// ! BUILDING_FS_SHELL
