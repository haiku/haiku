/*
 * Copyright 2011, John Scipione, jscipione@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef FS_DARWIN_H
#define FS_DARWIN_H

/*
 * Magic value that specify the use of the current working directory
 * to determine the target of relative file paths in the openat() and
 * similar syscalls.
 */
#define AT_FDCWD			-100	/* CWD FD for the *at() functions */

/*
 * Miscellaneous flags for the *at() syscalls.
 */
#define AT_EACCESS			0x100	/* faccessat() */
#define AT_SYMLINK_NOFOLLOW	0x200	/* fstatat(), fchmodat(), fchownat(),
										utimensat() */
#define AT_SYMLINK_FOLLOW	0x400	/* linkat() */
#define AT_REMOVEDIR		0x800	/* unlinkat() */

int faccessat(int fd, const char* path, int accessMode, int flag);
int fchmodat(int fd, const char* path, mode_t mode, int flag);
int fchownat(int fd, const char* path, uid_t owner, gid_t group, int flag);
int fstatat(int fd, const char *path, struct stat *st, int flag);
int mkdirat(int fd, const char *path, mode_t mode);
int mkfifoat(int fd, const char *path, mode_t mode);
int mknodat(int fd, const char *name, mode_t mode, dev_t dev);
int renameat(int oldFD, const char* oldPath, int newFD, const char* newPath);

ssize_t readlinkat(int fd, const char *path, char *buffer, size_t bufferSize);
int symlinkat(const char *oldPath, int fd, const char *newPath);
int unlinkat(int fd, const char *path, int flag);
int linkat(int oldFD, const char *oldPath, int newFD, const char *newPath,
	int flag);

int futimesat(int fd, const char *path, const struct timeval times[2]);

#endif // FS_DARWIN_H
