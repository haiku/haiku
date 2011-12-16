/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <unistd.h>

#include <errno_private.h>
#include <syscalls.h>
#include <syscall_utils.h>


ssize_t
readlink(const char *path, char *buffer, size_t bufferSize)
{
	return readlinkat(AT_FDCWD, path, buffer, bufferSize);
}


ssize_t
readlinkat(int fd, const char *path, char *buffer, size_t bufferSize)
{
	size_t linkLen = bufferSize;
	status_t status = _kern_read_link(fd, path, buffer, &linkLen);
	if (status < B_OK) {
		__set_errno(status);
		return -1;
	}

	// If the buffer is big enough, null-terminate the string. That's not
	// required by the standard, but helps non-conforming apps.
	if (linkLen < bufferSize)
		buffer[linkLen] = '\0';

	return linkLen;
}


int
symlink(const char *toPath, const char *symlinkPath)
{
	int status = _kern_create_symlink(-1, symlinkPath, toPath, 0);

	RETURN_AND_SET_ERRNO(status);
}


int
symlinkat(const char *toPath, int fd, const char *symlinkPath)
{
	RETURN_AND_SET_ERRNO(_kern_create_symlink(fd, symlinkPath, toPath, 0));
}


int
unlink(const char *path)
{
	int status = _kern_unlink(-1, path);

	RETURN_AND_SET_ERRNO(status);
}


int
unlinkat(int fd, const char *path, int flag)
{
	if ((flag & AT_REMOVEDIR) != 0)
		RETURN_AND_SET_ERRNO(_kern_remove_dir(fd, path));
	else
		RETURN_AND_SET_ERRNO(_kern_unlink(fd, path));
}


int
link(const char *toPath, const char *linkPath)
{
	RETURN_AND_SET_ERRNO(_kern_create_link(-1, linkPath, -1, toPath, true));
}


int
linkat(int toFD, const char *toPath, int linkFD, const char *linkPath, int flag)
{
	RETURN_AND_SET_ERRNO(_kern_create_link(linkFD, linkPath, toFD, toPath,
		(flag & AT_SYMLINK_FOLLOW) != 0));
}

