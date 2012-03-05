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
#include <sys/time.h>
#include <unistd.h>


int
eaccess(const char* path, int accessMode)
{
	uid_t uid = geteuid();
	int fileMode = 0;

	struct stat st;
	if (stat(path, &st) < 0) {
		// failed to get stat information on path
		return -1;
	}

	if (uid == 0) {
		// user is root
		// root has always read/write permission, but at least one of the
		// X bits must be set for execute permission
		fileMode = R_OK | W_OK;
		if ((st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0)
			fileMode |= X_OK;
	} else if (st.st_uid == uid) {
		// user is node owner
		if ((st.st_mode & S_IRUSR) != 0)
			fileMode |= R_OK;
		if ((st.st_mode & S_IWUSR) != 0)
			fileMode |= W_OK;
		if ((st.st_mode & S_IXUSR) != 0)
			fileMode |= X_OK;
	} else if (st.st_gid == getegid()) {
		// user is in owning group
		if ((st.st_mode & S_IRGRP) != 0)
			fileMode |= R_OK;
		if ((st.st_mode & S_IWGRP) != 0)
			fileMode |= W_OK;
		if ((st.st_mode & S_IXGRP) != 0)
			fileMode |= X_OK;
	} else {
		// user is one of the others
		if ((st.st_mode & S_IROTH) != 0)
			fileMode |= R_OK;
		if ((st.st_mode & S_IWOTH) != 0)
			fileMode |= W_OK;
		if ((st.st_mode & S_IXOTH) != 0)
			fileMode |= X_OK;
	}

	if ((accessMode & ~fileMode) != 0) {
		errno = EACCES;
		return -1;
	}

	return 0;
}


int
faccessat(int fd, const char* path, int accessMode, int flag)
{
	if ((flag & AT_SYMLINK_NOFOLLOW) == 0) {
		// do not dereference, instead return information about the link
		// itself
		// CURRENTLY UNSUPPORTED
		errno = ENOTSUP;
		return -1;
	}

	if (path && path[0] == '/') {
		// path is absolute, call access() ignoring fd
		return (flag & AT_EACCESS) == 0 ? eaccess(path, accessMode)
			: access(path, accessMode);
	}

	if (!fd) {
		// fd is not a valid file descriptor
		errno = EBADF;
		return -1;
	}

	if (fd == AT_FDCWD) {
		// fd is set to AT_FDCWD, call access()
		return (flag & AT_EACCESS) == 0 ? eaccess(path, accessMode)
			: access(path, accessMode);
	}

	struct stat dirst;
	if (fstat(fd, &dirst) < 0) {
		// failed to grab stat information, fstat() sets errno
		return -1;
	}

	if ((dirst.st_mode & S_IFDIR) != 0) {
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

	return (flag & AT_EACCESS) == 0 ? eaccess(path, accessMode)
		: access(path, accessMode);
}


int
fchmodat(int fd, const char* path, mode_t mode, int flag)
{
	if ((flag & AT_SYMLINK_NOFOLLOW) == 0) {
		// do not dereference, instead return information about the link
		// itself
		// CURRENTLY UNSUPPORTED
		errno = ENOTSUP;
		return -1;
	} else if (flag != 0) {
		errno = EINVAL;
		return -1;
	}

	if (path && path[0] == '/') {
		// path is absolute, call chmod() ignoring fd
		return chmod(path, mode);
	}

	if (!fd) {
		// fd is not a valid file descriptor
		errno = EBADF;
		return -1;
	}

	if (fd == AT_FDCWD) {
		// fd is set to AT_FDCWD, call chmod()
		return chmod(path, mode);
	}

	struct stat dirst;
	if (fstat(fd, &dirst) < 0) {
		// failed to grab stat information, fstat() sets errno
		return -1;
	}

	if ((dirst.st_mode & S_IFDIR) != 0) {
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

	return chmod(dirpath, mode);
}


int
fchownat(int fd, const char* path, uid_t owner, gid_t group, int flag)
{
	if ((flag & AT_SYMLINK_NOFOLLOW) == 0) {
		// do not dereference, instead return information about the link
		// itself
		// CURRENTLY UNSUPPORTED
		errno = ENOTSUP;
		return -1;
	} else if (flag != 0) {
		errno = EINVAL;
		return -1;
	}

	if (path && path[0] == '/') {
		// path is absolute, call chown() ignoring fd
		return chown(path, owner, group);
	}

	if (!fd) {
		// fd is not a valid file descriptor
		errno = EBADF;
		return -1;
	}

	if (fd == AT_FDCWD) {
		// fd is set to AT_FDCWD, call access()
		return chown(path, owner, group);
	}

	struct stat dirst;
	if (fstat(fd, &dirst) < 0) {
		// failed to grab stat information, fstat() sets errno
		return -1;
	}

	if ((dirst.st_mode & S_IFDIR) != 0) {
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

	return chown(dirpath, owner, group);
}


int
fstatat(int fd, const char *path, struct stat *st, int flag)
{
	if ((flag & AT_NO_AUTOMOUNT) == 0) {
		// Don't automount the terminal ("basename") component of pathname if
		// it is a directory that is an automount point.
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
		// path is absolute, call stat() ignoring fd
		return stat(path, st);
	}

	if (!fd) {
		// fd is not a valid file descriptor
		errno = EBADF;
		return -1;
	}

	if (fd == AT_FDCWD) {
		// fd is set to AT_FDCWD, call stat()
		return stat(path, st);
	}

	struct stat dirst;
	if (fstat(fd, &dirst) < 0) {
		// failed to grab stat information, fstat() sets errno
		return -1;
	}

	if ((dirst.st_mode & S_IFDIR) != 0) {
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

	return stat(dirpath, st);
}


int
mkdirat(int fd, const char *path, mode_t mode)
{
	if (path && path[0] == '/') {
		// path is absolute, call mkdir() ignoring fd
		return mkdir(path, mode);
	}

	if (!fd) {
		// fd is not a valid file descriptor
		errno = EBADF;
		return -1;
	}

	if (fd == AT_FDCWD) {
		// fd is set to AT_FDCWD, call mkdir()
		return mkdir(path, mode);
	}

	struct stat dirst;
	if (fstat(fd, &dirst) < 0) {
		// failed to grab stat information, fstat() sets errno
		return -1;
	}

	if ((dirst.st_mode & S_IFDIR) != 0) {
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

	return mkdir(dirpath, mode);
}


int
mkfifoat(int fd, const char *path, mode_t mode)
{
	if (path && path[0] == '/') {
		// path is absolute, call mkfifo() ignoring fd
		return mkfifo(path, mode);
	}

	if (!fd) {
		// fd is not a valid file descriptor
		errno = EBADF;
		return -1;
	}

	if (fd == AT_FDCWD) {
		// fd is set to AT_FDCWD, call mkfifo()
		return mkfifo(path, mode);
	}

	struct stat dirst;
	if (fstat(fd, &dirst) < 0) {
		// failed to grab stat information, fstat() sets errno
		return -1;
	}

	if ((dirst.st_mode & S_IFDIR) != 0) {
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

	return mkfifo(dirpath, mode);
}


int
mknodat(int fd, const char *path, mode_t mode, dev_t dev)
{
	if (path && path[0] == '/') {
		// path is absolute, call mknod() ignoring fd
		return mknod(path, mode, dev);
	}

	if (!fd) {
		// fd is not a valid file descriptor
		errno = EBADF;
		return -1;
	}

	if (fd == AT_FDCWD) {
		// fd is set to AT_FDCWD, call mknod()
		return mknod(path, mode, dev);
	}

	struct stat dirst;
	if (fstat(fd, &dirst) < 0) {
		// failed to grab stat information, fstat() sets errno
		return -1;
	}

	if ((dirst.st_mode & S_IFDIR) != 0) {
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

	return mknod(dirpath, mode, dev);
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
	if (path && path[0] == '/') {
		// path is absolute, call readlink() ignoring fd
		return readlink(path, buffer, bufferSize);
	}

	if (!fd) {
		// fd is not a valid file descriptor
		errno = EBADF;
		return -1;
	}

	if (fd == AT_FDCWD) {
		// fd is set to AT_FDCWD, call readlink()
		return readlink(path, buffer, bufferSize);
	}

	struct stat dirst;
	if (fstat(fd, &dirst) < 0) {
		// failed to grab stat information, fstat() sets errno
		return -1;
	}

	if ((dirst.st_mode & S_IFDIR) != 0) {
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

	return readlink(dirpath, buffer, bufferSize);
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
	if (path && path[0] == '/') {
		// path is absolute, call utimes() ignoring fd
		return utimes(path, times);
	}

	if (!fd) {
		// fd is not a valid file descriptor
		errno = EBADF;
		return -1;
	}

	if (fd == AT_FDCWD) {
		// fd is set to AT_FDCWD, call utimes()
		return utimes(path, times);
	}

	struct stat dirst;
	if (fstat(fd, &dirst) < 0) {
		// failed to grab stat information, fstat() sets errno
		return -1;
	}

	if ((dirst.st_mode & S_IFDIR) != 0) {
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

	return utimes(dirpath, times);
}
