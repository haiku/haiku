/*
 * Copyright 2011, John Scipione, jscipione@gmail.com.
 * Distributed under the terms of the MIT License.
 */

#include "fs_darwin.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>


int
faccessat(int fd, const char* path, int accessMode, int flag)
{
	if ((flag & AT_EACCESS) == 0) {
		// Perform access checks using the effective user and group IDs
		// CURRENTLY UNSUPPORTED
		errno = ENOTSUP;
		return -1;
	}

	if ((flag & AT_SYMLINK_NOFOLLOW) == 0) {
		// do not dereference, instead return information about the link
		// itself
		// CURRENTLY UNSUPPORTED
		errno = ENOTSUP;
		return -1;
	}

	if (path && path[0] == '/') {
		// path is absolute, call access() ignoring fd
		return access(path, accessMode);
	}

	if (!fd) {
		// fd is not a valid file descriptor
		errno = EBADF;
		return -1;
	}

	if (fd == AT_FDCWD) {
		// fd is set to AT_FDCWD, call access()
		return access(path, accessMode);
	}

	struct stat mystat;
	if (fstat(fd, &mystat) < 0) {
		// failed to grab stat information, fstat sets errno
		return -1;
	}

	if ((mystat.st_mode & S_IFDIR) != 0) {
		// fd does not point to a directory
		errno = ENOTDIR;
		return -1;
	}

	char *dirpath;
	dirpath = (char *)malloc(MAXPATHLEN);
	if (dirpath == NULL) {
		// ran out of memory allocating dirpath
		errno = ENOMEM;
		return -1;
	}

	if (fcntl(fd, F_GETPATH, dirpath) < 0) {
		// failed to get the path of fd, fcntl sets errno
		return -1;
	}

	if (strlcat(dirpath, path, MAXPATHLEN) > MAXPATHLEN) {
		// full path is too long, set errno and return
		errno = ENAMETOOLONG;
		return -1;
	}

	return access(dirpath, accessMode);
}


int
fchmodat(int fd, const char* path, mode_t mode, int flag)
{
	errno = ENOSYS;
	return -1;
}


int
fchownat(int fd, const char* path, uid_t owner, gid_t group, int flag)
{
	errno = ENOSYS;
	return -1;
}


int
fstatat(int fd, const char *path, struct stat *st, int flag)
{
	errno = ENOSYS;
	return -1;
}


int
mkdirat(int fd, const char *path, mode_t mode)
{
	errno = ENOSYS;
	return -1;
}


int
mkfifoat(int fd, const char *path, mode_t mode)
{
	errno = ENOSYS;
	return -1;
}


int
mknodat(int fd, const char *name, mode_t mode, dev_t dev)
{
	errno = ENOSYS;
	return -1;
}


int
renameat(int fromFD, const char* from, int toFD, const char* to)
{
	errno = ENOSYS;
	return -1;
}


ssize_t
readlinkat(int fd, const char *path, char *buffer, size_t bufferSize)
{
	errno = ENOSYS;
	return -1;
}


int
symlinkat(const char *toPath, int fd, const char *symlinkPath)
{
	errno = ENOSYS;
	return -1;
}


int
unlinkat(int fd, const char *path, int flag)
{
	errno = ENOSYS;
	return -1;
}


int
linkat(int toFD, const char *toPath, int linkFD, const char *linkPath, int flag)
{
	errno = ENOSYS;
	return -1;
}


int
futimesat(int fd, const char *path, const struct timeval times[2])
{
	errno = ENOSYS;
	return -1;
}
