/*
 * Copyright 2005-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef REMAPPED_FUNCTIONS_H
#define REMAPPED_FUNCTIONS_H


#ifdef __cplusplus
extern "C" {
#endif

int		_haiku_build_fchmod(int fd, mode_t mode);
int		_haiku_build_fchmodat(int fd, const char* path, mode_t mode, int flag);
int		_haiku_build_fstat(int fd, struct stat* st);
int		_haiku_build_fstatat(int fd, const char* path, struct stat* st,
			int flag);
int		_haiku_build_mkdirat(int fd, const char* path, mode_t mode);
int		_haiku_build_mkfifoat(int fd, const char* path, mode_t mode);
int		_haiku_build_utimensat(int fd, const char* path,
			const struct timespec times[2], int flag);
int		_haiku_build_futimens(int fd, const struct timespec times[2]);
int		_haiku_build_faccessat(int fd, const char* path, int accessMode,
			int flag);
int		_haiku_build_fchdir(int fd);
int		_haiku_build_close(int fd);
int		_haiku_build_dup(int fd);
int		_haiku_build_dup2(int fd1, int fd2);
int		_haiku_build_linkat(int toFD, const char* toPath, int pathFD,
			const char* path, int flag);
int		_haiku_build_unlinkat(int fd, const char* path, int flag);
ssize_t	_haiku_build_readlinkat(int fd, const char* path, char* buffer,
			size_t bufferSize);
int		_haiku_build_symlinkat(const char* toPath, int fd,
			const char* symlinkPath);
int		_haiku_build_ftruncate(int fd, off_t newSize);
int		_haiku_build_fchown(int fd, uid_t owner, gid_t group);
int		_haiku_build_fchownat(int fd, const char* path, uid_t owner,
			gid_t group, int flag);
int		_haiku_build_mknodat(int fd, const char* name, mode_t mode, dev_t dev);
int		_haiku_build_creat(const char* path, mode_t mode);
int		_haiku_build_open(const char* path, int openMode, mode_t permissions);
int		_haiku_build_openat(int fd, const char* path, int openMode,
			mode_t permissions);
int		_haiku_build_fcntl(int fd, int op, int argument);
int		_haiku_build_renameat(int fromFD, const char* from, int toFD,
			const char* to);

#ifdef HAIKU_HOST_PLATFORM_HAIKU

ssize_t	_haiku_build_fs_read_attr(int fd, const char *attribute, uint32 type,
					off_t pos, void *buffer, size_t readBytes);
ssize_t	_haiku_build_fs_write_attr(int fd, const char *attribute, uint32 type,
					off_t pos, const void *buffer, size_t readBytes);
int		_haiku_build_fs_remove_attr(int fd, const char *attribute);
int		_haiku_build_fs_stat_attr(int fd, const char *attribute,
					struct attr_info *attrInfo);

int		_haiku_build_fs_open_attr(const char *path, const char *attribute,
					uint32 type, int openMode);
int		_haiku_build_fs_fopen_attr(int fd, const char *attribute, uint32 type,
					int openMode);
int		_haiku_build_fs_close_attr(int fd);

void	*_haiku_build_fs_open_attr_dir(const char *path);
void	*_haiku_build_fs_lopen_attr_dir(const char *path);
void	*_haiku_build_fs_fopen_attr_dir(int fd);
int		_haiku_build_fs_close_attr_dir(void *dir);
void	*_haiku_build_fs_read_attr_dir(void *dir);
void	_haiku_build_fs_rewind_attr_dir(void *dir);

#endif

#ifdef __cplusplus
} // extern "C"
#endif


#endif	// REMAPPED_FUNCTIONS_H
