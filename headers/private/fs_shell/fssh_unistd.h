/*
 * Copyright 2004-2007, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_UNISTD_H
#define _FSSH_UNISTD_H


#include "fssh_defs.h"


/* access modes */
#define FSSH_R_OK	4
#define FSSH_W_OK	2
#define FSSH_X_OK	1
#define FSSH_F_OK	0

/* standard file descriptors */
#define FSSH_STDIN_FILENO	0
#define FSSH_STDOUT_FILENO	1
#define FSSH_STDERR_FILENO	2

/* lseek() constants */
#ifndef FSSH_SEEK_SET
#	define FSSH_SEEK_SET 0
#endif
#ifndef FSSH_SEEK_CUR
#	define FSSH_SEEK_CUR 1
#endif
#ifndef FSSH_SEEK_END
#	define FSSH_SEEK_END 2
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* file functions */
extern int			fssh_access(const char *path, int accessMode);

extern int			fssh_chdir(const char *path);
extern int			fssh_fchdir(int fd);
extern char			*fssh_getcwd(char *buffer, fssh_size_t size);

extern int			fssh_dup(int fd);
extern int			fssh_dup2(int fd1, int fd2);
extern int			fssh_close(int fd);
extern int			fssh_link(const char *name, const char *new_name);
extern int			fssh_unlink(const char *name);
extern int			fssh_rmdir(const char *path);

extern fssh_ssize_t	fssh_readlink(const char *path, char *buffer,
						fssh_size_t bufferSize);
extern int      	fssh_symlink(const char *from, const char *to);

extern int      	fssh_ftruncate(int fd, fssh_off_t newSize);
extern int      	fssh_truncate(const char *path, fssh_off_t newSize);
extern int			fssh_ioctl(int fd, unsigned long op, ...);

extern fssh_ssize_t	fssh_read(int fd, void *buffer, fssh_size_t count);
extern fssh_ssize_t	fssh_read_pos(int fd, fssh_off_t pos, void *buffer,
						fssh_size_t count);
extern fssh_ssize_t	fssh_pread(int fd, void *buffer, fssh_size_t count,
						fssh_off_t pos);
extern fssh_ssize_t	fssh_write(int fd, const void *buffer, fssh_size_t count);
extern fssh_ssize_t	fssh_write_pos(int fd, fssh_off_t pos, const void *buffer,
						fssh_size_t count);
extern fssh_ssize_t	fssh_pwrite(int fd, const void *buffer, fssh_size_t count,
						fssh_off_t pos);
extern fssh_off_t	fssh_lseek(int fd, fssh_off_t offset, int whence);

extern int			fssh_sync(void);
extern int			fssh_fsync(int fd);

/* access permissions */				
extern fssh_gid_t	fssh_getegid(void);
extern fssh_uid_t	fssh_geteuid(void);
extern fssh_gid_t	fssh_getgid(void);
extern int			fssh_getgroups(int groupSize, fssh_gid_t groupList[]);
extern fssh_uid_t	fssh_getuid(void);

#ifdef __cplusplus
}
#endif

#endif  /* _FSSH_UNISTD_H */
