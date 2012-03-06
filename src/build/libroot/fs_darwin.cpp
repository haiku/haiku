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

	if (path != NULL && path[0] == '/') {
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

	char *dirpath = (char *)malloc(MAXPATHLEN);
	if (dirpath == NULL) {
		// ran out of memory allocating dirpath
		errno = ENOMEM;
		return -1;
	}

	if (fcntl(fd, F_GETPATH, dirpath) < 0) {
		// failed to get the path of fd, fcntl sets errno
		free(dirpath);
		return -1;
	}

	if (strlcat(dirpath, path, MAXPATHLEN) > MAXPATHLEN) {
		// full path is too long, set errno and return
		errno = ENAMETOOLONG;
		free(dirpath);
		return -1;
	}

	int status = (flag & AT_EACCESS) == 0 ? eaccess(path, accessMode)
		: access(path, accessMode);
	free(dirpath);
	return status;
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

	if (path != NULL && path[0] == '/') {
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

	char *dirpath = (char *)malloc(MAXPATHLEN);
	if (dirpath == NULL) {
		// ran out of memory allocating dirpath
		errno = ENOMEM;
		free(dirpath);
		return -1;
	}

	if (fcntl(fd, F_GETPATH, dirpath) < 0) {
		// failed to get the path of fd, fcntl sets errno
		free(dirpath);
		return -1;
	}

	if (strlcat(dirpath, path, MAXPATHLEN) > MAXPATHLEN) {
		// full path is too long, set errno and return
		errno = ENAMETOOLONG;
		free(dirpath);
		return -1;
	}

	int status = chmod(dirpath, mode);
	free(dirpath);
	return status;
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

	if (path != NULL && path[0] == '/') {
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

	char *dirpath = (char *)malloc(MAXPATHLEN);
	if (dirpath == NULL) {
		// ran out of memory allocating dirpath
		errno = ENOMEM;
		return -1;
	}

	if (fcntl(fd, F_GETPATH, dirpath) < 0) {
		// failed to get the path of fd, fcntl sets errno
		free(dirpath);
		return -1;
	}

	if (strlcat(dirpath, path, MAXPATHLEN) > MAXPATHLEN) {
		// full path is too long, set errno and return
		errno = ENAMETOOLONG;
		free(dirpath);
		return -1;
	}

	int status = chown(dirpath, owner, group);
	free(dirpath);
	return status;
}


int
fstatat(int fd, const char *path, struct stat *st, int flag)
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

	if (path != NULL && path[0] == '/') {
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

	char *dirpath = (char *)malloc(MAXPATHLEN);
	if (dirpath == NULL) {
		// ran out of memory allocating dirpath
		errno = ENOMEM;
		return -1;
	}

	if (fcntl(fd, F_GETPATH, dirpath) < 0) {
		// failed to get the path of fd, fcntl sets errno
		free(dirpath);
		return -1;
	}

	if (strlcat(dirpath, path, MAXPATHLEN) > MAXPATHLEN) {
		// full path is too long, set errno and return
		errno = ENAMETOOLONG;
		free(dirpath);
		return -1;
	}

	int status = stat(dirpath, st);
	free(dirpath);
	return status;
}


int
mkdirat(int fd, const char *path, mode_t mode)
{
	if (path != NULL && path[0] == '/') {
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

	char *dirpath = (char *)malloc(MAXPATHLEN);
	if (dirpath == NULL) {
		// ran out of memory allocating dirpath
		errno = ENOMEM;
		return -1;
	}

	if (fcntl(fd, F_GETPATH, dirpath) < 0) {
		// failed to get the path of fd, fcntl sets errno
		free(dirpath);
		return -1;
	}

	if (strlcat(dirpath, path, MAXPATHLEN) > MAXPATHLEN) {
		// full path is too long, set errno and return
		errno = ENAMETOOLONG;
		free(dirpath);
		return -1;
	}

	int status = mkdir(dirpath, mode);
	free(dirpath);
	return status;
}


int
mkfifoat(int fd, const char *path, mode_t mode)
{
	if (path != NULL && path[0] == '/') {
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

	char *dirpath = (char *)malloc(MAXPATHLEN);
	if (dirpath == NULL) {
		// ran out of memory allocating dirpath
		errno = ENOMEM;
		return -1;
	}

	if (fcntl(fd, F_GETPATH, dirpath) < 0) {
		// failed to get the path of fd, fcntl sets errno
		free(dirpath);
		return -1;
	}

	if (strlcat(dirpath, path, MAXPATHLEN) > MAXPATHLEN) {
		// full path is too long, set errno and return
		errno = ENAMETOOLONG;
		free(dirpath);
		return -1;
	}

	int status = mkfifo(dirpath, mode);
	free(dirpath);
	return status;
}


int
mknodat(int fd, const char *path, mode_t mode, dev_t dev)
{
	if (path != NULL && path[0] == '/') {
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

	char *dirpath = (char *)malloc(MAXPATHLEN);
	if (dirpath == NULL) {
		// ran out of memory allocating dirpath
		errno = ENOMEM;
		return -1;
	}

	if (fcntl(fd, F_GETPATH, dirpath) < 0) {
		// failed to get the path of fd, fcntl sets errno
		free(dirpath);
		return -1;
	}

	if (strlcat(dirpath, path, MAXPATHLEN) > MAXPATHLEN) {
		// full path is too long, set errno and return
		errno = ENAMETOOLONG;
		free(dirpath);
		return -1;
	}

	return mknod(dirpath, mode, dev);
}


int
renameat(int oldFD, const char* oldPath, int newFD, const char* newPath)
{
	bool oldPathIsAbsolute = false;
	bool newPathIsAbsolute = false;

	if (oldPath != NULL && oldPath[0] == '/')
		oldPathIsAbsolute = true;

	if (newPath != NULL && newPath[0] == '/')
		newPathIsAbsolute = true;

	if (oldPathIsAbsolute && newPathIsAbsolute) {
		// oldPath and newPath are absolute, call rename() ignoring the fd's
		return rename(oldPath, newPath);
	}

	if ((!oldPathIsAbsolute && !oldFD) || (!newPathIsAbsolute && !newFD)) {
		// oldPath is not an absolute path and oldFD is not a valid file
		// descriptor or newPath is not an absolute path and newFD is not a
		// valid file descriptor
		errno = EBADF;
		return -1;
	}

	char *oldDirPath;
	char *newDirPath;

	if (oldPathIsAbsolute || oldFD == AT_FDCWD)
		oldDirPath = const_cast<char *>(oldPath);
	else {
		struct stat dirst;
		if (fstat(oldFD, &dirst) < 0) {
			// failed to grab stat information, fstat() sets errno
			return -1;
		}

		if ((dirst.st_mode & S_IFDIR) != 0) {
			// oldFd does not point to a directory
			errno = ENOTDIR;
			return -1;
		}

		oldDirPath = (char *)malloc(MAXPATHLEN);
		if (oldDirPath == NULL) {
			// ran out of memory allocating oldDirPath
			errno = ENOMEM;
			return -1;
		}

		if (fcntl(oldFD, F_GETPATH, oldDirPath) < 0) {
			// failed to get the path of oldFD, fcntl sets errno
			free(oldDirPath);
			return -1;
		}

		if (strlcat(oldDirPath, oldPath, MAXPATHLEN) > MAXPATHLEN) {
			// full path is too long, set errno and return
			errno = ENAMETOOLONG;
			free(oldDirPath);
			return -1;
		}
	}

	if (newPathIsAbsolute || newFD == AT_FDCWD)
		newDirPath = const_cast<char *>(newPath);
	else {
		struct stat dirst;
		if (fstat(newFD, &dirst) < 0) {
			// failed to grab stat information, fstat() sets errno
			return -1;
		}

		if ((dirst.st_mode & S_IFDIR) != 0) {
			// newFD does not point to a directory
			errno = ENOTDIR;
			return -1;
		}

		newDirPath = (char *)malloc(MAXPATHLEN);
		if (newDirPath == NULL) {
			// ran out of memory allocating newDirPath
			errno = ENOMEM;
			return -1;
		}

		if (fcntl(newFD, F_GETPATH, newDirPath) < 0) {
			// failed to get the path of newFD, fcntl sets errno
			free(newDirPath);
			return -1;
		}

		if (strlcat(newDirPath, newPath, MAXPATHLEN) > MAXPATHLEN) {
			// full path is too long, set errno and return
			errno = ENAMETOOLONG;
			free(newDirPath);
			return -1;
		}
	}

	int status = rename(oldDirPath, newDirPath);
	if (!oldPathIsAbsolute && oldFD != AT_FDCWD)
		free(oldDirPath);
	if (!newPathIsAbsolute && newFD != AT_FDCWD)
		free(newDirPath);
	return status;
}


ssize_t
readlinkat(int fd, const char *path, char *buffer, size_t bufferSize)
{
	if (path != NULL && path[0] == '/') {
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

	char *dirpath = (char *)malloc(MAXPATHLEN);
	if (dirpath == NULL) {
		// ran out of memory allocating dirpath
		errno = ENOMEM;
		return -1;
	}

	if (fcntl(fd, F_GETPATH, dirpath) < 0) {
		// failed to get the path of fd, fcntl sets errno
		free(dirpath);
		return -1;
	}

	if (strlcat(dirpath, path, MAXPATHLEN) > MAXPATHLEN) {
		// full path is too long, set errno and return
		errno = ENAMETOOLONG;
		free(dirpath);
		return -1;
	}

	int status = readlink(dirpath, buffer, bufferSize);
	free(dirpath);
	return status;
}


int
symlinkat(const char *toPath, int fd, const char *symlinkPath)
{
	if (symlinkPath != NULL && symlinkPath[0] == '/') {
		// symlinkPath is absolute, call symlink() ignoring fd
		return symlink(toPath, symlinkPath);
	}

	if (!fd) {
		// fd is not a valid file descriptor
		errno = EBADF;
		return -1;
	}

	if (fd == AT_FDCWD) {
		// fd is set to AT_FDCWD, call symlink()
		return symlink(toPath, symlinkPath);
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

	char *dirpath = (char *)malloc(MAXPATHLEN);
	if (dirpath == NULL) {
		// ran out of memory allocating dirpath
		errno = ENOMEM;
		return -1;
	}

	if (fcntl(fd, F_GETPATH, dirpath) < 0) {
		// failed to get the path of fd, fcntl sets errno
		free(dirpath);
		return -1;
	}

	if (strlcat(dirpath, symlinkPath, MAXPATHLEN) > MAXPATHLEN) {
		// full path is too long, set errno and return
		errno = ENAMETOOLONG;
		free(dirpath);
		return -1;
	}

	int status = symlink(dirpath, symlinkPath);
	free(dirpath);
	return status;
}


int
unlinkat(int fd, const char *path, int flag)
{
	if (path != NULL && path[0] == '/') {
		// path is absolute, call rmdir() or unlink() ignoring fd
		return (flag & AT_REMOVEDIR) == 0 ? rmdir(path) : unlink(path);
	}

	if (!fd) {
		// fd is not a valid file descriptor
		errno = EBADF;
		return -1;
	}

	if (fd == AT_FDCWD) {
		// fd is set to AT_FDCWD, call rmdir() or unlink()
		return (flag & AT_REMOVEDIR) == 0 ? rmdir(path) : unlink(path);
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

	char *dirpath = (char *)malloc(MAXPATHLEN);
	if (dirpath == NULL) {
		// ran out of memory allocating dirpath
		errno = ENOMEM;
		return -1;
	}

	if (fcntl(fd, F_GETPATH, dirpath) < 0) {
		// failed to get the path of fd, fcntl sets errno
		free(dirpath);
		return -1;
	}

	if (strlcat(dirpath, path, MAXPATHLEN) > MAXPATHLEN) {
		// full path is too long, set errno and return
		errno = ENAMETOOLONG;
		free(dirpath);
		return -1;
	}

	int status = (flag & AT_REMOVEDIR) == 0 ? rmdir(path) : unlink(path);
	free(dirpath);
	return status;
}


int
linkat(int oldFD, const char *oldPath, int newFD, const char *newPath,
	   int flag)
{
	if ((flag & AT_SYMLINK_FOLLOW) == 0) {
		// Dereference oldPath
		// CURRENTLY UNSUPPORTED
		errno = ENOTSUP;
		return -1;
	} else if (flag != 0) {
		errno = EINVAL;
		return -1;
	}

	bool oldPathIsAbsolute = false;
	bool newPathIsAbsolute = false;

	if (oldPath != NULL && oldPath[0] == '/')
		oldPathIsAbsolute = true;

	if (newPath != NULL && newPath[0] == '/')
		newPathIsAbsolute = true;

	if (oldPathIsAbsolute && newPathIsAbsolute) {
		// oldPath and newPath are both absolute, call link() ignoring the fd's
		return link(oldPath, newPath);
	}

	if ((!oldPathIsAbsolute && !oldFD) || (!newPathIsAbsolute && !newFD)) {
		// oldPath is not an absolute path and oldFD is not a valid file
		// descriptor or newPath is not an absolute path and newFD is not a
		// valid file descriptor
		errno = EBADF;
		return -1;
	}

	char *oldDirPath;
	char *newDirPath;

	if (oldPathIsAbsolute || oldFD == AT_FDCWD)
		oldDirPath = const_cast<char *>(oldPath);
	else {
		struct stat dirst;
		if (fstat(oldFD, &dirst) < 0) {
			// failed to grab stat information, fstat() sets errno
			return -1;
		}

		if ((dirst.st_mode & S_IFDIR) != 0) {
			// fd does not point to a directory
			errno = ENOTDIR;
			return -1;
		}

		oldDirPath = (char *)malloc(MAXPATHLEN);
		if (oldDirPath == NULL) {
			// ran out of memory allocating oldDirPath
			errno = ENOMEM;
			return -1;
		}

		if (fcntl(oldFD, F_GETPATH, oldDirPath) < 0) {
			// failed to get the path of oldFD, fcntl sets errno
			return -1;
			free(oldDirPath);
		}

		if (strlcat(oldDirPath, oldPath, MAXPATHLEN) > MAXPATHLEN) {
			// full path is too long, set errno and return
			errno = ENAMETOOLONG;
			free(oldDirPath);
			return -1;
		}
	}

	if (newPathIsAbsolute || newFD == AT_FDCWD)
		newDirPath = const_cast<char *>(newPath);
	else {
		struct stat dirst;
		if (fstat(newFD, &dirst) < 0) {
			// failed to grab stat information, fstat() sets errno
			return -1;
		}

		if ((dirst.st_mode & S_IFDIR) != 0) {
			// fd does not point to a directory
			errno = ENOTDIR;
			return -1;
		}

		newDirPath = (char *)malloc(MAXPATHLEN);
		if (newDirPath == NULL) {
			// ran out of memory allocating newDirPath
			errno = ENOMEM;
			return -1;
		}

		if (fcntl(newFD, F_GETPATH, newDirPath) < 0) {
			// failed to get the path of newFD, fcntl sets errno
			free(newDirPath);
			return -1;
		}

		if (strlcat(newDirPath, newPath, MAXPATHLEN) > MAXPATHLEN) {
			// full path is too long, set errno and return
			errno = ENAMETOOLONG;
			return -1;
		}
	}

	int status = link(oldDirPath, newDirPath);
	if (!oldPathIsAbsolute && oldFD != AT_FDCWD)
		free(oldDirPath);
	if (!newPathIsAbsolute && newFD != AT_FDCWD)
		free(newDirPath);
	return status;
}


int
futimesat(int fd, const char *path, const struct timeval times[2])
{
	if (path != NULL && path[0] == '/') {
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

	char *dirpath = (char *)malloc(MAXPATHLEN);
	if (dirpath == NULL) {
		// ran out of memory allocating dirpath
		errno = ENOMEM;
		return -1;
	}

	if (fcntl(fd, F_GETPATH, dirpath) < 0) {
		// failed to get the path of fd, fcntl sets errno
		free(dirpath);
		return -1;
	}

	if (strlcat(dirpath, path, MAXPATHLEN) > MAXPATHLEN) {
		// full path is too long, set errno and return
		errno = ENAMETOOLONG;
		free(dirpath);
		return -1;
	}

	int status = utimes(dirpath, times);
	free(dirpath);
	return status;
}
