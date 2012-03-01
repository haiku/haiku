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
#include <sys/stat.h>
#include <unistd.h>


int
faccessat(int fd, const char* path, int accessMode, int flag)
{
	return 0;
}


int
fchmodat(int fd, const char* path, mode_t mode, int flag)
{
	return 0;
}


int
fchownat(int fd, const char* path, uid_t owner, gid_t group, int flag)
{
	return 0;
}


int
fstatat(int fd, const char *path, struct stat *st, int flag)
{
	return 0;
}


int
mkdirat(int fd, const char *path, mode_t mode)
{
	return 0;
}


int
mkfifoat(int fd, const char *path, mode_t mode)
{
	return 0;
}


int
mknodat(int fd, const char *name, mode_t mode, dev_t dev)
{
	return 0;
}


int
renameat(int fromFD, const char* from, int toFD, const char* to)
{
	return 0;
}


ssize_t
readlinkat(int fd, const char *path, char *buffer, size_t bufferSize)
{
	return 0;
}


int
symlinkat(const char *toPath, int fd, const char *symlinkPath)
{
	return 0;
}


int
unlinkat(int fd, const char *path, int flag)
{
	return 0;
}


int
linkat(int toFD, const char *toPath, int linkFD, const char *linkPath, int flag)
{
	return 0;
}
